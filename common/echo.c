#include "net.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int echo_connection(int fd, int greeting)
{
    if (greeting) return net_send_all(fd, "Hello World!", 12);
    unsigned char bytes[16384];
    for (;;) {
        ssize_t count = net_receive(fd, bytes, sizeof bytes);
        if (count == 0) return 0;
        if (count < 0 || net_send_all(fd, bytes, (size_t)count) < 0) return -1;
    }
}

static int stdio_echo(int fd)
{
    FILE *in = NULL, *out = NULL;
    if (net_streams(fd, &in, &out) < 0) return -1;
    unsigned char bytes[1024];
    size_t used = 0;
    int byte, result = 0;
    while ((byte = getc(in)) != EOF) {
        bytes[used++] = (unsigned char)byte;
        if (byte == '\n' || used == sizeof bytes) {
            if (fwrite(bytes, 1, used, out) != used || fflush(out) != 0) { result = -1; break; }
            used = 0;
        }
    }
    if (result == 0 && used != 0 && (fwrite(bytes, 1, used, out) != used || fflush(out) != 0)) result = -1;
    if (ferror(in)) result = -1;
    if (fclose(out) != 0) result = -1;
    fclose(in);
    return result;
}

int net_run_echo_server(int argc, char **argv, int family, int greeting, int stdio_mode)
{
    NetServerOptions options;
    if (net_server_options(argc, argv, &options) < 0) return 2;
    if (greeting && argc < 4) options.limit = 1;
    if (net_init() < 0) { perror("signal setup"); return 1; }
    int listener = net_listener(options.host, options.port, SOCK_STREAM, family);
    if (listener < 0) { perror("listen"); return 1; }
    net_report_listener(listener);
    size_t handled = 0;
    int status = 0;
    while (!net_stopping && (options.limit == 0 || handled < options.limit)) {
        int fd = net_accept(listener);
        if (fd < 0) { if (!net_stopping) { perror("accept"); status = 1; } break; }
        if (stdio_mode) {
            (void)stdio_echo(fd); /* This function consumes the socket descriptor. */
        } else {
            (void)echo_connection(fd, greeting); /* A peer failure affects only this connection. */
            close(fd);
        }
        ++handled;
    }
    close(listener);
    return status;
}

int net_run_echo_client(int argc, char **argv, int family, int greeting, int interactive)
{
    const char *host, *port, *message = NULL;
    if (greeting || interactive) {
        if (argc != 3) { fprintf(stderr, "Usage: %s HOST PORT\n", argv[0]); return 2; }
        host = argv[1]; port = argv[2];
    } else {
        if (argc < 3 || argc > 4) { fprintf(stderr, "Usage: %s HOST MESSAGE [PORT]\n", argv[0]); return 2; }
        host = argv[1]; message = argv[2]; port = argc == 4 ? argv[3] : "7000";
        if (strlen(message) > 65535) { fputs("MESSAGE exceeds 65535 bytes\n", stderr); return 2; }
    }
    if (net_port(port, 0) < 0) { fputs("Invalid port\n", stderr); return 2; }
    if (net_init() < 0) return 1;
    int fd = net_connect(host, port, SOCK_STREAM, family);
    if (fd < 0) { perror("connect"); return 1; }
    int status = 0;
    unsigned char buffer[65536];
    if (greeting) {
        ssize_t count;
        while ((count = net_receive(fd, buffer, sizeof buffer)) > 0)
            if (fwrite(buffer, 1, (size_t)count, stdout) != (size_t)count) { status = 1; break; }
        if (count < 0) status = 1;
        putchar('\n');
    } else if (interactive) {
        char line[1025];
        while (fgets(line, sizeof line, stdin) != NULL) {
            if (strcmp(line, "q\n") == 0 || strcmp(line, "Q\n") == 0) break;
            size_t length = strlen(line);
            if (net_send_all(fd, line, length) < 0) { status = 1; break; }
            if (feof(stdin) && shutdown(fd, SHUT_WR) < 0) { status = 1; break; }
            if (net_receive_exact(fd, buffer, length) != 1) { status = 1; break; }
            if (fwrite(buffer, 1, length, stdout) != length) { status = 1; break; }
        }
        if (ferror(stdin)) status = 1;
    } else {
        size_t length = strlen(message);
        if (net_send_all(fd, message, length) < 0 || shutdown(fd, SHUT_WR) < 0 ||
            net_receive_exact(fd, buffer, length) != 1) status = 1;
        else {
            if (memcmp(buffer, message, length) != 0) { fputs("Echo payload mismatch\n", stderr); status = 1; }
            fputs("Received: ", stdout);
            if (fwrite(buffer, 1, length, stdout) != length) status = 1;
            putchar('\n');
        }
    }
    if (status) perror("TCP exchange");
    close(fd);
    return status;
}

