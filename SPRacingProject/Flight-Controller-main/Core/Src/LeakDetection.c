/* 
 * File:   LeakDetection.c
 * Author: Jose Machon
 *
 * Created on April 1, 2026, 9:24 AM
 *
 * The purpose of this code is to detect leaks inside the integrated 
 * enclosure by having a ISR tied to a leak pin. That triggers will affect 
 * FSM.
 *
 */

#include "LeakDetection.h"
#include "UartProtocol.h"
#include <stdbool.h>

#define LEAK_SENSOR_PORT GPIOB
#define LEAK_SENSOR_PIN  GPIO_PIN_5


volatile uint8_t leakInterruptFlag = 0;
volatile uint8_t leakLatched = false; //if leak detected, do not spam the UART.


/* 
 * This functions responds to the interupt and updates the FSM
 * as well as sends a packet via UART to the PI if the leak pin
 * goes off.
 */

void LeakSensor_Read(void)
{
    if (leakInterruptFlag)
        {
            leakInterruptFlag = false;

            if (!leakLatched) 
            {
                leakLatched = true; // prevent further spam.
                thrusters_disabled = true; // disable thrusters.
            }

            //send the leak event here
            int32_t timestamp_ms = HAL_GetTick();
            Protocol_SendPacket(4, ID_LEAK_DETECTED, &timestamp_ms);

            FSM_PostEvent(FSM_EVENT_LEAK_DETECTED); //update the STM32 FSM
        }
}


/* 
 * ISR function that responds to the leak pin triggering.
 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == LEAK_SENSOR_PIN)
    {
        leakInterruptFlag = 0;
    }
}