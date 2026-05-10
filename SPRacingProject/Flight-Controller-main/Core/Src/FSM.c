#include "PacketIDs.h"
#include "main.h"
#include<stdio.h>
#include "FSM.h"
#include "UartProtocol.h"
#include "ControlLoop.h"
#include "pwm.h"
#include "stm32f3xx_hal.h"

static SeaSTAR_FSM_t starfystate = MISSION_MODE_ACTIVE;

static volatile SeaSTAR_FSM_Event_t pending_event = FSM_EVENT_NONE;

void FSM_PostEvent(SeaSTAR_FSM_Event_t event)
{
    pending_event = event;
}

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
                printf("Transition: BOOT -> IDLE\r\n");
            }
            break;

        case MISSION_MODE_ACTIVE:

            if (event == FSM_EVENT_END_MISSION)
            {
                starfystate = MISSION_MODE_IDLE;
                printf("Transition: [ACTIVE] -> [IDLE]\r\n");
            }
            if (event == FSM_EVENT_COLLECT_WATER_SAMPLE)
            {
                starfystate = COLLECT_WATER_SAMPLE_MODE;
                printf("Transition: [ACTIVE] -> [COLLECTINGWATERSAMPLE]\r\n");
            }
            else if (event == FSM_EVENT_LEAK_DETECTED)
            {
                starfystate = FAILURE_MODE;
                printf("Transition: [ACTIVE] -> [FAILURE]\r\n");
            }
            break;

        case FAILURE_MODE:
        {
            Control_Update_Command(0, 0, 0, 0, 0, 0);
             //SHUTOFF ALL THRUSTERS IN THE EVENT OF AN EMERGENCY


            // if (event == FSM_EVENT_COMMUNICATION_RECONNECTION_SUCCESS)
            // {
            //     printf("Transition: [FAILURE] -> [IDLE]\r\n");
            // }

            break;
        }

        case COLLECT_WATER_SAMPLE_MODE:
        {
            PWM_SampleClose();

            starfystate = MISSION_MODE_ACTIVE;

            break;

        }
        default:
            starfystate = BOOT_MODE;
            break;
    }

    return 1;
}