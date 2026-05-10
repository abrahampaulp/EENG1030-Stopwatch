#ifndef STOPWATCH_H
#define STOPWATCH_H

#include "stm32l4xx_hal.h"
#include <stdint.h>

/*
 * Stopwatch state machine
 * STOPPED  — initial state, time frozen, red LED on
 * RUNNING  — counting up, green LED on
 */
typedef enum {
    SW_STOPPED = 0,
    SW_RUNNING  = 1
} StopwatchState;

/* Stopwatch time structure */
typedef struct {
    uint32_t milliseconds;   /* 0–999  */
    uint32_t seconds;        /* 0–59   */
    uint32_t minutes;        /* 0–99   */
} StopwatchTime;

void Stopwatch_Init(void);
void Stopwatch_Start(void);
void Stopwatch_Stop(void);
void Stopwatch_Reset(void);
void Stopwatch_Tick(void);          /* call every 1ms from timer interrupt */
StopwatchTime Stopwatch_GetTime(void);
StopwatchState Stopwatch_GetState(void);

#endif