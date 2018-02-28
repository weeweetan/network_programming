//
// Created by tw on 2018/2/27.
//
#include <sched.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include "Practical.h"

void HandleTCPClient(int clntSocket)
{
    char buffer[1024];

    ssize_t numBytesRcvd = recv(clntSocket, buffer, 1024, 0);
    if (numBytesRcvd < 0)
        DieWithSystemMessage("recv() failed");

    while (numBytesRcvd > 0) {
        ssize_t numBytesSent = send(clntSocket, buffer, numBytesRcvd, 0);
        if (numBytesSent < 0)
            DieWithSystemMessage("send() failed");
        else if (numBytesRcvd != numBytesSent)
            DieWithUserMessage("send()", "sent unexpected number of bytes");
        numBytesRcvd = recv(clntSocket, buffer, 1024, 0);
        if (numBytesRcvd < 0)
            DieWithSystemMessage("recv() failed");
    }
    close(clntSocket);
}

void PrintSocketAddress(const struct sockaddr *address, FILE *strem)
{
    if (address == NULL || strem == NULL)
        return;
    void *numericAddress;
    char addrBUffer[INET6_ADDRSTRLEN];
    in_port_t port;

    switch (address->sa_family) {
        case AF_INET:
            numericAddress = &((struct sockaddr_in *)address)->sin_addr;
            port = ntohs(((
            struct sockaddr_in *)address)->sin_port);
            break;
        case AF_INET6:
            numericAddress = &((struct sockaddr_in6 *)address)->sin6_addr;
            port = ntohs(((
            struct sockaddr_in6 *)address)->sin6_port);
            break;
        default:
            fputs("[unknown type]", strem);
            return;
    }

    if (inet_ntop(address->sa_family, numericAddress, addrBUffer, sizeof(addrBUffer)) == NULL)
        fputs("[invalid address]", strem);
    else {
        fprintf(strem, "%s", addrBUffer);
        if (port != 0)
            fprintf(strem, "-%u", port);
    }
}


static const int MAXPENDING = 5;

int SetupTCPServerSocket(const char *service) {
    //Construct the server address structure
    struct addrinfo addrCriteria;
    memset(&addrCriteria, 0, sizeof(addrCriteria));
    addrCriteria.ai_family = AF_UNSPEC;
    addrCriteria.ai_flags = AI_PASSIVE;
    addrCriteria.ai_socktype = SOCK_STREAM;
    addrCriteria.ai_protocol = IPPROTO_TCP;

    struct addrinfo *servAddr;
    int rtnVal = getaddrinfo(NULL, service, &addrCriteria, &servAddr);
    if (rtnVal != 0)
        DieWithUserMessage("getaddrinfo() failed", gai_strerror(rtnVal));
    int servSock = -1;
    struct addrinfo *addr = servAddr;
    while (addr != NULL)
    {
        servSock = socket(servAddr->ai_family, servAddr->ai_socktype, servAddr->ai_protocol);
        if (servSock < 0){
            addr = addr->ai_next;
            continue;
        }

        if ((bind(servSock, servAddr->ai_addr, servAddr->ai_addrlen) == 0) && (listen(servSock, MAXPENDING) == 0))
        {
            struct sockaddr_storage localAddr;
            socklen_t addrSize = sizeof(localAddr);
            if (getsockname(servSock, (struct sockaddr *)&localAddr, &addrSize) < 0)
                DieWithSystemMessage("getsockname() failed");
            fputs("Binding to ", stdout);
            PrintSocketAddress((struct sockaddr *)&localAddr, stdout);
            fputc('\n', stdout);
            break;
        }
        close(servSock);
        servSock = -1;
        addr = addr->ai_next;
    }
    freeaddrinfo(servAddr);
    return servSock;

}

int AcceptTCPConnection(int servSock) {
    struct sockaddr_storage clntAddr;
    socklen_t clntAddrLen = sizeof(clntAddr);

    int clntSock = accept(servSock, (struct sockaddr *)&clntAddr, &clntAddrLen);
    if (clntSock < 0)
        DieWithSystemMessage("accept() failed");
    fputs("Handling client ", stdout);
    PrintSocketAddress((struct sockaddr *)&clntAddr, stdout);
    fputc('\n', stdout);

    return clntSock;
}