# include "log.h"
# include "main.h"
# include "bxCAN.h"
# include "rs485.h"
#include "irayple_canopen.h"

#include "cli_app.h"
#include "main_app.h"
#include "canopen_app.h"

#include "FreeRTOS.h"
#include "task.h"


uint8_t SysClock_Init(void);


int main()
{
    SysClock_Init();
    SysTick_Init(72000000, 1000);


    bxCAN_Init(MBPS_1, 1, getTime);
    RS485_Init(BAUD_115200, 1);
    
    bxCAN_Filter_t filter;
    filter.BankNum = 0;
    filter.MailboxNum = 0;
    filter.FilterMode = CAN_FILTER_MODE_IDMASK;
    filter.FilterScale = CAN_FILTER_SCALE_32BIT;
    filter.FilterIdHigh = 0;
    filter.FilterIdLow = 0;
    filter.FilterMaskIdHigh = 0;
    filter.FilterMaskIdLow = 0;
    filter.FilterActivation = FILTER_ENABLE;

    bxCAN_SetFilter(&filter);
    bxCAN_InterruptEnable(bxCAN_INTERRUPT_FMPIE0 | bxCAN_INTERRUPT_FMPIE1);
    NVIC_EnableIRQ(CAN1_RX0_IRQn);
    NVIC_SetPriority(CAN1_RX0_IRQn, 5);

    log_init();
    xTaskCreate(CANOpen_App, "CANOpen_App", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(vCLITask, "CLI_App", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(Main_App, "Main_App", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

    vTaskStartScheduler();

    
    for(uint32_t i = 0; i < UINT32_MAX; i++)
    {
        printf("Time: %ld\n", i);
        delay(1000);
    }
}


uint8_t SysClock_Init(void)
{   
    uint8_t error_counter = 0;

    FLASH->ACR |= FLASH_ACR_LATENCY_2;

    RCC->CR |= RCC_CR_HSEON;

    while(!READ_BIT(RCC->CR, RCC_CR_HSERDY))
    {
        error_counter++;
        if (error_counter == 255)
            return 1;
    }

    RCC->CFGR = RCC_CFGR_PLLMULL9 | RCC_CFGR_PLLSRC;

    RCC->CR |= RCC_CR_PLLON;

    while(!READ_BIT(RCC->CR, RCC_CR_PLLRDY))
    {
        error_counter++;
        if (error_counter == 255)
            return 1;
    }

    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;

    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while(!READ_BIT(RCC->CFGR, RCC_CFGR_SWS_PLL))
    {
        error_counter++;
        if (error_counter == 255)
            return 1;
    }

    return 0;
}


void CAN1_RX0_IRQHandler(void)
{
    bxCAN_Message_t message;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (bxCAN_Receive(&message, 1) == CAN_OK)
    {
        CANOpen_App_SendCanMessage(&message, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

    __NOP();
}