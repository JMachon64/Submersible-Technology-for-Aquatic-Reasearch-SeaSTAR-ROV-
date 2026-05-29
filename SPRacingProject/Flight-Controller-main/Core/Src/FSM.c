/* 
 * File:   UartProtocol.h
 * Author: Jose Machon
 *
 * Created on April 1, 2026, 9:24 AM
 */

#include "PacketIDs.h"
#include "main.h"
#include <stdbool.h>
#include<stdio.h>
#include "FSM.h"
#include "UartProtocol.h"
#include "ControlLoop.h"
#include "pwm.h"
#include "stm32f3xx_hal.h"
#include "MS5837PressureSensor.h"
#include "TSYS01TemperatureSensor.h"

static SeaSTAR_FSM_t starfystate = BOOT_MODE;

static volatile SeaSTAR_FSM_Event_t pending_event = FSM_EVENT_NONE;

volatile uint8_t propulsion_initialized = false;
volatile uint8_t pi_healthy = true;

extern TSY01_TemperatureSensor_t temp_sensor;
extern MS5837_PressureSensor_t pressure_sensor;

void FSM_PostEvent(SeaSTAR_FSM_Event_t event)
{
    pending_event = event;
}

static SeaSTAR_FSM_t return_state = MISSION_MODE_IDLE;

uint8_t SeaSTAR_FSM(void)
{
    SeaSTAR_FSM_Event_t event = pending_event;
    pending_event = FSM_EVENT_NONE;


    switch (starfystate)
    {
        case BOOT_MODE:

            if (event == FSM_EVENT_BOOT_DONE)
            {

                starfystate = MISSION_MODE_IDLE;

                printf("Transition: [BOOT] -> [IDLE]\r\n");

                thrusters_disabled = true;

                break;
            }

            else if (event == FSM_EVENT_LEAK_DETECTED)
            {
                starfystate = FAILURE_MODE;
                printf("Transition: [FAILURE] -> [FAILURE]: LEAK DETECTED\r\n");
                break;
            }
            break;

        case MISSION_MODE_IDLE:

            Control_Update_Command(0, 0, 0, 0, 0);

            if (event == FSM_EVENT_START_MISSION)
            {
                thrusters_disabled = false;
                starfystate = MISSION_MODE_ACTIVE;
                printf("Transition: [IDLE] -> [ACTIVE]\r\n");
                break;
            }
            if (event == FSM_EVENT_COLLECT_WATER_SAMPLE)
            {
                return_state = MISSION_MODE_IDLE;
                Control_Update_Command(0, 0, 0, 0, 0);
                starfystate = COLLECT_WATER_SAMPLE_MODE;

                break;
            }
            else if (event == FSM_EVENT_LEAK_DETECTED)
            {
                starfystate = FAILURE_MODE;
                printf("Transition: [IDLE] -> [FAILURE]: LEAK DETECTED\r\n");
                break;
            }
            else if (event == FSM_EVENT_PI_FAILURE)
            {
                pi_healthy = false;
                heartbeat_enabled = false;
                starfystate = FAILURE_MODE;
                printf("Transition: [IDLE] -> [FAILURE]: PI FAILURE\r\n");
                break;
            }
            else if (event == FSM_EVENT_COMMUNICATION_CONNECTION_FAILURE)
            {
                comm_failure = true;
                starfystate = FAILURE_MODE;
                printf("Transition: [IDLE] -> [FAILURE]: COMMUNICATION\r\n");
                break;
            }
            break;

        case MISSION_MODE_ACTIVE:

            if (event == FSM_EVENT_END_MISSION)
            {
                starfystate = MISSION_MODE_IDLE;
                printf("Transition: [ACTIVE] -> [IDLE]\r\n");
                break;
            }
            if (event == FSM_EVENT_COLLECT_WATER_SAMPLE)
            {
                thrusters_disabled = true;
                return_state = MISSION_MODE_ACTIVE;

                starfystate = COLLECT_WATER_SAMPLE_MODE;

                break;
            }
            else if (event == FSM_EVENT_LEAK_DETECTED)
            {
                starfystate = FAILURE_MODE;
                printf("Transition: [ACTIVE] -> [FAILURE]\r\n");
                break;
            }
            else if (event == FSM_EVENT_PI_FAILURE)
            {
                pi_healthy = false;
                heartbeat_enabled = false;
                starfystate = FAILURE_MODE;
                printf("Transition: [ACTIVE] -> [FAILURE]\r\n");
                break;
            }
            else if (event == FSM_EVENT_COMMUNICATION_CONNECTION_FAILURE)
            {
                comm_failure = 1;
                starfystate = FAILURE_MODE;
                printf("Transition: [ACTIVE] -> [FAILURE]\r\n");
                break;
            }
            break;

        case FAILURE_MODE:
        {

            Control_Update_Command(0, 0, 0, 0, 0);
             //SHUTOFF ALL THRUSTERS IN THE EVENT OF AN EMERGENCY

            if (comm_failure == 0 && leakLatched == 0 && pi_healthy == 1)
            {
                heartbeat_enabled = true;

                starfystate = MISSION_MODE_IDLE;
                printf("Transition: [FAILURE] -> [IDLE]: RECONNECTION\r\n");
                break;
            }

            break;
        }

        case COLLECT_WATER_SAMPLE_MODE:
        {

            printf("[COLLECTING WATER SAMPLE]\r\n");

            thrusters_disabled = true;

            Control_Update_Command(0, 0, 0, 0, 0);

            HAL_Delay(2000);
            PWM_SampleClosedPosition();
            HAL_Delay(700);
            PWM_SampleOpenPosition();
            HAL_Delay(700);
            PWM_SampleClosedPosition();

            // sID_WATER_SAMPLE_COMPLETE SEND TELEMETRY COLLECTED AT THAT SAMPLE

            environmetal_telemetry_packet.depth       = (int32_t)(pressure_sensor.Depth_m * 1000.0f);
            environmetal_telemetry_packet.pressure_Pa = (int32_t)(pressure_sensor.Pressure_Pa) + pressure_offset;
            environmetal_telemetry_packet.temp        = (int32_t)(temp_sensor.Temperature_C * 1000.00f);
            environmetal_telemetry_packet.timestamp_ms = HAL_GetTick();
                
            Protocol_SendPacket(sizeof(environmental_packet_t), ID_WATER_SAMPLE_COMPLETE,  &environmetal_telemetry_packet);
            
            if(return_state == MISSION_MODE_IDLE){
                thrusters_disabled = true;
            }
            else{
                thrusters_disabled = false;
            }

            starfystate = return_state;
            break;

        }

        default:
            starfystate = BOOT_MODE;
            break;
    }

    return 1;
}