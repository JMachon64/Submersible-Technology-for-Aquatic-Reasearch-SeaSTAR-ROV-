/* 
 * File:   TelemetryStream.c
 * Author: Jose Machon
 *
 * Created on April 4, 2026, 9:06 AM
 */
 
#include "main.h"
#include "TelemetryStream.h"


/* 
 * Declaration of packet structs. One for environmental telemetry data being sent from the TSYS01 
 * as well as the MS5837 pressure sensor. Another for the positional euler angles.
 */
 
environmental_packet_t environmetal_telemetry_packet = {0};
positional_packet_t    positional_telemetry_packet   = {0};


/* 
 * Two helper functions for each type of telemetry being sent. Timestamps and 
 * sends the packet through UART. 
 */

void TelemetryStream_SendEnvironmental(environmental_packet_t *Data)
{
    environmetal_telemetry_packet.timestamp_ms = HAL_GetTick(); // TIMESTAMP

    //Packet 
    Protocol_SendPacket(sizeof(environmental_packet_t), 
    ID_ENVIRONMENTAL_TELEMETRY,
    &environmetal_telemetry_packet);

}

void TelemetryStream_SendOrientation(positional_packet_t *Data){

    positional_telemetry_packet.timestamp_ms = HAL_GetTick(); // TIMESTAMP

    //Packet 
    Protocol_SendPacket(sizeof(positional_packet_t), 
    ID_POSITIONAL_TELEMETRY, 
    &positional_telemetry_packet);
}

