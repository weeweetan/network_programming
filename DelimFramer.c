#include <errno.h>
#include "Framer.h"

int GetNextMsg(FILE *in, uint8_t *buf, size_t capacity)
{
    if (in == NULL || buf == NULL || capacity == 0) { errno = EINVAL; return -1; }
    size_t length = 0;
    for (;;) {
        int byte = getc(in);
        if (byte == EOF) {
            if (ferror(in) && errno == EINTR) { clearerr(in); continue; }
            if (length != 0) errno = EPROTO;
            return -1;
        }
        if (byte == '\n') return length > capacity ? -(int)length : (int)length;
        if (length == UINT16_MAX) { errno = EMSGSIZE; return -1; }
        if (length < capacity) buf[length] = (uint8_t)byte;
        ++length;
    }
}

int PutMsg(uint8_t buf[], size_t length, FILE *out)
{
    if (out == NULL || (length != 0 && buf == NULL) || length > UINT16_MAX) {
        errno = EINVAL; return -1;
    }
    for (size_t i = 0; i < length; ++i) if (buf[i] == '\n') { errno = EINVAL; return -1; }
    if ((length != 0 && fwrite(buf, 1, length, out) != length) ||
        putc('\n', out) == EOF || fflush(out) != 0) return -1;
    return (int)length;
}
