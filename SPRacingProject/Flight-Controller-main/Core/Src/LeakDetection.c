/* 
 * File:   UartProtocol.h
 * Author: Jose Machon
 *
 * Created on April 1, 2026, 9:24 AM
 */

#include "LeakDetection.h"
#include "UartProtocol.h"

#define LEAK_SENSOR_PORT GPIOB
#define LEAK_SENSOR_PIN  GPIO_PIN_5

volatile uint8_t leakInterruptFlag = 0;
volatile uint8_t leakLatched = false;

void LeakSensor_Read(void)
{
    if (leakInterruptFlag)
        {
            leakInterruptFlag = 0;

            if (!leakLatched)
            {
                leakLatched = true;
                thrusters_disabled = true;
                printf("LEAK DETECTED\r\n");


            }
                //send the leak event here
            int32_t timestamp_ms = HAL_GetTick();
            Protocol_SendPacket(4, ID_LEAK_DETECTED, &timestamp_ms);

            FSM_PostEvent(FSM_EVENT_LEAK_DETECTED);
        }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == LEAK_SENSOR_PIN)
    {
        leakInterruptFlag = 1;
    }
}