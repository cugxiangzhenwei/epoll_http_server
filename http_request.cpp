#include"http_request.h"
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdio.h>
#include<errno.h>
#include<unistd.h>
#include<assert.h>
#include <sys/sendfile.h>
#include<vector>
#include"redis_api.h"
#include"httpCommon.h"
#include"UrlCode.h"
#include"net_disk_core.h"

typedef std::map<int,http_Request *> RequestMapType;
static RequestMapType g_requestMap;

std::string strJsFunResizeImg ="\
<script language=\"JavaScript\">\
		function resizeimg(obj,maxW,maxH)\
		{\
			 var imgW=obj.width;\
			 var imgH=obj.height;\
			 if(imgW>maxW||imgH>maxH)\
			 {       \
					  var ratioA=imgW/maxW;\
					  var ratioB=imgH/maxH;\
					  if(ratioA>ratioB)\
					  {\
							   imgW=maxW;\
							   imgH=imgH / ratioA;\
					  }\
					  else\
					  {\
							   imgH=maxH;\
							   imgW=imgW / ratioB;\
					  }\
					  obj.width=imgW;\
					  obj.height=imgH;\
			 }\
		}\
	</script>\
";
int list_dir_images(char * & pszDataOut,const char * pszWorkDir,const char * url)
{
	printf("list_dir_images[%s]...\n",pszWorkDir);
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
		printf("Get dir items from redis cache key : %s\n",pszWorkDir);
		return iLen;
	}
	
	struct dirent * entry = NULL;
	DIR * pDir = NULL;
    struct stat statbuf;
	std::string strData = "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/></head>";
	strData += "<body>";
	pDir = opendir(pszWorkDir);
	std::vector<std::string> vFolders;
	std::vector<std::string> vImages;
	while(NULL!=(entry = readdir(pDir)))
	{
	  char szFullPath[1024];
	  sprintf(szFullPath,"%s%s",pszWorkDir,entry->d_name);
	//  printf("get one item [%s]\n",szFullPath);
      lstat(szFullPath,&statbuf);
      if( S_ISDIR(statbuf.st_mode) )
      {
		  if(strcmp(".",entry->d_name)==0 || strcmp("..",entry->d_name)==0)
          continue;
		 vFolders.push_back(szFullPath);
      }
      else
      {
		char * pExt = strstr(entry->d_name,".");
		if(pExt && (
		strcasecmp(pExt,".jpg")==0 ||
		strcasecmp(pExt,".jpeg")==0||
		strcasecmp(pExt,".png")==0||
		strcasecmp(pExt,".gif")==0))
		{
		 vImages.push_back(szFullPath);
		}	
      }
	}
	if(vImages.size() >0)
	{
		strData += strJsFunResizeImg;
	//	send(client,strJsFunResizeImg.c_str(),strJsFunResizeImg.length(),0);	
	}
	for(size_t i=0;i<vFolders.size();i++)
	{
		char * pName = strrchr((char*)vFolders[i].c_str(),'/');
		if(pName)
		{
			char name[256];
			strcpy(name,pName+1);
			char szBuffer[2048];
			sprintf(szBuffer,"&nbsp;<A href=\"%s%s/\">%s</A>&nbsp;&nbsp;[DIR]<br/>"
			,url,name,name);
			strData += szBuffer;
			//send(client,szBuffer,strlen(szBuffer),0);
		}
	}
	if(vImages.size()>0)
	{
		strData += "<div align=\"center\">";
		strData += "<div id=\"imgbox\" style=\"width:1920px;height:1080px;border:1px solid #CCCCCC\">";
	}
	for(size_t i=0;i<vImages.size();i++)
	{
		char * pName = strrchr((char*)vImages[i].c_str(),'/');
		if(pName)
		{
			char name[256];
			strcpy(name,pName+1);
			char szBuffer[2048];
			sprintf(szBuffer,"<a href='%s%s'><img src=%s%s onload=\"resizeimg(this,1200,800)\"></a>\n",url,name,url,name);
			strData += szBuffer;
		}
		else
		{
			printf("can't get filename of file :%s\n",vImages[i].c_str());
		}
	}
	if(vImages.size()>0)
	{
   		strData += "</div></div>";
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
	printf("list_dir_images[%s] return iLen=%ld\n",pszWorkDir,iLen);
	return iLen;
}
int list_dir_items(char * & pszDataOut,const char * pszWorkDir,const char * url)
{
	printf("list_dir_items[%s]...\n",pszWorkDir);
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
		printf("Get dir items from redis cache key : %s\n",pszWorkDir);
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
		// 2. 输出路径
		//(1). 输出第一项 根目录
		strData +="<A href=\"/\">/</A>";
		//(2). 其它目录
		std::string::size_type st = 1;
		std::string::size_type stNext = 1;
		std::string strUrl = url;
		while( (stNext = strUrl.find('/', st)) != std::string::npos)
		{   
			std::string strDirName =  strUrl.substr(st, stNext - st + 1); 
			std::string strSubUrl = strUrl.substr(0, stNext + 1); 

			strData +="&nbsp;|&nbsp;";
			strData +="<A href=\"";
			strData +=strSubUrl;
			strData +="\">";
			strData +=strDirName;
			strData +="</A>";
			// 下一个目录
			st = stNext + 1;
		}   
		strData +="<br /><hr />";
		bool bIsEmpty = true;
		while(NULL!=(entry = readdir(pDir)))
		{
		  char szFullPath[1024];
		  sprintf(szFullPath,"%s%s",pszWorkDir,entry->d_name);
	//	  printf("get one item [%s]\n",szFullPath);
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
				std::string strSizeBuf = GetFileSizeStr(statbuf.st_size);
				sprintf(szBuffer,"&nbsp;&nbsp;<A href=\"%s%s\">%400s</A>&nbsp;&nbsp;%s<br/>"
				,url,entry->d_name,entry->d_name,strSizeBuf.c_str());
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
	printf("list_dir_items[%s] return iLen=%ld\n",pszWorkDir,iLen);
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
	//	iFinished = 0;
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
			if(errno==EINTR) /* 中断错误 我们继续写*/   
            {  
                printf("[SeanSend]error errno==EINTR continue\n");  
                continue;  
            }  
            else if(errno==EAGAIN) /* EAGAIN : Resource temporarily unavailable*/   
            {  
              //  usleep(50);//等待一秒，希望发送缓冲区能得到释放  
                printf("[SeanSend]error errno==EAGAIN wait epoll write ready\n");  
               // continue;
				return 1;
            }  
            else /* 其他错误 没有办法,只好退了*/   
            {  
				printf("Send file error:error=%d,%s\n",errno,strerror(errno));
                return -1;  
            }  
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
	req->m_bKeeepAlive = false;
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
	req->Reset();
	delete req;
	g_requestMap.erase(iter);
}
int get_connectionCount()
{
	return g_requestMap.size();
}
int http_Request::read_header()
{	
	char szBuf[1024];
	printf("read_header begin call ...\n");
	while(1)
	{
		//printf("read_heade recv data begin call...\n");
		int iRev = recv(m_fd,szBuf,1023,0);
		//printf("read_heade recv return %d,data:\n%s\n",iRev,szBuf);
		if(iRev ==0)
		{
			printf("client %s:%d closed!\n",m_ClientIp,m_iClientPort);
			return 0;
		}
		else if(iRev == -1)
		{
			if(EAGAIN == errno || EWOULDBLOCK==errno)
			{
				if(m_strReceiveHeaders.find("\r\n\r\n")!=std::string::npos)
				{
					m_iState = state_parse_header; // 接收http头完毕，设置下一阶段为解析http头
					break;
				}
			}
			printf("%d recv error:%s\n",m_fd,strerror(errno));
			return -1;
		}
		else
		{
			szBuf[iRev]='\0';
			m_strReceiveHeaders += szBuf;
		}
	}
	printf("read_header return!\n");
	return 1;
}
int http_Request::parse_header()
{
	printf("parse_header begin call...\nrequest:%8p,headers in:\n%s\n",this,m_strReceiveHeaders.c_str());
	std::string strUrl = GetURL(m_strReceiveHeaders);
	m_http_method = GetMethod(m_strReceiveHeaders);
	m_bKeeepAlive = IsKeepAlive(m_strReceiveHeaders);
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
		m_iState = state_read_body;
	}
	else
		m_iState = state_prepare_response;
	printf("parse_header call finished!\n");
	return 1;
}
int http_Request::read_body()
{
	printf("read_body begin calll...\n");
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
	int iHeaderEnd = m_strReceiveHeaders.find("\r\n\r\n") + strlen("\r\n\r\n");
	m_strBodyData += m_strReceiveHeaders.c_str() + iHeaderEnd;
	printf("body data is:%s\n",m_strBodyData.c_str());
	char szBuf[1024];
	while(1)
	{
		//printf("read_body recv data begin call...\n");
		int iRev = recv(m_fd,szBuf,1023,0);
		//printf("read_body recv return %d,data:\n%s\n",iRev,szBuf);
		if(iRev ==0)
		{
			printf("client %s:%d closed!\n",m_ClientIp,m_iClientPort);
			close(m_fd);
			return 0;
		}
		else if(iRev == -1)
		{
			if(EAGAIN == errno || EWOULDBLOCK==errno)
			{
				int iLen = m_strBodyData.length();
				if(iLen == m_iContent_Length && m_iContent_Length >0)
				{
					m_iState = state_parse_body;
				}
				break;
			}

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
	printf("read_body  calll finished\n");
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
//	printf("prepare_post_response begin call...\n");
//	printf("request:%8p,post request url:%s,post data:%s\n",this,m_szURI,m_strBodyData.c_str());
	std::string strData = ProNetDiskRequest(m_strBodyData,m_szURI);
	m_iDataLength = strData.size();
	m_szDataSend = (char * )malloc(m_iDataLength);
	strcpy(m_szDataSend,strData.c_str());
	m_strResponseHeaders = GetResponseHeader("","application/json",m_iDataLength,-1,-1);
	/*
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
	}*/	
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
		if(g_iListMode==0)
			m_iDataLength = list_dir_items(m_szDataSend,strFullPath.c_str(),m_szURI);
		else
			m_iDataLength = list_dir_images(m_szDataSend,strFullPath.c_str(),m_szURI);
		m_strResponseHeaders = GetResponseHeader("","text/html",m_iDataLength,-1,-1);
		chdir(strFullPath.c_str());
	}
	else
	{
			printf("serve_file：%s\n",strFullPath.c_str());
			char * pRange = strstr((char*)m_strReceiveHeaders.c_str(),"Range: bytes=");
			m_iFileOffset  = 0;
			m_iReadbytes  = st.st_size;
			if(pRange)
			{
				pRange = pRange + strlen("Range: bytes=");
				char * pEnd = strstr(pRange,"\n");
				assert(pEnd!=NULL);
				pEnd = pEnd - 1;
				if(*pRange == '-') // -500 最后500个字节
				{
					sscanf(pRange,"-%lld",&m_iReadbytes);
					m_iFileOffset = st.st_size - m_iReadbytes;
				}
				else if(*pEnd == '-') // 500- 500字节以后的范围
				{
					 sscanf(pRange,"%lld-",&m_iFileOffset);
					 m_iReadbytes =  st.st_size - m_iFileOffset;
				}
				else // 100-500  100到500个字节之间的范围
				{
					long long iEnd = 0;
					sscanf(pRange,"%lld-%lld",&m_iFileOffset,&iEnd);
					m_iReadbytes = iEnd - m_iFileOffset;
					printf("request range:%lld-%lld,total=%lld\n",m_iFileOffset,iEnd,m_iReadbytes);
				}	 	
			}
			std::string strType = getContentTypeFromFileName(strFullPath.c_str());
			m_pFileServe = fopen(strFullPath.c_str(),"r");
			m_iFinishedBytes = 0;
			if(m_pFileServe==NULL)
			{
				printf("can't open file %s\n",strFullPath.c_str());
				return -1;
			}	
			if(0!=fseek(m_pFileServe,m_iFileOffset,SEEK_SET))
			{
				printf("fseek error:%s\n",strerror(errno));
				return -1;
			}
			long long iEnd = m_iFileOffset + m_iReadbytes;
			m_strResponseHeaders = GetResponseHeader(strFullPath.c_str(),strType.c_str(),m_iReadbytes,m_iFileOffset,iEnd);
	}
	printf("request:%8p,response:\n%s\n",this,m_strResponseHeaders.c_str());
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
		//发送缓冲区
		int nSendBuf = iLenSendThis; //设置为iLenSendThis
		setsockopt(m_fd,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));
        //在发送数据的时，不执行由系统缓冲区到socket缓冲区的拷贝，以提高程序的性能：
		int nZero = 0;
		setsockopt(m_fd,SOL_SOCKET,SO_SNDBUF,(char *)&nZero,sizeof(nZero));
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
//			printf("send_data send:%dbytes,but return %dbytes\n",iLenSendThis,iLen);
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
void http_Request::Reset()
{
	m_iState = state_read_header;
	m_strReceiveHeaders.clear();
	m_strResponseHeaders.clear();
	if(m_pFileServe)
	{
		fclose(m_pFileServe);
		m_pFileServe = NULL;
	}
	if(m_szDataSend)
	{
		free(m_szDataSend);
		m_szDataSend = NULL;
	}
	m_iDataLength = 0;
	m_iContent_Length = -1;
	m_iSendCompleteLen = 0;
	m_bKeeepAlive = false;
}
