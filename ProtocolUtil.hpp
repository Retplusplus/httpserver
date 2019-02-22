#ifndef __PROTOCOLUTIL_HPP__
#define __PROTOCOLUTIL_HPP__

#include<iostream>
#include<stdlib.h>
#include<algorithm>
#include<string>
#include<vector>
#include<unordered_map>
#include<sstream>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/stat.h>
#include<sys/sendfile.h>
#include<fcntl.h>
#include<sys/sendfile.h>
#include<sys/wait.h>

#define BACKLOG 5
#define BUFF_NUM 1024

#define NORMAL 0
#define WARNING 1
#define ERROR 2

#define WEBROOT "wwwroot"
#define HOMEPAGE "index.html"
#define HTML_404 "wwwroot/404.html"

const char *ErrLevel[]={
    "Normal",
    "Warning",
    "Error",
};

void log(std::string msg,int level,std::string file,int line)
{
    std::cout<<"["<<file<<":"<<line<<"]"<<msg<<"["<<ErrLevel[level]<<"]"<<std::endl;
}

#define LOG(msg,level) log(msg,level,__FILE__,__LINE__);

class Util{
    public:
	static void MakeKV(std::string s,std::string &k,std::string &v)
	{
	    std::size_t pos=s.find(": ");
	    k=s.substr(0,pos);
	    v=s.substr(pos+2);
	}
	static std::string IntToString(int &x)
	{
	    std::stringstream ss;
	    ss<<x;
	    return ss.str();
	}
	static std::string CodeToDesc(int code)
	{
	    switch(code)
	    {
		case 200:
		    return "ok";
		case 400:
		    return "bad requset";
		case 404:
		    return "Not Found";
		case 500:
		    return "server err";
		default:
		    break;
	    }
	    return "Unknow";
	}
	
	static std::string CodeToExceptFile(int code)
	{
	    std::cout<<"codetoexceptfile:"<<code<<std::endl;
	    switch(code)
	    {
		case 404:
		    return HTML_404;
		case 500:
		    return HTML_404;
		case 400:
		    return HTML_404;
		default:
		    return " ";
	    }
	}
	static std::string SuffixToContent(std::string &suffix)
	{
	    if(suffix==".css")
	    {
		return "text/";
	    }
	    if(suffix==".js")
	    {
		return "application/x-javascript";
	    }
	    if(suffix==".html")
	    {
		return "text/html";
	    }
	    if(suffix==".jpg")
	    {
		return "application/x-jpg";
	    }
	    return "text/html";
	}
	static int FileSize(std::string &path)
	{
	    struct stat st;
	    stat(path.c_str(),&st);
	    return st.st_size;
	}
};

class SocketApi{
    public:
	static int Socket()
	{
	    int sock=socket(AF_INET,SOCK_STREAM,0);
	    if(sock<0)
	    {
		LOG("socket error!",ERROR);
		exit(2);
	    }
	    int  opt=1;
	    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));//经close后设置为重用该socket
	    return sock;
	}
	static void Bind(int sock,int port)
	{
	    struct sockaddr_in local;//定义一个结构体：1地址类型，2端口，3ip地址
	    bzero(&local,sizeof(local));//将local字符串的数据清零
	    local.sin_family=AF_INET;//用iPv4
	    local.sin_port=htons(port);//2个字节整形主机字节序转化为网络字节序
	    local.sin_addr.s_addr=htonl(INADDR_ANY);//转化为网络字节序
	    
	    if(bind(sock,(struct sockaddr*)&local,sizeof(local))<0)
	    {
		LOG("bind error!",ERROR);
		exit(3);
	    }
	}
	static void Listen(int sock)
	{
	    if(listen(sock,BACKLOG)<0)
	    {
		LOG("listen error",ERROR);
		exit(4);
	    }
	}
	static int Accept(int listen_sock,std::string &ip,int &port)//连接			
	{
	    struct sockaddr_in peer;
	    socklen_t len=sizeof(peer);
	    int sock=accept(listen_sock,(struct sockaddr*)&peer,&len);
	    if(sock<0)
	    {
		LOG("accept error",WARNING);
		return -1;
	    }
	    port=ntohs(peer.sin_port);
	    ip=inet_ntoa(peer.sin_addr);//转化为.分开的十进制ip地址
	    return sock;
	}

};

