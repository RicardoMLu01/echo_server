#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>

void select_server(int listenfd);

constexpr int PORT = 2345;
constexpr int BUFMAXLEN = 128;
int client[FD_SETSIZE];
char sendBuf[BUFMAXLEN] = {0};
char recvBuf[BUFMAXLEN] = {0};

int main(int argc, char * argv[])
{
    int sockfd, connfd;

    struct sockaddr_in server_addr, client_addr;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if(bind(sockfd, (struct sockaddr*)(&server_addr), sizeof(struct sockaddr)) == -1)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    if(listen(sockfd, SOMAXCONN) == -1)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout<<"server start... "<<std::endl;
    select_server(sockfd);
    
    close(sockfd);
    return 0;
}



void select_server(int listenfd)
{
    int maxfd, connfd, max_client_index = 0, recv_bytes;
    struct sockaddr_in server_addr, client_addr;
    fd_set allset, fdset;
    size_t i;
    socklen_t sock_len = sizeof(sockaddr_in);

    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    maxfd = listenfd;
    
	/*初始化客户端套接字描述符数组*/
    for(i = 0; i < FD_SETSIZE; i++)
        client[i] = -1;

    while(1)
    {
        fdset = allset;  //每次都更新内核的监听描述符集
        if(select(maxfd + 1, &fdset, NULL, NULL, NULL) < 0)
        {
            perror("select failed");
            exit(EXIT_FAILURE);
        }
        
		/*select返回可能有两种情况*/
		/*1.监听描述符可读，有了新连接*/
        if(FD_ISSET(listenfd, &fdset))
        {
            if((connfd = accept(listenfd,  (struct sockaddr *)(&client_addr), &sock_len)) < 0)
            {
                perror("accept failed");
                exit(EXIT_FAILURE);
            }
            /*找到client中的空位子填进去*/
            for(i = 0; i < FD_SETSIZE; i++)
            {
                if(client[i] == -1)
                {
                    client[i] = connfd;
                    break;
                }
            }
            if(i == FD_SETSIZE)
            {
                std::cout<<"client array full"<<std::endl;
                exit(EXIT_FAILURE);
            }
            std::cout<<"accept client ip: "<<inet_ntoa(client_addr.sin_addr)<<" on socket: "<<connfd<<std::endl;
            /*更新索引信息和内核描述符集*/
            max_client_index = (i > max_client_index) ? i : max_client_index;
            FD_SET(connfd, &allset);
            maxfd = (connfd > maxfd) ? connfd : maxfd;  
        }
        
		/*2.client中的套接字描述符有可读事件*/
        for(i = 0; i <= max_client_index; i++)
        {
            if(client[i] == -1)  //未监听
                continue;
            if(FD_ISSET(client[i], &fdset))
            {
            	/*连接已断开*/
                if((recv_bytes = read(client[i], recvBuf, BUFMAXLEN)) == 0)
                {
                    close(client[i]);
                    FD_CLR(client[i], &allset);
                    std::cout<<"sockfd "<<client[i]<<" is closed"<<std::endl;
                    client[i] = -1;
                }
                else //回写
                    write(client[i], recvBuf, recv_bytes);                
            }
        }
    }
}

