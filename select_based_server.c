#include "common/net.h"

int main(int argc, char **argv)
{
    return net_run_select(argc, argv, 1);
}
