CC = g++
OBJS = epoll_server.o http_request.o UrlCode.o httpCommon.o fileType.o redis_api.o	net_disk_core.o CodeFun.o \
		AccountRegister.o AccountLogin.o NetDiskFileList.o ModifyPassword.o TokenManager.o	

LIBS = -lhiredis -lmysqlcppconn	-ljsoncpp
FLAGS = -W -Wall -g
TARGET = s

$(TARGET) : $(OBJS)
	$(CC) $(FLAGS) $(OBJS) $(LIBS) -o $(TARGET)

epoll_server.o : epoll_server.cpp
	$(CC) $(FLAGS) -c epoll_server.cpp

http_request.o : http_request.cpp http_request.h
	$(CC) $(FLAGS) -c http_request.cpp

httpCommon.o : httpCommon.cpp httpCommon.h
	$(CC) $(FLAGS) -c httpCommon.cpp

UrlCode.o : UrlCode.cpp UrlCode.h
	$(CC) $(FLAGS) -c UrlCode.cpp

fileType.o : fileType.cpp
	$(CC) $(FLAGS) -c fileType.cpp

redis_api.o : redis_api.cpp redis_api.h
	$(CC) $(FLAGS) -c redis_api.cpp

net_disk_core.o : net_disk_core.cpp net_disk_core.h
	$(CC) $(FLAGS) -c net_disk_core.cpp

CodeFun.o : CodeFun.cpp CodeFun.h
	$(CC) $(FLAGS) -c CodeFun.cpp

AccountRegister.o : AccountRegister.cpp
	$(CC) $(FLAGS) -c AccountRegister.cpp

AccountLogin.o : AccountLogin.cpp
	$(CC) $(FLAGS) -c AccountLogin.cpp

NetDiskFileList.o : NetDiskFileList.cpp
	$(CC) $(FLAGS) -c NetDiskFileList.cpp

ModifyPassword.o : ModifyPassword.cpp
	$(CC) $(FLAGS) -c ModifyPassword.cpp

TokenManager.o : TokenManager.cpp
	$(CC) $(FLAGS) -c TokenManager.cpp

clean:
	rm $(OBJS) $(TARGET)
