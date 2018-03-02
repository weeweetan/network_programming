//
// Created by tw on 2018/3/2.
//
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "Practical.h"
#include "VoteProtocol.h"
#include "VoteEncoding.h"
#include "Framer.h"

static  uint64_t counts[MAX_CANDIDATE + 1];

int main(int argc, char *argv[]) {
    if (argc != 2)
        DieWithUserMessage("Parametes(s)", "<Server Port/Service>");

    int servSock = SetupTCPServerSocket(argv[1]);

    for (;;)
    {
        int clntSock = AcceptTCPConnection(servSock);

        //Create an input stream from the socket
        FILE *channel = fdopen(clntSock, "r+");
        if (channel == NULL)
            DieWithSystemMessage("fdopen() failed");

        //Receive messages until connection closes;
        int mSize;
        uint8_t  inBuf[MAX_WIRE_SIZE];
        VoteInfo v;
        while ((mSize = GetNextMsg(channel, inBuf, MAX_WIRE_SIZE)) > 0) {
            memset(&v, 0, sizeof(v));
            printf("Recevied message (%d bytes)\n", mSize);
            if (Decode(inBuf, mSize, &v)) {
                if (!v.isResponse) {
                    v.isResponse = true;
                    if (v.condidate >= 0 && v.condidate <= MAX_CANDIDATE) {
                        if (!v.isInquiry)
                            counts[v.condidate] += 1;
                        v.count = counts[v.condidate];
                    }
                }
                uint8_t outBuf[MAX_WIRE_SIZE];
                mSize = Encode(&v, outBuf, MAX_WIRE_SIZE);
                if (PutMsg(outBuf, mSize, channel) < 0) {
                    fputs("Error framing/outputting message\n",stdout);
                    break;
                } else {
                    printf("Processed %s for candidate %d;current count is %llu.\n", (v.isInquiry ? "inquiry" : "vote"), v.condidate, v.count);
                }
                fflush(channel);
            } else {
                fputs("Parse error, closing connection.\n", stderr);
                break;
            }
        }
        puts("Client finished");
        fclose(channel);
    }
}