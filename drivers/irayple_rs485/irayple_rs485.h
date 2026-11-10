#ifndef __IRAYPLE_RS485__
#define __IRAYPLE_RS485__

#include <stdint.h>
#include <stdbool.h>

#define IRAYPLE_RS485_RESPONSE_SIZE   0x15
#define IRAYPLE_RS485_REQUEST_SIZE    0x2


typedef struct
{

    uint32_t timestamp;

    uint8_t idx;
    uint8_t data[IRAYPLE_RS485_RESPONSE_SIZE];

} IraypleRS485_Response_t;


typedef struct
{
    
    bool visibility;

    uint32_t id;

    float y;
    float x;
    float angle;

    bool logging_enabled;
    
} IraypleRS485_Instance_t;

extern const uint8_t IraypleRS485_RequestData[2];

bool IraypleRS485_Init(IraypleRS485_Instance_t* instance);
bool IraypleRS485_parseData(IraypleRS485_Instance_t* instance, IraypleRS485_Response_t* raw);


#endif