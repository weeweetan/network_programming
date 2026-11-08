#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "check.h"
#include "Practical.h"
#include "common/net.h"

typedef struct { int fd, valid; size_t count; } Reader;
static void *read_fragments(void *argument)
{
    Reader *reader = argument;
    unsigned char bytes[257];
    reader->valid = 1;
    for (;;) {
        ssize_t count = net_receive(reader->fd, bytes, sizeof bytes);
        if (count == 0) break;
        if (count < 0) { reader->valid = 0; break; }
        for (ssize_t i = 0; i < count; ++i) {
            if (bytes[i] != (unsigned char)((reader->count + (size_t)i) % 251)) reader->valid = 0;
        }
        reader->count += (size_t)count;
    }
    return NULL;
}

int main(void)
{
    CHECK(net_init() == 0);
    uint64_t value = 99;
    CHECK(net_uint("18446744073709551615", UINT64_MAX, &value) == 0 && value == UINT64_MAX);
    CHECK(net_uint("18446744073709551616", UINT64_MAX, &value) == -1);
    CHECK(net_port("0", 1) == 0 && net_port("0", 0) == -1);
    CHECK(net_port("65535", 0) == 0 && net_port("65536", 0) == -1 && net_port("-1", 0) == -1);
    CHECK(net_uint("", 100, &value) == -1 && net_uint("1x", 100, &value) == -1);

    int pair[2]; CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    int small = 1024; CHECK(setsockopt(pair[0], SOL_SOCKET, SO_SNDBUF, &small, sizeof small) == 0);
    Reader reader = {pair[1], 0, 0};
    pthread_t thread; CHECK(pthread_create(&thread, NULL, read_fragments, &reader) == 0);
    const size_t size = 1024 * 1024;
    unsigned char *bytes = malloc(size); CHECK(bytes != NULL);
    for (size_t i = 0; i < size; ++i) bytes[i] = (unsigned char)(i % 251);
    CHECK(net_send_all(pair[0], bytes, size) == 0 && shutdown(pair[0], SHUT_WR) == 0);
    CHECK(pthread_join(thread, NULL) == 0 && reader.valid && reader.count == size);
    free(bytes); close(pair[0]); close(pair[1]);

    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    CHECK(net_send_all(pair[1], "xy", 2) == 0 && shutdown(pair[1], SHUT_WR) == 0);
    unsigned char buffer[4]; CHECK(net_receive_exact(pair[0], buffer, 4) == -1 && errno == EPROTO);
    CHECK(net_receive_exact(pair[0], buffer, 1) == 0);
    close(pair[1]); CHECK(net_send_all(pair[0], "x", 1) == -1); close(pair[0]);

    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    FILE *in = NULL, *out = NULL; CHECK(net_streams(pair[0], &in, &out) == 0);
    CHECK(net_send_all(pair[1], "a", 1) == 0 && getc(in) == 'a');
    CHECK(putc('b', out) != EOF && fflush(out) == 0);
    CHECK(net_receive_exact(pair[1], buffer, 1) == 1 && buffer[0] == 'b');
    CHECK(fclose(in) == 0 && fclose(out) == 0);
    CHECK(fcntl(pair[0], F_GETFD) == -1 && errno == EBADF); close(pair[1]);

    struct sockaddr_in6 a = {0}, b = {0};
    a.sin6_family = b.sin6_family = AF_INET6;
    a.sin6_port = b.sin6_port = htons(1234);
    a.sin6_addr = b.sin6_addr = in6addr_loopback;
    CHECK(SockAddrsEqual((struct sockaddr *)&a, (struct sockaddr *)&b));
    b.sin6_scope_id = 1; CHECK(!SockAddrsEqual((struct sockaddr *)&a, (struct sockaddr *)&b));
    return EXIT_SUCCESS;
}
