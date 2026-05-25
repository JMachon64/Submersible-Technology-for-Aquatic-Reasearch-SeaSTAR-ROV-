#ifndef MPU6050_H
#define MPU6050_H

#include <stdio.h>
#include <stdint.h>
#include <i2c.h>

#ifdef __cplusplus
extern "C"{
#endif


typedef struct{
    double Ax;
    double Ay;
    double Az;

    double Gx;
    double Gy;
    double Gz;

    double Temp;
} MPU6050_t;


uint8_t MPU6050_Init(I2C_HandleTypeDef *hi2c);

// void MPU6050_Read_Accel( MPU6050_t *data);

// void MPU6050_Read_Gyro(MPU6050_t *data);

// void MPU6050_Read_Temp(MPU6050_t *data);

void MPU6050_Read_All(MPU6050_t *data);



#ifdef __cplusplus
}
#endif

#endif