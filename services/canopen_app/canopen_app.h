#ifndef __CANOPEN_APP_H__
#define __CANOPEN_APP_H__

#include "canopen.h"


void CANOpen_App(void *pvParameters);
void CANOpen_App_SendCanMessage(bxCAN_Message_t *msg, BaseType_t *pxHigherPriorityTaskWoken);


#endif