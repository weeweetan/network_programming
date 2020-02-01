//
// Created by 11135 on 2020/2/1.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define BUF_SIZE 1024
void error_handling(char *message);

typedef struct {
    u_int version:8;
    u_int reserved:24;
    u_int fcc_client_port:16;
    u_int multicast_port:16;
    u_int multicast_address:32;
    u_int stb_id:32;
}fcc;

int main(int argc, char *argv[]) {
    fcc client = {1, 0, 1024, 2048, 4280221697, 0};
    int sock;
    struct sockaddr_in serv_addr, from_adr;
    char message[BUF_SIZE];
    ssize_t str_len=0;
    socklen_t adr_sz;

    if (argc != 3) {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        error_handling("socket() error");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));


    size_t len = sizeof(client);
    sendto(sock, &client, sizeof(client), 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    adr_sz = sizeof(from_adr);
    str_len=recvfrom(sock, message, BUF_SIZE, 0, (struct sockaddr*)&from_adr, &adr_sz);
    message[str_len]=0;
    printf("Message from server: %s\n", message);

    close(sock);
    return 0;
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}