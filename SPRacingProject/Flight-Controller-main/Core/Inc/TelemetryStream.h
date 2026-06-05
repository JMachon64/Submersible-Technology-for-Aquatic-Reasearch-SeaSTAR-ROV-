/* 
 * File:   TelemetryStream.h
 * Author: Jose Machon
 *
 * Created on April 1, 2026, 9:24 AM
 */

#ifndef TELEMETRYSTREAM_H
#define TELEMETRYSTREAM_H

#include <stdint.h>
#include <stdio.h>
#include "MS5837PressureSensor.h"
#include "UartProtocol.h"
#include "PacketIDs.h"
#include "stm32f3xx_hal.h"
#include "stm32f3xx_hal_tim.h"
#include "Uart.h"

/* 
 * Structs storing relevant data to send via UART packets.
 */

typedef struct{

    int32_t temp;
    int32_t depth;
    int32_t pressure_Pa;
    // int32_t pressure_Bar;
    uint32_t timestamp_ms;

}environmental_packet_t;

typedef struct{

    int16_t yaw;
    int16_t pitch;
    int16_t roll;
    uint32_t timestamp_ms;

}positional_packet_t;

/* 
 * Sampling flags linked to hardware timers.
 */

extern volatile uint32_t packetsendcounter;
extern volatile uint8_t sample_flag;

extern volatile uint8_t sensorcounter;
extern volatile uint8_t sensorsampleflag;

extern volatile uint8_t environmentalcounter;
extern volatile uint8_t environmentalsampleflag;

extern volatile uint8_t orientationcounter;
extern volatile uint8_t orientationsampleflag;

extern volatile uint8_t powercounter;
extern volatile uint8_t powersampleflag;

extern environmental_packet_t environmetal_telemetry_packet;
extern positional_packet_t positional_telemetry_packet;


/* 
 * 
 * 
 * 
 */

void TelemetryStream_SendEnvironmental(environmental_packet_t *Data);

void TelemetryStream_SendOrientation(positional_packet_t *Data);

#endif