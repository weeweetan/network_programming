#ifndef NETWORK_PROGRAMMING_FRAMER_H
#define NETWORK_PROGRAMMING_FRAMER_H
#include <stdint.h>
#include <stdio.h>
/* A zero-length frame returns 0; EOF/malformed/I/O failure returns -1.
 * An oversized complete frame is drained and returns its negative wire length.
 * GetNextMsg requires a nonempty destination. Messages are bytes, not C strings.
 */
int GetNextMsg(FILE *in, uint8_t *buf, size_t bufSize);
int PutMsg(uint8_t buf[], size_t msgSize, FILE *out);
#endif
