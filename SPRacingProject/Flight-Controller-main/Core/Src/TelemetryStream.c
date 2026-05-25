/* 
 * File:   UartProtocol.h
 * Author: Jose Machon
 *
 * Created on April 1, 2026, 9:24 AM
 */
 
#include "main.h"
#include "TelemetryStream.h"


environmental_packet_t environmetal_telemetry_packet = {0};
positional_packet_t positional_telemetry_packet = {0};
power_packet_t power_telemetry_packet = {0};

void TelemetryStream_SendEnvironmental(environmental_packet_t *Data)
{
    environmetal_telemetry_packet.timestamp_ms = HAL_GetTick();
    
    // printf("TEMP: %ld, DEPTH:  %ld, PRESSURE_PA: %ld, TIME: %ld\r\n",
    //      Data -> temp,
    //      Data -> depth,
    //      Data -> pressure_Pa,  
    //      Data -> timestamp_ms);

    Protocol_SendPacket(sizeof(environmental_packet_t), ID_ENVIRONMENTAL_TELEMETRY,  &environmetal_telemetry_packet);

}

void TelemetryStream_SendOrientation(positional_packet_t *Data){

    positional_telemetry_packet.timestamp_ms = HAL_GetTick();
    // printf("YAW: %d, PITCH:  %d, ROLL: %d, TIME: %ld\r\n",
    //      Data -> yaw,
    //      Data -> pitch,
    //      Data -> roll,  
    //      Data -> timestamp_ms);

    Protocol_SendPacket(sizeof(positional_packet_t), ID_POSITIONAL_TELEMETRY, &positional_telemetry_packet);
}

void TelemetryStream_SendPowerStatus(power_packet_t *Data){
    
    power_telemetry_packet.timestamp_ms = HAL_GetTick();
    // printf("VOLTAGE: %d, CURRENT:  %d, POWER: %d, TIME: %ld\r\n",
    //      Data -> voltage_mv,
    //      Data -> current_ma,
    //      Data -> power_mw,  
    //      Data -> timestamp_ms);

    Protocol_SendPacket(sizeof(power_packet_t), ID_POWER_STATUS_TELEMETRY, &power_telemetry_packet);
}