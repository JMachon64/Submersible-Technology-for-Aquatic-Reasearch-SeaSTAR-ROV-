/* 
 * File:   TSYS01Temperature_Sensor.h
 * Author: Jose Machon
 *
 * Created on April 1, 2026, 9:24 AM
 */

#include "stm32f3xx_hal_i2c.h"
#include "stm32f3xx.h"
#include "main.h"
#include <stdio.h>
#include <math.h>
#include "stm32f3xx_hal_i2c.h"


#define TSYS01_ADDR                        0x77  
#define TSYS01_RESET                       0x1E
#define TSYS01_ADC_READ                    0x00
#define TSYS01_ADC_TEMP_CONV               0x48
#define TSYS01_PROM_READ                   0XA0

/* 
 * 
 * 
 * 
 */
 
typedef struct {

    I2C_HandleTypeDef *hi2c;

    uint16_t Address;
    uint16_t Coefficients[8];
    uint32_t ADC_Value;
    float Temperature_C;

}TSY01_TemperatureSensor_t;

/* 
 * 
 * 
 * 
 */
 
uint8_t Init_TSYS01(TSY01_TemperatureSensor_t *Sensor,I2C_HandleTypeDef *hi2c );

uint8_t Read_TSYS01(TSY01_TemperatureSensor_t *Sensor);

void Calculate_TSYS01(TSY01_TemperatureSensor_t *Sensor);