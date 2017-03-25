#include"httpCommon.h"
#include<unistd.h>
std::string g_strHomeDir;
int g_iListMode; // 0-All ,1-images
const char *HTTP_METHOD_STR[] = {
"NONE HTTP METHOD",
"OPTIONS",
"HEAD",
"GET",
"POST",
"PUT",
"DELETE",
"TRACE",
"CONNECT",
 NULL
};
void SetHomeDir(const char * pszStrHomeDir)
{
	if(pszStrHomeDir == NULL)
	{
		char buf[256];
		char * pReturn = getcwd(buf,256);
		if(pReturn!=NULL)
			g_strHomeDir = buf;
	}
	else
		g_strHomeDir = pszStrHomeDir;

	printf("设置工作目录为：%s\n",g_strHomeDir.c_str());
}
void SetListMode(int iMode)
{
	if(iMode >0) iMode =1;
	else iMode = 0;
	g_iListMode = iMode;
	if(g_iListMode==0)
		printf("当前模式显示所有文件！\n");
	else
		printf("当前模式显示图片！\n");
}
HTTP_METHOD GetMethod(const std::string & strHeader)
{
	HTTP_METHOD hm = HTTP_NONE;
	for(int i=0; HTTP_METHOD_STR[i]!=NULL;i++)
	{
		if(strncmp(HTTP_METHOD_STR[i],strHeader.c_str(),strlen(HTTP_METHOD_STR[i]))==0)
		{
			hm = (HTTP_METHOD)(i);		
			break;
		}	
	}
	return hm;
}
std::string GetURL(const std::string & strHeader)
{
	if(strHeader.size()<=3)
	{	
		printf("GetURL param strHeader[%s] is error,\
		file:%s,line:%d,function:%s\n",strHeader.c_str(),__FILE__,__LINE__,__func__);
		return "";
	}
//	printf("GetURL:%d before while\n",__LINE__);

	int iPos = 0;
	while(!IsSpace(strHeader[iPos]))
 		iPos ++; // find first blank space

	while(IsSpace(strHeader[iPos]))
 		iPos ++; // skip blank space
	
//	printf("GetURL:%d after while\n",__LINE__);
	std::string strUrl(1024,'\0');	
	int iLen = 0;
	while(!IsSpace(strHeader[iPos]))
	{
		strUrl[iLen++] = strHeader[iPos];
 		iPos ++; // skip not blank character
	}
	strUrl[iLen] = '\0';
	return strUrl;
}
std::string get_oneline(int iSocket,bool & bError)
{	
	int iRead = 0;
    char c='\0';
	int i= 0;
//	printf("get_oneline开始获取http头一行数据...\n");
	char * pData = (char*)malloc(1024);
	if(pData==NULL)
	{
		bError = true;
		fprintf(stderr,"file:%s,line:%d,function:%s,malloc memory 1024 failed!\n",__FILE__,__LINE__,__func__);
		return "";
	}
	bError = false;
	while(c!='\n')
	{
		iRead = recv(iSocket,&c,1,0);
		pData[i] = c;
		if(iRead >0 && c == '\r')
		{
			iRead = recv(iSocket,&c,1,MSG_PEEK);
			if(iRead >0 && c == '\n')
				iRead = recv(iSocket,&c,1,0);
			else
				c = '\n';
					
			pData[i] = '\0';
			break;				
		}
		else if(iRead ==-1)
		{
			printf("get_oneline ,recv failed :%s,retun:%d\n",strerror(errno),iRead);
			bError = true;
			break;
		}
		else if(iRead == 0)
		{
			printf("the connection is closed by client!\n");
			bError = true;
			break;
		}	
		i++;	
	}
	if(bError)
		return "";

//	printf("get_oneline获取http头一行数据结束！\n");
	std::string str = pData;
	free(pData);
	return str;
}
bool IsKeepAlive(const std::string & strHeader)
{
	//Connection: Keep-Alive
	char * pConnetion = strstr((char*)strHeader.c_str(),"Connection:");
	if(pConnetion)
	{
		char szConnection[15];
		sscanf(pConnetion,"Connection: %s",szConnection);
		if(strcasecmp(szConnection,"Keep-Alive")==0)
			return true;
	}
	return false;
}

