#ifndef HMC5883L_H
#define HMC5883L_H

#include <stdio.h>
#include <stdint.h>
#include <i2c.h>

#ifdef __cplusplus
extern "C"{
#endif


typedef struct{
    double Hx;
    double Hy;
    double Hz;
} HMC5883L_t;


uint8_t HMC5883L_Init(I2C_HandleTypeDef *hi2c);





#ifdef __cplusplus
}
#endif

#endif