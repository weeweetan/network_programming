#include "stream.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

NetPeer *net_peer_create(int fd)
{
    if (net_nonblocking(fd) < 0) return NULL;
    NetPeer *peer = calloc(1, sizeof *peer);
    if (peer != NULL) peer->fd = fd;
    return peer;
}

int net_peer_readable(const NetPeer *peer) { return !peer->eof && peer->end - peer->start < NET_QUEUE_CAPACITY; }

void net_transform(unsigned char *bytes, size_t length, int rot13)
{
    if (!rot13) return;
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = bytes[i];
        if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M')) bytes[i] = (unsigned char)(c + 13);
        else if ((c >= 'n' && c <= 'z') || (c >= 'N' && c <= 'Z')) bytes[i] = (unsigned char)(c - 13);
    }
}

int net_peer_step(NetPeer *peer, int rot13)
{
    /* A bounded amount of work per readiness callback prevents monopolization.
     * ET callers rearm EPOLLONESHOT after each step, including backpressure.
     */
    for (unsigned iteration = 0; iteration < 64; ++iteration) {
        int progress = 0;
        if (peer->start < peer->end) {
#ifdef MSG_NOSIGNAL
            ssize_t sent = send(peer->fd, peer->bytes + peer->start, peer->end - peer->start, MSG_NOSIGNAL);
#else
            ssize_t sent = send(peer->fd, peer->bytes + peer->start, peer->end - peer->start, 0);
#endif
            if (sent > 0) { peer->start += (size_t)sent; progress = 1; }
            else if (sent == 0) { errno = EPIPE; return -1; }
            else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) return -1;
        }
        if (peer->start == peer->end) peer->start = peer->end = 0;
        else if (peer->start != 0) {
            memmove(peer->bytes, peer->bytes + peer->start, peer->end - peer->start);
            peer->end -= peer->start;
            peer->start = 0;
        }
        if (net_peer_readable(peer)) {
            size_t room = NET_QUEUE_CAPACITY - peer->end;
            if (room > 16384) room = 16384;
            ssize_t received = recv(peer->fd, peer->bytes + peer->end, room, 0);
            if (received > 0) {
                net_transform(peer->bytes + peer->end, (size_t)received, rot13);
                peer->end += (size_t)received;
                progress = 1;
            } else if (received == 0) { peer->eof = 1; progress = 1; }
            else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) return -1;
        }
        if (peer->eof && peer->start == peer->end) return 1;
        if (!progress || net_stopping) break;
    }
    return 0;
}
