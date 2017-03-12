#include"http_request.h"
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdio.h>
#include<errno.h>
#include<unistd.h>
#include"httpCommon.h"
#include"UrlCode.h"
#include<assert.h>
typedef std::map<int,http_Request *> RequestMapType;
static RequestMapType g_requestMap;

int list_dir_items(char ** pszDataOut,const char * pszWorkDir,const char * url)
{
	printf("list_dir_items调用...\n");
	//sendheaders(client,url,"text/html",-1,-1,-1);
	struct dirent * entry = NULL;
	DIR * pDir = NULL;
    struct stat statbuf;
	std::string strData = "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/></head>\n";
	strData +="<body>\n";
	pDir = opendir(pszWorkDir);
	while(NULL!=(entry = readdir(pDir)))
	{
	  char szFullPath[1024];
	  sprintf(szFullPath,"%s%s",pszWorkDir,entry->d_name);
	  printf("get one item [%s]\n",szFullPath);
      lstat(szFullPath,&statbuf);
      if( S_ISDIR(statbuf.st_mode) )
      {
		  if(strcmp(".",entry->d_name)==0 || strcmp("..",entry->d_name)==0)
          continue;

        char szBuffer[2048];
		sprintf(szBuffer,"&nbsp;<A href=\"%s%s/\">%s</A>&nbsp;&nbsp;[DIR]<br/>"
		,url,entry->d_name, entry->d_name);
		strData += szBuffer;
		strData +="\n";
      }
      else
      {
            char szBuffer[2048];
			sprintf(szBuffer,"&nbsp;<A href=\"%s%s\">%s</A>&nbsp;&nbsp;%ld&nbsp;bytes<br/>"
			,url,entry->d_name,entry->d_name,statbuf.st_size );
		strData += szBuffer;
		strData +="\n";
      }
	}
	strData += "</body></html>";
	printf("list_dir_items调用结束!\n");
	*pszDataOut = (char *)malloc(strData.length());
	strcpy(*pszDataOut,strData.c_str());	
	return strData.length();
}
int cat(int client, FILE *resource,long long & iReadBytes) {
    //返回文件数据
	printf("cat 开始发送文件...\n");
    char buf[1024];	
	long long iFinished = 0;
	int iReturn = 1;
   	int iRead = fread(buf,sizeof(char),1024,resource); 
	do{
		if(iRead)
		{
			int iRev = send(client,buf,iRead,0);
			if(iRev ==-1)
			{
				printf("cat function Send erro occur:%s\n",strerror(errno));
				iReturn = -1;;
				break;
			}
			else if(iRev ==0)
			{
				printf("client closed connect!\n");
				iReturn = 0;
				break;
			}
   			iFinished += iRead;
			if(iFinished == iReadBytes)
			{
				break;
			}
			iRead = fread(buf,sizeof(char),1024,resource); 
		}
		
	}while(iRead >0);
	printf("cat 发送文件调用结束!\n");
	return iReturn;
}


