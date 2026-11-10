#ifndef __PEPPERL_FUCHS_CANOPEN_H__
#define __PEPPERL_FUCHS_CANOPEN_H__


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

} PepperlFuchsCANOpen_Instance_t;



bool PepperlFuchsCANOpen_Init(PepperlFuchsCANOpen_Instance_t *instance);
void PepperlFuchsCANOpen_EnableLogging(PepperlFuchsCANOpen_Instance_t *instance);
void PepperlFuchsCANOpen_DisableLogging(PepperlFuchsCANOpen_Instance_t *instance);

#endif