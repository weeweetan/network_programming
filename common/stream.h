#ifndef NETWORK_EXAMPLES_STREAM_H
#define NETWORK_EXAMPLES_STREAM_H
#include "net.h"

enum { NET_QUEUE_CAPACITY = 65536, NET_MAX_CLIENTS = 128 };
typedef struct NetPeer {
    int fd;
    int eof;
    size_t start, end;
    unsigned char bytes[NET_QUEUE_CAPACITY];
    struct NetPeer *next;
} NetPeer;
NetPeer *net_peer_create(int fd);
/* -1 peer error, 0 live, 1 EOF with all output drained. Nonblocking only. */
int net_peer_step(NetPeer *peer, int rot13);
int net_peer_readable(const NetPeer *peer);
void net_transform(unsigned char *bytes, size_t length, int rot13);
#endif
