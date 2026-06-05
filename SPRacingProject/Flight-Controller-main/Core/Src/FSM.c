/* 
 * File:   FSM.c
 * Author: Jose Machon
 *
 * Created on April 20, 2026, 11:35 AM
 *
 * This is the Finite State Machine code for the SeaSTAR, it had 5 total states
 * and is deterministic. This state machine reacts to events both on the STM32 
 * and ones on the PI. It reacts to the PI through UART for sampling commands 
 * and PI failure events.
 *
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

static SeaSTAR_FSM_t starfystate = BOOT_MODE; //start out in BOOT mode.

static volatile SeaSTAR_FSM_Event_t pending_event = FSM_EVENT_NONE;

volatile uint8_t propulsion_initialized = false;
volatile uint8_t pi_healthy = true;

extern TSY01_TemperatureSensor_t temp_sensor;
extern MS5837_PressureSensor_t pressure_sensor;


/* 
 * Event checker that the state machine reacts to.
 */

void FSM_PostEvent(SeaSTAR_FSM_Event_t event)
{
    pending_event = event; //update event.
}

static SeaSTAR_FSM_t return_state = MISSION_MODE_IDLE; //default to IDLE if anything happens.


/* 
 * State machine, 5 states, deterministic behavior.
 */ 

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
                thrusters_disabled = true;

                break;
            }

            else if (event == FSM_EVENT_LEAK_DETECTED)
            {
                starfystate = FAILURE_MODE;
                break;
            }
            break;

        case MISSION_MODE_IDLE:

            Control_Update_Command(0, 0, 0, 0, 0);

            if (event == FSM_EVENT_START_MISSION)
            {
                thrusters_disabled = false;
                starfystate = MISSION_MODE_ACTIVE;
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
                break;
            }
            else if (event == FSM_EVENT_PI_FAILURE)
            {
                pi_healthy = false;
                heartbeat_enabled = false;
                starfystate = FAILURE_MODE;
                break;
            }
            else if (event == FSM_EVENT_COMMUNICATION_CONNECTION_FAILURE)
            {
                comm_failure = true;
                starfystate = FAILURE_MODE;
                break;
            }
            break;

        case MISSION_MODE_ACTIVE:

            if (event == FSM_EVENT_END_MISSION)
            {
                starfystate = MISSION_MODE_IDLE;
                break;
            }
            if (event == FSM_EVENT_COLLECT_WATER_SAMPLE)
            {
                thrusters_disabled = true;
                return_state = MISSION_MODE_ACTIVE; //store last known state to be able to return.
                starfystate = COLLECT_WATER_SAMPLE_MODE;

                break;
            }
            else if (event == FSM_EVENT_LEAK_DETECTED)
            {
                starfystate = FAILURE_MODE;
                break;
            }
            else if (event == FSM_EVENT_PI_FAILURE)
            {
                pi_healthy = false; 
                heartbeat_enabled = false;
                starfystate = FAILURE_MODE;
                break;
            }
            else if (event == FSM_EVENT_COMMUNICATION_CONNECTION_FAILURE)
            {
                comm_failure = 1;
                starfystate = FAILURE_MODE;
                break;
            }
            break;

        case FAILURE_MODE:
        {

            Control_Update_Command(0, 0, 0, 0, 0);

             //SHUTOFF ALL THRUSTERS IN THE EVENT OF AN EMERGENCY

            if (comm_failure == 0 && leakLatched == 0 && pi_healthy == 1) // Only exit if theres no leak and other criticals come back.
            {
                heartbeat_enabled = true; //start responding to pings again
                starfystate = MISSION_MODE_IDLE;
                break;
            }
            break;
        }

        case COLLECT_WATER_SAMPLE_MODE:
        {

            printf("[COLLECTING WATER SAMPLE]\r\n");

            thrusters_disabled = true;

            Control_Update_Command(0, 0, 0, 0, 0);

            // actuation sequence for the NISKIN bottle.
            HAL_Delay(2000);
            PWM_SampleClosedPosition(); // start 
            HAL_Delay(700);
            PWM_SampleOpenPosition(); //   release mechanism close bottle.
            HAL_Delay(700);
            PWM_SampleClosedPosition(); // reset latch position for re arming.

            // ID_WATER_SAMPLE_COMPLETE SEND TELEMETRY COLLECTED AT THAT SAMPLE

            environmetal_telemetry_packet.depth       = (int32_t)(pressure_sensor.Depth_m * 1000.0f);
            environmetal_telemetry_packet.pressure_Pa = (int32_t)(pressure_sensor.Pressure_Pa) + pressure_offset;
            environmetal_telemetry_packet.temp        = (int32_t)(temp_sensor.Temperature_C * 1000.00f);
            environmetal_telemetry_packet.timestamp_ms = HAL_GetTick();
                
            Protocol_SendPacket(sizeof(environmental_packet_t), ID_WATER_SAMPLE_COMPLETE,  &environmetal_telemetry_packet);
            
            //Depending on return state keep thrusters disable or re enable movement.

            if(return_state == MISSION_MODE_IDLE){ thrusters_disabled = true;}
            else{thrusters_disabled = false;}

            starfystate = return_state;

            break;

        }

        default:
            starfystate = BOOT_MODE;
            break;
    }

    return 1;
}