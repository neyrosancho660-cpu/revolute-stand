#ifndef __KINCO_H__
#define __KINCO_H__

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "cli.h"
#include "bxCAN.h"
#include "canopen.h"


typedef enum
{
    CONTROL_WORD                = 0x6040,
    STATUS_WORD                 = 0x6041,
    MODES_OF_OPERATION          = 0x6060,
    MODES_OF_OPERATION_DISPLAY  = 0x6061,
    PROFILE_VELOCITY            = 0x6081,
    PROFILE_ACCELERATION        = 0x6083,      
    PROFILE_DECELERATION        = 0x6084,
    QUICK_STOP_DECELERATION     = 0x6085,
    QUICK_STOP_OPTION_CODE      = 0x605A,
    TARGET_POSITION             = 0x607A,
    TARGET_VELOCITY             = 0x60FF,
    MOTION_PROFILE_TYPE         = 0x6086,  
    POSITION_ACTUAL_VALUE       = 0x6063,
    VELOCITY_ACTUAL_VALUE       = 0x606C,
    TARGET_POSITION_OPTION      = 0x60B9,
    TARGET_VELOCITY_OPTION      = 0x60BA,
    TARGET_TORQUE_OPTION        = 0x60BB

} Kinco_SDOIndex_t;


typedef union
{
    uint16_t byte;
    struct
    {
        uint16_t ready_on               : 1;    // Bit 0
        uint16_t switch_on              : 1;    // Bit 1
        uint16_t operation_enabled      : 1;    // Bit 2
        uint16_t fault                  : 1;    // Bit 3
        uint16_t voltage_enabled        : 1;    // Bit 4
        uint16_t quick_stop             : 1;    // Bit 5
        uint16_t switch_disabled        : 1;    // Bit 6
        uint16_t warning                : 1;    // Bit 7
        uint16_t manufacturer_specific  : 1;    // Bit 8
        uint16_t remote                 : 1;    // Bit 9
        uint16_t target_reached         : 1;    // Bit 10
        uint16_t internal_limit         : 1;    // Bit 11
        uint16_t set_point_ack          : 1;    // Bit 12
        uint16_t following_error        : 1;    // Bit 13
        uint16_t communication_found    : 1;    // Bit 14
        uint16_t reference_found        : 1;    // Bit 15
    } bits;

} Kinco_StatusWord_t;


typedef union
{
    uint16_t byte;
    struct
    {
        uint16_t switch_on_command          : 1;
        uint16_t enable_voltage             : 1;
        uint16_t quick_stop_command         : 1;
        uint16_t enable_operation           : 1;
        uint16_t operation_mode_specific_1  : 3;
        uint16_t fault_reset                : 1;
        uint16_t halt                       : 1;
        uint16_t operation_mode_specific_2  : 1;
        uint16_t reserved                   : 1;
        uint16_t manufacture_specific       : 1;
    } bits;

} Kinco_ControlWord_t;


typedef enum
{
    Kinco_OperationMode_ProfilePosition = 1,
    Kinco_OperationMode_ProfileVelocity = 3,
    Kinco_OperationMode_Homing          = 6,

} Kinco_OperationMode_t;    


typedef enum
{
    SHUTDOWN                        = 0x0006,               
    SWITCH_ON                       = 0x0007,              
    DISABLE_VOLTAGE                 = 0x0000,
    QUICK_STOP                      = 0x000B,
    DISABLE_OPERATION               = 0x0007,    
    ENABLE_OPERATION                = 0x000F,     
    FAULT_RESET                     = 0x0080,
    OPERATION_ENABLED               = 0x001F,
    ENABLE_CHANGE_SET_IMMEDIATELY   = 0x1000,
    ENABLE_ABS_POSITION             = 0x002F,
    START_ABS_POSITION              = 0x003F,
    ENABLE_REL_POSITION             = 0x004F,                                       
    START_REL_POSITION              = 0x005F
    
} Kinco_Command_t;


typedef struct
{  
    bool is_connected;

    CANOpen_Node_t node;
    
    Kinco_StatusWord_t status_word;
    Kinco_ControlWord_t control_word;
    
    float speed;
    float position;

    bool logging_enabled;

} KincoInstance_t;


extern cli_command_t motor_cmd;


bool KincoCANOpen_Init(void);

bool KincoCANOpen_Shutdown(void);
bool KincoCANOpen_SwitchOn(void);
bool KincoCANOpen_EnableOperation(void);
bool KincoCANOpen_DisableOperation(void);

bool KincoCANOpen_SetMode(Kinco_OperationMode_t mode);
bool KincoCANOpen_SetTargetSpeed(float speed);
bool KincoCANOpen_SetTargetPosition(float position);
bool KincoCANOpen_SetAcceleration(float acceleration);
bool KincoCANOpen_SetDeceleration(float deceleration);
bool KincoCANOpen_SetProfileVelocity(float speed);
bool KincoCANOpen_StartPositioning(bool immediately,bool absolute);


void KincoCANOpen_EnableLogging(void);
void KincoCANOpen_DisableLogging(void);

extern KincoInstance_t kinco;

#endif