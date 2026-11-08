#include "stream.h"
#include <errno.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

static uint32_t interests(const NetPeer *peer, int edge)
{
    uint32_t events = 0;
    if (net_peer_readable(peer)) events |= EPOLLIN | EPOLLRDHUP;
    if (peer->start < peer->end) events |= EPOLLOUT;
    if (edge) events |= EPOLLET | EPOLLONESHOT;
    return events;
}

static void remove_peer(int epoll_fd, NetPeer **list, NetPeer *peer)
{
    NetPeer **entry = list;
    while (*entry != NULL && *entry != peer) entry = &(*entry)->next;
    if (*entry != NULL) *entry = peer->next;
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, peer->fd, NULL);
    close(peer->fd);
    free(peer);
}

int net_run_epoll(int argc, char **argv, int edge)
{
    NetServerOptions options;
    if (net_server_options(argc, argv, &options) < 0) return 2;
    if (net_init() < 0) return 1;
    int listener = net_listener(options.host, options.port, SOCK_STREAM, AF_UNSPEC);
    if (listener < 0) { perror("listen"); return 1; }
    int poller = epoll_create1(EPOLL_CLOEXEC);
    if (poller < 0 || net_nonblocking(listener) < 0) { if (poller >= 0) close(poller); close(listener); return 1; }
    struct epoll_event registration = {0};
    registration.events = EPOLLIN;
    registration.data.ptr = NULL;
    if (epoll_ctl(poller, EPOLL_CTL_ADD, listener, &registration) < 0) { close(poller); close(listener); return 1; }
    NetPeer *peers = NULL;
    size_t accepted = 0, active = 0;
    int registered = 1, status = 0;
    net_report_listener(listener);
    while (!net_stopping && (active != 0 || options.limit == 0 || accepted < options.limit)) {
        int accepting = active < NET_MAX_CLIENTS && (options.limit == 0 || accepted < options.limit);
        if (accepting != registered) {
            if (epoll_ctl(poller, accepting ? EPOLL_CTL_ADD : EPOLL_CTL_DEL, listener, accepting ? &registration : NULL) < 0) { status = 1; break; }
            registered = accepting;
        }
        struct epoll_event events[64];
        int count = epoll_wait(poller, events, 64, 200);
        if (count < 0) { if (errno == EINTR) continue; perror("epoll_wait"); status = 1; break; }
        for (int i = 0; i < count; ++i) {
            if (events[i].data.ptr == NULL) {
                while (active < NET_MAX_CLIENTS && (options.limit == 0 || accepted < options.limit)) {
                    int fd = net_accept(listener);
                    if (fd < 0) { if (errno != EAGAIN && errno != EWOULDBLOCK && !net_stopping) status = 1; break; }
                    ++accepted;
                    NetPeer *peer = net_peer_create(fd);
                    if (peer == NULL) { close(fd); continue; }
                    struct epoll_event event = {0};
                    event.events = interests(peer, edge); event.data.ptr = peer;
                    if (epoll_ctl(poller, EPOLL_CTL_ADD, fd, &event) < 0) { close(fd); free(peer); continue; }
                    peer->next = peers; peers = peer; ++active;
                }
            } else {
                NetPeer *peer = events[i].data.ptr;
                int result = net_peer_step(peer, 0);
                struct epoll_event event = {0};
                event.events = interests(peer, edge); event.data.ptr = peer;
                if (result != 0 || epoll_ctl(poller, EPOLL_CTL_MOD, peer->fd, &event) < 0) {
                    remove_peer(poller, &peers, peer); --active;
                }
            }
        }
    }
    while (peers != NULL) remove_peer(poller, &peers, peers);
    close(poller); close(listener);
    return status;
}
