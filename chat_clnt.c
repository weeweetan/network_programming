#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "common/net.h"

static int send_line(int fd, const char *label, const unsigned char *line, size_t length)
{
    unsigned char message[1100];
    size_t prefix = strlen(label);
    memcpy(message, label, prefix); memcpy(message + prefix, line, length);
    return net_send_all(fd, message, prefix + length);
}

int main(int argc, char **argv)
{
    if (argc != 4 || net_port(argv[2], 0) < 0 || strlen(argv[3]) == 0 || strlen(argv[3]) > 16 ||
        strpbrk(argv[3], "\r\n") != NULL) {
        fprintf(stderr, "Usage: %s HOST PORT NAME (1..16 bytes)\n", argv[0]); return 2;
    }
    if (net_init() < 0) return 1;
    int fd = net_connect(argv[1], argv[2], SOCK_STREAM, AF_UNSPEC);
    if (fd < 0) { perror("connect"); return 1; }
    char label[32]; snprintf(label, sizeof label, "[%s] ", argv[3]);
    unsigned char line[1025];
    size_t used = 0;
    int input_open = 1, status = 0;
    int64_t deadline = 0;
    while (!net_stopping) {
        struct pollfd events[2] = {{fd, POLLIN, 0}, {input_open ? STDIN_FILENO : -1, POLLIN, 0}};
        if (!input_open && net_now_ms() >= deadline) { fputs("Chat shutdown timeout\n", stderr); status = 1; break; }
        int ready = poll(events, 2, 200);
        if (ready < 0) { if (errno == EINTR) continue; status = 1; break; }
        if (events[0].revents & (POLLIN | POLLHUP | POLLERR)) {
            unsigned char bytes[4096];
            ssize_t count = net_receive(fd, bytes, sizeof bytes);
            if (count == 0) break;
            if (count < 0 || fwrite(bytes, 1, (size_t)count, stdout) != (size_t)count) { status = 1; break; }
            fflush(stdout);
        }
        if (input_open && (events[1].revents & (POLLIN | POLLHUP))) {
            unsigned char bytes[1024];
            ssize_t count = read(STDIN_FILENO, bytes, sizeof bytes);
            if (count < 0) { if (errno == EINTR) continue; status = 1; break; }
            if (count == 0) {
                if (used != 0 && send_line(fd, label, line, used) < 0) { status = 1; break; }
                used = 0; input_open = 0;
            } else for (ssize_t i = 0; i < count; ++i) {
                line[used++] = bytes[i];
                if (used == sizeof line) { fputs("Chat line exceeds 1024 bytes\n", stderr); status = 2; input_open = 0; break; }
                if (bytes[i] == '\n') {
                    if (used == 2 && (line[0] == 'q' || line[0] == 'Q') && line[1] == '\n') {
                        used = 0; input_open = 0; break;
                    }
                    if (send_line(fd, label, line, used) < 0) { status = 1; input_open = 0; break; }
                    used = 0;
                }
            }
            if (!input_open) { shutdown(fd, SHUT_WR); deadline = net_now_ms() + 5000; }
            if (status != 0) break;
        }
    }
    close(fd);
    return status;
}
