//
// Created by tw on 2018/3/2.
//

#ifndef NETWORK_PROGRAMMING_VOTEENCODING_H
#define NETWORK_PROGRAMMING_VOTEENCODING_H

#include <stdint.h>
#include <unistd.h>
#include "VoteProtocol.h"

bool Decode(uint8_t *inBuf, size_t mSize, VoteInfo *v);
size_t Encode(const VoteInfo *v, uint8_t *outBuf, const size_t bufSize);

#endif //NETWORK_PROGRAMMING_VOTEENCODING_H
