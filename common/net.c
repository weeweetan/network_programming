#include "net.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

volatile sig_atomic_t net_stopping;
static void stop_server(int number) { (void)number; net_stopping = 1; }

int net_init(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof action);
    sigemptyset(&action.sa_mask);
    action.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &action, NULL) < 0) return -1;
    action.sa_handler = stop_server;
    if (sigaction(SIGINT, &action, NULL) < 0 || sigaction(SIGTERM, &action, NULL) < 0) return -1;
    return 0;
}

int net_uint(const char *text, uint64_t maximum, uint64_t *value)
{
    if (text == NULL || *text == '\0' || value == NULL) return -1;
    uint64_t number = 0;
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
        if (*p < '0' || *p > '9') return -1;
        unsigned digit = *p - '0';
        if (digit > maximum || number > (maximum - digit) / 10) return -1;
        number = number * 10 + digit;
    }
    *value = number;
    return 0;
}

int net_port(const char *text, int allow_zero)
{
    uint64_t value;
    if (net_uint(text, 65535, &value) < 0 || (!allow_zero && value == 0)) {
        errno = EINVAL;
        return -1;
    }
    return 0;
}

int net_server_options(int argc, char **argv, NetServerOptions *options)
{
    uint64_t limit = 0;
    if (argc < 2 || argc > 4 || net_port(argv[1], 1) < 0 ||
        (argc == 4 && net_uint(argv[3], 1000000, &limit) < 0)) {
        fprintf(stderr, "Usage: %s PORT [BIND_ADDRESS [LIMIT]]\n", argv[0]);
        return -1;
    }
    options->port = argv[1];
    options->host = argc >= 3 ? argv[2] : "127.0.0.1";
    options->limit = (size_t)limit;
    return 0;
}

int64_t net_now_ms(void)
{
    struct timespec time;
    if (clock_gettime(CLOCK_MONOTONIC, &time) < 0) return -1;
    return (int64_t)time.tv_sec * 1000 + time.tv_nsec / 1000000;
}

int net_wait(int fd, short events, int64_t deadline)
{
    for (;;) {
        int64_t remaining = deadline - net_now_ms();
        if (remaining <= 0) { errno = ETIMEDOUT; return -1; }
        struct pollfd item = {fd, events, 0};
        int timeout = remaining > INT_MAX ? INT_MAX : (int)remaining;
        int result = poll(&item, 1, timeout);
        if (result > 0) {
            if (item.revents & POLLNVAL) { errno = EBADF; return -1; }
            return 0; /* The actual I/O reports EOF, connection errors or HUP. */
        }
        if (result == 0) { errno = ETIMEDOUT; return -1; }
        if (errno != EINTR || net_stopping) return -1;
    }
}

int net_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    return flags < 0 ? -1 : fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int net_timeouts(int fd, int milliseconds)
{
    struct timeval timeout = {milliseconds / 1000, (milliseconds % 1000) * 1000};
    return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) < 0 ||
           setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout) < 0 ? -1 : 0;
}

static int prepare(int fd)
{
    int flags = fcntl(fd, F_GETFD);
    if (flags < 0 || fcntl(fd, F_SETFD, flags | FD_CLOEXEC) < 0) return -1;
#ifdef SO_NOSIGPIPE
    int one = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof one) < 0) return -1;
#endif
    return 0;
}

int net_listener(const char *host, const char *port, int type, int family)
{
    if (net_port(port, 1) < 0) return -1;
    struct addrinfo hints = {0}, *addresses = NULL;
    hints.ai_family = family;
    hints.ai_socktype = type;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
    int result = getaddrinfo(host, port, &hints, &addresses);
    if (result != 0) { fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result)); errno = EINVAL; return -1; }
    int listener = -1, last_error = EADDRNOTAVAIL;
    for (struct addrinfo *address = addresses; address != NULL; address = address->ai_next) {
        int fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (fd < 0) { last_error = errno; continue; }
        int one = 1;
        if (prepare(fd) == 0 && setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one) == 0 &&
            bind(fd, address->ai_addr, address->ai_addrlen) == 0 &&
            (type != SOCK_STREAM || listen(fd, 128) == 0)) {
            listener = fd;
            break;
        }
        last_error = errno;
        close(fd);
    }
    freeaddrinfo(addresses);
    if (listener < 0) errno = last_error;
    return listener;
}

