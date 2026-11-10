#include "log.h"
#include "stdio.h"
#include "rs485.h"
#include "SEGGER_RTT.h"
#include "irayple_rs485.h"


const uint8_t IraypleRS485_RequestData[2] = {0xC8, 0x37};


bool IraypleRS485_Init(IraypleRS485_Instance_t* instance)
{
    IraypleRS485_Response_t response;
    if (RS485_Transmit(IraypleRS485_RequestData, IRAYPLE_RS485_REQUEST_SIZE, 10))
        return false;

    if (RS485_Receive(response.data, IRAYPLE_RS485_RESPONSE_SIZE, 10))
        return false;

    if (!IraypleRS485_parseData(instance, &response))
        return false;

    return true;

}


static uint8_t IraypleRS485_CalculateCRC(uint8_t* data, uint16_t size)
{
    uint8_t crc = data[0];

    for (uint16_t i = 1; i < size; i++)
    {
        crc ^= data[i];
    }

    return crc;
}


static void IraypleRS485_parseVisibility(IraypleRS485_Instance_t* instance, uint8_t* visibility_raw)
{
    instance->visibility = (bool)(*visibility_raw & 0x40);
}


static void IraypleRS485_parseX(IraypleRS485_Instance_t* instance, uint8_t* x_raw)
{
    int x = (x_raw[0] & 0x7) * 0x80 * 0x4000 + x_raw[1] * 0x4000 + x_raw[2] * 0x80 + x_raw[3];

    if ((x >> 23) > 0)
        x += 0xFF000000;
    
    instance->x = (float)(x) / 10;
}


static void IraypleRS485_parseY(IraypleRS485_Instance_t* instance, uint8_t* y_raw)
{
    int16_t y = y_raw[0] * 0x80 + y_raw[1];

    if ((y >> 13) > 0)
        y += 0xC000;
    
    instance->y = (float)(y) / 10;
}


static void IraypleRS485_parseAngle(IraypleRS485_Instance_t* instance, uint8_t* angle_raw)
{
    instance->angle = (float)(angle_raw[0] * 0x80 + angle_raw[1]) / 10;
}


static void IraypleRS485_parseTagID(IraypleRS485_Instance_t* instance, uint8_t* tag_raw)
{
    instance->id = tag_raw[0] * 0x4000 * 0x4000 + tag_raw[1] * 0x80 * 0x4000 + tag_raw[2] * 0x4000 + tag_raw[3] * 0x80 + tag_raw[4];
}


bool IraypleRS485_parseData(IraypleRS485_Instance_t* instance, IraypleRS485_Response_t* raw)
{
    uint8_t crc;

    crc = IraypleRS485_CalculateCRC(raw->data, 20);
    if (crc != raw->data[20])
        return false;

    IraypleRS485_parseVisibility(instance, raw->data + 1);
    IraypleRS485_parseX(instance, raw->data + 2);
    IraypleRS485_parseY(instance, raw->data + 6);
    IraypleRS485_parseAngle(instance, raw->data + 10);
    IraypleRS485_parseTagID(instance, raw->data + 13);
    
    if (instance->logging_enabled)
    {   
        log_write("Time: %lu | ID: %lu | Angle: %d\n", raw->timestamp, instance->id, (int)(instance->angle * 1000));
    }

    return true;
}