//
// Created by tw on 2018/2/26.
//
#include <stdio.h>
#include <sys/socket.h>
#include <stdbool.h>
#ifndef NETWORK_PROGRAMMING_PRACTICAL_H
#define NETWORK_PROGRAMMING_PRACTICAL_H

void DieWithUserMessage(const char *msg, const char *detail);
void DieWithSystemMessage(const char *msg);
void HandleTCPClient(int clntSocket);
void PrintSocketAddress(const struct sockaddr *address, FILE *stream);
int SetupTCPClientSocket(const char *host, const char *service);
int SetupTCPServerSocket(const char *service);
int AcceptTCPConnection(int servSock);
bool SockAddrsEqual(const struct sockaddr *addr1, const struct sockaddr *addr2);
#endif //NETWORK_PROGRAMMING_PRACTICAL_H
