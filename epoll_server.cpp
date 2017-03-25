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
#include <fcntl.h>
#include"http_request.h"
#include"httpCommon.h"
#include"redis_api.h"
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

void clean_connection(int fd,int epollfd)
{
	close(fd);
	remove_request(fd);
	delete_event(epollfd,fd,EPOLLIN);
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
	int iMode = 0;
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
		else if(strcasecmp(argv[i],"-m")==0 && i < argc -1)
		{
			i++;
			iMode =  atoi(argv[i]);
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
	printf("server port:%d\n",iport);
    SetHomeDir(strHome.c_str());
	SetListMode(iMode);
	//忽略SIGPIPE信号的方法,避免socket关闭后，发送数据SIGPIPE信号导致进程退出
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;//设定接受到指定信号后的动作为忽略
	sa.sa_flags = 0;
	if (sigemptyset(&sa.sa_mask) == -1 || //初始化信号集为空
	sigaction(SIGPIPE, &sa, 0) == -1) { //屏蔽SIGPIPE信号
	perror("failed to ignore SIGPIPE; sigaction");
	exit(EXIT_FAILURE);
	}
	RedisAPI * redis = RedisAPI::Instance();
	redis->connect("127.0.0.1",6379,2000);
    listenfd = socket_bind(iport);
	//设置为非阻塞
	int x;  
	x=fcntl(listenfd,F_GETFL,0);  
	int s = fcntl(listenfd,F_SETFL,x | O_NONBLOCK);  
	if ( s == -1 )
	{
		printf("error to set socket:%d to nonblock,%s\n",listenfd,strerror(errno));
		abort ();
	}
    if(listen(listenfd,LISTENQ)==-1)
	{
		perror("listen failed :");
		exit(0);
	}
	int iProcessCount = 6;
	int a = 0;
	while((iProcessCount = iProcessCount/2)>0)
	{
		a++;
	}
	printf("fock %d times\n",a);
	for(int i=0;i<a;i++)
	{
		int iRev = fork();
		if(iRev ==-1)
		{
			printf("fork error :%s\n",strerror(errno));
		}
		else if(iRev ==0)
		{
			printf("sub process fock return!\n");
		}
		else 
		{
				printf("main process fork return!\n");
		}
	}
//	freopen("log.txt","w+",stdout);
    do_epoll(listenfd);
//	fclose(stdout);
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
    add_event(epollfd,listenfd,EPOLLIN|EPOLLET); //set Edge-triggled
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
//	printf("handle_events call ...,intput n=%d\n",num);
    //进行选好遍历
    for (i = 0;i < num;i++)
    {
        fd = events[i].data.fd;
        //根据描述符的类型和事件类型进行处理
			 if ( ( events[i].events & EPOLLERR ) ||( events[i].events & EPOLLHUP ) )
            {
                fprintf ( stderr, "epoll error\n" );
                close ( events[i].data.fd );
                continue;
            }
		
        if ((fd == listenfd) &&(events[i].events & EPOLLIN))
            handle_accpet(epollfd,listenfd);
        else if (events[i].events & EPOLLIN)
            do_read(epollfd,fd,buf);
        else if (events[i].events & EPOLLOUT)
            do_write(epollfd,fd,buf);
    }
//	printf("handle_events return!\n<----------------------------------------------------->\n");
}
static void handle_accpet(int epollfd,int listenfd)
{
	//  在ET模式下必须循环accept到返回-1为止
//	printf("handle_accpet call...\n");
	while(1)
	{
		int clifd;
		struct sockaddr_in cliaddr;
		socklen_t  cliaddrlen;
//		printf("begin accept...\n");
		clifd = accept(listenfd,(struct sockaddr*)&cliaddr,&cliaddrlen);
//		printf("end accept!\n");
		if (clifd == -1)
		{
			if((errno == EAGAIN) || (errno == EWOULDBLOCK))
			{
				//printf("end accept loop!\n");
				break;
			}
			else
			{
				perror("accpet error:");
				break;
			}
		}
		else
		{
			std::string ip = inet_ntoa(cliaddr.sin_addr);
			int iport = cliaddr.sin_port;
			printf("--------------------------->>>>>>>>>>>>\naccept a new client: %s:%d,current process id:%d\n",ip.c_str(),iport,getpid());
			//添加一个客户描述符和事件
			add_request(epollfd,clifd,ip.c_str(),iport);
			//设置为非阻塞
			int x;  
			x=fcntl(clifd,F_GETFL,0);  
			int s = fcntl(clifd,F_SETFL,x | O_NONBLOCK);  
			if ( s == -1 )
			{
				printf("error to set socket:%d to nonblock,%s\n",clifd,strerror(errno));
				abort ();
			}
			add_event(epollfd,clifd,EPOLLIN|EPOLLET);// set edge-triggled mode
			printf("当前连接数:%d\n",get_connectionCount());
		}
  }
//	printf("handle_accpet call finished!\n");
}
static void do_read(int epollfd,int fd,char *)
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
			return;
		}
	}
	if(req->m_iState == state_parse_header)
	{
		if(req->parse_header()<=0)
		{
			clean_connection(fd,epollfd);
			return;
		}
	}
	if(req->m_iState == state_read_body)
	{
		if(req->read_body()<=0)
		{
			clean_connection(fd,epollfd);
			return;
		}
	}
	if(req->m_iState == state_parse_body)
	{
		if(req->parse_body()<=0)
		{
			clean_connection(fd,epollfd);
			return;
		}
	}
	if(req->m_iState == state_prepare_response) //准备回应了
	{
		if(req->prepare_response()<=0)
		{
			clean_connection(fd,epollfd);
			return;
		}
	}
	if(req->m_iState == state_send_response)
		modify_event(epollfd,fd,EPOLLOUT|EPOLLET);	 // modify to write style ,send reponse
}

static void do_write(int epollfd,int fd,char *)
{	
	http_Request * req = find_request(fd);
	if(!req)
	{
		fprintf(stderr,"erro ,find_request(%d) return NULL\n",fd);
		return;
	}
	if(req->m_iState == state_send_response)
	{
		if(req->send_response() <=0)
		{
			clean_connection(fd,epollfd);
			return;
		} 
	}
	if(req->m_iState == state_send_data)
	{
		if(req->send_data()<=0)
		{
			clean_connection(fd,epollfd);
			return;
		}
	}
	if(req->m_iState == state_finish)
	{
		if(req->m_bKeeepAlive)
		{
			printf("%s:%d保持连接\n<<<<<<<<<<--------------------------------------\n",req->m_ClientIp,req->m_iClientPort);
			req->Reset();
			modify_event(epollfd,fd,EPOLLIN|EPOLLET);	 // modify to read style ,contine to receive header
		}
		else
		{
			printf("%s:%d关闭连接\n<<<<<<<<<<--------------------------------------\n",req->m_ClientIp,req->m_iClientPort);
			clean_connection(fd,epollfd);
		}
		printf("当前连接数:%d\n",get_connectionCount());
		return;
	}
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
