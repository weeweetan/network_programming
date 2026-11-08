#ifndef NETWORK_PROGRAMMING_PRACTICAL_H
#define NETWORK_PROGRAMMING_PRACTICAL_H
#include <stdbool.h>
#include <stdio.h>
#include <sys/socket.h>
void DieWithUserMessage(const char *message, const char *detail);
void DieWithSystemMessage(const char *message);
/* HandleTCPClient borrows its socket; the caller owns close(). */
void HandleTCPClient(int socket);
void PrintSocketAddress(const struct sockaddr *address, FILE *stream);
int SetupTCPClientSocket(const char *host, const char *service);
int SetupTCPServerSocket(const char *service);
int AcceptTCPConnection(int listener);
bool SockAddrsEqual(const struct sockaddr *left, const struct sockaddr *right);
#endif
