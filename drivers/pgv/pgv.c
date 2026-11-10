#include "pgv.h"
#include "cli.h"
#include "time.h"
#include "rs485.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"


PGV_Instance_t pgv = {.type=PGV_TYPE_IRAYPLE_RS485, .frequency=20};
StreamBufferHandle_t buffer;
IraypleRS485_Response_t response;

 
static void PGV_CLIHandler(int argc, char **argv);
static void IraypleRS485_Task(void *pvParameters);


cli_command_t pgv_cli = {
    .name = "pgv",
    .help = "help",
    .handler = PGV_CLIHandler
};


bool PGV_Init(void)
{   
    if (pgv.is_connected)
        return true;

    if (pgv.type == PGV_TYPE_IRAYPLE_CANOPEN)
    {
        if (IraypleCANOpen_Init(&pgv.irayple_canopen))
        {
            pgv.is_connected = true;
            return true;
        }
    }

    else if (pgv.type == PGV_TYPE_PEPPERL_FUCHS_CANOPEN)
    {
        if (PepperlFuchsCANOpen_Init(&pgv.pepperl_fuchs_canopen))
        {
            pgv.is_connected = true;
            return true;
        }
    }

    else if (pgv.type == PGV_TYPE_IRAYPLE_RS485)
    {
        if (!IraypleRS485_Init(&pgv.irayple_rs485))
            return false;
        
        pgv.is_connected = true;
        xTaskCreate(IraypleRS485_Task, "IraypleRS485_Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);

        return true;
    }
    
    pgv.is_connected = false;
    return false;
}

float PGV_GetAngle(void)
{
    if (pgv.type == PGV_TYPE_IRAYPLE_CANOPEN)
        return pgv.irayple_canopen.angle;
    else if (pgv.type == PGV_TYPE_PEPPERL_FUCHS_CANOPEN)
        return pgv.pepperl_fuchs_canopen.angle;
    else if (pgv.type == PGV_TYPE_IRAYPLE_RS485)
        return pgv.irayple_rs485.angle;

    return 0.0;
}


float PGV_GetX(void)
{
    if (pgv.type == PGV_TYPE_IRAYPLE_CANOPEN)
        return pgv.irayple_canopen.x;
    else if (pgv.type == PGV_TYPE_PEPPERL_FUCHS_CANOPEN)
        return pgv.pepperl_fuchs_canopen.x;
    else if (pgv.type == PGV_TYPE_IRAYPLE_RS485)
        return pgv.irayple_rs485.x;

    return 0.0;
}


float PGV_GetY(void)
{
    if (pgv.type == PGV_TYPE_IRAYPLE_CANOPEN)
        return pgv.irayple_canopen.y;
    else if (pgv.type == PGV_TYPE_PEPPERL_FUCHS_CANOPEN)
        return pgv.pepperl_fuchs_canopen.y;
    else if (pgv.type == PGV_TYPE_IRAYPLE_RS485)
        return pgv.irayple_rs485.y;

    
    return 0.0;
}


uint32_t PGV_GetTagID(void)
{
    if (pgv.type == PGV_TYPE_IRAYPLE_CANOPEN)
        return pgv.irayple_canopen.id;
    else if (pgv.type == PGV_TYPE_PEPPERL_FUCHS_CANOPEN)
        return pgv.pepperl_fuchs_canopen.id;
    else if (pgv.type == PGV_TYPE_IRAYPLE_RS485)
        return pgv.irayple_rs485.id;

    return 0;
}


bool PGV_GetVisibility(void)
{
    if (pgv.type == PGV_TYPE_IRAYPLE_CANOPEN)
        return pgv.irayple_canopen.y;
    else if (pgv.type == PGV_TYPE_PEPPERL_FUCHS_CANOPEN)
        return pgv.pepperl_fuchs_canopen.visibility;
    else if (pgv.type == PGV_TYPE_IRAYPLE_RS485)
        return pgv.irayple_rs485.visibility;

    return false;
}


void PGV_EnableLogging(void)
{
    if (pgv.type == PGV_TYPE_IRAYPLE_CANOPEN)
        pgv.irayple_canopen.logging_enabled = true;
    else if (pgv.type == PGV_TYPE_PEPPERL_FUCHS_CANOPEN)
        pgv.pepperl_fuchs_canopen.logging_enabled = true;
    else if (pgv.type == PGV_TYPE_IRAYPLE_RS485)
        pgv.irayple_rs485.logging_enabled = true;
    
}


void PGV_DisableLogging(void)
{
    if (pgv.type == PGV_TYPE_IRAYPLE_CANOPEN)
        pgv.irayple_canopen.logging_enabled = false;
    else if (pgv.type == PGV_TYPE_PEPPERL_FUCHS_CANOPEN)
        pgv.pepperl_fuchs_canopen.logging_enabled = false;
    else if (pgv.type == PGV_TYPE_IRAYPLE_RS485)
        pgv.irayple_rs485.logging_enabled = false;
}


static void PGV_CLIHandler(int argc, char **argv)
{
    if (argc < 2) 
    {
        printf("Usage: motor <start|stop>\n");
        return;
    }

    else if (strcmp(argv[1], "id") == 0)
    {   
        if (pgv.type == PGV_TYPE_IRAYPLE_CANOPEN)
            pgv.irayple_canopen.node.id = atoi(argv[2]);
        else if (pgv.type == PGV_TYPE_PEPPERL_FUCHS_CANOPEN)
            pgv.pepperl_fuchs_canopen.node.id = atoi(argv[2]);
    }

    else if (strcmp(argv[1], "type") == 0)
    {
        if (strcmp(argv[2], "irayple_canopen") == 0)
            pgv.type = PGV_TYPE_IRAYPLE_CANOPEN;
        else if (strcmp(argv[2], "pepperl_fuchs_canopen") == 0)
            pgv.type = PGV_TYPE_PEPPERL_FUCHS_CANOPEN;
        else if (strcmp(argv[2], "irayple_rs485") == 0)
            pgv.type = PGV_TYPE_IRAYPLE_RS485;
    }
    
    else if (strcmp(argv[1], "connect") == 0)
    {
        PGV_Init();
    }

    else if (strcmp(argv[1], "frequency") == 0)
    {
        uint32_t frequency = atoi(argv[2]);
        pgv.frequency = frequency;
    }
    
    else if (strcmp(argv[1], "enable_logging") == 0)
    {
        PGV_EnableLogging();
    }

    else if (strcmp(argv[1], "disable_logging") == 0)
    {
        PGV_DisableLogging();
    }

    else
    {
        printf("Incorrect command\n");
    }
}


static void IraypleRS485_Task(void *pvParameters)
{
    (void)(pvParameters);
    buffer = xStreamBufferCreate(IRAYPLE_RS485_RESPONSE_SIZE, IRAYPLE_RS485_RESPONSE_SIZE);
    TickType_t last_wake_time = xTaskGetTickCount();
    RS485_EnableRXIRQ(6);

    size_t size = 1;

    for(;;)
    {   
        if (RS485_Transmit(IraypleRS485_RequestData, IRAYPLE_RS485_REQUEST_SIZE, 1))
        {
            continue;
        }

        xStreamBufferReset(buffer);
        size = xStreamBufferReceive(buffer, &response.data, IRAYPLE_RS485_RESPONSE_SIZE, pdMS_TO_TICKS(pgv.frequency));

        if (size == IRAYPLE_RS485_RESPONSE_SIZE)
        {   
            IraypleRS485_parseData(&pgv.irayple_rs485, &response);
        }
        else
        {
            __NOP();
        }
        
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(pgv.frequency));
    }
}


void UART5_IRQHandler(void)
{   
    uint8_t byte;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    RS485_Receive(&byte, 1, 1);
    response.timestamp = getTime();
    xStreamBufferSendFromISR(buffer, &byte, 1, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}