int net_connect(const char *host, const char *port, int type, int family)
{
    if (net_port(port, 0) < 0) return -1;
    struct addrinfo hints = {0}, *addresses = NULL;
    hints.ai_family = family;
    hints.ai_socktype = type;
    hints.ai_flags = AI_NUMERICSERV;
    int result = getaddrinfo(host, port, &hints, &addresses);
    if (result != 0) { fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result)); errno = EINVAL; return -1; }
    int connection = -1, last_error = ECONNREFUSED;
    int64_t deadline = net_now_ms() + 5000;
    for (struct addrinfo *address = addresses; address != NULL; address = address->ai_next) {
        int fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (fd < 0) { last_error = errno; continue; }
        int flags = fcntl(fd, F_GETFL);
        if (flags < 0 || prepare(fd) < 0 || net_nonblocking(fd) < 0) { last_error = errno; close(fd); continue; }
        result = connect(fd, address->ai_addr, address->ai_addrlen);
        if (result < 0 && errno == EINPROGRESS && net_wait(fd, POLLOUT, deadline) == 0) {
            int error = 0;
            socklen_t length = sizeof error;
            if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &length) == 0) {
                result = error == 0 ? 0 : -1;
                if (error != 0) errno = error;
            }
        }
        if (result == 0 && fcntl(fd, F_SETFL, flags) == 0 && net_timeouts(fd, 5000) == 0) {
            connection = fd;
            break;
        }
        last_error = errno;
        close(fd);
    }
    freeaddrinfo(addresses);
    if (connection < 0) errno = last_error;
    return connection;
}

int net_accept(int listener)
{
    for (;;) {
        int fd = accept(listener, NULL, NULL);
        if (fd >= 0) {
            if (prepare(fd) == 0 && net_timeouts(fd, 5000) == 0) return fd;
            int error = errno; close(fd); errno = error; return -1;
        }
        if (errno != EINTR || net_stopping) return -1;
    }
}

ssize_t net_receive(int fd, void *buffer, size_t length)
{
    ssize_t result;
    do { result = recv(fd, buffer, length, 0); } while (result < 0 && errno == EINTR && !net_stopping);
    return result;
}

int net_send_all(int fd, const void *buffer, size_t length)
{
    const unsigned char *bytes = buffer;
    while (length > 0) {
        size_t chunk = length > INT_MAX ? INT_MAX : length;
#ifdef MSG_NOSIGNAL
        ssize_t result = send(fd, bytes, chunk, MSG_NOSIGNAL);
#else
        ssize_t result = send(fd, bytes, chunk, 0);
#endif
        if (result < 0 && errno == EINTR && !net_stopping) continue;
        if (result <= 0) { if (result == 0) errno = EPIPE; return -1; }
        bytes += result;
        length -= (size_t)result;
    }
    return 0;
}

int net_receive_exact(int fd, void *buffer, size_t length)
{
    unsigned char *bytes = buffer;
    size_t used = 0;
    while (used < length) {
        ssize_t result = net_receive(fd, bytes + used, length - used);
        if (result < 0) return -1;
        if (result == 0) { if (used == 0) return 0; errno = EPROTO; return -1; }
        used += (size_t)result;
    }
    return 1;
}

void net_report_listener(int fd)
{
    struct sockaddr_storage address;
    socklen_t length = sizeof address;
    char host[1025], service[16];
    if (getsockname(fd, (struct sockaddr *)&address, &length) == 0 &&
        getnameinfo((struct sockaddr *)&address, length, host, sizeof host, service, sizeof service,
                    NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
        printf("LISTEN %s %s\n", host, service);
        fflush(stdout);
    }
}

int net_streams(int fd, FILE **input, FILE **output)
{
    *input = *output = NULL;
    int flags = fcntl(fd, F_GETFD);
    if (flags < 0 || fcntl(fd, F_SETFD, flags | FD_CLOEXEC) < 0) {
        int error = errno; close(fd); errno = error; return -1;
    }
    /* dup() clears FD_CLOEXEC; create the second descriptor with it atomically. */
    int copy = fcntl(fd, F_DUPFD_CLOEXEC, 0);
    if (copy < 0) { int error = errno; close(fd); errno = error; return -1; }
    FILE *reader = fdopen(fd, "rb");
    if (reader == NULL) { int error = errno; close(fd); close(copy); errno = error; return -1; }
    FILE *writer = fdopen(copy, "wb");
    if (writer == NULL) { int error = errno; fclose(reader); close(copy); errno = error; return -1; }
    *input = reader; *output = writer;
    return 0;
}
