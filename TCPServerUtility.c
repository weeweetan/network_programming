#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include "Practical.h"
#include "common/net.h"

/* Borrows the descriptor: the accepting caller closes it exactly once. */
void HandleTCPClient(int fd)
{
    unsigned char bytes[16384];
    for (;;) {
        ssize_t length = net_receive(fd, bytes, sizeof bytes);
        if (length <= 0 || net_send_all(fd, bytes, (size_t)length) < 0) return;
    }
}

void PrintSocketAddress(const struct sockaddr *address, FILE *stream)
{
    if (address == NULL || stream == NULL) return;
    socklen_t length;
    if (address->sa_family == AF_INET) length = sizeof(struct sockaddr_in);
    else if (address->sa_family == AF_INET6) length = sizeof(struct sockaddr_in6);
    else { fputs("[unknown address]", stream); return; }
    char host[1025], port[16];
    if (getnameinfo(address, length, host, sizeof host, port, sizeof port, NI_NUMERICHOST | NI_NUMERICSERV) != 0)
        fputs("[invalid address]", stream);
    else fprintf(stream, address->sa_family == AF_INET6 ? "[%s]:%s" : "%s:%s", host, port);
}

int SetupTCPServerSocket(const char *service)
{
    int fd = net_listener("127.0.0.1", service, SOCK_STREAM, AF_UNSPEC);
    if (fd >= 0) net_report_listener(fd);
    return fd;
}

int AcceptTCPConnection(int listener) { return net_accept(listener); }

bool SockAddrsEqual(const struct sockaddr *left, const struct sockaddr *right)
{
    if (left == NULL || right == NULL) return left == right;
    if (left->sa_family != right->sa_family) return false;
    if (left->sa_family == AF_INET) {
        const struct sockaddr_in *a = (const struct sockaddr_in *)left, *b = (const struct sockaddr_in *)right;
        return a->sin_port == b->sin_port && a->sin_addr.s_addr == b->sin_addr.s_addr;
    }
    if (left->sa_family == AF_INET6) {
        const struct sockaddr_in6 *a = (const struct sockaddr_in6 *)left, *b = (const struct sockaddr_in6 *)right;
        return a->sin6_port == b->sin6_port && a->sin6_scope_id == b->sin6_scope_id &&
            memcmp(&a->sin6_addr, &b->sin6_addr, sizeof a->sin6_addr) == 0;
    }
    return false;
}
