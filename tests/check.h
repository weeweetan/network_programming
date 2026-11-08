#ifndef NETWORK_EXAMPLES_TEST_CHECK_H
#define NETWORK_EXAMPLES_TEST_CHECK_H
#include <stdio.h>
#include <stdlib.h>
#define CHECK(value) do { if (!(value)) { \
    fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #value); \
    exit(EXIT_FAILURE); \
} } while (0)
#endif
