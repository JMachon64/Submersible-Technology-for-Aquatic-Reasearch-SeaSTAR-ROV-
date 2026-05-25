

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
 * 
 * 
 * 
 */ 
 
void LeakSensor_Read(void);

