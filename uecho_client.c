#include "common/net.h"

/* Shared I/O keeps the chapter entry point and its documented mode. */
int main(int argc, char **argv)
{
    return net_run_udp_client(argc, argv, 1, 1);
}
