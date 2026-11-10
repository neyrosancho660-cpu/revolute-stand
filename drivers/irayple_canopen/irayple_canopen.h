#ifndef __IRAYPLE_CANOPEN_H__
#define __IRAYPLE_CANOPEN_H__


#include "canopen.h"


typedef struct
{
    
    CANOpen_Node_t node;

    bool visibility;

    uint32_t id;

    float y;
    float x;
    float angle;

    bool logging_enabled;

} IraypleCANOpen_Instance_t;



bool IraypleCANOpen_Init(IraypleCANOpen_Instance_t *instance);
void IraypleCANOpen_EnableLogging(IraypleCANOpen_Instance_t *instance);
void IraypleCANOpen_DisableLogging(IraypleCANOpen_Instance_t *instance);

#endif