class Http_Response{
    public:
	std::string status_line;
	std::vector<std::string> response_header;
	std::string blank;
	std::string response_text;
    private:
	int code;
	std::string path;
	int recource_size;
    public:
	Http_Response():blank("\r\n"),code(200),recource_size(0)
	{}
	int &Code()
	{
	    return code;
	}
	void SetPath(std::string &path_)
	{
	    path=path_;
	}
	std::string &Path()
	{
	    return path;
	}
	void SetRecourceSize(int rs)
	{
	    recource_size=rs;
	}
	int RecourceSize()
	{
	    return recource_size;
	}
	void MakeStatusLine()
	{
	    status_line="HTTP/1.0 ";
	    status_line+=" ";
	    status_line+=Util::IntToString(code);
	    status_line+=" ";
	    status_line+=Util::CodeToDesc(code);
	    status_line+="\r\n";
	    LOG("make status line done!",NORMAL);
	}
	void MakeResponseHeader()
	{
	    std::string line;
	    std::string suffix;
	    line="Content-Type: ";
	    std::size_t pos=path.rfind('.');
	    if(std::string::npos!=pos)
	    {
		suffix=path.substr(pos);
		transform(suffix.begin(),suffix.end(),suffix.begin(),::tolower);
	    }
	    line+=Util::SuffixToContent(suffix);
	    line+="\r\n";
	    line="Content-Length: ";
	    line+=Util::IntToString(recource_size);
	    line+="\r\n";
	    line+="\r\n";
	    response_header.push_back(line);
	    line="\r\n";
	    response_header.push_back(line);
	    LOG("make response header done!",NORMAL);
	}
	~Http_Response()
	{}
};

class Http_Request{
    public:
	std::string request_line;
	std::vector<std::string> request_header;
	std::string blank;
	std::string request_text;
    private:
	std::string method;
	std::string uri;
	std::string version;
	std::string path;
//	int recource_size;
	std::string query_string;
	std::unordered_map<std::string,std::string> header_kv;
	bool cgi;
    public:
	Http_Request():path(WEBROOT),cgi(false),blank("\r\n")
	{}
    	void RequestLineParse()//将 GET /a/b/c http/1.1以空格分成三份分别存入
	{
	    std::stringstream ss(request_line);
	    ss>>method>>uri>>version;
	    std::cout<<"**********"<<std::endl;
	    std::cout<<method<<std::endl;
	    std::cout<<uri<<std::endl;
	    std::cout<<version<<std::endl;
	    transform(method.begin(),method.end(),method.begin(),::toupper);
	}
	void UriParse()//对数据进行分析，是否带有参数
	{
	    if(method=="GET")
	    {
		std::size_t pos=uri.find('?');
		if(pos!=std::string::npos)
		{
		    path+=uri.substr(0,pos);
		    query_string=uri.substr(pos+1);
		}
		else
		{
		    path+=uri;
		}				
	    }
	    else
	    {
		path+=uri;
	    }
	    if(path[path.size()-1]=='/')
	    {
		path+=HOMEPAGE;
	    }
	}
	void HeaderParse()
	{
	    std::string k,v;
	    for(auto it=request_header.begin();it!=request_header.end();it++)
	    {
		Util::MakeKV(*it,k,v);
		header_kv.insert({k,v});
	    }
	}
	bool IsMethodLegal()
	{
	    std::cout<<"method: "<<method<<std::endl;
	    if(method!="GET"&&method!="POST")
	    {
		return false;
	    }
	    return true;
	}
	int IsPathLegal(Http_Response *rsp)
	{
	    int rs=0;
	    struct stat st;
	    if(stat(path.c_str(),&st)<0)
	    {
		return 404;
	    }
	    else
	    {
		rs=st.st_size;

		if(S_ISDIR(st.st_mode)){
		    path+="/";
		    path+=HOMEPAGE;
		    stat(path.c_str(),&st);
		    rs=st.st_size;
		}
		else if((st.st_mode&S_IXUSR)||\
			(st.st_mode&S_IXGRP)||\
			(st.st_mode&S_IXOTH))
		{
		    cgi=true;
		}
		else
		{
		    //TODO
	    	}
	    }
	    rsp->SetPath(path);
	    rsp->SetRecourceSize(rs);
	    LOG("Path is ok!",NORMAL);
	    return 200;
	}
	bool IsNeedRecv()
	{
	    return method=="POST"?true:false;
	}
	bool IsCgi()
	{
	    return cgi;
	}
	int ContentLength()
	{
	    int content_length=-1;
	    std::string cl=header_kv["Content-Length"];
	    std::stringstream ss(cl);
	    ss>>content_length;
	    return content_length;
	}
	std::string GetParam()
	{
	    if(method=="GET")
	    {
		return query_string;
	    }
	    else
	    {
		return request_text;
	    }
	}
	~Http_Request()
	{}
};

