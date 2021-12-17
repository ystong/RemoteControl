#ifndef EPOLLS_H
#define EPOLLS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h>

#define IPADDRESS "192.168.3.20"
#define PORT 5566
#define MAXSIZE 1024
#define LISTENQ 5
#define FDSIZE 1000
#define EPOLLEVENTS 10

extern char crf[3];

//创建套接字并进行绑定
int socket_bind(const char* ip,int port);
//IO多路复用epoll
void do_epoll(int listenfd);
//事件处理函数
void
handle_events(int epollfd,struct epoll_event *events,int num,int listenfd,char *buf);
//处理接收到的连接
void handle_accpet(int epollfd,int listenfd);
//读字符串并执行控制命令
void do_read_ctl(int epollfd,int fd,char *buf);
//写处理
void do_write(int epollfd,int fd);
//添加事件
void add_event(int epollfd,int fd,int state);
//修改事件
void modify_event(int epollfd,int fd,int state);
//删除事件
void delete_event(int epollfd,int fd,int state);

#endif
