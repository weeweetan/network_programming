#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "common/net.h"

enum { CHAT_CLIENTS = 128, CHAT_LINE = 2048 };
typedef struct { pthread_t thread; int fd, started, done; } Worker;
static Worker workers[CHAT_CLIENTS];
static size_t active;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t drained = PTHREAD_COND_INITIALIZER;

static void broadcast(const unsigned char *bytes, size_t length)
{
    pthread_mutex_lock(&lock);
    for (size_t i = 0; i < CHAT_CLIENTS; ++i) if (workers[i].started && !workers[i].done) {
        size_t offset = 0;
        while (offset < length) {
            int flags = MSG_DONTWAIT;
#ifdef MSG_NOSIGNAL
            flags |= MSG_NOSIGNAL;
#endif
            ssize_t count = send(workers[i].fd, bytes + offset, length - offset, flags);
            if (count > 0) offset += (size_t)count;
            else if (count < 0 && errno == EINTR && !net_stopping) continue;
            else { shutdown(workers[i].fd, SHUT_RDWR); break; }
        }
    }
    pthread_mutex_unlock(&lock);
}

static void *serve(void *argument)
{
    Worker *worker = argument;
    const int fd = worker->fd;
    unsigned char bytes[4096], line[CHAT_LINE];
    size_t used = 0;
    int failed = 0;
    while (!net_stopping && !failed) {
        ssize_t count = net_receive(fd, bytes, sizeof bytes);
        if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) continue;
        if (count <= 0) break;
        for (ssize_t i = 0; i < count; ++i) {
            if (used == sizeof line) { failed = 1; break; }
            line[used++] = bytes[i];
            if (bytes[i] == '\n') { broadcast(line, used); used = 0; }
        }
    }
    if (!failed && used != 0 && !net_stopping) broadcast(line, used);
    pthread_mutex_lock(&lock);
    close(fd); /* Broadcasts only request shutdown; this worker owns close(). */
    worker->done = 1;
    --active;
    pthread_cond_signal(&drained);
    pthread_mutex_unlock(&lock);
    return NULL;
}

static void reap(void)
{
    for (size_t i = 0; i < CHAT_CLIENTS; ++i) {
        pthread_mutex_lock(&lock);
        int finished = workers[i].started && workers[i].done;
        pthread_mutex_unlock(&lock);
        if (finished) {
            pthread_join(workers[i].thread, NULL);
            pthread_mutex_lock(&lock);
            workers[i].started = 0;
            pthread_mutex_unlock(&lock);
        }
    }
}

int main(int argc, char **argv)
{
    NetServerOptions options;
    if (net_server_options(argc, argv, &options) < 0) return 2;
    if (net_init() < 0) return 1;
    int listener = net_listener(options.host, options.port, SOCK_STREAM, AF_UNSPEC);
    if (listener < 0) { perror("listen"); return 1; }
    if (net_nonblocking(listener) < 0) { close(listener); return 1; }
    net_report_listener(listener);
    size_t accepted = 0;
    int status = 0;
    while (!net_stopping && (options.limit == 0 || accepted < options.limit)) {
        struct pollfd event = {listener, POLLIN, 0};
        int ready = poll(&event, 1, 200);
        if (ready < 0) { if (errno == EINTR) continue; status = 1; break; }
        if (ready == 0) { reap(); continue; }
        int fd = net_accept(listener);
        if (fd < 0) { if (errno == EAGAIN || errno == EWOULDBLOCK) continue; if (!net_stopping) status = 1; break; }
        ++accepted;
        int flags = fcntl(fd, F_GETFL);
        if (flags < 0 || fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) < 0) { close(fd); continue; }
        reap();
        pthread_mutex_lock(&lock);
        size_t slot = 0;
        while (slot < CHAT_CLIENTS && workers[slot].started) ++slot;
        if (slot == CHAT_CLIENTS) { pthread_mutex_unlock(&lock); close(fd); continue; }
        Worker *worker = &workers[slot];
        worker->fd = fd; worker->started = 1; worker->done = 0; ++active;
        int error = pthread_create(&worker->thread, NULL, serve, worker);
        if (error != 0) { --active; worker->started = 0; close(fd); status = 1; }
        pthread_mutex_unlock(&lock);
    }
    close(listener);
    pthread_mutex_lock(&lock);
    while (active != 0) {
        if (net_stopping) for (size_t i = 0; i < CHAT_CLIENTS; ++i)
            if (workers[i].started && !workers[i].done) shutdown(workers[i].fd, SHUT_RDWR);
        struct timespec deadline;
        clock_gettime(CLOCK_REALTIME, &deadline);
        deadline.tv_nsec += 100000000;
        if (deadline.tv_nsec >= 1000000000) { ++deadline.tv_sec; deadline.tv_nsec -= 1000000000; }
        pthread_cond_timedwait(&drained, &lock, &deadline);
    }
    pthread_mutex_unlock(&lock);
    reap(); /* Join all completed workers before destroying synchronization state. */
    pthread_cond_destroy(&drained);
    pthread_mutex_destroy(&lock);
    return status;
}
