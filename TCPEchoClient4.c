//
// Created by tw on 2018/2/27.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Practical.h"

int main(int argc, char *argv[])
{
    if (argc < 3||argc > 4)
        DieWithUserMessage("Parameter(s)", "<Server Address> <Echo Word> [<Server Port>]");
    char *servIP = argv[1];
    char *echoString = argv[2];

    in_port_t servPort = (argc == 4) ? atoi(argv[3]) : 7;

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0)
        DieWithSystemMessage("socket() failed");

    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;

    int rthVal = inet_pton(AF_INET, servIP, &servAddr.sin_addr.s_addr);
    if (rthVal == 0)
        DieWithUserMessage("inet_pton() failed", "invalid address string");
    else if (rthVal < 0)
        DieWithSystemMessage("inet_pton() failed");
    servAddr.sin_port = htons(servPort);

    if (connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
        DieWithSystemMessage("connect() failed");

    size_t echoStringLen = strlen(echoString);

    ssize_t numBytes = send(sock, echoString, echoStringLen, 0);
    if (numBytes < 0)
        DieWithSystemMessage("send() failed");
    else if (numBytes != echoStringLen)
        DieWithUserMessage("send()", "sent unexpected number of bytes");

    unsigned int totalBytesRcvd = 0;
    fputs("Received: ", stdout);
    while (totalBytesRcvd < echoStringLen) {
        char buffer[BUFSIZ];
        numBytes = recv(sock, buffer, BUFSIZ-1, 0);
        if (numBytes < 0)
            DieWithSystemMessage("recv() failed");
        else if (numBytes == 0)
            DieWithUserMessage("recv()", "connection cloese prematurely");
        totalBytesRcvd += numBytes;
        buffer[numBytes] = '\0';
        fputs(buffer, stdout);
    }
    fputs("\n", stdout);

    close(sock);
    exit(0);
}

