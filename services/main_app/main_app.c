#include "pgv.h"
#include "kinco.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdbool.h"
#include "string.h"
#include "main_app.h"

#include "FreeRTOS.h"
#include "task.h"


static bool homing(void);
static void running(void);
static bool initialization(void);
static void Test_CLIHandler(int argc, char **argv);



uint32_t id = 122;
float speed = 300;
float base_angle = 45;
uint32_t time = 60 * 1000;

MainApp_Status_t state = MAIN_APP_STATUS_STANDBY;

cli_command_t test_cli = {
    .name = "test",
    .help = "help",
    .handler = Test_CLIHandler
};


void Main_App(void *pvParameters)
{   
    (void)(pvParameters);
    
    while (1)
    {   
        
        switch (state)
        {
        case MAIN_APP_STATUS_STANDBY:
            break;
        
        case MAIN_APP_STATUS_START:
            if (initialization())
                state = MAIN_APP_STATUS_HOMING;
            else
                state = MAIN_APP_STATUS_STANDBY;
            break;

        case MAIN_APP_STATUS_HOMING:     
            if (homing())
                state = MAIN_APP_STATUS_RUNNING;
            else
                state = MAIN_APP_STATUS_STANDBY;
            break;
        
        case MAIN_APP_STATUS_RUNNING:
            running();
            state = MAIN_APP_STATUS_STANDBY;
            break;

        default:
            break;
        }


        vTaskDelay(100);
    }
}


static void running(void)
{   
    printf("Start\n");
    PGV_EnableLogging();
    KincoCANOpen_EnableLogging();
    KincoCANOpen_EnableOperation();

    KincoCANOpen_SetMode(Kinco_OperationMode_ProfileVelocity);
    KincoCANOpen_SetTargetSpeed(speed);

    vTaskDelay(pdMS_TO_TICKS(time));

    PGV_DisableLogging();
    KincoCANOpen_DisableLogging();

    printf("Stop\n");

    KincoCANOpen_SetTargetSpeed(0);
    vTaskDelay(10000);
    KincoCANOpen_DisableOperation();
}


static bool initialization(void)
{
    if(!PGV_Init())
    {
        printf("ERROR:\tCONNOT CONNECT TO PGV\n");
        return false;
    }


    if(!KincoCANOpen_Init())
    {
        printf("ERROR:\tCONNOT CONNECT TO KINCO\n");
        return false;
    }

    return true;
}


static bool homing(void)
{
    KincoCANOpen_EnableOperation();

    if (!PGV_GetVisibility() || (PGV_GetTagID()!=id))
    {
        KincoCANOpen_SetMode(Kinco_OperationMode_ProfileVelocity);
        KincoCANOpen_SetTargetSpeed(1);
    }

    while(!PGV_GetVisibility() || (!(PGV_GetTagID() == id)))
    {
        vTaskDelay(10);
    }

    KincoCANOpen_SetTargetSpeed(0);
    vTaskDelay(5000);

    KincoCANOpen_SetMode(Kinco_OperationMode_ProfilePosition);
    KincoCANOpen_SetTargetPosition(base_angle - PGV_GetAngle());
    KincoCANOpen_StartPositioning(true, false);

    while(kinco.status_word.bits.target_reached)
    {
        vTaskDelay(100);
    }


    while(!kinco.status_word.bits.target_reached)
    {
        vTaskDelay(100);
    }
    
    vTaskDelay(3000);
    KincoCANOpen_DisableOperation();
    vTaskDelay(10000);
    printf("Motor offset: %d\tPGV offset: %d\n\r", (int)(kinco.position * 1000), (int)(PGV_GetAngle() * 1000));

    return true;
}


static void Test_CLIHandler(int argc, char **argv)
{
    (void)(argc);

    if (strcmp(argv[1], "time") == 0)
    {
        time = atoi(argv[2]);
    }

    else if (strcmp(argv[1], "id") == 0)
    {
        id = atoi(argv[2]);
    }

    else if (strcmp(argv[1], "speed") == 0)
    {
        speed = atof(argv[2]);
    }

    else if (strcmp(argv[1], "start") == 0)
    {
        state = 1;
    }
    
    else if (strcmp(argv[1], "stop") == 0)
    {
        state = 0;
    }
    
    else if (strcmp(argv[1], "angle") == 0)
    {
        base_angle = atof(argv[2]);
    }
}
