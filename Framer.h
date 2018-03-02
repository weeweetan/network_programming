//
// Created by tw on 2018/3/2.
//

#ifndef NETWORK_PROGRAMMING_FRAMER_H
#define NETWORK_PROGRAMMING_FRAMER_H

#include <stdio.h>
#include <stdint.h>

int GetNextMsg(FILE *in, uint8_t *buf, size_t bufSize);
int PutMsg(uint8_t buf[], size_t msgSize, FILE *out);
#endif //NETWORK_PROGRAMMING_FRAMER_H
