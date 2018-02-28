//
// Created by tw on 2018/2/28.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include "Practical.h"

bool SockAddrsEqual(const struct sockaddr *addr1, const struct sockaddr *addr2)
{
    if (addr1 == NULL || addr2 == NULL)
        return addr1==addr2;
    else if (addr1->sa_family != addr2->sa_family)
        return  false;
    else if(addr1->sa_family == AF_INET)
    {
        struct sockaddr_in *ipv4Addr1 = (struct sockaddr_in *)addr1;
        struct sockaddr_in *ipv4Addr2 = (struct sockaddr_in *)addr2;
        return ipv4Addr1->sin_addr.s_addr == ipv4Addr2->sin_addr.s_addr &&
               ipv4Addr1->sin_port == ipv4Addr2->sin_port;

    }
    else if (addr1->sa_family == AF_INET6)
    {
        struct sockaddr_in6 *ipv6Addr1 = (struct sockaddr_in6 *)addr1;
        struct sockaddr_in6 *ipv6Addr2 = (struct sockaddr_in6 *)addr2;
        return memcmp(&ipv6Addr1->sin6_addr, &ipv6Addr2->sin6_addr, sizeof(struct in6_addr)) == 0
               && ipv6Addr1->sin6_port == ipv6Addr2->sin6_port;
    }
    else
        return false;
}



int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4)
        DieWithUserMessage("Parameter(s)", "<Server Address/Name> <Echo Word> [<Server Port/Service>]");
    char *server = argv[1];
    char *echoString = argv[2];

    size_t echoStringLen = strlen(echoString);
    if (echoStringLen > 1024)
        DieWithUserMessage(echoString, "string too long");

    char *servProt = (argc == 4) ? argv[3] : "echo";

    struct addrinfo addrCriteria;
    memset(&addrCriteria, 0, sizeof(addrCriteria));
    addrCriteria.ai_family = AF_UNSPEC;
    addrCriteria.ai_socktype = SOCK_DGRAM;
    addrCriteria.ai_protocol = IPPROTO_UDP;

    struct addrinfo *servAddr;
    int rtnVal = getaddrinfo(server, servProt, &addrCriteria, &servAddr);
    if (rtnVal != 0)
        DieWithUserMessage("getaddrinfo() failed", gai_strerror(rtnVal));

    int sock = socket(servAddr->ai_family, servAddr->ai_socktype, servAddr->ai_protocol);
    if (sock < 0)
        DieWithSystemMessage("socket() failed");

    ssize_t numBytes = sendto(sock, echoString, echoStringLen, 0, servAddr->ai_addr, servAddr->ai_addrlen);
    if (numBytes < 0)
        DieWithSystemMessage("sendto() failed");
    else if (numBytes != echoStringLen)
        DieWithUserMessage("sendto() error", "sent unexpected number of bytes");

    struct sockaddr_storage fromAddr;
    socklen_t  fromAddrLen = sizeof(fromAddr);
    char buffer[1024+1];
    numBytes = recvfrom(sock, buffer, 1024, 0, (
    struct sockaddr *)&fromAddr, &fromAddrLen);
    if (numBytes < 0)
        DieWithSystemMessage("recvfrom() failed");
    else if(numBytes != echoStringLen)
        DieWithUserMessage("recvfrom() error", "received unexpected number of bytes");
    if (!SockAddrsEqual(servAddr->ai_addr, (struct sockaddr *)&fromAddr))
        DieWithUserMessage("recvfrom()", "received a packet form unknown source");

    free(servAddr);
    buffer[echoStringLen] = '\0';
    printf("received: %s\n", buffer);
    close(sock);
    exit(0);

}
