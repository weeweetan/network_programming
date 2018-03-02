//
// Created by tw on 2018/3/2.
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "Practical.h"
#include "Framer.h"
static const char DELIMITER = '\n';

int GetNextMsg(FILE *in, uint8_t *buf, size_t bufSize) {
    int count = 0;
    int nextChar;
    while (count < bufSize) {
        nextChar = getc(in);
        if (nextChar == EOF) {
            if (count > 0)
                DieWithUserMessage("GetNextMsg()", "Stream ended prematurely");
            else
                return -1;
        }
        if (nextChar == DELIMITER)
            break;
        buf[count++] = nextChar;
    }
    if (nextChar != DELIMITER) {
        return -count;
    } else {
        return count;
    }
}

int PutMsg(uint8_t buf[], size_t msgSize, FILE *out) {
    int i;
    for (i = 0; i < msgSize; i++)
        if (buf[i] == DELIMITER)
            return -1;
    if (fwrite(buf, 1, msgSize, out) != msgSize)
        return -1;
    fputc(DELIMITER, out);
    fflush(out);
    return msgSize;
}