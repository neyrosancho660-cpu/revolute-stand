#include "canopen.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "stdio.h"

QueueHandle_t qCANRxRaw;


void CANOpen_App(void *pvParameters) 
{   
    (void)(pvParameters);
    
    bxCAN_Message_t rxMsg;
    qCANRxRaw = xQueueCreate(100, sizeof(bxCAN_Message_t));

    while(1) 
    {
        if (xQueueReceive(qCANRxRaw, &rxMsg, portMAX_DELAY) == pdPASS) 
        {   
            uint8_t nodeId = rxMsg.id & 0x7F;
            uint16_t func  = rxMsg.id & 0x780;

            for (int i = 0; i < MAX_NODES; i++) 
            {
                if (registered_nodes[i] && registered_nodes[i]->id == nodeId) 
                {

                    if (func == 0x580)
                        xQueueSend(registered_nodes[i]->qSDO, &rxMsg, 0);
                    else 
                    {
                        int cb_index = (func >> 8) - 1;
                        
                        if (cb_index >= 0 && cb_index < 4 && registered_nodes[i]->callbacks[cb_index] != NULL)
                        {
                            registered_nodes[i]->callbacks[cb_index](&rxMsg, registered_nodes[i]->context);
                        }
                    }

                    break;
                }
            }
        }
    }
}


void CANOpen_App_SendCanMessage(bxCAN_Message_t *msg, BaseType_t *pxHigherPriorityTaskWoken) 
{   
    if (qCANRxRaw != NULL)
    {
        xQueueSendFromISR(qCANRxRaw, msg, pxHigherPriorityTaskWoken);
    }
}