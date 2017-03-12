CC = g++
OBJS = epoll_server.o http_request.o UrlCode.o httpCommon.o fileType.o
FLAGS = -W -Wall -g
TARGET = s

$(TARGET) : $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o $(TARGET)

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

clean:
	rm $(OBJS) $(TARGET)
