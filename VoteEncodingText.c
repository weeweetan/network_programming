//
// Created by tw on 2018/3/2.
//
/**
 * Routines for Text encoding of vote messages.
 * Wire Format:
 * "Voting <v|i> [R] <candidate ID> <count>"
 *
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "VoteEncoding.h"
#include "VoteProtocol.h"

static const char *MAGIC = "Voting";
static const char *VOTESTR = "v";
static const char *INQSTR = "s";
static const char *RESPONSESTR = "R";
static const char *DELIMSTR = " ";

enum {
    BASE = 10
};

/**
 * Encode voting message info as text string.
 * WARNING: Message will be silently truncate if buffer if too small
 * Invariants (e.g 0 <=candidate <= 1000) not checked.
 *
 */
size_t Encode(const VoteInfo *v, uint8_t *outBuf, const size_t bufSize) {
    uint8_t *bufPtr = outBuf;
    long size = (size_t) bufSize;
    int rv = snprintf((char *)bufPtr, size, "%s %c %s %d", MAGIC, (v->isInquiry ? 'i':'v'), (v->isResponse ? "R" : ""), v->condidate);
    bufPtr += rv;
    size -= rv;
    if (v->isResponse) {
        rv = snprintf((char *) bufPtr, size, " %llu", v->count);
        bufPtr += rv;
    }
    return (size_t) (bufPtr - outBuf);
}

/**
 * Extract message information form given buffer.
 * Note:modifies input buffer.
 */
 bool Decode(uint8_t *inBuf, const size_t mSize, VoteInfo *v) {
     char *token;
     token = strtok((char *)inBuf, DELIMSTR);
     //Check for maginc
     if (token == NULL || strcmp(token, MAGIC) != 0)
         return false;

     //Get vote/inquiry indicator
     token = strtok(NULL, DELIMSTR);
     if (token == NULL)
         return false;

     if (strcmp(token, VOTESTR) == 0)
         v->isInquiry = false;
     else if (strcmp(token, INQSTR) == 0)
         v->isInquiry = true;
     else
         return false;

     //Next token is either Response flag or candidate ID
     token = strtok(NULL, DELIMSTR);
     if (token == NULL)
         return false;

     if (strcmp(token, RESPONSESTR) == 0) {
         v->isResponse = true;
         token = strtok(NULL, DELIMSTR);
         if (token == NULL)
             return false;
     } else {
         v->isResponse = false;
     }
     //Get candidate
     v->condidate = atoi(token);
     if (v->isResponse) {
         token = strtok(NULL, DELIMSTR);
         if (token == NULL)
             return false;
         v->count = strtoll(token, NULL, BASE);
     } else {
         v->count = 0L;
     }
     return true;
 }
