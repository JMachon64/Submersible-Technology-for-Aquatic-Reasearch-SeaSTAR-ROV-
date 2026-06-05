/* 
 * File:   LeakDetection.h
 * Author: Jose Machon
 *
 * Created on April 1, 2026, 9:24 AM
 */

#include "main.h"
#include "stm32f3xx_hal.h"
#include "stm32f3xx_it.h"
#include "FSM.h"
#include "UartProtocol.h"
#include "PacketIDs.h"
#include <stdint.h>
#include <stdio.h>

/* 
 * 
 * 
 * 
 */

extern volatile uint8_t leakInterruptFlag;
extern volatile uint8_t leakLatched;

/* 
 * Function triggered by ISR continously polled from the RTOS task
 * sends event update to FSM and sends out UART packet when triggered.
 */ 
 
void LeakSensor_Read(void);

