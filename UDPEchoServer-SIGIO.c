//
// Created by tw on 2018/3/2.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "Practical.h"

void UseIdleTime();
void SIGIOHandler(int signalType);

int servSock;

int main(int argc, char *argv[]) {

    if (argc != 2)
        DieWithUserMessage("Parameter(s)", "<Server Port/Service>");
    char *service = argv[1];
    struct addrinfo addrCriteria;
    memset(&addrCriteria, 0, sizeof(addrCriteria));
    addrCriteria.ai_family = AF_UNSPEC;
    addrCriteria.ai_flags = AI_PASSIVE;
    addrCriteria.ai_socktype = SOCK_DGRAM;
    addrCriteria.ai_protocol = IPPROTO_UDP;

    struct addrinfo *servAddr;
    int rtnVal = getaddrinfo(NULL, service, &addrCriteria, &servAddr);
    if (rtnVal != 0)
        DieWithUserMessage("getaddrinfo() failed", gai_strerror(rtnVal));

    //Create socket for incoming connections
    servSock = socket(servAddr->ai_family, servAddr->ai_socktype, servAddr->ai_protocol);
    if (servSock < 0)
        DieWithSystemMessage("socket() failed");

    //Bind to the local address
    if (bind(servSock, servAddr->ai_addr, servAddr->ai_addrlen) < 0)
        DieWithSystemMessage("bind() failed");

    //Free address list allocated by getaddrinfo()
    freeaddrinfo(servAddr);

    struct sigaction handler;
    handler.sa_handler = SIGIOHandler;
    //Create mask that all signals
    if (sigfillset(&handler.sa_mask) < 0)
        DieWithSystemMessage("sigfillset() failed");
    handler.sa_flags = 0;

    if (sigaction(SIGIO, &handler, 0) < 0)
        DieWithSystemMessage("sigaction() failed for SIGNO");

    //we must own the socket to receive the SIGIO delivery
    if (fcntl(servSock, F_SETFL, O_NONBLOCK | FASYNC) < 0)
        DieWithSystemMessage("Unable to put clinet sock into non-blocking/async mode");

    for (;;)
        UseIdleTime();
}

void UseIdleTime() {
    puts(".");
    sleep(3);
}

void SIGIOHandler(int signalType) {
    ssize_t numByteRcvd;
    do {
        struct sockaddr_storage clntAddr;
        socklen_t clntLen = sizeof(clntAddr);
        char buffer[1024];
        numByteRcvd = recvfrom(servSock, buffer, 1024, 0,
                               (struct sockaddr *)&clntAddr, &clntLen);
        if (numByteRcvd < 0) {
            if (errno != EWOULDBLOCK)
                DieWithSystemMessage("recvfrom() failed");
        } else {
            fprintf(stdout, "Handling client ");
            PrintSocketAddress((
            struct sockaddr *)&clntAddr, stdout);
            fputc('\n', stdout);

            ssize_t numBytesSent = sendto(servSock, buffer, numByteRcvd, 0, (
            struct sockaddr *)&clntAddr, sizeof(clntAddr));
            if (numBytesSent < 0)
                DieWithSystemMessage("sendto() failed");
            else if (numBytesSent != numByteRcvd)
                DieWithUserMessage("sendto()", "sent unexpected number of bytes");
        }
    } while (numByteRcvd >= 0);
}
