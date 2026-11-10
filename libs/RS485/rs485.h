#ifndef __RS485_H__
#define __RS485_H__

#include "stm32f1xx.h"
#include "timer.h"


typedef enum
{
    RS485_STATUS_OK,
    RS485_STATUS_TIMEOUT
} RS485_Status_t;

typedef enum
{
    BAUD_4800,
    BAUD_9600,
    BAUD_19200,
    BAUD_38400,
    BAUD_57600,
    BAUD_115200
} RS485_Baudrate_t;


RS485_Status_t RS485_Init(RS485_Baudrate_t baudrate, uint32_t timeout);
RS485_Status_t RS485_Transmit(const uint8_t* data, uint16_t size, uint32_t timeout);
RS485_Status_t RS485_Receive(uint8_t* data, uint16_t size, uint32_t timeout);
void RS485_EnableRXIRQ(uint32_t priority);

#endif