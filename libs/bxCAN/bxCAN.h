#ifndef __bxCAN_H__
#define __bxCAN_H__

#include <stdbool.h>
#include "stm32f1xx.h"


#define bxCAN_INTERRUPT_TMEIE       0x00001
#define bxCAN_INTERRUPT_FMPIE0      0x00002
#define bxCAN_INTERRUPT_FFIE0       0x00004
#define bxCAN_INTERRUPT_FOVIE0      0x00008
#define bxCAN_INTERRUPT_FMPIE1      0x00010
#define bxCAN_INTERRUPT_FFIE1       0x00020
#define bxCAN_INTERRUPT_FOVIE1      0x00040
#define bxCAN_INTERRUPT_EWGIE       0x00100
#define bxCAN_INTERRUPT_EPVIE       0x00200
#define bxCAN_INTERRUPT_BOFIE       0x00400
#define bxCAN_INTERRUPT_LECIE       0x00800
#define bxCAN_INTERRUPT_ERRIE       0x08000
#define bxCAN_INTERRUPT_WKUIE       0x10000
#define bxCAN_INTERRUPT_SLKIE       0x20000


typedef enum
{

    MBPS_1,
    KBPS_800,
    KBPS_500,
    KBPS_250,
    KBPS_200,
    KBPS_125,
    KBPS_100,
    KBPS_50

} bxCAN_Baudrate_t;


typedef enum
{

    CAN_OK,
    CAN_ERROR,
    CAN_INAK_ERROR,
    CAN_TERR,
    CAN_ALST,
    CAN_BUSY,
    CAN_TIMEOUT

} bxCAN_Status_t;


typedef enum
{

    CAN_FILTER_MODE_IDMASK,
    CAN_FILTER_MODE_IDLIST

} bxCAN_FilterMode_t;


typedef enum
{

    CAN_FILTER_SCALE_16BIT,
    CAN_FILTER_SCALE_32BIT

} bxCAN_FilterScale_t;


typedef enum
{

    FILTER_DISABLE,
    FILTER_ENABLE

} bxCAN_FilterActivation_t;


typedef union
{
    uint8_t byte[8];
    uint32_t word[2];
} bxCAN_Data_t;


typedef struct 
{   
    uint32_t timestamp;

    uint32_t id;
    
    bool rtr;
    bool ide;

    uint8_t dlc;
    bxCAN_Data_t data;

} bxCAN_Message_t;


typedef struct 
{
    
    uint8_t BankNum;
    uint8_t MailboxNum;

    bxCAN_FilterMode_t FilterMode;
    bxCAN_FilterScale_t FilterScale;

    uint16_t FilterIdHigh;
    uint16_t FilterIdLow;

    uint16_t FilterMaskIdHigh;
    uint16_t FilterMaskIdLow;

    bxCAN_FilterActivation_t FilterActivation;

} bxCAN_Filter_t;


bxCAN_Status_t bxCAN_SetFilter(bxCAN_Filter_t* filter);
bxCAN_Status_t bxCAN_Init(bxCAN_Baudrate_t baudrate, uint32_t timeout, uint32_t (*time)(void));
bxCAN_Status_t bxCAN_Transmit(bxCAN_Message_t* message, uint32_t timeout);
bxCAN_Status_t bxCAN_Receive(bxCAN_Message_t* message, uint32_t timeout);
void bxCAN_InterruptEnable(uint32_t interrupt);

#endif