void sendheaders(int client, const char */*filename*/,const char *pszFileType,long long iStreamLen,long long iRangeBegin,long long iRangeEnd) {
    //先返回文件头部信息
	printf("sendheaders开始发送http响应头\n");
    char buf[1024];
	bool bIsPartial = false;
    if(iStreamLen >0 &&  (iRangeEnd - iRangeBegin) != iStreamLen)
		bIsPartial = true;
	if(!bIsPartial)
		strcpy(buf, "HTTP/1.0 200 OK\r\n");
	else
	{
		strcpy(buf,"HTT/1.0 206 Partial Content\r\n");
		printf("http response code 206:partial Content\n");
	}
    int iRev = send(client, buf, strlen(buf), 0);
	if(iRev < 0)
	{
		printf("send failed return:%d,error:%s\n",iRev,strerror(errno));
		return;
	}
    strcpy(buf, SERVER_STRING);
   	iRev =  send(client, buf, strlen(buf), 0);
	if(iRev < 0)
	{
		printf("send failed return:%d,error:%s\n",iRev,strerror(errno));
		return;
	}
    sprintf(buf, "Content-Type: %s\r\n",pszFileType);
    iRev = send(client, buf, strlen(buf), 0);
	if(iRev < 0)
	{
		printf("send failed return:%d,error:%s\n",iRev,strerror(errno));
		return;
	}
	
//	std::string strTypeStream = "application/octet-stream";
	sprintf(buf,"Content-Length: %lld\r\n",iStreamLen);
	iRev = send(client,buf,strlen(buf),0);
	if(iRev < 0)
	{
		printf("send failed return:%d,error:%s\n",iRev,strerror(errno));
		return;
	}
	strcpy(buf,"Accept-Ranges: bytes\r\n");
	iRev =send(client,buf,strlen(buf),0);
	if(iRev < 0)
	{
		printf("send failed return:%d,error:%s\n",iRev,strerror(errno));
		return;
	}
	strcpy(buf,"ETag: \"2f38a6cac7cec51:160c\"\r\n");
	iRev = send(client,buf,strlen(buf),0);
	if(iRev < 0)
	{
		printf("send failed return:%d,error:%s\n",iRev,strerror(errno));
		return;
	}
	if(bIsPartial)
	{
		sprintf(buf,"Content-Range: bytes %lld-%lld/%lld",iRangeBegin,iRangeEnd,iStreamLen);	
    	iRev = send(client,buf,strlen(buf),0);
		if(iRev < 0)
		{
			printf("send failed return:%d,error:%s\n",iRev,strerror(errno));
			return;
		}
		printf("%s\n",buf);
	}
    strcpy(buf, "\r\n");
 	iRev = send(client, buf, strlen(buf), 0);
	if(iRev < 0)
	{
		printf("send failed return:%d,error:%s\n",iRev,strerror(errno));
		return;
	}
	printf("发送http响应头完毕!\n");
}
std::string GetResponseHeader( const char */*filename*/,const char *pszFileType,long long iStreamLen,long long iRangeBegin,long long iRangeEnd)
{
    char buf[1024];
	bool bIsPartial = false;
    if(iStreamLen >0 && iRangeBegin >=0 && iRangeEnd >=0 && (iRangeEnd - iRangeBegin) != iStreamLen)
		bIsPartial = true;
	if(!bIsPartial)
		strcpy(buf, "HTTP/1.0 200 OK\r\n");
	else
	{
		strcpy(buf,"HTT/1.0 206 Partial Content\r\n");
	}
	std::string strHeader = buf;
    strHeader += SERVER_STRING;

	sprintf(buf, "Content-Type: %s;charset=utf-8\r\n",pszFileType);
	strHeader += buf;

	sprintf(buf,"Content-Length: %lld\r\n",iStreamLen);
	strHeader += buf;

	strHeader += "Accept-Ranges: bytes\r\n";

//	strcpy(buf,"ETag: \"2f38a6cac7cec51:160c\"\r\n");
//	strHeader += buf;
	if(bIsPartial)
	{
		sprintf(buf,"Content-Range: bytes %lld-%lld/%lld",iRangeBegin,iRangeEnd,iStreamLen);	
		strHeader += buf;
	}
	strHeader +="\r\n";
	return strHeader;
}
void not_found(int client) {
    //返回http 404协议
    char buf[1024];
    //先发送http协议头
    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    //再发送serverName
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    //再发送Content-Type
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    //发送换行符
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    //发送html主体内容
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

std::string get_404_ResponseHeader(std::string & strPage404Data,const char * uri)
{
    //返回http 404协议
	std::string strHeader = "HTTP/1.0 404 NOT FOUND\r\n";
	strHeader += SERVER_STRING;
	strHeader +="Content-Type: text/html\r\n";
	strHeader +="\r\n";
    
	//html主体内容
	strPage404Data = "<HTML><TITLE>Not Found</TITLE>\r\n";
	strPage404Data +="<BODY><P>请求的页面[" + std::string(uri);
	strPage404Data +="]无法找到\r\n";
	strPage404Data +="</BODY></HTML>\r\n";
	return strHeader;
}


