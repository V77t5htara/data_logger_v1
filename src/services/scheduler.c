#include "scheduler.h"
#include "stm32f1xx_hal.h"

#define MAX_TASKS 8

static Task tasks[MAX_TASKS];

void Scheduler_Init(void)
{
    for (uint32_t i = 0; i < MAX_TASKS; ++i) {
        tasks[i].enabled = 0;
    }
}

int Scheduler_Add(TaskCallback callback, uint32_t period_ms)
{
    for (uint32_t i = 0; i < MAX_TASKS; ++i) {
        if (!tasks[i].enabled) {
            tasks[i].callback = callback;
            tasks[i].period_ms = period_ms;
            tasks[i].last_run = HAL_GetTick();
            tasks[i].enabled = 1;
            return (int)i;
        }
    }
    return -1;
}

void Scheduler_Run(void)
{
    uint32_t now = HAL_GetTick();

    for (uint32_t i = 0; i < MAX_TASKS; ++i) {
        if (!tasks[i].enabled || !tasks[i].callback) continue;

        if ((uint32_t)(now - tasks[i].last_run) >= tasks[i].period_ms) {
            tasks[i].last_run += tasks[i].period_ms;
            tasks[i].callback();
        }
    }
}
