#ifndef __CLI_APP_H__
#define __CLI_APP_H__


typedef struct {

    char command[100];
    
} CLI_Command_t;


void vCLITask(void *pvParameters);


#endif // __CLI_APP_H__