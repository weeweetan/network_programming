#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>

#include <linux/ip.h>
#include <netinet/tcp.h>

#define MAXLINE 4096

int main(int argc, char** argv)
{
    int listenfd, connfd;
    struct sockaddr_in servaddr, client;
    socklen_t clen;
    char buff[4096];
    int n;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);

    }
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(6666);
    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        printf("bind socket error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);

    }


    struct tcp_md5sig cmd;
    inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
    memcpy(&cmd.tcpm_addr, &servaddr, sizeof(servaddr));
    cmd.tcpm_keylen = 5;
    memcpy(cmd.tcpm_key, "1234567890", 5);
    setsockopt(listenfd, SOL_TCP, TCP_MD5SIG,  &cmd, sizeof(cmd));

    if (listen(listenfd, 10) == -1) {
        printf("listen socket error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);

    }
    printf("======waiting for client's request======\n");
    while (1) {
        clen = sizeof(struct sockaddr);
        if ((connfd = accept(listenfd, (struct sockaddr*)&client, &clen)) == -1) {
            printf("accept socket error: %s(errno: %d)", strerror(errno), errno);
            continue;

        }

        n = recv(connfd, buff, MAXLINE, 0);
        buff[n] ='\0';
        printf("recv msg from client: %s\n", buff);
        close(connfd);

    }
    close(listenfd);
    return 0;

}
