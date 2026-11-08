#include "stream.h"
#include <errno.h>
#include <stdlib.h>
#include <sys/select.h>
#include <unistd.h>

int net_run_select(int argc, char **argv, int rot13)
{
    NetServerOptions options;
    if (net_server_options(argc, argv, &options) < 0) return 2;
    if (net_init() < 0) return 1;
    int listener = net_listener(options.host, options.port, SOCK_STREAM, AF_UNSPEC);
    if (listener < 0) { perror("listen"); return 1; }
    if (listener >= FD_SETSIZE || net_nonblocking(listener) < 0) { fputs("Listener cannot be monitored by select\n", stderr); close(listener); return 1; }
    NetPeer *peers[FD_SETSIZE] = {0};
    size_t active = 0, accepted = 0;
    int status = 0;
    net_report_listener(listener);
    while (!net_stopping && (active != 0 || options.limit == 0 || accepted < options.limit)) {
        fd_set reads, writes;
        FD_ZERO(&reads); FD_ZERO(&writes);
        int accepting = options.limit == 0 || accepted < options.limit;
        if (accepting && active < NET_MAX_CLIENTS) FD_SET(listener, &reads);
        int maximum = listener;
        for (int fd = 0; fd < FD_SETSIZE; ++fd) if (peers[fd] != NULL) {
            if (net_peer_readable(peers[fd])) FD_SET(fd, &reads);
            if (peers[fd]->start < peers[fd]->end) FD_SET(fd, &writes);
            if (fd > maximum) maximum = fd;
        }
        struct timeval timeout = {0, 200000};
        int ready = select(maximum + 1, &reads, &writes, NULL, &timeout);
        if (ready < 0) { if (errno == EINTR) continue; perror("select"); status = 1; break; }
        if (FD_ISSET(listener, &reads)) {
            while (active < NET_MAX_CLIENTS && (options.limit == 0 || accepted < options.limit)) {
                int fd = net_accept(listener);
                if (fd < 0) { if (errno != EAGAIN && errno != EWOULDBLOCK && !net_stopping) { perror("accept"); status = 1; } break; }
                ++accepted;
                if (fd >= FD_SETSIZE) { close(fd); continue; }
                NetPeer *peer = net_peer_create(fd);
                if (peer == NULL) { close(fd); continue; }
                peers[fd] = peer; ++active;
            }
        }
        for (int fd = 0; fd <= maximum; ++fd) {
            if (peers[fd] == NULL || (!FD_ISSET(fd, &reads) && !FD_ISSET(fd, &writes))) continue;
            if (net_peer_step(peers[fd], rot13) != 0) {
                close(fd); free(peers[fd]); peers[fd] = NULL; --active;
            }
        }
    }
    for (int fd = 0; fd < FD_SETSIZE; ++fd) if (peers[fd] != NULL) { close(fd); free(peers[fd]); }
    close(listener);
    return status;
}
