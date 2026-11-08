#include "common/net.h"

int main(int argc, char **argv)
{
    return net_run_libevent(argc, argv, 0);
}
