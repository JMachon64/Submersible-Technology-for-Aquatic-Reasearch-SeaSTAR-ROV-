
#include "main.h"
#include "TelemetryStream.h"
#include "UartProtocol.h"
#include "PacketIDs.h"
#include "stm32f3xx_hal.h"
#include "stm32f3xx_hal_tim.h"
#include <stdio.h>
#include "Uart.h"

environmental_packet_t environmetal_telemetry_packet = {0};
positional_packet_t positional_telemetry_packet = {0};

void TelemetryStream_SendEnvironmental(environmental_packet_t *Data)
{
    Data -> timestamp_ms = HAL_GetTick();

    printf("TEMP: %ld, DEPTH:  %ld, PRESSURE_PA: %ld, TIME: %ld\r\n",
         Data -> temp,
         Data -> depth,
         Data -> pressure_Pa,  
         Data -> timestamp_ms);

    Protocol_QueuePacket(sizeof(environmental_packet_t), ID_ENVIRONMENTAL_TELEMETRY,  &environmetal_telemetry_packet);

}



void TelemetryStream_SendOrientation(positional_packet_t *Data){

    Data -> timestamp_ms = HAL_GetTick();

   // Protocol_SendPacket(sizeof(positional_packet_t), ID_POSITIONAL_TELEMETRY, &positional_telemetry_packet);
    printf("\n\n");

}