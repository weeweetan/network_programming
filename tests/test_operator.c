#include <errno.h>
#include <string.h>
#include "check.h"
#include "common/operator.h"

int main(void)
{
    const int32_t values[] = {10, -2, 7};
    int32_t result = 0;
    CHECK(net_calculate(values, 3, '+', &result) == 0 && result == 15);
    CHECK(net_calculate(values, 3, '-', &result) == 0 && result == 5);
    CHECK(net_calculate(values, 3, '*', &result) == 0 && result == -140);
    CHECK(net_calculate(values, 0, '+', &result) == -1 && errno == EINVAL);
    CHECK(net_calculate(values, 3, '/', &result) == -1);
    const int32_t overflow[] = {INT32_MAX, 1};
    CHECK(net_calculate(overflow, 2, '+', &result) == -1 && errno == ERANGE);
    const int32_t negative[] = {INT32_MIN, -1};
    CHECK(net_calculate(negative, 2, '*', &result) == -1 && errno == ERANGE);
    unsigned char unaligned[5] = {0};
    net_put_i32(unaligned + 1, -2147483647);
    const unsigned char expected[] = {0x80, 0, 0, 1};
    CHECK(memcmp(unaligned + 1, expected, 4) == 0 && net_get_i32(unaligned + 1) == -2147483647);
    return EXIT_SUCCESS;
}
