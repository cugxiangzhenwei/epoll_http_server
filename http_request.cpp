#include"http_request.h"
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdio.h>
#include<errno.h>
#include<unistd.h>
typedef std::map<int,http_Request *> RequestMapType;
static RequestMapType g_requestMap;

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
	bzero(req->m_szURI,sizeof(char)*MAXSIZE);
	_add_request(req);
	return req;
}
void remove_request(int fd)
{
	RequestMapType::iterator iter = g_requestMap.find(fd);	
	if(iter==g_requestMap.end())
		return;
	delete iter->second;
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
	m_iState = state_prepare_response;
	return 1;
}
int http_Request::prepare_header()
{
	printf("prepare reponse header!\n");
	m_iState = state_send_response;
	return 1;
}
int http_Request::send_header()
{
	printf("send response!\n");
	m_iState = state_send_data;
	return 1;
}
int http_Request::send_data()
{
	printf("send data!\n");
	m_iState = state_finish;
	return 1;
}
