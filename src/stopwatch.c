#include "stopwatch.h"

static volatile StopwatchState _state = SW_STOPPED;
static volatile uint32_t _total_ms    = 0;   /* total elapsed milliseconds */

void Stopwatch_Init(void)
{
    _state    = SW_STOPPED;
    _total_ms = 0;
}

void Stopwatch_Start(void)  { _state = SW_RUNNING;  }
void Stopwatch_Stop(void)   { _state = SW_STOPPED;  }

void Stopwatch_Reset(void)
{
    _state    = SW_STOPPED;
    _total_ms = 0;
}

/*
 * Called every 1ms from TIM6 interrupt.
 * Only increments when state is RUNNING.
 */
void Stopwatch_Tick(void)
{
    if (_state == SW_RUNNING)
        _total_ms++;
}

StopwatchTime Stopwatch_GetTime(void)
{
    /* Take a snapshot — disable interrupts briefly so _total_ms
       doesn't change mid-read (atomic read for 32-bit on Cortex-M4) */
    uint32_t ms = _total_ms;

    StopwatchTime t;
    t.milliseconds = ms % 1000;
    t.seconds      = (ms / 1000) % 60;
    t.minutes      = (ms / 60000) % 100;
    return t;
}

StopwatchState Stopwatch_GetState(void)
{
    return _state;
}