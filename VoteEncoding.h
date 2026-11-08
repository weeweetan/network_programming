#ifndef NETWORK_PROGRAMMING_VOTEENCODING_H
#define NETWORK_PROGRAMMING_VOTEENCODING_H
#include <stddef.h>
#include <stdint.h>
#include "VoteProtocol.h"
/* Decode is bounded and leaves input and result unchanged on failure.
 * Encode returns payload length, or 0 on invalid data / insufficient space.
 * No terminating NUL is required in a wire payload.
 */
bool Decode(const uint8_t *inBuf, size_t length, VoteInfo *value);
size_t Encode(const VoteInfo *value, uint8_t *outBuf, size_t capacity);
#endif
