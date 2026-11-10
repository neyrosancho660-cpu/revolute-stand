#include "log.h"
#include "timer.h"
#include "kinco.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SEGGER_RTT.h"

KincoInstance_t kinco = {.node.id = 1};


static void TPDO1_cllbck(bxCAN_Message_t *msg, void *context);
static void TPDO2_cllbck(bxCAN_Message_t *msg, void *context);
static void TPDO3_cllbck(bxCAN_Message_t *msg, void *context);
static void TPDO4_cllbck(bxCAN_Message_t *msg, void *context);

static int Kinco_ConvertSpeed_RPM2DEC(float speed);
static float Kinco_ConvertSpeed_DEC2RPM(int speed);
static int Kinco_ConvertPosition_RPM2DEC(float position);
static float Kinco_ConvertPosition_DEC2RPM(int position);
static int Kinco_ConvertAcceleration_RPS2DEC(float acceleration);
static void KincoCANOpen_CLIHandler(int argc, char **argv);


cli_command_t motor_cmd = {
    .name = "kinco",
    .help = "help",
    .handler = KincoCANOpen_CLIHandler
};


bool KincoCANOpen_Init(void)
{   
    if (kinco.is_connected)
        return true;

    kinco.node.context = &kinco;

    kinco.node.callbacks[0] = TPDO1_cllbck;
    kinco.node.callbacks[1] = TPDO2_cllbck;
    kinco.node.callbacks[2] = TPDO3_cllbck;
    kinco.node.callbacks[3] = TPDO4_cllbck;

    if (!CANOpen_InitNode(&kinco.node))
        return false;


    CANOpen_SendNMT(&kinco.node, NMT_Operational);

    kinco.is_connected = true;

    return true;
}

// --- Control Functions --- //
bool KincoCANOpen_Shutdown(void)
{
    return CANOpen_SDO_Download(&kinco.node, CONTROL_WORD, 0x00, SHUTDOWN, 2, 10);
}

bool KincoCANOpen_SwitchOn(void)
{
    return CANOpen_SDO_Download(&kinco.node, CONTROL_WORD, 0x00, SWITCH_ON, 2, 10);
}

bool KincoCANOpen_EnableOperation(void)
{
    return CANOpen_SDO_Download(&kinco.node, CONTROL_WORD, 0x00, ENABLE_OPERATION, 2, 10);
}

bool KincoCANOpen_DisableOperation(void)
{
    return CANOpen_SDO_Download(&kinco.node, CONTROL_WORD, 0x00, DISABLE_OPERATION, 2, 10);
}

bool KincoCANOpen_SetMode(Kinco_OperationMode_t mode)
{
    return CANOpen_SDO_Download(&kinco.node, MODES_OF_OPERATION, 0x00, mode, 1, 10);
}

bool KincoCANOpen_SetTargetSpeed(float speed)
{
    int speed_dec = Kinco_ConvertSpeed_RPM2DEC(speed);
    return CANOpen_SDO_Download(&kinco.node, TARGET_VELOCITY, 0x00, (uint32_t)speed_dec, 4, 10);
}

bool KincoCANOpen_SetTargetPosition(float position)
{
    int position_dec = Kinco_ConvertPosition_RPM2DEC(position);
    return CANOpen_SDO_Download(&kinco.node, TARGET_POSITION, 0x00, (uint32_t)position_dec, 4, 10);
}

bool KincoCANOpen_SetAcceleration(float acceleration)
{
    int acceleration_dec = Kinco_ConvertAcceleration_RPS2DEC(acceleration);
    return CANOpen_SDO_Download(&kinco.node, PROFILE_ACCELERATION, 0x00, (uint32_t)acceleration_dec, 4, 10);
}

bool KincoCANOpen_SetDeceleration(float deceleration)
{
    int deceleration_dec = Kinco_ConvertAcceleration_RPS2DEC(deceleration);
    return CANOpen_SDO_Download(&kinco.node, PROFILE_DECELERATION, 0x00, (uint32_t)deceleration_dec, 4, 10);
}

bool KincoCANOpen_SetProfileVelocity(float speed)
{
    int speed_dec = Kinco_ConvertSpeed_RPM2DEC(speed);
    return CANOpen_SDO_Download(&kinco.node, PROFILE_VELOCITY, 0x00, (uint32_t)speed_dec, 4, 10);
}

bool KincoCANOpen_StartPositioning(bool immediately,bool absolute)
{
    uint16_t command = absolute ? START_ABS_POSITION : START_REL_POSITION;
    if (immediately)
        command |= ENABLE_CHANGE_SET_IMMEDIATELY;

    return CANOpen_SDO_Download(&kinco.node, CONTROL_WORD, 0x00, command, 2, 10);
}

void KincoCANOpen_EnableLogging(void)
{
    kinco.logging_enabled = true;
}

void KincoCANOpen_DisableLogging(void)
{
    kinco.logging_enabled = false;
}


