#include "Practical.h"
#include "common/net.h"

int SetupTCPClientSocket(const char *host, const char *service)
{
    return net_connect(host, service, SOCK_STREAM, AF_UNSPEC);
}
