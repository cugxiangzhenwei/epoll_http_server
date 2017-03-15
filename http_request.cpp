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
#include"redis_api.h"
typedef std::map<int,http_Request *> RequestMapType;
static RequestMapType g_requestMap;

int list_dir_items(char * & pszDataOut,const char * pszWorkDir,const char * url)
{
	printf("list_dir_items调用...\n");
	std::string strCache = RedisAPI::Instance()->get_string(pszWorkDir);
	if(!strCache.empty())
	{
		size_t iLen = strCache.length();
		pszDataOut = (char *)malloc(iLen+1);
		if(pszDataOut == NULL)
		{
			printf("Failed to malloc memory %lu bytes\n",strCache.length());
			return -1;
		}
		strcpy(pszDataOut,strCache.c_str());
		printf("Get dir items from redis cache key : %s\n",pszDataOut);
		return iLen;
	}
	
	struct dirent * entry = NULL;
	DIR * pDir = NULL;
    struct stat statbuf;
	std::string strData = "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/></head>\n";
	strData +="<body>\n";
	pDir = opendir(pszWorkDir);
	if(pDir == NULL)
	{
		strData += "you can't access this path!\n";
		printf("can't opendir [%s]\n",pszWorkDir);
	}
	else
	{
		bool bIsEmpty = true;
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
				bIsEmpty = false;
		  }
		  else
		  {
				char szBuffer[2048];
				sprintf(szBuffer,"&nbsp;<A href=\"%s%s\">%s</A>&nbsp;&nbsp;%ld&nbsp;bytes<br/>"
				,url,entry->d_name,entry->d_name,statbuf.st_size );
				strData += szBuffer;
				strData +="\n";
				bIsEmpty = false;
		  }
		}
		if(bIsEmpty)
			strData +="<center><font size=7 face=i\"黑体\" >当前目录为空!</font> </center>";
	}
	strData += "</body></html>";
	size_t iLen = strData.length();
	pszDataOut = (char *)malloc(iLen+1);
	if(pszDataOut == NULL)
	{
		printf("Failed to malloc memory %lu bytes\n",strData.length());
		return -1;
	}
	strcpy(pszDataOut,strData.c_str());
	RedisAPI::Instance()->set_string(pszWorkDir,strData);
	return iLen;
}
int cat(int client, FILE *resource,long long iReadBytes) {
    //返回文件数据
	//printf("cat 开始发送文件...\n");
    char buf[1024];	
	long long iFinished = 0;
	int iReturn = 1;
	char * pData = buf;
   	int iRead = fread(buf,sizeof(char),iReadBytes,resource); 
	do{
		if(iRead)
		{
			do
			{
				int iRev = send(client,pData,iRead,0);
				if(iRev ==-1)
				{
					printf("cat function Send erro occur:%s\n",strerror(errno));
					iReturn = -1;
					break;
				}
				else if(iRev ==0)
				{
					printf("client closed connect!\n");
					iReturn = 0;
					break;
				}
				iFinished += iRev;
				if(iFinished == iReadBytes)
				{
					break;
				}
				if(iRev < iRead) // need send mutilple times
				{
					pData = buf + iRev; // calucate next begin
					iRead = iRead - iRev; // calucate other bytes to need send	
				}
				else
					break;

			}while(1);
			iReadBytes = iReadBytes - iRead;
			iRead = fread(buf,sizeof(char),iReadBytes,resource); 
			pData = buf;
		}
		
	}while(iRead >0);
	//printf("cat 发送文件调用结束!\n");
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
	if(req->m_pFileServe)
	{
		fclose(req->m_pFileServe);
		req->m_pFileServe = NULL;
	}
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
	m_http_method = GetMethod(m_strReceiveHeaders);
	if(strUrl.empty())
	{
		printf("can't get url from header!\n");
		return -1;
	}
	std::string strDecode = UrlDecode(strUrl);
	strcpy(m_szURI,strDecode.c_str());
	printf(	"url:%s\nDecodeUrl:%s\nHTTP请求方法类型:%s\n",strUrl.c_str(),m_szURI,HTTP_METHOD_STR[m_http_method]);
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
		m_iDataLength = list_dir_items(m_szDataSend,strFullPath.c_str(),m_szURI);
		m_strResponseHeaders = GetResponseHeader("","text/html",m_iDataLength,-1,-1);
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
			m_iFinishedBytes = 0;
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
	int iRev = 1;
	if(m_pFileServe!=NULL)
	{
		long long iRead = m_iReadbytes - m_iFinishedBytes;
		if(iRead > DATA_SIZE_ONCE)
			iRead = DATA_SIZE_ONCE; // once send 256
		iRev = cat(m_fd,m_pFileServe,iRead);
		if(iRev ==1)
		{
			m_iFinishedBytes += iRead;
			if(m_iFinishedBytes == m_iReadbytes)
			{
				fclose(m_pFileServe);
				m_pFileServe = NULL;	
				printf("serve file %s finished\n",m_szURI);
			}
			else
				return 1; // return 1 ,but not finish ,wait next call ,cotinue send file data
		}
	}
	else
	{
//		printf("m_szDataSend=%p\n",m_szDataSend);
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
		printf("send data finished!\n");
		free(m_szDataSend);
		m_szDataSend = NULL;
	}
	if(iRev ==1)
		m_iState = state_finish;
 	//	printf("send data  call finished!\n");
	return iRev;
}
