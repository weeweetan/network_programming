//
// Created by tw on 2018/2/26.
//

#include <sys/socket.h>
#ifndef NETWORK_PROGRAMMING_PRACTICAL_H
#define NETWORK_PROGRAMMING_PRACTICAL_H

void DieWithUserMessage(const char *msg, const char *detail);
void DieWithSystemMessage(const char *msg);
void HandleTCPClient(int clntSocket);
void PrintSocketAddress(const struct sockaddr *address, FILE *stream);
#endif //NETWORK_PROGRAMMING_PRACTICAL_H
