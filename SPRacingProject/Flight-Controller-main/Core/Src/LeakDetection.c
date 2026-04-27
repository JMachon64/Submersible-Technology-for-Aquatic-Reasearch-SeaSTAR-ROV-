#include "LeakDetection.h"
#include "main.h"
#include <stdio.h>
#include "stm32f3xx_hal.h"
#include "stm32f3xx_it.h"
#include "FSM.h"
#include "UartProtocol.h"
#include "PacketIDs.h"


volatile uint8_t leakInterruptFlag = 0;
volatile uint8_t leakLatched = 0;

void LeakSensor_Read(void)
{
    if (leakInterruptFlag)
        {
            leakInterruptFlag = 0;

            if (!leakLatched)
            {
                leakLatched = 1;
                printf("LEAK DETECTED\r\n");

                //send the event here
                int32_t timestamp_ms = HAL_GetTick();
                Protocol_QueuePacket(4, ID_LEAK_DETECTED, &timestamp_ms);
                FSM_PostEvent(FSM_EVENT_LEAK_DETECTED);

            }
        }
    // return (HAL_GPIO_ReadPin(LEAK_SENSOR_PORT, LEAK_SENSOR_PIN) == GPIO_PIN_SET);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{

    if (GPIO_Pin == LEAK_SENSOR_PIN)
    {
        leakInterruptFlag = 1;
    }
}