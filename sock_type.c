//
// Created by Administrator on 2019/1/2.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

void error_handling(char *message);

int main(int argc, char *argv[]) {
    int tcp_sock, udp_sock;
    int sock_type;
    socklen_t optlen;
    int stat;

    optlen= sizeof(sock_type);
    tcp_sock=socket(PF_INET, SOCK_STREAM, 0);
    udp_sock=socket(PF_INET, SOCK_DGRAM, 0);

    printf("SOCK_STREAM: %d\n", SOCK_STREAM);
    printf("SOCK_DGRAM: %d\n", SOCK_DGRAM);

    stat=getsockopt(tcp_sock, SOL_SOCKET, SO_TYPE, (void*)&sock_type, &optlen);
    if (stat) {
        error_handling("getsockopt() error!");
    }
    printf("Socket type one: %d\n", sock_type);
    stat=getsockopt(udp_sock, SOL_SOCKET, SO_TYPE, (void*)&sock_type, &optlen);
    if (stat) {
        error_handling("getsockopt() error!");
    }
    printf("Socket type two: %d\n", sock_type);
    close(tcp_sock);
    close(udp_sock);
    return 0;
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}