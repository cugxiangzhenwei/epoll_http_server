CC = g++
OBJS = epoll_server.o http_request.o
FLAGS = -W -Wall -g
TARGET = s

$(TARGET) : $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o $(TARGET)

epoll_server.o : epoll_server.cpp
	$(CC) $(FLAGS) -c epoll_server.cpp

http_request.o : http_request.cpp http_request.h
	$(CC) $(FLAGS) -c http_request.cpp

clean:
	rm $(OBJS) $(TARGET)
