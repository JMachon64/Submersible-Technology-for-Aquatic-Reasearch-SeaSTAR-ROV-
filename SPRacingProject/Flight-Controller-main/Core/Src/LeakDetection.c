#include "LeakDetection.h"
#include "UartProtocol.h"

#define LEAK_SENSOR_PORT GPIOB
#define LEAK_SENSOR_PIN  GPIO_PIN_5

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
                Protocol_SendPacket(4, ID_LEAK_DETECTED, &timestamp_ms);
                FSM_PostEvent(FSM_EVENT_LEAK_DETECTED);

            }
        }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == LEAK_SENSOR_PIN)
    {
        leakInterruptFlag = 1;
    }
}