#include "stream.h"
#include <errno.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct EventServer EventServer;
typedef struct EventPeer {
    int fd, eof;
    EventServer *server;
    NetPeer *raw;
    struct event *read_event, *write_event;
    struct bufferevent *buffered;
    struct EventPeer *next;
} EventPeer;

struct EventServer {
    int listener, buffered;
    struct event_base *base;
    struct event *accept_event, *timer;
    NetServerOptions options;
    size_t accepted, active;
    EventPeer *peers;
};

static void drop(EventPeer *peer)
{
    EventServer *server = peer->server;
    EventPeer **position = &server->peers;
    while (*position != NULL && *position != peer) position = &(*position)->next;
    if (*position != NULL) *position = peer->next;
    if (peer->read_event != NULL) event_free(peer->read_event);
    if (peer->write_event != NULL) event_free(peer->write_event);
    if (peer->buffered != NULL) bufferevent_free(peer->buffered);
    else close(peer->fd);
    free(peer->raw);
    free(peer);
    --server->active;
}

static int raw_sync(EventPeer *peer)
{
    if (net_peer_readable(peer->raw)) {
        if (event_add(peer->read_event, NULL) < 0) return -1;
    } else event_del(peer->read_event);
    if (peer->raw->start < peer->raw->end) {
        if (event_add(peer->write_event, NULL) < 0) return -1;
    } else event_del(peer->write_event);
    return 0;
}

static void raw_io(evutil_socket_t fd, short events, void *context)
{
    (void)fd; (void)events;
    EventPeer *peer = context;
    if (net_peer_step(peer->raw, 1) != 0 || raw_sync(peer) < 0) drop(peer);
}

static int buffered_pump(EventPeer *peer)
{
    struct evbuffer *input = bufferevent_get_input(peer->buffered);
    struct evbuffer *output = bufferevent_get_output(peer->buffered);
    while (evbuffer_get_length(input) != 0 && evbuffer_get_length(output) < NET_QUEUE_CAPACITY) {
        unsigned char bytes[16384];
        size_t room = NET_QUEUE_CAPACITY - evbuffer_get_length(output);
        if (room > sizeof bytes) room = sizeof bytes;
        int count = evbuffer_remove(input, bytes, room);
        if (count <= 0) return -1;
        net_transform(bytes, (size_t)count, 1);
        if (evbuffer_add(output, bytes, (size_t)count) < 0) return -1;
    }
    if (peer->eof && evbuffer_get_length(input) == 0 && evbuffer_get_length(output) == 0) return 1;
    if (!peer->eof && evbuffer_get_length(output) < NET_QUEUE_CAPACITY)
        bufferevent_enable(peer->buffered, EV_READ);
    else bufferevent_disable(peer->buffered, EV_READ);
    return 0;
}

static void buffered_io(struct bufferevent *event, void *context)
{
    (void)event;
    EventPeer *peer = context;
    if (buffered_pump(peer) != 0) drop(peer);
}

static void buffered_event(struct bufferevent *event, short flags, void *context)
{
    (void)event;
    EventPeer *peer = context;
    if (flags & (BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT)) { drop(peer); return; }
    if (flags & BEV_EVENT_EOF) {
        peer->eof = 1;
        if (buffered_pump(peer) != 0) drop(peer);
    }
}

static int can_accept(const EventServer *server)
{
    return server->active < NET_MAX_CLIENTS &&
        (server->options.limit == 0 || server->accepted < server->options.limit);
}

static void accept_ready(evutil_socket_t listener, short flags, void *context)
{
    (void)flags;
    EventServer *server = context;
    while (can_accept(server)) {
        int fd = net_accept((int)listener);
        if (fd < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK && !net_stopping) perror("accept");
            break;
        }
        ++server->accepted;
        if (net_nonblocking(fd) < 0) { close(fd); continue; }
        EventPeer *peer = calloc(1, sizeof *peer);
        if (peer == NULL) { close(fd); continue; }
        peer->server = server; peer->fd = fd;
        peer->next = server->peers; server->peers = peer; ++server->active;
        if (server->buffered) {
            peer->buffered = bufferevent_socket_new(server->base, fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
            if (peer->buffered == NULL) { drop(peer); continue; }
            bufferevent_setcb(peer->buffered, buffered_io, buffered_io, buffered_event, peer);
            bufferevent_setwatermark(peer->buffered, EV_READ, 0, NET_QUEUE_CAPACITY);
            bufferevent_setwatermark(peer->buffered, EV_WRITE, NET_QUEUE_CAPACITY / 4, 0);
            if (bufferevent_enable(peer->buffered, EV_READ | EV_WRITE) < 0) { drop(peer); continue; }
        } else {
            peer->raw = net_peer_create(fd);
            peer->read_event = event_new(server->base, fd, EV_READ | EV_PERSIST, raw_io, peer);
            peer->write_event = event_new(server->base, fd, EV_WRITE | EV_PERSIST, raw_io, peer);
            if (peer->raw == NULL || peer->read_event == NULL || peer->write_event == NULL || raw_sync(peer) < 0)
                drop(peer);
        }
    }
    if (!can_accept(server)) event_del(server->accept_event);
}

static void tick(evutil_socket_t fd, short flags, void *context)
{
    (void)fd; (void)flags;
    EventServer *server = context;
    if (net_stopping || (server->options.limit != 0 && server->accepted >= server->options.limit && server->active == 0)) {
        event_base_loopbreak(server->base);
        return;
    }
    if (can_accept(server)) event_add(server->accept_event, NULL);
}

int net_run_libevent(int argc, char **argv, int buffered)
{
    EventServer server;
    memset(&server, 0, sizeof server);
    if (net_server_options(argc, argv, &server.options) < 0) return 2;
    if (net_init() < 0) return 1;
    server.buffered = buffered;
    server.listener = net_listener(server.options.host, server.options.port, SOCK_STREAM, AF_UNSPEC);
    if (server.listener < 0) { perror("listen"); return 1; }
    int status = 1;
    server.base = event_base_new();
    if (server.base == NULL || net_nonblocking(server.listener) < 0) goto cleanup;
    server.accept_event = event_new(server.base, server.listener, EV_READ | EV_PERSIST, accept_ready, &server);
    server.timer = event_new(server.base, -1, EV_PERSIST, tick, &server);
    if (server.accept_event == NULL || server.timer == NULL) goto cleanup;
    struct timeval interval = {0, 100000};
    if (event_add(server.accept_event, NULL) < 0 || event_add(server.timer, &interval) < 0) goto cleanup;
    net_report_listener(server.listener);
    if (event_base_dispatch(server.base) >= 0) status = 0;
cleanup:
    while (server.peers != NULL) drop(server.peers);
    if (server.accept_event != NULL) event_free(server.accept_event);
    if (server.timer != NULL) event_free(server.timer);
    if (server.base != NULL) event_base_free(server.base);
    close(server.listener);
    return status;
}
