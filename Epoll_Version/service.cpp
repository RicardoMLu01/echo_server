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

void epoll_et(int listenfd);
void epoll_lt(int listenfd);
void add_fd(int epollfd, int fd, bool enable_et);

constexpr int PORT = 2345;
constexpr int BUFMAXLEN = 4;
constexpr int EPOLL_SIZE = 128;

int client[FD_SETSIZE];
struct epoll_event ep_events[EPOLL_SIZE];
char recv_buf[BUFMAXLEN];

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
    
    std::cout<<"server start... "<<std::endl;
    //epoll_lt(sockfd);
    epoll_et(sockfd);
    
    close(sockfd);
    return 0;
}


void epoll_et(int listenfd)
{
    int epollfd, i, event_num, connfd, recv_bytes, write_bytes;
    struct epoll_event event;
    struct sockaddr_in client_addr;
    socklen_t sock_len = sizeof(sockaddr_in);

    if((epollfd = epoll_create(EPOLL_SIZE)) <0)
    {
        perror("epoll_create failed");
        exit(EXIT_FAILURE);
    }
    add_fd(epollfd, listenfd, true);

    while(1)
    {
        if((event_num = epoll_wait(epollfd, ep_events, EPOLL_SIZE, -1)) < 0)
        {
            perror("epoll_wait failed");
            exit(EXIT_FAILURE);
        }
        /*遍历返回的就绪描述符集*/
        for(i = 0; i < event_num; i++)
        {
            int sock = ep_events[i].data.fd;
            /*新的客户端连接*/
            if(sock == listenfd)
            {
                if((connfd = accept(listenfd,  (struct sockaddr *)(&client_addr), &sock_len)) < 0)
                {
                    perror("accept failed");
                    exit(EXIT_FAILURE);
                }
                add_fd(epollfd, connfd, true);
                std::cout<<"accept client ip: "<<inet_ntoa(client_addr.sin_addr)<<" on socket: "<<connfd<<std::endl;
            }
            /*描述符就绪*/
            else if(ep_events[i].events & EPOLLIN)
            {
                std::cout<<"listen event on sockfd:"<<ep_events[i].data.fd<<std::endl;
                /*循环读取数据*/
                while(1)
                {
                    memset(recv_buf, 0, BUFMAXLEN);
                    recv_bytes = read(sock, recv_buf, BUFMAXLEN);
                    if(recv_bytes == 0)
                    {
                        epoll_ctl(epollfd, EPOLL_CTL_DEL, sock, ep_events);
                        close(sock);
                        std::cout<<"sockfd "<<sock<<" is closed"<<std::endl;
                        break;
                    }
                    /*已读完*/
                    else if((recv_bytes < 0) && (errno == EAGAIN))
                    {
                        std::cout<<"read finished on sockfd:"<<ep_events[i].data.fd<<std::endl;
                        break;
                    }
                    else
                        write_bytes = write(sock, recv_buf, recv_bytes);
                }
            }
        }
    }
}


void add_fd(int epollfd, int fd, bool enable_et)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    if(enable_et)
        event.events |= EPOLLET;
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event) < 0)
    {
        perror("epoll_ctl failed");
        exit(EXIT_FAILURE);
    }
	/*设置描述符为非阻塞*/
    int op = fcntl(fd, F_GETFL);
    op = op | O_NONBLOCK;
    fcntl(fd, F_SETFL, op);
}

void epoll_lt(int listenfd)
{
    int epollfd, i, event_num, connfd, recv_bytes;
    struct epoll_event event;
    struct sockaddr_in client_addr;
    socklen_t sock_len = sizeof(sockaddr_in);

    if((epollfd = epoll_create(EPOLL_SIZE)) <0)
    {
        perror("epoll_create failed");
        exit(EXIT_FAILURE);
    }
    add_fd(epollfd, listenfd, false);

    while(1)
    {
        if((event_num = epoll_wait(epollfd, ep_events, EPOLL_SIZE, -1)) < 0)
        {
            perror("epoll_wait failed");
            exit(EXIT_FAILURE);
        }
        /*遍历返回的就绪描述符集*/
        for(i = 0; i < event_num; i++)
        {
            int sock = ep_events[i].data.fd;
            /*新的客户端连接*/
            if(sock == listenfd)
            {
                if((connfd = accept(listenfd,  (struct sockaddr *)(&client_addr), &sock_len)) < 0)
                {
                    perror("accept failed");
                    exit(EXIT_FAILURE);
                }
                add_fd(epollfd, connfd, false);
                std::cout<<"accept client ip: "<<inet_ntoa(client_addr.sin_addr)<<" on socket: "<<connfd<<std::endl;
            }
             /*描述符就绪*/
            else if(ep_events[i].events & EPOLLIN)
            {
                std::cout<<"listen event on sockfd:"<<ep_events[i].data.fd<<std::endl;
                memset(recv_buf, 0, BUFMAXLEN);
                if((recv_bytes = read(sock, recv_buf, BUFMAXLEN)) <= 0)
                {
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, sock, NULL);
                    close(sock);
                    std::cout<<"sockfd "<<sock<<" is closed"<<std::endl;
                }
                else
                    write(sock, recv_buf, recv_bytes);
            }
        }
    }
}
