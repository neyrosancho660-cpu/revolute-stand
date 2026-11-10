#include "bxCAN.h"
#include "timer.h"


uint8_t __BRP[8] =  { 2,  3,  4,  9, 10, 18, 20, 45};
uint8_t __SEG1[8] = {15, 12, 15, 13, 15, 13, 15, 13};
uint8_t __SEG2[8] = { 2,  2,  2,  2,  2,  2,  2,  2};
uint32_t (*time_func)(void) = 0;

bxCAN_Status_t bxCAN_Init(bxCAN_Baudrate_t baudrate, uint32_t timeout, uint32_t (*time)(void))
{   
    timer timeout_t;

    if (time != 0)
    {
        time_func = time;
    }

    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;

    GPIOB->CRH &= ~(GPIO_CRH_CNF8_Msk | GPIO_CRH_MODE8_Msk);
    GPIOB->CRH |= GPIO_CRH_CNF8_0;

    GPIOB->CRH &= ~(GPIO_CRH_CNF9_Msk | GPIO_CRH_MODE9_Msk);
    GPIOB->CRH |= (GPIO_CRH_CNF9_1 | GPIO_CRH_MODE9_Msk);

    AFIO->MAPR |= AFIO_MAPR_CAN_REMAP_1;

    CLEAR_BIT(CAN1->MCR, CAN_MCR_SLEEP);

    setTimer(&timeout_t, timeout);
    while(READ_BIT(CAN1->MSR, CAN_MSR_SLAK))
    {
        if(isTimerOver(&timeout_t))
            return CAN_TIMEOUT;
    }
    
    SET_BIT(CAN1->MCR, CAN_MCR_INRQ);

    setTimer(&timeout_t, timeout);
    while(!READ_BIT(CAN1->MSR, CAN_MSR_INAK))
    {
        if(isTimerOver(&timeout_t))
            return CAN_TIMEOUT;
    }

    CAN1->BTR = ((__SEG1[baudrate] - 1) << CAN_BTR_TS1_Pos) |
                ((__SEG2[baudrate] - 1) << CAN_BTR_TS2_Pos) |
                ((__BRP[baudrate] - 1) << CAN_BTR_BRP_Pos);
    

    CLEAR_BIT(CAN1->MCR, CAN_MCR_INRQ);

    setTimer(&timeout_t, timeout);
    while(READ_BIT(CAN1->MSR, CAN_MSR_INAK))
    {
        if(isTimerOver(&timeout_t))
            return CAN_TIMEOUT;
    }

    return CAN_OK;
}


bxCAN_Status_t bxCAN_MailBoxTransmit(uint8_t mailbox_num, bxCAN_Message_t* message)
{   
    uint8_t RQCP_bit = mailbox_num * 8;
    uint8_t ALST_bit = mailbox_num * 8 + 2;
    uint8_t TERR_bit = mailbox_num * 8 + 3;

    if (message->ide)
        CAN1->sTxMailBox[mailbox_num].TIR = message->id << CAN_TI0R_EXID_Pos;
    else
        CAN1->sTxMailBox[mailbox_num].TIR = message->id << CAN_TI0R_STID_Pos;

    CAN1->sTxMailBox[mailbox_num].TDTR &= ~CAN_TDT0R_DLC_Msk;
    if (!message->rtr)
    {
        CAN1->sTxMailBox[mailbox_num].TDTR |= message->dlc & CAN_TDT0R_DLC_Msk;

        CAN1->sTxMailBox[mailbox_num].TDLR = message->data.word[0];
        CAN1->sTxMailBox[mailbox_num].TDHR = message->data.word[1];
    }

    CAN1->sTxMailBox[mailbox_num].TIR |= CAN_TI0R_TXRQ;
    
    while (READ_BIT(CAN1->TSR, (0x1 << RQCP_bit)) == 0x0)
        __NOP();

    if (READ_BIT(CAN1->TSR, (0x1 << TERR_bit)))
        return CAN_TERR;

    if (READ_BIT(CAN1->TSR, (0x1 << ALST_bit)))
        return CAN_ALST;

    return CAN_OK;
}


