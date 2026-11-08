#include <errno.h>
#include <limits.h>
#include "operator.h"

int32_t net_get_i32(const unsigned char *bytes)
{
    uint32_t value = ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) | ((uint32_t)bytes[2] << 8) | bytes[3];
    return (int32_t)(value <= INT32_MAX ? (int64_t)value : (int64_t)value - INT64_C(4294967296));
}

void net_put_i32(unsigned char *bytes, int32_t value)
{
    uint32_t bits = (uint32_t)value;
    bytes[0] = (unsigned char)(bits >> 24); bytes[1] = (unsigned char)(bits >> 16);
    bytes[2] = (unsigned char)(bits >> 8); bytes[3] = (unsigned char)bits;
}

int net_calculate(const int32_t *values, size_t count, char operation, int32_t *result)
{
    if (values == NULL || result == NULL || count == 0 || count > 64 ||
        (operation != '+' && operation != '-' && operation != '*')) { errno = EINVAL; return -1; }
    int64_t answer = values[0];
    for (size_t i = 1; i < count; ++i) {
        if (operation == '+') answer += values[i];
        else if (operation == '-') answer -= values[i];
        else answer *= values[i];
        if (answer < INT32_MIN || answer > INT32_MAX) { errno = ERANGE; return -1; }
    }
    *result = (int32_t)answer;
    return 0;
}
