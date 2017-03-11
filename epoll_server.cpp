#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h>
#include"http_request.h"
//函数声明
//创建套接字并进行绑定
static int socket_bind(int port);
//IO多路复用epoll
static void do_epoll(int listenfd);
//事件处理函数
static void
handle_events(int epollfd,struct epoll_event *events,int num,int listenfd,char *buf);
//处理接收到的连接
static void handle_accpet(int epollfd,int listenfd);
//读处理
static void do_read(int epollfd,int fd,char *buf);
//写处理
static void do_write(int epollfd,int fd,char *buf);
//添加事件
static void add_event(int epollfd,int fd,int state);
//修改事件
static void modify_event(int epollfd,int fd,int state);
//删除事件
static void delete_event(int epollfd,int fd,int state);
void PrintHelp()
{
	printf("请指定端口号和工作路径！\n示例:\n./s.exe -p 5000 -d /Usr/xiangzhenwei\n");
}

int main(int argc,char *argv[])
{
 	if(argc <3)
	{
		PrintHelp();
		return 0;
	}
	int i=1;
	int iport = -1;
	std::string strHome = "";
	while(argv[i]!=NULL)
	{
		if(strcasecmp(argv[i],"-p")==0 && i < argc -1)
		{
			i++;
			iport =  atoi(argv[i]);
		}	
		else if(strcasecmp(argv[i],"-d")==0 && i < argc -1)
		{
			i++;
			strHome = argv[i];
		}
		else
			i++;
	}
	if(iport <0 || strHome.empty())
	{
		printf("端口号或工作目录参数不正确!\n");
		PrintHelp();
		return 0;
	}   int  listenfd;
    listenfd = socket_bind(iport);
   if(listen(listenfd,LISTENQ)==-1)
	{
		perror("listen failed :");
		exit(0);
	}
    do_epoll(listenfd);
    return 0;
}

static int socket_bind(int port)
{
    int  listenfd;
    struct sockaddr_in servaddr;
    listenfd = socket(AF_INET,SOCK_STREAM,0);
    if (listenfd == -1)
    {
        perror("socket error:");
        exit(1);
    }
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);
    if (bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1)
    {
        perror("bind error: ");
        exit(1);
    }
    return listenfd;
}

static void do_epoll(int listenfd)
{
    int epollfd;
    struct epoll_event events[EPOLLEVENTS];
    int ret;
    char buf[MAXSIZE];
    memset(buf,0,MAXSIZE);
    //创建一个描述符
    epollfd = epoll_create(FDSIZE);
    //添加监听描述符事件
    add_event(epollfd,listenfd,EPOLLIN);
    for ( ; ; )
    {
        //获取已经准备好的描述符事件
        ret = epoll_wait(epollfd,events,EPOLLEVENTS,-1);
        handle_events(epollfd,events,ret,listenfd,buf);
    }
    close(epollfd);
}

static void
handle_events(int epollfd,struct epoll_event *events,int num,int listenfd,char *buf)
{
    int i;
    int fd;
    //进行选好遍历
    for (i = 0;i < num;i++)
    {
        fd = events[i].data.fd;
        //根据描述符的类型和事件类型进行处理
        if ((fd == listenfd) &&(events[i].events & EPOLLIN))
            handle_accpet(epollfd,listenfd);
        else if (events[i].events & EPOLLIN)
            do_read(epollfd,fd,buf);
        else if (events[i].events & EPOLLOUT)
            do_write(epollfd,fd,buf);
    }
}
static void handle_accpet(int epollfd,int listenfd)
{
    int clifd;
    struct sockaddr_in cliaddr;
    socklen_t  cliaddrlen;
    clifd = accept(listenfd,(struct sockaddr*)&cliaddr,&cliaddrlen);
    if (clifd == -1)
        perror("accpet error:");
    else
    {
        printf("accept a new client: %s:%d\n",inet_ntoa(cliaddr.sin_addr),cliaddr.sin_port);
        //添加一个客户描述符和事件
		add_request(epollfd,clifd);
        add_event(epollfd,clifd,EPOLLIN);
    }
}

static void do_read(int epollfd,int fd,char *buf)
{
	http_Request * req = find_request(fd);
	if(!req)
	{
		fprintf(stderr,"erro ,find_request(%d) return NULL\n",fd);
		return;
	}
    // 每一步内部修改下一步该执行的状态，后面根据状态做出响应,如果状态没变，则下次进来继续执行该状态
	if(req->m_iState == state_read_header)
	{
		if(req->read_header()<=0)
		{
			remove_request(fd);
        	delete_event(epollfd,fd,EPOLLIN);
		}
	}
	if(req->m_iState == state_parse_header)
	{
		if(req->parse_header()<=0)
		{
			close(fd);
			remove_request(fd);
			delete_event(epollfd,fd,EPOLLIN);
		}
	}
	if(req->m_iState == state_prepare_response) //准备回应了
	{
		if(req->prepare_header()<=0)
		{
			close(fd);
			remove_request(fd);
			delete_event(epollfd,fd,EPOLLIN);
		}
	}
	if(req->m_iState == state_send_response)
	{
		modify_event(epollfd,fd,EPOLLOUT);	 // modify to write style ,send reponse
		if(req->send_header() <=0)
		{
			
			remove_request(fd);
			delete_event(epollfd,fd,EPOLLOUT);
		} 
	}
	if(req->m_iState == state_send_data)
	{
		if(req->send_data()<=0)
		{
			remove_request(fd);
			delete_event(epollfd,fd,EPOLLOUT);
		}
	}
	if(req->m_iState == state_finish)
	{
		close(fd);
		remove_request(fd);
		delete_event(epollfd,fd,EPOLLOUT);
		printf("%d finish request!\n",fd);
	}
	return;	
	int nread;
    nread = read(fd,buf,MAXSIZE);
    if (nread == -1)
    {
        perror("read error:");
        close(fd);
		remove_request(fd);
        delete_event(epollfd,fd,EPOLLIN);
    }
    else if (nread == 0)
    {
        fprintf(stderr,"client close.\n");
        close(fd);	
		remove_request(fd);
        delete_event(epollfd,fd,EPOLLIN);
    }
    else
    {
        printf("read message is : %s",buf);
        //修改描述符对应的事件，由读改为写
        modify_event(epollfd,fd,EPOLLOUT);
    }
}

static void do_write(int epollfd,int fd,char *buf)
{	
	http_Request * req = find_request(fd);
	if(!req)
	{
		fprintf(stderr,"erro ,find_request(%d) return NULL\n",fd);
		return;
	}
    if(req->m_iState == state_send_response)
	{
		req->send_header();
	}
	else if(req->m_iState == state_send_data)
	{
		req->send_data();
	}
	return;

    int nwrite;
    nwrite = write(fd,buf,strlen(buf));
    if (nwrite == -1)
    {
        perror("write error:");
        close(fd);
		remove_request(fd);
        delete_event(epollfd,fd,EPOLLOUT);
    }
    else
        modify_event(epollfd,fd,EPOLLIN);
    memset(buf,0,MAXSIZE);
}

static void add_event(int epollfd,int fd,int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev);
}

static void delete_event(int epollfd,int fd,int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,&ev);
}

static void modify_event(int epollfd,int fd,int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev);
}
