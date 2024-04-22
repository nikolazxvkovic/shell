//
// Created by barnabas on 19-3-24.
//
#ifndef OS_PIPELINE_H
#define OS_PIPELINE_H
#define EXIT_COMMAND_NOT_FOUND 127
#define EXIT_EXIT 100

#include "scanner.h"
#include <stdlib.h>
List buildPipeline(List lp);
pid_t* executePipeline(PipeLine pipeLine, int* pipeFDs, int* status, int isBackground);

#endif //OS_PIPELINE_H
