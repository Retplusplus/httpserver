#ifndef __HTTPSERVER_HPP__
#define __HTTPSERVER_HPP__

#include<iostream>
#include<pthread.h>
#include"ProtocolUtil.hpp"
#include"ThreadPool.hpp"

class HttpServer{
    private:
	int listen_sock;
	int port;
    public:
	HttpServer(int port_)//构造函数
	    :port(port_),
	    listen_sock(-1)
	{}
	void InitServer()//初始化服务器
	{
	    listen_sock=SocketApi::Socket();//创建套接字
	    SocketApi::Bind(listen_sock,port);//绑定套接字
	    SocketApi::Listen(listen_sock);//监听
	}
	void Start()
	{
	    for(;;){
		std::string peer_ip;
		int peer_port;
		int sock=SocketApi::Accept(listen_sock,peer_ip,peer_port);
		if(sock>=0)
		{
		    std::cout<<peer_ip<<":"<<peer_port<<std::endl;
		    Task t(sock,Entry::HandlerRequest);
		    singleton::GetInstance()->PushTask(t);
		   // pthread_t tid;
		   // pthread_create(&tid,NULL,Entry::HandlerRequest,(void*)sockp);
		}
	    }
	}
	~HttpServer()
	{
	    if(listen_sock>=0)
	    {
		close(listen_sock);
	    }
	}
};

#endif