int net_run_udp_server(int argc, char **argv)
{
    NetServerOptions options;
    if (net_server_options(argc, argv, &options) < 0) return 2;
    if (net_init() < 0) return 1;
    int fd = net_listener(options.host, options.port, SOCK_DGRAM, AF_UNSPEC);
    if (fd < 0) { perror("bind UDP"); return 1; }
    net_report_listener(fd);
    size_t handled = 0;
    int status = 0;
    while (!net_stopping && (options.limit == 0 || handled < options.limit)) {
        unsigned char buffer[65536];
        struct sockaddr_storage address;
        socklen_t length = sizeof address;
        ssize_t count = recvfrom(fd, buffer, sizeof buffer, 0, (struct sockaddr *)&address, &length);
        if (count < 0) {
            if (errno == EINTR) continue;
            perror("recvfrom"); status = 1; break;
        }
        ssize_t sent;
        do { sent = sendto(fd, buffer, (size_t)count, 0, (struct sockaddr *)&address, length); }
        while (sent < 0 && errno == EINTR && !net_stopping);
        if (sent != count) { if (!net_stopping) perror("sendto"); status = 1; break; }
        ++handled;
    }
    close(fd);
    return status;
}

static int udp_exchange(int fd, const char *message, size_t length, int retries)
{
    unsigned char buffer[65536];
    for (int attempt = 0; attempt < retries; ++attempt) {
        ssize_t sent;
        do { sent = send(fd, message, length, 0); } while (sent < 0 && errno == EINTR && !net_stopping);
        if (sent < 0 || (size_t)sent != length) return -1;
        struct iovec vector = {buffer, sizeof buffer};
        struct msghdr header;
        memset(&header, 0, sizeof header);
        header.msg_iov = &vector; header.msg_iovlen = 1;
        ssize_t received;
        do { received = recvmsg(fd, &header, 0); } while (received < 0 && errno == EINTR && !net_stopping);
        if (received < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == ETIMEDOUT)) continue;
        if (received < 0) return -1;
        if ((header.msg_flags & MSG_TRUNC) || (size_t)received != length || memcmp(buffer, message, length) != 0) {
            errno = EPROTO; return -1;
        }
        fputs("Received: ", stdout);
        if (fwrite(buffer, 1, length, stdout) != length) return -1;
        putchar('\n');
        return 0;
    }
    errno = ETIMEDOUT;
    return -1;
}

int net_run_udp_client(int argc, char **argv, int interactive, int retries)
{
    const char *host, *port, *message = NULL;
    if (interactive) {
        if (argc != 3) { fprintf(stderr, "Usage: %s HOST PORT\n", argv[0]); return 2; }
        host = argv[1]; port = argv[2];
    } else {
        if (argc < 3 || argc > 4) { fprintf(stderr, "Usage: %s HOST MESSAGE [PORT]\n", argv[0]); return 2; }
        host = argv[1]; message = argv[2]; port = argc == 4 ? argv[3] : "7000";
        if (strlen(message) > 65507) { fputs("UDP payload exceeds 65507 bytes\n", stderr); return 2; }
    }
    if (net_port(port, 0) < 0) { fputs("Invalid port\n", stderr); return 2; }
    if (net_init() < 0) return 1;
    int fd = net_connect(host, port, SOCK_DGRAM, AF_UNSPEC);
    if (fd < 0 || net_timeouts(fd, retries > 1 ? 200 : 1000) < 0) { perror("UDP connect"); if (fd >= 0) close(fd); return 1; }
    int status = 0;
    if (interactive) {
        char line[1024];
        while (fgets(line, sizeof line, stdin) != NULL) {
            if (strcmp(line, "q\n") == 0 || strcmp(line, "Q\n") == 0) break;
            if (udp_exchange(fd, line, strlen(line), retries) < 0) { status = 1; break; }
        }
        if (ferror(stdin)) status = 1;
    } else if (udp_exchange(fd, message, strlen(message), retries) < 0) status = 1;
    if (status) perror("UDP exchange");
    close(fd);
    return status;
}
