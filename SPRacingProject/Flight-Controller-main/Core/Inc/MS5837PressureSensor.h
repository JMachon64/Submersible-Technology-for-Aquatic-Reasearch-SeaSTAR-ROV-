/* 
 * File:  MS5837PressureSensor.h
 *
 * Author: Jose Machon
 *
 * Created on April 1, 2026, 9:24 AM
 */

#ifndef MS5837_PRESSURE_SENSOR_H
#define MS5837_PRESSURE_SENSOR_H

#include "main.h"
#include "stm32f3xx_hal_i2c.h"
#include "stm32f3xx.h"

#include <stdio.h>
#include <math.h>
#include <stdint.h>

/* 
 * 
 * 
 * 
 */

#define  MS5837_ADDR              0x76
#define  MS5837_RESET             0x1E
#define  MS5837_ADC_READ          0x00
#define  MS5837_PROM_READ         0xA0
#define  MS5837_CONVERT_D1_8192   0x4A
#define  MS5837_CONVERT_D2_8192   0x5A

/* 
 * Sensor conversion values.
 * 
 * 
 */ 

#define  MS5837Pa     100.0f
#define  MS5837bar    0.001f
#define  MS5837mbar   1.0f

#define MS5837_30BA   0
#define MS5837_02BA   1
#define MS5837_UNRECOGNISED   255

/* 
 * Sensor sensitivities depending on the model, used to calculate depth.
 * 
 * 
 */ 

#define  MS5837_02BA_MAX_SENSITIVITY   49000
#define  MS5837_02BA_30BA_SEPARATION   37000
#define  MS5837_30BA_MIN_SENSITIVITY   26000

/* 
 * Sampling callibration value.
 */  

#define SURFACE_PRESSURE_AVERAGE 50

extern volatile  uint8_t baseline_set;
extern volatile float surface_pressure_pa;
extern volatile float pressure_offset;

/* 
 * Struct storing all relevant sensor data.
 */

typedef struct {

    I2C_HandleTypeDef *hi2c;

    uint16_t Address;
    uint16_t Model;
    uint16_t Coefficients[8];

    uint32_t D2_ADC_Temperature;
    uint32_t D1_ADC_Pressure;
    
    uint32_t Temperature;
    uint32_t Pressure;

    float Pressure_Pa;
    float Pressure_bar;
    float Pressure_mbar;
    float Depth_m;

}MS5837_PressureSensor_t;

/* 
 * 
 * 
 * 
 */

// initilization function
uint8_t Init_MS5837(MS5837_PressureSensor_t *Sensor,I2C_HandleTypeDef *hi2c);

float Callibrate_MS5837(MS5837_PressureSensor_t *Sensor, uint16_t Samples);

uint8_t Read_MS5837(MS5837_PressureSensor_t *Sensor);

void Calculate_MS5837(MS5837_PressureSensor_t *Sensor);

float MS5837_GetPressure(MS5837_PressureSensor_t *Sensor, float conversion);
float MS5837_GetTemp(MS5837_PressureSensor_t *Sensor);
float MS5837_GetDepth(MS5837_PressureSensor_t *Sensor, float fluiddensity);

uint8_t  MS5837_GetModel(MS5837_PressureSensor_t *Sensor);
uint8_t crc4(uint16_t *Coefficients);



// Unused function for our needs, did not test this.
float MS5837_GetAltitude(MS5837_PressureSensor_t *Sensor);

#endif