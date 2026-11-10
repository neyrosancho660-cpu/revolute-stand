#include "stdbool.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "SEGGER_RTT.h"


xSemaphoreHandle log_mutex;


bool log_init(void) 
{
    log_mutex = xSemaphoreCreateMutex();

    if (log_mutex == NULL)
        return false;

    return true;
}


void log_write(const char * sFormat, ...) 
{   
    if (xSemaphoreTake(log_mutex, portMAX_DELAY) == pdTRUE) 
    {
        va_list ParamList;

        va_start(ParamList, sFormat);
        SEGGER_RTT_vprintf(0, sFormat, &ParamList);
        va_end(ParamList);
        xSemaphoreGive(log_mutex);
    }
}