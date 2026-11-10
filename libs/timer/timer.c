#include "timer.h"


uint32_t volatile counter = 0;


void SysTick_Init(uint32_t core_freq, uint32_t timer_freq)
{   

    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;

    TIM6->PSC = 1;
    TIM6->ARR = (core_freq /(2 * timer_freq)) - 1;
    TIM6->CNT = 0;

    TIM6->DIER |= TIM_DIER_UIE;
    TIM6->CR1 |= TIM_CR1_CEN;

    NVIC_EnableIRQ(TIM6_IRQn);
}


void setTimer(timer* timer_instance, uint32_t timeout)
{   
    timer_instance -> start_time = counter;
    timer_instance -> timeout = timeout;
}


uint8_t isTimerOver(timer* timer_instance)
{
    return counter - timer_instance->start_time > timer_instance->timeout;
}


uint32_t getTime(void)
{
   return counter;
}


void delay(uint32_t time)
{
    uint32_t start = counter;
    while (counter - start < time);
}


void TIM6_IRQHandler(void)
{   
    
    if (TIM6->SR & TIM_SR_UIF)
    {
        counter++;
    }

    CLEAR_BIT(TIM6->SR, TIM_SR_UIF);
}
