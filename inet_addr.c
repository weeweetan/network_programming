//
// Created by Administrator on 2018/12/28.
//
#include <stdio.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    char *addr1="10.0.0.0";
    char *addr2="10.255.255.255";
    char *addr3="11.255.255.255";

    unsigned long conv_addr=inet_addr(addr1);
    if (conv_addr==INADDR_NONE) {
        printf("Error occurred! \n");
    } else {
        printf("Network ordered integer addr: %#lx \n", conv_addr);
        printf("Network ordered integer addr: %lu \n", conv_addr);
        printf("Host ordered integer addr: %u \n", ntohl(conv_addr));
    }

    conv_addr=inet_addr(addr2);
    if (conv_addr==INADDR_NONE) {
        printf("Error occurred! \n");
    } else {
        printf("Network ordered integer addr: %#lx \n", conv_addr);
        printf("Network ordered integer addr: %lu \n", conv_addr);
        printf("Host ordered integer addr: %u \n", ntohl(conv_addr));
    }

    conv_addr=inet_addr(addr3);
    if (conv_addr==INADDR_NONE) {
        printf("Error occurred! \n");
    } else {
        printf("Network ordered integer addr: %#lx \n", conv_addr);
        printf("Network ordered integer addr: %lu \n", conv_addr);
        printf("Host ordered integer addr: %u \n", ntohl(conv_addr));
    }

    return 0;
}
