#ifndef __MAIN_APP_H__
#define __MAIN_APP_H__

#include "cli.h"


typedef enum
{

    MAIN_APP_STATUS_STANDBY = 0,
    MAIN_APP_STATUS_START,
    MAIN_APP_STATUS_STOP,
    MAIN_APP_STATUS_HOMING,
    MAIN_APP_STATUS_RUNNING,
    

} MainApp_Status_t;

extern cli_command_t test_cli;

void Main_App(void *pvParameters);


#endif // __MAIN_APP_H__