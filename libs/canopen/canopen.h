#ifndef CANOPEN_H
#define CANOPEN_H

#include "bxCAN.h"
#include "FreeRTOS.h"
#include "queue.h"

#define MAX_NODES 4


typedef void (*TPDO_Callback_t)(bxCAN_Message_t *msg, void *context);


typedef enum {

    NMT_Operational    = 0x01,
    NMT_PreOperational = 0x80,
    NMT_ResetNode      = 0x81,
    NMT_Stop           = 0x02

} NMT_Command_t;


typedef struct {

    uint8_t id;

    QueueHandle_t qSDO;

    TPDO_Callback_t callbacks[4];
    void *context;

} CANOpen_Node_t;


extern CANOpen_Node_t* registered_nodes[MAX_NODES];


void CANOpen_ProcessNodeMessages(CANOpen_Node_t *node);
bool CANOpen_InitNode(CANOpen_Node_t *node);
bool CANOpen_SendNMT(CANOpen_Node_t *node, NMT_Command_t command);
uint8_t CANOpen_SDO_Upload(CANOpen_Node_t *node, uint16_t index, uint8_t subindex, uint32_t *pOut);
uint8_t CANOpen_SDO_Download(CANOpen_Node_t *node, uint16_t index, uint8_t subindex, uint32_t val, uint8_t size, uint16_t timeout);

#endif