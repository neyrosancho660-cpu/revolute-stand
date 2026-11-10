#ifndef __PGV_H__
#define __PGV_H__

#include "cli.h"
#include "irayple_rs485.h"
#include "irayple_canopen.h"
#include "pepperl_fuchs_canopen.h"

typedef enum {

    PGV_TYPE_IRAYPLE_CANOPEN = 0,
    PGV_TYPE_PEPPERL_FUCHS_CANOPEN,
    PGV_TYPE_IRAYPLE_RS485
    
} PGV_Type_t;


typedef struct {

    PGV_Type_t type;

    bool is_connected;

    IraypleRS485_Instance_t irayple_rs485;
    IraypleCANOpen_Instance_t irayple_canopen;
    PepperlFuchsCANOpen_Instance_t pepperl_fuchs_canopen;
    
    uint32_t frequency;

} PGV_Instance_t;


extern cli_command_t pgv_cli;


bool PGV_Init(void);

float PGV_GetX(void);
float PGV_GetY(void);
float PGV_GetAngle(void);
uint32_t PGV_GetTagID(void);
bool PGV_GetVisibility(void);

void PGV_EnableLogging(void);
void PGV_DisableLogging(void);


#endif