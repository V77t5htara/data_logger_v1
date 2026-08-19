#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

typedef void (*TaskCallback)(void);

typedef struct {
    TaskCallback callback;
    uint32_t period_ms;
    uint32_t last_run;
    uint8_t enabled;
} Task;

void Scheduler_Init(void);
int Scheduler_Add(TaskCallback callback, uint32_t period_ms);
void Scheduler_Run(void);

#endif
