#ifndef NETWORK_EXAMPLES_OPERATOR_H
#define NETWORK_EXAMPLES_OPERATOR_H
#include <stddef.h>
#include <stdint.h>
int net_calculate(const int32_t *values, size_t count, char operation, int32_t *result);
int32_t net_get_i32(const unsigned char *bytes);
void net_put_i32(unsigned char *bytes, int32_t value);
#endif
