#ifndef __TIMER_H__
#define __TIMER_H__

#include "stm32f1xx.h"

typedef struct
{
    uint32_t start_time;
    uint32_t timeout;

} timer;


void SysTick_Init(uint32_t core_freq, uint32_t timer_freq);
void setTimer(timer* timer_instance, uint32_t timeout);
uint8_t isTimerOver(timer* timer_instance);
void delay(uint32_t time);
uint32_t getTime(void);

#endif