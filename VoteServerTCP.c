#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>
#include "common/net.h"
#include "Framer.h"
#include "VoteEncoding.h"

static uint64_t counts[MAX_CANDIDATE + 1];

static void serve(int fd)
{
    FILE *in = NULL, *out = NULL;
    if (net_streams(fd, &in, &out) < 0) { perror("fdopen"); return; }
    uint8_t payload[MAX_WIRE_SIZE];
    int length;
    while (!net_stopping && (length = GetNextMsg(in, payload, sizeof payload)) >= 0) {
        VoteInfo value = {0};
        if (!Decode(payload, (size_t)length, &value) || value.isResponse) {
            fputs("Invalid vote request; closing peer\n", stderr);
            break;
        }
        if (!value.isInquiry) {
            if (counts[value.condidate] == UINT64_MAX) { fputs("Vote counter exhausted\n", stderr); break; }
            ++counts[value.condidate];
        }
        value.isResponse = true;
        value.count = counts[value.condidate];
        size_t response_length = Encode(&value, payload, sizeof payload);
        if (response_length == 0 || PutMsg(payload, response_length, out) < 0) break;
    }
    fclose(out);
    fclose(in);
}

int main(int argc, char **argv)
{
    NetServerOptions options;
    if (net_server_options(argc, argv, &options) < 0) return 2;
    if (net_init() < 0) return 1;
    int listener = net_listener(options.host, options.port, SOCK_STREAM, AF_UNSPEC);
    if (listener < 0) { perror("listen"); return 1; }
    net_report_listener(listener);
    size_t handled = 0;
    int status = 0;
    while (!net_stopping && (options.limit == 0 || handled < options.limit)) {
        int fd = net_accept(listener);
        if (fd < 0) { if (!net_stopping) { perror("accept"); status = 1; } break; }
        serve(fd); /* FILE streams consume and close the socket. */
        ++handled;
    }
    close(listener);
    return status;
}
