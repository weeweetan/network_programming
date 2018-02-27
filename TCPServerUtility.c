//
// Created by tw on 2018/2/27.
//
#include <sched.h>
#include <sys/socket.h>
#include <unistd.h>
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
