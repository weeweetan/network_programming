#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "common/net.h"

static int retry_ssl(SSL *ssl, int result, int fd, int64_t deadline)
{
    int error = SSL_get_error(ssl, result);
    if (net_stopping) { errno = EINTR; return -1; }
    if (error == SSL_ERROR_WANT_READ) return net_wait(fd, POLLIN, deadline);
    if (error == SSL_ERROR_WANT_WRITE) return net_wait(fd, POLLOUT, deadline);
    if (error == SSL_ERROR_SYSCALL && errno == EINTR) return 0;
    return -1;
}

static int valid_name(const char *name)
{
    size_t length = strlen(name);
    if (length == 0 || length > 253) return 0;
    for (const unsigned char *p = (const unsigned char *)name; *p; ++p)
        if (!isalnum(*p) && *p != '.' && *p != '-' && *p != ':') return 0;
    return 1;
}

int main(int argc, char **argv)
{
    if (argc < 3 || argc > 5 || net_port(argv[2], 0) < 0) {
        fprintf(stderr, "Usage: %s HOST PORT [CA_FILE [SERVER_NAME]]\n", argv[0]);
        return 2;
    }
    const char *name = argc == 5 ? argv[4] : argv[1];
    if (!valid_name(name)) { fputs("Invalid verification name\n", stderr); return 2; }
    if (net_init() < 0) return 1;
    int fd = -1, status = 1, connected = 0;
    SSL_CTX *context = SSL_CTX_new(TLS_client_method());
    SSL *ssl = NULL;
    if (context == NULL || SSL_CTX_set_min_proto_version(context, TLS1_2_VERSION) != 1) goto cleanup;
    SSL_CTX_set_verify(context, SSL_VERIFY_PEER, NULL);
    if (argc >= 4) {
        if (SSL_CTX_load_verify_locations(context, argv[3], NULL) != 1) goto cleanup;
    } else if (SSL_CTX_set_default_verify_paths(context) != 1) goto cleanup;

    ssl = SSL_new(context);
    if (ssl == NULL) goto cleanup;
    unsigned char address[sizeof(struct in6_addr)];
    int ip = inet_pton(AF_INET, name, address) == 1 || inet_pton(AF_INET6, name, address) == 1;
    if (ip) {
        if (X509_VERIFY_PARAM_set1_ip_asc(SSL_get0_param(ssl), name) != 1) goto cleanup;
    } else {
        if (SSL_set1_host(ssl, name) != 1 || SSL_set_tlsext_host_name(ssl, name) != 1) goto cleanup;
        SSL_set_hostflags(ssl, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
    }
    fd = net_connect(argv[1], argv[2], SOCK_STREAM, AF_UNSPEC);
    if (fd < 0 || net_nonblocking(fd) < 0 || SSL_set_fd(ssl, fd) != 1) goto cleanup;
    int64_t deadline = net_now_ms() + 5000;
    for (;;) {
        int result = SSL_connect(ssl);
        if (result == 1) { connected = 1; break; }
        if (retry_ssl(ssl, result, fd, deadline) < 0) goto cleanup;
    }
    if (SSL_get_verify_result(ssl) != X509_V_OK) goto cleanup;
    fprintf(stderr, "Verified TLS connection (%s)\n", SSL_get_version(ssl));

    char request[1024];
    int default_port = strcmp(argv[2], "443") == 0;
    int length = snprintf(request, sizeof request, "GET / HTTP/1.1\r\nHost: %s%s%s%s%s\r\nConnection: close\r\n\r\n",
                          strchr(name, ':') != NULL ? "[" : "", name, strchr(name, ':') != NULL ? "]" : "",
                          default_port ? "" : ":", default_port ? "" : argv[2]);
    if (length < 0 || (size_t)length >= sizeof request) goto cleanup;
    size_t offset = 0;
    deadline = net_now_ms() + 10000;
    while (offset < (size_t)length) {
        int result = SSL_write(ssl, request + offset, length - (int)offset);
        if (result > 0) offset += (size_t)result;
        else if (retry_ssl(ssl, result, fd, deadline) < 0) goto cleanup;
    }
    for (;;) {
        unsigned char bytes[16384];
        int received = SSL_read(ssl, bytes, sizeof bytes);
        if (received > 0) {
            if (fwrite(bytes, 1, (size_t)received, stdout) != (size_t)received) goto cleanup;
        } else {
            int error = SSL_get_error(ssl, received);
            if (error == SSL_ERROR_ZERO_RETURN) { status = 0; break; }
            if (retry_ssl(ssl, received, fd, deadline) < 0) goto cleanup;
        }
        if (net_now_ms() >= deadline) { errno = ETIMEDOUT; goto cleanup; }
    }
cleanup:
    if (status != 0) {
        fputs("TLS exchange failed\n", stderr);
        ERR_print_errors_fp(stderr);
        if (errno == ETIMEDOUT) perror("TLS timeout");
    }
    if (connected) (void)SSL_shutdown(ssl); /* Nonblocking: never wait indefinitely during cleanup. */
    SSL_free(ssl);
    if (fd >= 0) close(fd);
    SSL_CTX_free(context);
    return status;
}
