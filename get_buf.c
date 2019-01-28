//
// Created by Administrator on 2019/1/2.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

void error_handling(char *message);

int main(int argc, char *argv[]) {
    int sock;
    socklen_t len;
    int snd_buf, rcv_buf, stat;

    len= sizeof(snd_buf);
    sock=socket(PF_INET, SOCK_STREAM, 0);

    stat=getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (void*)&snd_buf, &len);
    if (stat) {
        error_handling("getsockopt() error!");
    }

    len= sizeof(rcv_buf);
    stat=getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (void*)&rcv_buf, &len);
    if (stat) {
        error_handling("getsockopt() error!");
    }

    printf("Input buffer size: %d\n", rcv_buf);
    printf("Output buffer size: %d\n", snd_buf);
    close(sock);
    return 0;
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}