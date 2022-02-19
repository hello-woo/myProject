/*************************************************************************
	> File Name: http_conn.h
	> Author: 
	> Mail: 
	> Created Time: Sun 20 Feb 2022 03:34:06 AM CST
 ************************************************************************/

#ifndef _HTTP_CONN_H
#define _HTTP_CONN_H
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include "locker.h"
#include <sys/uio.h>


class http_conn
{
private:
	int m_sockfd; //该HTTP连接的socket;
	sockaddr_in m_address; //客户端地址通信的socket地址
public:
	static int m_epollfd; //所有socket上的事件都被注册到同一个epoll对象中
	static int m_user_count; //统计用户数量

	http_conn(){}
	~http_conn(){}
	void process();//处理客户端请求

	void init(int sockfd,const sockaddr_in& addr);//初始化连接
	void close_conn();//关闭连接
	bool read();//非阻塞读
	bool write();//非阻塞写。
};


#endif
