//
// Created by tw on 2018/2/27.
//
#include <sched.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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
