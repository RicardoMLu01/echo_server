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

using namespace std;

constexpr int PORT = 2345;
constexpr int BUFMAXLEN = 128;

int main(int argc, char * argv[])
{
    int sockfd;
    char send_buf[BUFMAXLEN] = {0};
    char recv_buf[BUFMAXLEN] = {0};
    struct sockaddr_in server_addr;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(sockfd, (struct sockaddr*)(&server_addr), sizeof(struct sockaddr_in)) == -1)
    {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    int recv_bytes = 0, str_len, recv_cnt;
    while(1)
    {
        cout<<"please input..."<<endl;
        fgets(send_buf, sizeof(send_buf), stdin);
        if (!strcmp(send_buf, "q\n"))
            break;

        str_len= send(sockfd, send_buf, strlen(send_buf), 0);
        recv_bytes = 0;
		/*循环读取，将读到的数据依次存入recvBuf*/
        while(recv_bytes < str_len)
        {
            recv_cnt= recv(sockfd, &recv_buf[recv_bytes], 16, 0);
            recv_bytes += recv_cnt;
        }
        cout<<"receive "<<recv_bytes <<"bytes from server:"<<recv_buf<<endl;

        memset(send_buf, 0, sizeof(send_buf));
        memset(recv_buf, 0, sizeof(recv_buf));
    }
    close(sockfd);    
    return 0;
}

