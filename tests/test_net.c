#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
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

static int inherited_socket(int fd, uint64_t device, uint64_t inode)
{
    struct stat status;
    return fstat(fd, &status) == 0 && S_ISSOCK(status.st_mode) &&
        (uint64_t)status.st_dev == device && (uint64_t)status.st_ino == inode;
}

static int probe_inheritance(int argc, char **argv)
{
    uint64_t read_fd, write_fd, control_fd, device, inode;
    if (argc != 7 || strcmp(argv[1], "--probe-inheritance") != 0 ||
        net_uint(argv[2], INT_MAX, &read_fd) < 0 || net_uint(argv[3], INT_MAX, &write_fd) < 0 ||
        net_uint(argv[4], INT_MAX, &control_fd) < 0 || net_uint(argv[5], UINT64_MAX, &device) < 0 ||
        net_uint(argv[6], UINT64_MAX, &inode) < 0) return 2;
    if (fcntl((int)control_fd, F_GETFD) < 0) return 3;
    /* The loader may reuse a closed fd number. Check socket identity, not just the number. */
    if (inherited_socket((int)read_fd, device, inode) || inherited_socket((int)write_fd, device, inode)) {
        fputs("FILE stream socket survived exec\n", stderr);
        return 4;
    }
    return 0;
}

static void check_stream_exec(const char *self, int initially_cloexec)
{
    int pair[2]; CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    if (initially_cloexec) CHECK(fcntl(pair[0], F_SETFD, FD_CLOEXEC) == 0);
    FILE *in = NULL, *out = NULL; CHECK(net_streams(pair[0], &in, &out) == 0);
    int read_fd = fileno(in), write_fd = fileno(out);
    CHECK(read_fd != write_fd && (fcntl(pair[1], F_GETFD) & FD_CLOEXEC) == 0);
    struct stat identity; CHECK(fstat(read_fd, &identity) == 0);
    char descriptors[3][32], device[32], inode[32];
    CHECK(snprintf(descriptors[0], sizeof descriptors[0], "%d", read_fd) > 0);
    CHECK(snprintf(descriptors[1], sizeof descriptors[1], "%d", write_fd) > 0);
    CHECK(snprintf(descriptors[2], sizeof descriptors[2], "%d", pair[1]) > 0);
    CHECK(snprintf(device, sizeof device, "%" PRIu64, (uint64_t)identity.st_dev) > 0);
    CHECK(snprintf(inode, sizeof inode, "%" PRIu64, (uint64_t)identity.st_ino) > 0);
    pid_t child = fork(); CHECK(child >= 0);
    if (child == 0) {
        execl(self, self, "--probe-inheritance", descriptors[0], descriptors[1], descriptors[2],
              device, inode, (char *)NULL);
        _exit(127);
    }
    int status;
    pid_t waited;
    do { waited = waitpid(child, &status, 0); } while (waited < 0 && errno == EINTR);
    CHECK(waited == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
    /* exec must only affect the child: both parent streams still work. */
    unsigned char byte;
    CHECK(net_send_all(pair[1], "a", 1) == 0 && getc(in) == 'a');
    CHECK(putc('b', out) != EOF && fflush(out) == 0);
    CHECK(net_receive_exact(pair[1], &byte, 1) == 1 && byte == 'b');
    CHECK(fclose(in) == 0 && fclose(out) == 0);
    CHECK(fcntl(read_fd, F_GETFD) == -1 && errno == EBADF);
    CHECK(fcntl(write_fd, F_GETFD) == -1 && errno == EBADF);
    close(pair[1]);
}

int main(int argc, char **argv)
{
    if (argc > 1) return probe_inheritance(argc, argv);
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

    check_stream_exec(argv[0], 1);
    check_stream_exec(argv[0], 0);
    FILE *in = stdout, *out = stderr;
    CHECK(net_streams(-1, &in, &out) == -1 && errno == EBADF && in == NULL && out == NULL);
    int readonly = open("/dev/null", O_RDONLY); CHECK(readonly >= 0);
    CHECK(net_streams(readonly, &in, &out) == -1 && in == NULL && out == NULL);
    CHECK(fcntl(readonly, F_GETFD) == -1 && errno == EBADF);

    struct sockaddr_in6 a = {0}, b = {0};
    a.sin6_family = b.sin6_family = AF_INET6;
    a.sin6_port = b.sin6_port = htons(1234);
    a.sin6_addr = b.sin6_addr = in6addr_loopback;
    CHECK(SockAddrsEqual((struct sockaddr *)&a, (struct sockaddr *)&b));
    b.sin6_scope_id = 1; CHECK(!SockAddrsEqual((struct sockaddr *)&a, (struct sockaddr *)&b));
    return EXIT_SUCCESS;
}