bxCAN_Status_t bxCAN_FIFOMailBoxReceive(uint8_t mailbox_num, bxCAN_Message_t* message)
{
    if (time_func != 0)
        message->timestamp = time_func();
    else
        message->timestamp = 0;
        
    message->ide = READ_BIT(CAN1->sFIFOMailBox[mailbox_num].RIR, CAN_RI0R_IDE);
    message->rtr = READ_BIT(CAN1->sFIFOMailBox[mailbox_num].RIR, CAN_RI0R_RTR);

    if (message->ide)
        message->id = CAN1->sFIFOMailBox[mailbox_num].RIR >> CAN_RI0R_EXID_Pos;
    else
        message->id = CAN1->sFIFOMailBox[mailbox_num].RIR >> CAN_RI0R_STID_Pos;

    if(!message->rtr)
    {
        message->dlc = CAN1->sFIFOMailBox[mailbox_num].RDTR & CAN_RDT0R_DLC_Msk;
        message->data.word[0] = CAN1->sFIFOMailBox[mailbox_num].RDLR;
        message->data.word[1] = CAN1->sFIFOMailBox[mailbox_num].RDHR;
    }

    switch (mailbox_num)
    {
    case 0:
        CAN1->RF0R |= CAN_RF0R_RFOM0;
        break;
    
    case 1:
        CAN1->RF1R |= CAN_RF1R_RFOM1;
        break;
    }

    return CAN_OK;
}


bxCAN_Status_t bxCAN_Transmit(bxCAN_Message_t* message, uint32_t timeout)
{   
    timer timeout_t;
    setTimer(&timeout_t, timeout);
    while (1)
    {
        if (READ_BIT(CAN1->TSR, CAN_TSR_TME0))
            return bxCAN_MailBoxTransmit(0, message);

        if (READ_BIT(CAN1->TSR, CAN_TSR_TME1))
            return bxCAN_MailBoxTransmit(1, message);

        if (READ_BIT(CAN1->TSR, CAN_TSR_TME2))
            return bxCAN_MailBoxTransmit(2, message);
        
        if (isTimerOver(&timeout_t))
            return CAN_TIMEOUT;
    }
}


bxCAN_Status_t bxCAN_Receive(bxCAN_Message_t* message, uint32_t timeout)
{
    timer timeout_t;
    setTimer(&timeout_t, timeout);
    while (1)
    {
        if (READ_BIT(CAN1->RF0R, CAN_RF0R_FMP0))
            return bxCAN_FIFOMailBoxReceive(0, message);

        if (READ_BIT(CAN1->RF1R, CAN_RF1R_FMP1))
            return bxCAN_FIFOMailBoxReceive(1, message);
        
        if (isTimerOver(&timeout_t))
            return CAN_TIMEOUT;
    }
}


bxCAN_Status_t bxCAN_SetFilter(bxCAN_Filter_t* filter)
{   
    uint32_t FilterBitPostion = (0x1 << filter->BankNum);

    CAN1->FMR |= CAN_FMR_FINIT;


    if (filter->FilterMode == CAN_FILTER_MODE_IDMASK)
        CAN1->FM1R &= ~(uint32_t)FilterBitPostion;
    else
        CAN1->FM1R |= (uint32_t)FilterBitPostion;


    if (filter->FilterScale == CAN_FILTER_SCALE_16BIT)
    {
        CAN1->FS1R &= ~FilterBitPostion;

        CAN1->sFilterRegister[filter->BankNum].FR1 = ((filter->FilterMaskIdLow) << 16U) | (filter->FilterIdLow);
        CAN1->sFilterRegister[filter->BankNum].FR2 = ((filter->FilterMaskIdHigh) << 16U) | (filter->FilterIdHigh);
    }

    if (filter->FilterScale == CAN_FILTER_SCALE_32BIT)
    {   
        CAN1->FS1R |= FilterBitPostion;

        CAN1->sFilterRegister[filter->BankNum].FR1 = ((filter->FilterIdHigh) << 16U) | (filter->FilterIdLow);
        CAN1->sFilterRegister[filter->BankNum].FR2 = ((filter->FilterMaskIdHigh) << 16U) | (filter->FilterMaskIdLow);
    }

    CAN1->FFA1R &= ~FilterBitPostion;
    if (filter->MailboxNum)
        CAN1->FFA1R |= FilterBitPostion;
    
    CAN1->FA1R &= ~FilterBitPostion;
    if (filter->FilterActivation == FILTER_ENABLE)
        CAN1->FA1R |= FilterBitPostion;

    CAN1->FMR &= ~((uint32_t)CAN_FMR_FINIT);
    

    return CAN_OK;
}


void bxCAN_InterruptEnable(uint32_t interrupt)
{
    CAN1->IER |= interrupt;
}


void bxCAN_InterruptDisable(uint32_t interrupt)
{
    CAN1->IER &= ~interrupt;
}