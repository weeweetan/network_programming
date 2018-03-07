//
// Created by tw on 2018/3/2.
//
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include "Practical.h"

void InterruptSignalHandler(int signalType);

int main(int argc, char *arv[]) {
    struct sigaction handler;

    //Set InterruptSignalHandler() as handler function
    handler.sa_handler = InterruptSignalHandler;
    //Create mask that blocks all signals
    if (sigfillset(&handler.sa_mask) < 0)
        DieWithSystemMessage("sigfillset() failed");
    handler.sa_flags = 0;

    //Set signal handling for interrupt signal
    if (sigaction(SIGINT, &handler, 0) < 0)
        DieWithSystemMessage("sigaction() failed for SIGINT");

    for (;;)
        pause();
    exit(0);
}

void InterruptSignalHandler(int signalType) {
    puts("Interrupt Received. Exiting program");
    exit(1);
}
