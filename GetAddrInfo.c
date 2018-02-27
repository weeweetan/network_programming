//
// Created by tw on 2018/2/27.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include "Practical.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
        DieWithUserMessage("Parameter(s)", "<Address/Name> <Port/Service>");

    char *addrString = argv[1];
    char *portString = argv[2];

    struct addrinfo addrCriteria;
    memset(&addrCriteria, 0, sizeof(addrCriteria));
    addrCriteria.ai_family = AF_UNSPEC;
    addrCriteria.ai_socktype = SOCK_STREAM;
    addrCriteria.ai_protocol = IPPROTO_TCP;

    struct addrinfo *addrList;
    int rtlVal=getaddrinfo(addrString, portString, &addrCriteria, &addrList);
    if (rtlVal != 0)
        DieWithUserMessage("getaddrinfo() failed", gai_strerror(rtlVal));
    struct addrinfo *addr = addrList;
    while (addr!=NULL) {
        PrintSocketAddress(addr->ai_addr, stdout);
        fputc('\n', stdout);
        addr = addr->ai_next;
    }
    freeaddrinfo(addrList);
    exit(0);
}
