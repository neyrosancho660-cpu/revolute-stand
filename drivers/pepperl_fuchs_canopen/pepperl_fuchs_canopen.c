#include "log.h"
#include "pepperl_fuchs_canopen.h"
#include <stdio.h>

static void TPDO1_cllbck(bxCAN_Message_t *msg, void *context);
static void TPDO2_cllbck(bxCAN_Message_t *msg, void *context);
static void TPDO3_cllbck(bxCAN_Message_t *msg, void *context);
static void TPDO4_cllbck(bxCAN_Message_t *msg, void *context);


bool PepperlFuchsCANOpen_Init(PepperlFuchsCANOpen_Instance_t *instance)
{ 
    instance->node.context = instance;

    if (!CANOpen_InitNode(&instance->node))
        return false;

    instance->node.callbacks[0] = TPDO1_cllbck;
    instance->node.callbacks[1] = TPDO2_cllbck;
    instance->node.callbacks[2] = TPDO3_cllbck;
    instance->node.callbacks[3] = TPDO4_cllbck;

    CANOpen_SendNMT(&instance->node, NMT_Operational);

    return true;
}


void PepperlFuchsCANOpen_EnableLogging(PepperlFuchsCANOpen_Instance_t *instance)
{
    instance->logging_enabled = true;
}


void PepperlFuchsCANOpen_DisableLogging(PepperlFuchsCANOpen_Instance_t *instance)
{
    instance->logging_enabled = false;
}


static void TPDO1_cllbck(bxCAN_Message_t *msg, void *context)
{
   PepperlFuchsCANOpen_Instance_t *instance = (PepperlFuchsCANOpen_Instance_t *)context;

   instance->y = (float)(msg->data.byte[0] << 24 | msg->data.byte[1] << 16 | msg->data.byte[2] << 8 | msg->data.byte[3]) / 10;
   instance->visibility = (bool)(msg->data.byte[4] & 0x8);
}


static void TPDO2_cllbck(bxCAN_Message_t *msg, void *context)
{
    PepperlFuchsCANOpen_Instance_t *instance = (PepperlFuchsCANOpen_Instance_t *)context;

    instance->x = (float)(msg->data.byte[0] << 24 | msg->data.byte[1] << 16 | msg->data.byte[2] << 8 | msg->data.byte[3]) / 10;
    instance->angle = (float)(msg->data.byte[4] << 8 | msg->data.byte[5]) / 10;
    if (instance->logging_enabled)
      log_write("Time: %lu | ID: %lu | Angle: %d\n", msg->timestamp, instance->id, (int)(instance->angle * 1000));
}


static void TPDO3_cllbck(bxCAN_Message_t *msg, void *context)
{
    PepperlFuchsCANOpen_Instance_t *instance = (PepperlFuchsCANOpen_Instance_t *)context;

    instance->visibility = msg->data.byte[0] & 0x8;
    instance->id = msg->data.byte[4] << 24 | msg->data.byte[5] << 16 | msg->data.byte[6] << 8 | msg->data.byte[7];
    if (instance->logging_enabled)
      log_write("Time: %lu | ID: %lu | Angle: %d\n", msg->timestamp, instance->id, (int)(instance->angle * 1000));
}


static void TPDO4_cllbck(bxCAN_Message_t *msg, void *context)
{   
    (void)(msg);
    (void)(context);
}