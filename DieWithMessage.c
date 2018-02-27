//
// Created by tw on 2018/2/27.
//
#include <stdio.h>
#include <stdlib.h>
#include "Practical.h"

void DieWithUserMessage(const char *msg, const char *detail)
{
    fputs(msg, stderr);
    fputs(": ", stderr);
    fputs(detail, stderr);
    fputs("\n", stderr);
    exit(1);
}

void DieWithSystemMessage(const char *msg)
{
    perror(msg);
    exit(1);
}



