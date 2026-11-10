#include "cli.h"
#include "pgv.h"
#include "kinco.h"
#include "cli_app.h"
#include "main_app.h"
#include "SEGGER_RTT.h"

#include "FreeRTOS.h"
#include "task.h"


void vCLITask(void *pvParameters) 
{   
    (void)(pvParameters);
    
    static uint16_t idx = 0;
    static char cmd_buffer[128];
    
    CLI_RegisterCommand(&motor_cmd);
    CLI_RegisterCommand(&pgv_cli);
    CLI_RegisterCommand(&test_cli);

    for (;;) 
    {
        char c;
        uint8_t size = SEGGER_RTT_Read(0, &c, 1);
        if (size > 0) 
        {
            if (c == '\r' || c == '\n') 
            {
                cmd_buffer[idx] = '\0';

                if (idx > 0) 
                {
                    CLI_ProcessLine(cmd_buffer);
                }

                idx = 0;
            }

            else if (c == '\b' || c == 127) 
            {
                if (idx > 0) {
                    idx--;
                }
            }

            else if (idx < sizeof(cmd_buffer) - 1) 
            {
                cmd_buffer[idx++] = c;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

