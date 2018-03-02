//
// Created by tw on 2018/3/2.
//
/**
 * Routines for binary encoding of vote messages
 */


#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <netinet/in.h>
#include "Practical.h"
#include "VoteProtocol.h"

enum {
    REQUSET_SIZE = 4,
    RESPONSE_SIZE = 12,
    COUNT_SHIFT = 32,
    INQUIRE_FLAG = 0x0100,
    RESPONSE_FLAG = 0x0200,
    MAGIC = 0x5400,
    MAGIC_MASK = 0xfc00
};

typedef struct voteMsgBin voteMsgBin;
struct voteMsgBin {
    uint16_t header;
    uint16_t candidateID;
    uint16_t countHigh;
    uint16_t countLow;
};

size_t Encode(VoteInfo *v, uint8_t *outBuf, size_t bufSize) {
    if ((v->isResponse && bufSize < sizeof(struct voteMsgBin)) || bufSize < 2 * sizeof(uint16_t))
        DieWithUserMessage("Output buffer too small", "");
    voteMsgBin *vm = (voteMsgBin *)outBuf;
    memset(outBuf, 0, sizeof(voteMsgBin));
    vm->header = MAGIC;
    if (v->isInquiry)
        vm->header |= INQUIRE_FLAG;
    if (v->isResponse)
        vm->header |= RESPONSE_FLAG;
    vm->header = htons(vm->header);
    vm->candidateID = htons(v->condidate);
    if (v->isResponse) {
        vm->countHigh = htonl(v->count >> COUNT_SHIFT);
        vm->countLow = htonl((uint32_t) v->count);
        return RESPONSE_SIZE;
    } else {
        return RESPONSE_SIZE;
    }
}


/*
 * Extract message info from given buffer.
 * Leave input unchanged.
 */
bool Decode(uint8_t *inBuf, size_t mSize, VoteInfo *v) {
    voteMsgBin *vm = (voteMsgBin *)inBuf;

    uint16_t header = ntohs(vm->header);
    if ((mSize < RESPONSE_SIZE) || ((header & MAGIC_MASK) != MAGIC))
        return false;
    /* message is big enough and includes correct magic number */
    v->isResponse = ((header & RESPONSE_FLAG) != 0);
    v->isInquiry = ((header & INQUIRE_FLAG) != 0);
    v->condidate = ntohs(vm->candidateID);
    if (v->isResponse && mSize >= RESPONSE_SIZE) {
        v->count = ((uint64_t) ntohl(vm->countHigh) << COUNT_SHIFT) | (uint64_t) ntohl(vm->countLow);
    }
    return true;
}