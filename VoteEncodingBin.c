#include <errno.h>
#include "VoteEncoding.h"

enum { MAGIC = 0x5400, INQUIRY = 0x0100, RESPONSE = 0x0200 };

size_t Encode(const VoteInfo *value, uint8_t *out, size_t capacity)
{
    if (value == NULL || out == NULL || value->condidate < 0 || value->condidate > MAX_CANDIDATE) {
        errno = EINVAL; return 0;
    }
    size_t length = value->isResponse ? 12 : 4;
    if (capacity < length) { errno = EMSGSIZE; return 0; }
    unsigned header = MAGIC | (value->isInquiry ? INQUIRY : 0) | (value->isResponse ? RESPONSE : 0);
    out[0] = (uint8_t)(header >> 8); out[1] = (uint8_t)header;
    out[2] = (uint8_t)((unsigned)value->condidate >> 8); out[3] = (uint8_t)value->condidate;
    if (value->isResponse)
        for (unsigned i = 0; i < 8; ++i) out[4 + i] = (uint8_t)(value->count >> (56 - i * 8));
    return length;
}

bool Decode(const uint8_t *in, size_t length, VoteInfo *value)
{
    if (in == NULL || value == NULL || length < 4) return false;
    unsigned header = ((unsigned)in[0] << 8) | in[1];
    if ((header & 0xfcffu) != MAGIC) return false;
    VoteInfo parsed = {0};
    parsed.isInquiry = (header & INQUIRY) != 0;
    parsed.isResponse = (header & RESPONSE) != 0;
    if (length != (parsed.isResponse ? 12u : 4u)) return false;
    parsed.condidate = (int)(((unsigned)in[2] << 8) | in[3]);
    if (parsed.condidate > MAX_CANDIDATE) return false;
    if (parsed.isResponse)
        for (unsigned i = 0; i < 8; ++i) parsed.count = (parsed.count << 8) | in[4 + i];
    *value = parsed;
    return true;
}
