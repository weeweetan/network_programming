#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common/net.h"
#include "common/operator.h"

static int number(int64_t minimum, int64_t maximum, int32_t *value)
{
    char line[128], *end;
    if (fgets(line, sizeof line, stdin) == NULL) return -1;
    if (strchr(line, '\n') == NULL && !feof(stdin)) return -1;
    errno = 0;
    long long parsed = strtoll(line, &end, 10);
    if (end == line || errno != 0) return -1;
    while (isspace((unsigned char)*end)) ++end;
    if (*end != '\0' || parsed < minimum || parsed > maximum) return -1;
    *value = (int32_t)parsed;
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 3 || net_port(argv[2], 0) < 0) { fprintf(stderr, "Usage: %s HOST PORT\n", argv[0]); return 2; }
    int32_t count;
    fputs("Operand count: ", stdout);
    if (number(1, 64, &count) < 0) { fputs("Expected 1..64 operands\n", stderr); return 2; }
    unsigned char packet[64 * 4 + 2]; packet[0] = (unsigned char)count;
    for (int32_t i = 0; i < count; ++i) {
        int32_t value;
        printf("Operand %" PRId32 ": ", i + 1);
        if (number(INT32_MIN, INT32_MAX, &value) < 0) { fputs("Invalid int32 operand\n", stderr); return 2; }
        net_put_i32(packet + 1 + (size_t)i * 4, value);
    }
    char line[32];
    fputs("Operator (+ - *): ", stdout);
    if (fgets(line, sizeof line, stdin) == NULL) return 2;
    char *operator = line;
    while (isspace((unsigned char)*operator)) ++operator;
    if (*operator == '\0') return 2;
    char operation = *operator++;
    while (isspace((unsigned char)*operator)) ++operator;
    if (*operator != '\0' || (operation != '+' && operation != '-' && operation != '*')) return 2;
    size_t length = (size_t)count * 4 + 2;
    packet[length - 1] = (unsigned char)operation;
    if (net_init() < 0) return 1;
    int fd = net_connect(argv[1], argv[2], SOCK_STREAM, AF_UNSPEC);
    if (fd < 0) { perror("connect"); return 1; }
    unsigned char response[4];
    int status = net_send_all(fd, packet, length) < 0 || net_receive_exact(fd, response, sizeof response) != 1;
    if (status) fputs("Invalid or incomplete arithmetic response\n", stderr);
    else printf("Operation result: %" PRId32 "\n", net_get_i32(response));
    close(fd);
    return status;
}
