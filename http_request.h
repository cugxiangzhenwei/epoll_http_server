#include<string>
#include<map>
#include"global_def.h"
#include"httpCommon.h"
typedef enum
{
	state_read_header = 0,
	state_parse_header,
	state_prepare_response,
	state_send_response,
	state_prepare_data,
	state_send_data,
	state_finish
}http_process_state;

struct http_Request
{
	//clinet information
	int m_fd; 
	char m_ClientIp[128]; // client ip address
	int  m_iClientPort; // client port
	int m_epollfd;
	http_process_state m_iState; // 请求当前所处的状态,根据当前状态执行对应的处理函数
	char   m_szURI[MAXSIZE]; // 请求的URI
	HTTP_METHOD m_http_method; //http请求类型
	std::string m_strReceiveHeaders;  //接收消息的htpp头
	std::string m_strResponseHeaders; //发送的http头
	char *m_szDataSend;
	int  m_iDataLength;
	// file paramters 
	FILE * m_pFileServe;
	long long m_iFileOffset;
	long long m_iReadbytes;
	long long m_iFinishedBytes;

	int read_header();
	int parse_header();
	int prepare_header();
	int send_header();
	int prepare_data(); // 准备实体数据，可能被多次调用
	int send_data(); // 发送实体数据，可能被多次调用
};

http_Request * find_request(int fd);
http_Request * add_request(int epollfd,int fd,const char * ip,int iport);
void remove_request(int fd);
