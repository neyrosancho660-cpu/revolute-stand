#include "canopen.h"
#include <string.h>

#define MAX_NODES 4
CANOpen_Node_t* registered_nodes[MAX_NODES] = {NULL};


static bool CANOpen_RegisterNode(CANOpen_Node_t *node);
static void CANOpen_UnregisterNode(CANOpen_Node_t *node);


bool CANOpen_InitNode(CANOpen_Node_t *node) 
{
    node->qSDO = xQueueCreate(1, sizeof(bxCAN_Message_t));

    if (!CANOpen_RegisterNode(node)) 
    {
        vQueueDelete(node->qSDO);

        return false;
    }

    uint32_t devType = 0;
    if (CANOpen_SDO_Upload(node, 0x1000, 0, &devType)) 
    {
        CANOpen_UnregisterNode(node);
        vQueueDelete(node->qSDO);

        return false;
    }

    return true;
}


bool CANOpen_SendNMT(CANOpen_Node_t *node, NMT_Command_t command) 
{
    bxCAN_Message_t request = {.id = 0x000, .dlc = 2};

    request.data.byte[0] = (uint8_t)command;
    request.data.byte[1] = node->id;

    if (bxCAN_Transmit(&request, 1) != CAN_OK) 
    {
        return false;
    }

    return true;
}


uint8_t CANOpen_SDO_Download(CANOpen_Node_t *node, uint16_t index, uint8_t subindex, uint32_t val, uint8_t size, uint16_t timeout) 
{
    bxCAN_Message_t request = {0};
    bxCAN_Message_t response = {0};

    request.id = 0x600 + node->id;
    request.dlc = 8;
    request.ide = false;
    request.rtr = false;
    
    request.data.byte[0] = 0x1 << 5 | (4 - size) << 2 | 0x3;
    request.data.byte[1] = index & 0xFF;
    request.data.byte[2] = (index >> 8) & 0xFF;
    request.data.byte[3] = subindex;
    
    for(int i = 0; i < size; i++) 
    {
        request.data.byte[4 + i] = (val >> (8 * i)) & 0xFF;
    }

    xQueueReset(node->qSDO);
    if (bxCAN_Transmit(&request, 10) != CAN_OK) 
        return 0x1;

    if (xQueueReceive(node->qSDO, &response, pdMS_TO_TICKS(timeout)) == pdTRUE)
        if (response.data.byte[0] == 0x60)
            return 0x0;

    return 0x1;
}


uint8_t CANOpen_SDO_Upload(CANOpen_Node_t *node, uint16_t index, uint8_t subindex, uint32_t *pOut) 
{
    bxCAN_Message_t request = {0};
    bxCAN_Message_t response = {0};

    request.id = 0x600 + node->id;
    request.dlc = 8;
    request.ide = false;
    request.rtr = false;

    request.data.word[0] = 0;
    request.data.word[1] = 0;

    request.data.byte[0] = 0x40;
    request.data.byte[1] = index & 0xFF;
    request.data.byte[2] = (index >> 8) & 0xFF;
    request.data.byte[3] = subindex;

    xQueueReset(node->qSDO);
    if (bxCAN_Transmit(&request, 1) != CAN_OK)
        return 0x1;

    if (xQueueReceive(node->qSDO, &response, pdMS_TO_TICKS(100)) == pdTRUE) 
    {
        if (response.data.byte[0] == 0x80) 
            return 0x1;

        uint8_t size = 4 - ((response.data.byte[0] >> 2) & 0x03);
        *pOut = 0;
        memcpy(pOut, &response.data.byte[4], size);

        return 0x0;
    }

    return 0x1;
}


static bool CANOpen_RegisterNode(CANOpen_Node_t *node) 
{
    for(int i = 0; i < MAX_NODES; i++) 
    {
        if(registered_nodes[i] == NULL) 
        {
            registered_nodes[i] = node;
            return true;
        }
    }

    return false;
}

static void CANOpen_UnregisterNode(CANOpen_Node_t *node) 
{
    for(int i = 0; i < MAX_NODES; i++)
    {
        if(registered_nodes[i] && registered_nodes[i]->id == node->id) 
        {
            registered_nodes[i] = NULL;
            break;
        }
    }
}