// --- CLI Handler --- //
static void KincoCANOpen_SetHandler(int argc, char **argv)
{
    if (argc < 2) 
    {
        return;
    }

    if (strcmp(argv[2], "id") == 0)
    {
        kinco.node.id = atoi(argv[3]);
    }

    else if (strcmp(argv[2], "profile_speed") == 0)
    {
        int mode = atoi(argv[3]);
        KincoCANOpen_SetProfileVelocity(mode);
    }

    else if (strcmp(argv[2], "acceleration") == 0)
    {
        float acceleration = atof(argv[3]);
        KincoCANOpen_SetAcceleration(acceleration);
    }

    else if (strcmp(argv[2], "deceleration") == 0)
    {
        float deceleration = atof(argv[3]);
        KincoCANOpen_SetDeceleration(deceleration);
    } 
}


static void KincoCANOpen_CLIHandler(int argc, char **argv) 
{   

    if (argc < 2) 
    {
        return;
    }
    
    if (strcmp(argv[1], "connect") == 0)
    {
       KincoCANOpen_Init();
    }

    else if (strcmp(argv[1], "set") == 0)
    {
        KincoCANOpen_SetHandler(argc, argv);
    }

    else if (strcmp(argv[1], "enable_operation") == 0)
    {
        KincoCANOpen_EnableOperation();
    }

    else if (strcmp(argv[1], "disable_operation") == 0)
    {
        KincoCANOpen_DisableOperation();
    }

    else if (strcmp(argv[1], "spin") == 0)
    {
        float speed = atof(argv[2]);
        KincoCANOpen_SetMode(Kinco_OperationMode_ProfileVelocity);
        KincoCANOpen_SetTargetSpeed(speed);
    }

    else if (strcmp(argv[1], "go") == 0)
    {
        float position = atof(argv[2]);
        KincoCANOpen_SetMode(Kinco_OperationMode_ProfilePosition);
        KincoCANOpen_SetTargetPosition(position);
        KincoCANOpen_StartPositioning(true, false);
    }

    else if (strcmp(argv[1], "goto") == 0)
    {
        float position = atof(argv[2]);
        KincoCANOpen_SetMode(Kinco_OperationMode_ProfilePosition);
        KincoCANOpen_SetTargetPosition(position);
        KincoCANOpen_StartPositioning(true, true);
    }

    else if (strcmp(argv[1], "enable_logging") == 0)
    {
        KincoCANOpen_EnableLogging();
    }

    else if (strcmp(argv[1], "disable_logging") == 0)
    {
        KincoCANOpen_DisableLogging();
    }

    else 
    {
        printf("Unknown Command.\n");
    }
}


// --- Callbacks --- //
static void TPDO1_cllbck(bxCAN_Message_t *msg, void *context)
{
    (void)context;

    int speed_raw = (msg->data.byte[7] << 24) | (msg->data.byte[6] << 16) | (msg->data.byte[5] << 8) | msg->data.byte[4];
    int position_raw = (msg->data.byte[3] << 24) | (msg->data.byte[2] << 16) | (msg->data.byte[1] << 8) | msg->data.byte[0];

    kinco.speed = Kinco_ConvertSpeed_DEC2RPM(speed_raw);
    kinco.position = Kinco_ConvertPosition_DEC2RPM(position_raw);


    if (kinco.logging_enabled)
        log_write("Time: %lu | Speed: %d RPM | Position: %d\n", msg->timestamp, (int)(kinco.speed * 1000), (int)(kinco.position * 1000));
}

static void TPDO2_cllbck(bxCAN_Message_t *msg, void *context)
{
    (void)context;

    kinco.control_word.byte = (msg->data.byte[1] << 8) | msg->data.byte[0];
    kinco.status_word.byte = (msg->data.byte[3] << 8) | msg->data.byte[2];

    // if (kinco.logging_enabled)
    //     printf("Time: %8lu | Control Word: 0x%04X | Status Word: 0x%04X | Speed: %8d RPM | Position: %8d\n", msg->timestamp, kinco.control_word.byte, kinco.status_word.byte,(int)(kinco.speed * 1000), (int)(kinco.position * 1000));
}

static void TPDO3_cllbck(bxCAN_Message_t *msg, void *context)
{
    (void)msg;
    (void)context;
}

static void TPDO4_cllbck(bxCAN_Message_t *msg, void *context)
{
    (void)msg;
    (void)context;
}


// --- Conversion Functions --- //
static float Kinco_ConvertSpeed_DEC2RPM(int speed)
{
    return (float)speed * 1875 / (512 * 10000);
}

static float Kinco_ConvertPosition_DEC2RPM(int position)
{
    return (float)position * 9 / 250;
}

static int Kinco_ConvertSpeed_RPM2DEC(float speed)
{
    return (int)speed * 512 * 10000 / 1875;
}

static int Kinco_ConvertPosition_RPM2DEC(float position)
{
    return (int)position * 250 / 9;
}

static int Kinco_ConvertAcceleration_RPS2DEC(float acceleration)
{
    return (int)(acceleration * 65536 * 10000 / 4000000);
}