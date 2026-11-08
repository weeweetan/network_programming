#include <errno.h>
#include "Framer.h"

static int read_bytes(FILE *in, uint8_t *bytes, size_t length)
{
    size_t used = 0;
    while (used < length) {
        size_t count = fread(bytes + used, 1, length - used, in);
        used += count;
        if (used == length) return 0;
        if (ferror(in) && errno == EINTR) { clearerr(in); continue; }
        if (feof(in)) errno = EPROTO;
        return -1;
    }
    return 0;
}

int GetNextMsg(FILE *in, uint8_t *buf, size_t bufSize)
{
    if (in == NULL || buf == NULL || bufSize == 0) { errno = EINVAL; return -1; }
    int first = getc(in);
    if (first == EOF) return -1;
    int second = getc(in);
    if (second == EOF) { errno = EPROTO; return -1; }
    size_t length = ((unsigned)first << 8) | (unsigned)second;
    size_t copied = length < bufSize ? length : bufSize;
    if (read_bytes(in, buf, copied) < 0) return -1;
    size_t extra = length - copied;
    uint8_t discard[1024];
    while (extra != 0) {
        size_t chunk = extra < sizeof discard ? extra : sizeof discard;
        if (read_bytes(in, discard, chunk) < 0) return -1;
        extra -= chunk;
    }
    return length > bufSize ? -(int)length : (int)length;
}

int PutMsg(uint8_t buf[], size_t length, FILE *out)
{
    if (out == NULL || (length != 0 && buf == NULL) || length > UINT16_MAX) {
        errno = EINVAL; return -1;
    }
    uint8_t header[2] = {(uint8_t)(length >> 8), (uint8_t)length};
    if (fwrite(header, 1, sizeof header, out) != sizeof header ||
        (length != 0 && fwrite(buf, 1, length, out) != length) || fflush(out) != 0) return -1;
    return (int)length;
}
