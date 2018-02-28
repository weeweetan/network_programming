//
// Created by tw on 2018/2/28.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "Practical.h"

int main(int argc, char *argv[])
{
    if (argc < 3 || argc > 4)
        DieWithUserMessage("Parameter(s)", "<Server Address/Name> <Echo Word> [<Server Port/Service>]");
    char *server = argv[1];
    char *echoString = argv[2];
    char *service = (argc == 4) ? argv[3] : "echo";

    //Create a connected TCP socket
    int sock = SetupTCPClientSocket(server, service);
    if (sock < 0)
        DieWithUserMessage("SetupTCPClientSocket() failed", "unable to connect");
    size_t echoStringLen = strlen(echoString);

    //Send the string to the server
    ssize_t numBytes = send(sock, echoString, echoStringLen, 0);
    if (numBytes < 0)
        DieWithSystemMessage("send() failed");
    else if (numBytes != echoStringLen)
        DieWithUserMessage("send()", "sent unexpected number of bytes");

    //Receive the same string back from the server
    unsigned int totalByteRcvd = 0;
    fputs("Received: ", stdout);
    while (totalByteRcvd < echoStringLen) {
        char buffer[BUFSIZ];
        numBytes = recv(sock, buffer, BUFSIZ - 1, 0);
        if (numBytes < 0)
            DieWithSystemMessage("recv()");
        else if (numBytes == 0)
            DieWithUserMessage("recv()", "connection closed prematurely");
        totalByteRcvd +=numBytes;
        buffer[numBytes] = '\0';
        fputs(buffer, stdout);
    }
    fputc('\n', stdout);
    close(sock);
    exit(0);
}
