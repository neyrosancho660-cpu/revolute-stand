#include "rs485.h"


static uint16_t __BRR[] = 
{
    7500,
    3750,
    1875,
    1250,
    625,
    312
};


RS485_Status_t RS485_Init(RS485_Baudrate_t baudrate, uint32_t timeout)
{

    timer timeout_t;

    RCC->APB1ENR |= RCC_APB1ENR_UART5EN;
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPDEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
    
    AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;

    GPIOC->CRH &= ~(GPIO_CRH_CNF12_Msk | GPIO_CRH_MODE12_Msk);
    GPIOC->CRH |= (GPIO_CRH_CNF12_1 | GPIO_CRH_MODE12_Msk);

    GPIOD->CRL &= ~(GPIO_CRL_CNF2_Msk | GPIO_CRL_MODE2_Msk);
    GPIOD->CRL |= GPIO_CRL_CNF2_0;

    GPIOA->CRH &= ~(GPIO_CRH_CNF15_Msk | GPIO_CRH_MODE15_Msk);
    GPIOA->CRH |= (GPIO_CRH_MODE15_Msk);
    GPIOA->BSRR |= GPIO_BSRR_BR15;

    UART5->CR1 |= USART_CR1_UE;
    UART5->BRR = __BRR[baudrate];

    UART5->CR1 |= USART_CR1_TE | USART_CR1_RE;

    setTimer(&timeout_t, timeout);
    while (!READ_BIT(UART5->SR, USART_SR_TXE))
    {
        if (isTimerOver(&timeout_t))
            return RS485_STATUS_TIMEOUT;
    }

    return RS485_STATUS_OK;
}


RS485_Status_t RS485_TransmitByte(uint8_t byte, timer* timeout_t)
{

    while (!READ_BIT(UART5->SR, USART_SR_TXE))
    {
        if (isTimerOver(timeout_t))
            return RS485_STATUS_TIMEOUT;
    }

    UART5->DR = byte;

    while (!READ_BIT(UART5->SR, USART_SR_TXE))
    {
        if (isTimerOver(timeout_t))
            return RS485_STATUS_TIMEOUT;
    }

    return RS485_STATUS_OK;
}


RS485_Status_t RS485_Transmit(const uint8_t* data, uint16_t size, uint32_t timeout)
{
    timer timeout_t;
    setTimer(&timeout_t, timeout);

    GPIOA->BSRR |= GPIO_BSRR_BS15;
    
    RS485_Status_t status = RS485_STATUS_OK;
    for(uint16_t i = 0; i < size; i++)
    {
        status = RS485_TransmitByte(data[i], &timeout_t);
        if (status)
            break;
    }

    while (!READ_BIT(UART5->SR, USART_SR_TC))
    {
        if (isTimerOver(&timeout_t))
            return RS485_STATUS_TIMEOUT;
    }

    GPIOA->BSRR |= GPIO_BSRR_BR15;
    
    return RS485_STATUS_OK;
}


RS485_Status_t RS485_ReceiveByte(uint8_t* byte, timer* timer)
{
    while(!READ_BIT(UART5->SR, USART_SR_RXNE))
    {
        if(isTimerOver(timer))
            return RS485_STATUS_TIMEOUT;
    }

    *byte = UART5->DR;

    return RS485_STATUS_OK;
}


RS485_Status_t RS485_Receive(uint8_t* data, uint16_t size, uint32_t timeout)
{
    timer timeout_t;
    RS485_Status_t status = RS485_STATUS_OK;

    setTimer(&timeout_t, timeout);
    for (uint16_t i = 0; i < size; i++)
    {
        status = RS485_ReceiveByte(data + i, &timeout_t);
        if (status)
            return status;
    }

    return RS485_STATUS_OK;
}


void RS485_EnableRXIRQ(uint32_t priority)
{
    NVIC_SetPriority(UART5_IRQn, priority);
    UART5->CR1 |= USART_CR1_RXNEIE;
    NVIC_EnableIRQ(UART5_IRQn);
}