#include<string>
#include<map>
#include"global_def.h"
#include"httpCommon.h"
typedef enum
{
	state_read_header = 0,
	state_parse_header,
	state_read_body, // 例如读取http post 请求的body数据
	state_parse_body, //例如解析http post 请求的Body数据
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
	
	long long  m_iContent_Length; // http body 数据接收时，客户端指定的body数据大小
	std::string m_strBodyData; //http body 数据

	std::string m_strResponseHeaders; //发送的http头
	char *m_szDataSend;
	int  m_iDataLength;
	int  m_iSendCompleteLen;
	// file paramters 
	FILE * m_pFileServe;
	long long m_iFileOffset;
	long long m_iReadbytes;
	long long m_iFinishedBytes;

	int read_header();  // read http head line and headers data
	int parse_header(); // parse http method type and uri
	int read_body();// read body data ,eg post data
	int parse_body(); // parse post data ,eg josn\xml style format data

	int prepare_post_response(); // according to request URI ,prepare repsonse data and headers
	int prepare_get_response(); // accord to request url,get resource data and response
	int prepare_response(); // wille call prepare_post_response or prepare_get_response

	int send_response(); // send repsonse code data and headers to client
	int prepare_data(); // 准备实体数据，可能被多次调用
	int send_data(); // 发送实体数据，可能被多次调用
};

http_Request * find_request(int fd);
http_Request * add_request(int epollfd,int fd,const char * ip,int iport);
void remove_request(int fd);
