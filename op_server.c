#include <stdio.h>
#include <unistd.h>
#include "common/net.h"
#include "common/operator.h"

static void serve(int fd)
{
    unsigned char count;
    if (net_receive_exact(fd, &count, 1) != 1 || count == 0 || count > 64) return;
    unsigned char packet[64 * 4 + 1];
    size_t length = (size_t)count * 4 + 1;
    if (net_receive_exact(fd, packet, length) != 1) return;
    int32_t values[64], result;
    for (size_t i = 0; i < count; ++i) values[i] = net_get_i32(packet + i * 4);
    if (net_calculate(values, count, (char)packet[length - 1], &result) < 0) return;
    unsigned char response[4]; net_put_i32(response, result);
    (void)net_send_all(fd, response, sizeof response);
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
        if (fd < 0) { if (!net_stopping) status = 1; break; }
        serve(fd); close(fd); ++handled;
    }
    close(listener);
    return status;
}
