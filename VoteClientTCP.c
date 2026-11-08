#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "common/net.h"
#include "Framer.h"
#include "VoteEncoding.h"

int main(int argc, char **argv)
{
    uint64_t candidate;
    if ((argc != 4 && argc != 5) || net_port(argv[2], 0) < 0 ||
        net_uint(argv[3], MAX_CANDIDATE, &candidate) < 0 || (argc == 5 && strcmp(argv[4], "I") != 0)) {
        fprintf(stderr, "Usage: %s HOST PORT CANDIDATE [I]\n", argv[0]);
        return 2;
    }
    if (net_init() < 0) return 1;
    int fd = net_connect(argv[1], argv[2], SOCK_STREAM, AF_UNSPEC);
    if (fd < 0) { perror("connect"); return 1; }
    FILE *in = NULL, *out = NULL;
    if (net_streams(fd, &in, &out) < 0) { perror("fdopen"); return 1; }
    VoteInfo request = {0}, response = {0};
    request.condidate = (int)candidate;
    request.isInquiry = argc == 5;
    uint8_t payload[MAX_WIRE_SIZE];
    size_t length = Encode(&request, payload, sizeof payload);
    int status = 1;
    if (length != 0 && PutMsg(payload, length, out) >= 0) {
        int received = GetNextMsg(in, payload, sizeof payload);
        if (received >= 0 && Decode(payload, (size_t)received, &response) && response.isResponse &&
            response.condidate == request.condidate && response.isInquiry == request.isInquiry) {
            printf("candidate=%d inquiry=%d count=%" PRIu64 "\n", response.condidate, response.isInquiry, response.count);
            status = 0;
        } else fputs("Invalid or incomplete vote response\n", stderr);
    } else perror("send vote");
    if (fclose(out) != 0) status = 1;
    fclose(in);
    return status;
}
