#include <stdint.h>

#define LEAK_SENSOR_PORT GPIOB
#define LEAK_SENSOR_PIN  GPIO_PIN_5

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