class Connect{
    private:
	int sock;
    public:
	Connect(int sock_):sock(sock_)
	{}
	int RecvOneLine(std::string &line_)//对数据报每行抓取
	{
	    char buff[BUFF_NUM];
	    int i=0;
	    char c='x';
	    while(c!='\n'&&i<BUFF_NUM-1)
	    {
		ssize_t s=recv(sock,&c,1,0);
		if(s>0)
		{
		    if(c=='\r')
		    {
			recv(sock,&c,1,MSG_PEEK);
			if(c=='\n')
    			{
			    recv(sock,&c,1,0);
		    	}
			else
			{
			    c='\n';
    			}
		    }
		    buff[i++]=c;
		}
		else
		{
		    break;
		}
	    }
	    buff[i]=0;
	    line_=buff;
	    return i;
	}
	void RecvRequestHeader(std::vector<std::string>& v)
	{
	    std::string line="x";
	    while(line!="\n")
	    {
		RecvOneLine(line);
		if(line!="\n")
		{
		    v.push_back(line);
		}
	    }
	    LOG("Header Recv is ok!",NORMAL);
	}
		
	void RecvText(std::string &text,int content_length)
	{
	    char c;
	    for(auto i=0;i<content_length;i++)
	    {
		recv(sock,&c,1,0);
		text.push_back(c);
	    }
	}
	void SendStatusLine(Http_Response *rsp)
	{
	    std::string &sl=rsp->status_line;
	    send(sock,sl.c_str(),sl.size(),0);
	}
	void SendHeader(Http_Response *rsp)
	{
	    std::vector<std::string> &v=rsp->response_header;
	    for(auto it=v.begin();it!=v.end();it++)
	    {
		send(sock,it->c_str(),it->size(),0);
	    }
	}
	void SendText(Http_Response *rsp,bool cgi_)
	{
	    if(!cgi_)
	    {
		std::string &path=rsp->Path();
		std::cout<<"&path:"<<path.c_str()<<std::endl;
		int fd=open(path.c_str(),O_RDONLY);
		//int fd=open("text/html",O_RDONLY);
		if(fd<0)
		{
		    LOG("open file error!",WARNING);
		    return;
		}
		sendfile(sock,fd,NULL,rsp->RecourceSize());
		close(fd);
	    }
	    else
	    {
		std::string &rsp_text=rsp->response_text;
		send(sock,rsp_text.c_str(),rsp_text.size(),0);
	    }
	}
	void ClearRequest()
	{
	    std::string line;
	    while(line!="\n")
	    {
		RecvOneLine(line);
	    }
	}
	~Connect()
	{
	    close(sock);
	}
};
class Entry{
    public:
	static int ProcessCgi(Connect *conn,Http_Request *rq,Http_Response *rsp)
	{
	    int input[2];
	    int output[2];
	    pipe(input);
	    pipe(output);
	 
	    std::string bin=rsp->Path();
	    std::string param=rq->GetParam();
	    int size=param.size();
	    std::string param_size="Content-Length=";
	    param_size +=Util::IntToString(size);

	    std::string &response_text=rsp->response_text;

	    pid_t id=fork();
	    if(id<0)
	    {
		LOG("fork error!",WARNING);
		return 500;
		}
	    else if(id==0)//child
	    {
		close(input[1]);
		close(output[0]);
		putenv((char*)param_size.c_str());
		dup2(input[0],0);
		dup2(output[1],1);

		execl(bin.c_str(),bin.c_str(),NULL);
		exit(1);
	    }
	    else//father
	    {
		close(input[0]);
		close(output[1]);
		char c;
		for(auto i=0; i<size;i++)
		{
		    c=param[i];
		    write(input[1],&c,1);
		}
		waitpid(id,NULL,0);
		while(read(output[0],&c,1)>0)
		{
		    response_text.push_back(c);
		}

		rsp->MakeStatusLine();
		rsp->SetRecourceSize(response_text.size());
		rsp->MakeResponseHeader();

		conn->SendStatusLine(rsp);
		conn->SendHeader(rsp);
		conn->SendText(rsp,true);

	    }
	    return 200;
	}
	static int ProcessNonCgi(Connect *conn,Http_Request *rq,Http_Response *rsp)
	{
	    rsp->MakeStatusLine();
	    rsp->MakeResponseHeader();
	    //rsp->MakeResponseText(rq);

	    conn->SendStatusLine(rsp);
	    conn->SendHeader(rsp);
	    conn->SendText(rsp,false);
	    LOG("send response done!",NORMAL);
	    return 200;
	}
	static int ProcessResponse(Connect *conn,Http_Request *rq,Http_Response *rsp)
	{
	    if(rq->IsCgi())
	    {
		LOG("MakeResponse Use Cgi",NORMAL);
		return ProcessCgi(conn,rq,rsp);
	    }
	    else
	    {
		LOG("makeResponse use non cgi!",NORMAL);
		return ProcessNonCgi(conn,rq,rsp);
	    }
	}
	static void HandlerRequest(int sock)
	{
	    pthread_detach(pthread_self());
#ifdef _DEBUG_
	    char buf[10240];
	    read(sock,buf,sizeof(buf));
	    std::cout<<"#####################"<<std::endl;
	    std::cout<<buf<<std::endl;
	    std::cout<<"#####################"<<std::endl;
#else			
	    Connect *conn=new Connect(sock);
	    Http_Request *rq=new Http_Request();
	    Http_Response *rsp=new Http_Response();
	    int &code=rsp->Code();

	    conn->RecvOneLine(rq->request_line);
	    rq->RequestLineParse();
	    if(!rq->IsMethodLegal())
	    {
		code=400;
		conn->ClearRequest();
		LOG("Request Method is not Legal",WARNING);
		goto end;
	    }

	    rq->UriParse();

	    std::cout<<"**"<<code<<std::endl;
	    if((code=(rq->IsPathLegal(rsp)))!=200)
	    {
		std::cout<<"**"<<code<<std::endl;
		conn->ClearRequest();
		LOG("file is not exist!",WARNING);
		goto end;
	    }
	    conn->RecvRequestHeader(rq->request_header);
	    rq->HeaderParse();
	    if(rq->IsNeedRecv())
	    {
		LOG("POTH Method,need recv begin!",NORMAL);
		conn->RecvText(rq->request_text,rq->ContentLength());
	    }
	    LOG("Http request recv done,ok!",NORMAL);
	    code=ProcessResponse(conn,rq,rsp);
end:
	    if(code!=200)
	    {
		std::string except_path=Util::CodeToExceptFile(code);
		int rs=Util::FileSize(except_path);
		rsp->SetPath(except_path);
		rsp->SetRecourceSize(rs);
		ProcessNonCgi(conn,rq,rsp);
	    }
	    delete conn;
	    delete rq;
	    delete rsp;
#endif
	}
};
#endif
