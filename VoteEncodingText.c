#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "VoteEncoding.h"

static int decimal(const char *text, uint64_t maximum, uint64_t *value)
{
    if (*text == '\0') return -1;
    uint64_t number = 0;
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
        if (*p < '0' || *p > '9') return -1;
        unsigned digit = *p - '0';
        if (digit > maximum || number > (maximum - digit) / 10) return -1;
        number = number * 10 + digit;
    }
    *value = number;
    return 0;
}

size_t Encode(const VoteInfo *value, uint8_t *out, size_t capacity)
{
    if (value == NULL || out == NULL || value->condidate < 0 || value->condidate > MAX_CANDIDATE) {
        errno = EINVAL; return 0;
    }
    char text[128];
    int length = value->isResponse ?
        snprintf(text, sizeof text, "Voting %c R %d %" PRIu64, value->isInquiry ? 'i' : 'v', value->condidate, value->count) :
        snprintf(text, sizeof text, "Voting %c %d", value->isInquiry ? 'i' : 'v', value->condidate);
    if (length < 0 || (size_t)length >= sizeof text || (size_t)length > capacity) {
        errno = EMSGSIZE; return 0;
    }
    memcpy(out, text, (size_t)length);
    return (size_t)length;
}

bool Decode(const uint8_t *in, size_t length, VoteInfo *value)
{
    if (in == NULL || value == NULL || length == 0 || length > MAX_WIRE_SIZE || memchr(in, 0, length) != NULL)
        return false;
    char text[MAX_WIRE_SIZE + 1];
    memcpy(text, in, length); text[length] = '\0';
    char *tokens[6], *save = NULL;
    size_t count = 0;
    for (char *p = strtok_r(text, " \t", &save); p != NULL; p = strtok_r(NULL, " \t", &save)) {
        if (count == 6) return false;
        tokens[count++] = p;
    }
    if (count < 3 || strcmp(tokens[0], "Voting") != 0 ||
        (strcmp(tokens[1], "i") != 0 && strcmp(tokens[1], "v") != 0)) return false;
    VoteInfo parsed = {0};
    parsed.isInquiry = strcmp(tokens[1], "i") == 0;
    parsed.isResponse = strcmp(tokens[2], "R") == 0;
    size_t candidate_index = parsed.isResponse ? 3 : 2;
    if (count != (parsed.isResponse ? 5u : 3u)) return false;
    uint64_t candidate;
    if (decimal(tokens[candidate_index], MAX_CANDIDATE, &candidate) < 0) return false;
    parsed.condidate = (int)candidate;
    if (parsed.isResponse && decimal(tokens[4], UINT64_MAX, &parsed.count) < 0) return false;
    *value = parsed;
    return true;
}
