//
// Created by tw on 2018/2/27.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Practical.h"

static const int MAXPENDING = 5;

int main(int argc, char *argv[])
{
    if (argc != 2)
        DieWithUserMessage("Prameter(s)", "<Server Port>");
    in_port_t  servPort = atoi(argv[1]);

    int servSock;
    if ((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithSystemMessage("socket() failed");

    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(servPort);

    if (bind(servSock, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0)
        DieWithSystemMessage("bind() failed");
    if (listen(servSock, MAXPENDING) < 0)
        DieWithSystemMessage("listen() failed");

    for (;;) {
        struct sockaddr_in clntAddr;
        socklen_t clntAddrLen = sizeof(clntAddr);
        int clntSock=accept(servSock, (struct sockaddr*)&clntAddr, &clntAddrLen);
        if (clntSock < 0)
            DieWithSystemMessage("accept() failed");

        char clntName[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &clntAddr.sin_addr.s_addr, clntName, sizeof(clntName)) != NULL)
            printf("Handling client %s/%d\n", clntName, ntohs(clntAddr.sin_port));
        else
            puts("Unalbe to get client address");

        HandleTCPClient(clntSock);
    }

}
