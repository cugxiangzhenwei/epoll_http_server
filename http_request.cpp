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
#include <sys/sendfile.h>
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
				sprintf(szBuffer,"&nbsp;<A href=\"%s%s/\">%400s</A>&nbsp;&nbsp;[DIR]<br/>"
				,url,entry->d_name, entry->d_name);
				strData += szBuffer;
				strData +="\n";
				bIsEmpty = false;
		  }
		  else
		  {
				char szBuffer[2048];
				sprintf(szBuffer,"&nbsp;<A href=\"%s%s\">%400s</A>&nbsp;&nbsp;%ld&nbsp;bytes<br/>"
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
int cat(int client, FILE *resource,long long iReadBytes,long long & iFinished,long long iFileOffset) 
{
    //返回文件数据
	printf("cat 开始发送文件...\n");
	int iReturn = 1;
	// new type to send file to socket,do need swap kenel to user then to kenel space
	int filefd = fileno(resource);
 	struct stat stat_buf;  
    fstat( filefd, &stat_buf ); 
	iFinished = 0;
	long long iAllBytes = iReadBytes;
	while(iFinished <stat_buf.st_size && iFinished < iAllBytes)
	{
		off_t oft = iFileOffset + iFinished;// 计算本次发送的起始位置	
		iReadBytes = iAllBytes - iFinished; // all other need read bytes,may be not to file end , eg 206 partial-Coent
		if(iReadBytes > DATA_SIZE_ONCE)
			iReadBytes = DATA_SIZE_ONCE;

	//	printf("begin to send file,offset=%ld,iReadBytes=%lld\n",oft,iReadBytes);
		if((iReturn= sendfile(client, filefd, &oft,iReadBytes)) == -1)
		{
			printf("Send file error:%s\n",strerror(errno));
			return -1;
		}
	//	printf("sendfile return:%d\n",iReturn);
		iFinished += iReturn;
	}
	printf("cat 发送文件完毕!\n");
	return 1;
	// 以下是老的方式发送文件，从内核拷贝到用户空间，然后又到socket的内核空间,以下代码是发送整个文件的，发送部分文件需要修改
    char buf[1024];	
	char *pData = NULL;
	iReturn = fseek(resource,iFinished,SEEK_SET);
	if(iReturn!=0)
	{
		printf("error to seek pos:%lld\n",iFinished);
		return iReturn;
	}
	while(1) // loop to read data
	{
		int iRead = fread(buf,sizeof(char),iReadBytes,resource); 
		if(iRead<=0)
		{
			if(feof(resource))
				break;
			if(ferror(resource))
			{
				printf("fread error occur:%s\n",strerror(errno));
				iReturn = -1;
				break;
			}
		}
		int iOffSet = 0;
		int iRev = 0;
		while(iOffSet < iRead)// loop to send finish this read data
		{
			pData = buf + iOffSet;
			iRev = send(client,pData,iRead - iOffSet,0);
			if(iRev ==-1)
			{
				printf("cat function Send erro occur:%s\n",strerror(errno));
				return  -1;
			}
			else if(iRev ==0)
			{
				printf("client %d closed connect!\n",client);
				return 0;
			}
			iOffSet += iRev;
			iFinished += iRev;
		}
	}
	printf("cat 发送文件完毕!\n");
	return 1;
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
http_Request * add_request(int epollfd,int fd,const char * ip,int iport)
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
	req->m_iContent_Length = -1;
	req->m_iSendCompleteLen = 0;
	req->m_iClientPort = iport;
	strcpy(req->m_ClientIp,ip);
//	bzero(req->m_ClientIp,sizeof(char)*128);
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
	char szBuf[1024];
	printf("read_header begin call ...\n");
	while(1)
	{
		printf("read_heade recv data begin call...\n");
		int iRev = recv(m_fd,szBuf,1023,0);
		printf("read_heade recv return %d\n",iRev);
		if(iRev ==0)
		{
			printf("client %s:%d closed!\n",m_ClientIp,m_iClientPort);
			return 0;
		}
		else if(iRev == -1)
		{
			if(EAGAIN == errno || EWOULDBLOCK==errno)
					break; // header readfinished

			printf("%d recv error:%s\n",m_fd,strerror(errno));
			return -1;
		}
		else
		{
			szBuf[iRev]='\0';
			m_strReceiveHeaders += szBuf;
			//printf("header:%s\n",m_strReceiveHeaders.c_str());
			int iLen = m_strReceiveHeaders.length();
			char c = szBuf[iRev-1];
			if(iLen >4 && m_strReceiveHeaders[iLen-2] == '\r' && c=='\n')
			{
				if(m_strReceiveHeaders[iLen-3]=='\n' && m_strReceiveHeaders[iLen-4]=='\r')
				{
					m_iState = state_parse_header; // 接收http头完毕，设置下一阶段为解析http头
					break;
				}
			}
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
	if(m_http_method == HTTP_POST)
	{
		printf("begin to read body data！\n");
		m_iState = state_read_body;
	}
	else
		m_iState = state_prepare_response;
	return 1;
}
int http_Request::read_body()
{
	//	printf("read_body begin calll...\n");
	if(m_iContent_Length == -1)
	{
		const char * pContentLength = strstr(m_strReceiveHeaders.c_str(),"Content-Length");
		if(pContentLength!=NULL)
		{
			sscanf(pContentLength,"Content-Length: %lld",&m_iContent_Length);
			printf("Content-Length:%lld\n",m_iContent_Length);
		}
		else
		{
			m_iContent_Length = 0;
			printf("Content-Length参数未指定！\n");
		}
	}
	char szBuf[1024];
	while(1)
	{
		int iRev = recv(m_fd,szBuf,1023,0);
		if(iRev ==0)
		{
			printf("client %s:%d closed!\n",m_ClientIp,m_iClientPort);
			close(m_fd);
			return 0;
		}
		else if(iRev == -1)
		{
			if(EAGAIN == errno || EWOULDBLOCK==errno)
				break; // body data read finished

			printf("%d recv error:%s\n",m_fd,strerror(errno));
			close(m_fd);
			return -1;
		}
		else
		{
			szBuf[iRev] = '\0';
			m_strBodyData += szBuf;
			int iLen = m_strBodyData.length();
			if(m_iContent_Length >0 && iLen==m_iContent_Length)
				m_iState = state_parse_body;
		}
	}
	return 1;
}
int http_Request::parse_body()
{
	printf("\nparse_body begin call...\n");
	printf("post body data is:\n%s\n",m_strBodyData.c_str());
	m_iState = state_prepare_response;
	return 1;
}
int http_Request:: prepare_post_response()
{
	printf("prepare_post_response begin call...\n");
	printf("post request url:%s,post data:%s\n",m_szURI,m_strBodyData.c_str());
	if(strcasecmp(m_szURI,"/login")==0)
	{
		std::string strData ="{token:\"1234567890\",id:1,name=\"xiangzhenwei\"}";
		m_iDataLength = strData.size();
		m_szDataSend = (char * )malloc(m_iDataLength);
		strcpy(m_szDataSend,strData.c_str());
		m_strResponseHeaders = GetResponseHeader("","application/json",m_iDataLength,-1,-1);
	}
	else
	{
		std::string strData;
		m_strResponseHeaders = get_404_ResponseHeader(strData,m_szURI);
		m_iDataLength =  strData.size();
		m_szDataSend = (char * )malloc(m_iDataLength);
		strcpy(m_szDataSend,strData.c_str());
	}	
	m_iState =state_send_response;
	return 1;
}
int http_Request::prepare_get_response()
{
	std::string strFullPath = g_strHomeDir + m_szURI;
	printf("get resource full path:%s,iSockClient:%d\n",strFullPath.c_str(),m_fd);
	struct stat st;
	if(stat(strFullPath.c_str(),&st)==-1)
	{
		printf("%s路径无法找到\n",strFullPath.c_str());
		std::string strData;
		m_strResponseHeaders = get_404_ResponseHeader(strData,m_szURI);
		m_iDataLength =  strData.size();
		m_szDataSend = (char * )malloc(m_iDataLength);
		strcpy(m_szDataSend,strData.c_str());
		m_iState =state_send_response;
		return 1;
	}
	
	if(S_ISDIR(st.st_mode))
	{
		printf("枚举目录内的文件：%s\n",strFullPath.c_str());
		if(strFullPath[strFullPath.size()-1]!='/')
			strFullPath.append(1,'/');
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
int http_Request::prepare_response()
{
	printf("prepare reponse header!\n");
	if(m_http_method == HTTP_GET)
		return prepare_get_response();
	else if(m_http_method == HTTP_POST)
		return prepare_post_response();
	else
	{
		printf("not supported response type:%s\n",HTTP_METHOD_STR[m_http_method]);
		return -1;
	}
}
int http_Request::send_response()
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
		iRev = cat(m_fd,m_pFileServe,m_iReadbytes,m_iFinishedBytes,m_iFileOffset);
		if(iRev ==1)
		{
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
		assert(m_szDataSend!=NULL);
		char * pData  = m_szDataSend + m_iSendCompleteLen;
		int iLenSendThis = m_iDataLength - m_iSendCompleteLen;
		assert(iLenSendThis>0);
		int iLen = send(m_fd,pData,iLenSendThis,0);
		if(iLen == -1)
		{
			if(EAGAIN == errno || EWOULDBLOCK==errno)
			{
				printf("send_data io not ready\n");
				return 1; // io not ready
			}
			printf("erro to send data:%s\n",strerror(errno));
			return -1; //error occured
		}
		else if(iLen ==0)
		{
			printf("client closed!\n");
			return 0; //client closed 
		
		}
		else
		{
			printf("send_data send:%dbytes,but return %dbytes\n",iLenSendThis,iLen);
		}
		m_iSendCompleteLen += iLen;
	}
	if(m_iSendCompleteLen == m_iDataLength)
	{
 		printf("send data  call finished!\n");
		m_iState = state_finish;
	}
	return iRev;
}
