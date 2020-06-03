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

constexpr int PORT = 2345;
constexpr int BUFMAXLEN = 128;

int main(int argc, char * argv[])
{
    int sockfd, connfd;
    char sendBuf[BUFMAXLEN] = {0};
    char recvBuf[BUFMAXLEN] = {0};
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

    socklen_t sock_len = sizeof(sockaddr_in);

    std::cout<<"server start... "<<std::endl;

    int recvBytes;
    while(1)
    {
        if((connfd = accept(sockfd,  (struct sockaddr *)(&client_addr), &sock_len)) < 0)
        {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }
        std::cout<<"accept client ip: "<<inet_ntoa(client_addr.sin_addr)<<std::endl;
		/*循环读取数据*/
        while((recvBytes = recv(connfd, recvBuf, BUFMAXLEN, 0)) != 0)
        {
            std::cout<<"receive "<<recvBytes<<"bytes from client:"<<recvBuf<<std::endl;
            write(connfd, recvBuf, recvBytes);
            memset(recvBuf, 0, sizeof(recvBuf));
        }
		/*返回值为0表示客户端已经断开连接*/
        if(!recvBytes)
            std::cout<<"sockfd "<<connfd<<" is closed"<<std::endl;
    }

    close(connfd);
    close(sockfd);
    return 0;
}