http_Request * find_request(int fd)
{
	RequestMapType::iterator iter = g_requestMap.find(fd);	
	if(iter==g_requestMap.end())
		return NULL;
	return iter->second;
}
void _add_request(http_Request * req)
{
	if(!req) return;
	g_requestMap.insert(std::make_pair(req->m_fd,req));
}
http_Request * add_request(int epollfd,int fd)
{
	http_Request * req = new http_Request;
	req->m_fd = fd;
	req->m_epollfd = epollfd;
	req->m_iState = state_read_header; //初始状态为读取http响应头
	req->m_strReceiveHeaders.clear();
	req->m_strResponseHeaders.clear();
	req->m_pFileServe = NULL;
	req->m_szDataSend = NULL;
	req->m_iDataLength = 0;
	bzero(req->m_szURI,sizeof(char)*MAXSIZE);
	_add_request(req);
	return req;
}
void remove_request(int fd)
{
	RequestMapType::iterator iter = g_requestMap.find(fd);	
	if(iter==g_requestMap.end())
		return;
	
	http_Request * req = iter->second;
	delete req;
	g_requestMap.erase(iter);
}
int http_Request::read_header()
{	
	char c;
	int iRev = recv(m_fd,&c,1,0);
	if(iRev ==0)
	{
		printf("client %d closed!\n",m_fd);
		close(m_fd);
		return 0;
	}
	else if(iRev == -1)
	{
		printf("%d recv error:%s\n",m_fd,strerror(errno));
		close(m_fd);
		return -1;
	}
	else
	{
		m_strReceiveHeaders.append(1,c);
		int iLen = m_strReceiveHeaders.length();
		if(iLen >4 && m_strReceiveHeaders[iLen-2] == '\r' && c=='\n')
		{
			if(m_strReceiveHeaders[iLen-3]=='\n' && m_strReceiveHeaders[iLen-4]=='\r')
				m_iState = state_parse_header; // 接收http头完毕，设置下一阶段为解析http头
		}
	}
	return 1;
}
int http_Request::parse_header()
{
	printf("headers in:\n%s\n",m_strReceiveHeaders.c_str());
	std::string strUrl = GetURL(m_strReceiveHeaders);
	if(strUrl.empty())
	{
		printf("can't get url from header!\n");
		return -1;
	}
	std::string strDecode = UrlDecode(strUrl);
	strcpy(m_szURI,strDecode.c_str());
	printf(	"url:%s\nDecodeUrl:%s\n",strUrl.c_str(),m_szURI);
	m_iState = state_prepare_response;
	return 1;
}
int http_Request::prepare_header()
{
	printf("prepare reponse header!\n");
	std::string strFullPath = g_strHomeDir + m_szURI;
	printf("get resource full path:%s,iSockClient:%d\n",strFullPath.c_str(),m_fd);
	struct stat st;
	if(stat(strFullPath.c_str(),&st)==-1)
	{
		printf("%s路径无法找到\n",strFullPath.c_str());
		std::string strData;
		m_strResponseHeaders = get_404_ResponseHeader(strData);
		m_iDataLength =  strData.size();
		m_szDataSend = (char * )malloc(m_iDataLength);
		strcpy(m_szDataSend,strData.c_str());
		m_iState =state_send_response;
		return 1;
	}
	
	if(S_ISDIR(st.st_mode))
	{
		printf("枚举目录内的文件：%s\n",strFullPath.c_str());
		m_strResponseHeaders = GetResponseHeader("","text/html",-1,-1,-1);
		m_iDataLength = list_dir_items(&m_szDataSend,strFullPath.c_str(),m_szURI);
		chdir(strFullPath.c_str());
	}
	else
	{
			printf("serve_file：%s\n",strFullPath.c_str());
			char * pRange = strstr((char*)m_strReceiveHeaders.c_str(),"Range: bytes=");
			long long iFileOffset = 0;
			long long iReadBytes  = st.st_size;
			if(pRange)
			{
				pRange = pRange + strlen("Range: bytes=");
				char * pEnd = strstr(pRange,"\n");
				assert(pEnd!=NULL);
				pEnd = pEnd - 1;
				if(*pRange == '-') // -500 最后500个字节
				{
					sscanf(pRange,"-%lld",&iReadBytes);
					iFileOffset = st.st_size - iReadBytes;
				}
				else if(*pEnd == '-') // 500- 500字节以后的范围
				{
					 sscanf(pRange,"%lld-",&iFileOffset);
					 iReadBytes =  st.st_size - iFileOffset;
				}
				else // 100-500  100到500个字节之间的范围
				{
					long long iEnd = 0;
					sscanf(pRange,"%lld-%lld",&iFileOffset,&iEnd);
					iReadBytes = iEnd - iFileOffset;
				}	 	
			}
			std::string strType = getContentTypeFromFileName(strFullPath.c_str());
			m_pFileServe = fopen(strFullPath.c_str(),"r");
			m_iFileOffset = iFileOffset;
			m_iReadbytes =  iReadBytes;
			if(m_pFileServe==NULL)
			{
				printf("can't open file %s\n",strFullPath.c_str());
				return -1;
			}	
			if(0!=fseek(m_pFileServe,iFileOffset,SEEK_SET))
			{
				printf("fseek error:%s\n",strerror(errno));
				return -1;
			}
			m_strResponseHeaders = GetResponseHeader(strFullPath.c_str(),strType.c_str(),st.st_size,iFileOffset,iFileOffset + iReadBytes);
	}
	m_iState = state_send_response;
	return 1;
}
int http_Request::send_header()
{
	printf("send response:\n%s\n",m_strResponseHeaders.c_str());
	int iSendLen = 0;
	int iAll =  m_strResponseHeaders.length();
	const char * pData = m_strResponseHeaders.c_str();
	while(iSendLen < iAll)
	{
		pData  = m_strResponseHeaders.c_str() + iSendLen;
		int iLen = send(m_fd,pData,1,0);
		if(iLen <= 0)
			break;

		iSendLen += iLen;
	}
	if(iSendLen == iAll)
		m_iState = state_send_data;
	else
	{
		printf("error to send headers!\n");
		return -1;
	}
	return 1;
}
int http_Request::send_data()
{
	printf("send data begin call ...\n");
	int iRev = 1;
	if(m_pFileServe!=NULL)
	{
		iRev = cat(m_fd,m_pFileServe,m_iReadbytes);
		if(iRev ==1)
		{
			fclose(m_pFileServe);
			m_pFileServe = NULL;
		}
	}
	else
	{
		assert(m_szDataSend!=NULL);
		int iSendLen = 0;
		int iAll =  m_iDataLength;
		char * pData = m_szDataSend;
		while(iSendLen < iAll)
		{
			pData  = m_szDataSend + iSendLen;
			int iLen = send(m_fd,pData,1,0);
			if(iLen <= 0)
				return iLen; //error ,client closed or error occured
			iSendLen += iLen;
		}
		free(m_szDataSend);
		m_szDataSend = NULL;
	}
	if(iRev ==1)
		m_iState = state_finish;
	printf("send data  call finished!\n");
	return iRev;
}
