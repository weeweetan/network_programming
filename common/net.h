#ifndef NETWORK_EXAMPLES_NET_H
#define NETWORK_EXAMPLES_NET_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <poll.h>
#include <signal.h>

typedef struct {
    const char *port;
    const char *host;
    size_t limit;
} NetServerOptions;
extern volatile sig_atomic_t net_stopping;
int net_init(void);
int net_server_options(int argc, char **argv, NetServerOptions *options);
int net_uint(const char *text, uint64_t maximum, uint64_t *value);
int net_port(const char *text, int allow_zero);
int net_listener(const char *host, const char *port, int type, int family);
int net_connect(const char *host, const char *port, int type, int family);
int net_accept(int listener);
int net_nonblocking(int fd);
int net_timeouts(int fd, int milliseconds);
int64_t net_now_ms(void);
int net_wait(int fd, short events, int64_t deadline);
ssize_t net_receive(int fd, void *buffer, size_t length);
int net_send_all(int fd, const void *buffer, size_t length);
int net_receive_exact(int fd, void *buffer, size_t length);
void net_report_listener(int fd);
/* Consumes fd on both success and failure; each FILE owns a distinct fd. */
int net_streams(int fd, FILE **input, FILE **output);
int net_run_echo_server(int argc, char **argv, int family, int greeting, int stdio_mode);
int net_run_echo_client(int argc, char **argv, int family, int greeting, int interactive);
int net_run_udp_server(int argc, char **argv);
int net_run_udp_client(int argc, char **argv, int interactive, int retries);
int net_run_select(int argc, char **argv, int rot13);
int net_run_epoll(int argc, char **argv, int edge);
int net_run_libevent(int argc, char **argv, int buffered);
#endif
