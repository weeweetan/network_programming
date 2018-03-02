//
// Created by tw on 2018/3/2.
//

#ifndef NETWORK_PROGRAMMING_VOTEPROTOCOL_H
#define NETWORK_PROGRAMMING_VOTEPROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
struct VoteInfo {
    uint64_t count;
    int condidate;
    bool isInquiry;
    bool isResponse;
};

typedef struct VoteInfo VoteInfo;

enum {
    MAX_CANDIDATE = 1000,
    MAX_WIRE_SIZE = 500
};
#endif //NETWORK_PROGRAMMING_VOTEPROTOCOL_H
