//
// Created by tw on 2018/3/2.
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "Practical.h"
#include "VoteProtocol.h"
#include "Framer.h"
#include "VoteEncoding.h"

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4)
        DieWithUserMessage("Parameter(s)", "<Server> <Port/Services> <candiate>[I]");

    char *server = argv[1];
    char *service = argv[2];
    int candi = atoi(argv[3]);
    if (candi < 0 || candi > MAX_CANDIDATE)
        DieWithUserMessage("Candidate # not valid", argv[3]);

    bool inq = argc > 4 && strcmp(argv[4], "I") == 0;

    //Create a connection TCP socket
    int sock = SetupTCPClientSocket(server, service);
    if (sock < 0)
        DieWithUserMessage("SetupTCPClientSocket() falied", "unable to connect");
    FILE *str = fdopen(sock, "r+");
    if (str == NULL)
        DieWithSystemMessage("fdopen() failed");

    //Set up info for a request
    VoteInfo vi;
    memset(&vi, 0, sizeof(vi));

    vi.isInquiry = inq;
    vi.condidate = candi;

    uint8_t  outBuf[MAX_WIRE_SIZE];
    size_t  reqSize = Encode(&vi, outBuf, MAX_WIRE_SIZE);

    printf("Sending %d-byte %s for candidate %d..\n", reqSize, (
    inq ? "inquiry" : "vote"), candi);

    //Frame and send
    if (PutMsg(outBuf, reqSize, str) < 0)
        DieWithSystemMessage("PutMsg() failed");

    //Receive and print response
    uint8_t inBuf[MAX_WIRE_SIZE];
    size_t resSize = GetNextMsg(str, inBuf, MAX_WIRE_SIZE);
    if (Decode(inBuf, resSize, &vi)) {
        printf("Received:\n");
        if (vi.isResponse)
            printf("Response to ");
        if (vi.isInquiry)
            printf("inquiry");
        else
            printf("vote ");
        printf("for candidate %d\n", vi.condidate);
        if (vi.isResponse)
            printf(" count = %llu\n", vi.count);
    }
    fclose(str);
    exit(0);
}
