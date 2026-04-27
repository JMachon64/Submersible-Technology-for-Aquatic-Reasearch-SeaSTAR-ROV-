#include "MPU6050.h"
#include "cmsis_os2.h"
#include <i2c.h>
#include <stdint.h>

#define MPU6050_ADDR 0xD0

#define MPU6050_REG_SELF_TEST_X         0x0D        /**< self test x register */
#define MPU6050_REG_SELF_TEST_Y         0x0E        /**< self test y register */
#define MPU6050_REG_SELF_TEST_Z         0x0F        /**< self test z register */
#define MPU6050_REG_SELF_TEST_A         0x10        /**< self test a register */
#define MPU6050_REG_SMPRT_DIV           0x19        /**< smprt div register */
#define MPU6050_REG_CONFIG              0x1A        /**< configure register */
#define MPU6050_REG_GYRO_CONFIG         0x1B        /**< gyro configure register */
#define MPU6050_REG_ACCEL_CONFIG        0x1C        /**< accel configure register */
#define MPU6050_REG_MOTION_THRESHOLD    0x1F        /**< motion threshold register */
#define MPU6050_REG_MOTION_DURATION     0x20        /**< motion duration register */
#define MPU6050_REG_FIFO_EN             0x23        /**< fifo enable register */
#define MPU6050_REG_I2C_MST_CTRL        0x24        /**< i2c master ctrl register */
#define MPU6050_REG_I2C_MST_STATUS      0x36        /**< i2c master status register */
#define MPU6050_REG_I2C_MST_DELAY_CTRL  0x67        /**< i2c master delay ctrl register */
#define MPU6050_REG_I2C_SLV0_ADDR       0x25        /**< iic slave0 address register */
#define MPU6050_REG_I2C_SLV0_REG        0x26        /**< iic slave0 reg register */
#define MPU6050_REG_I2C_SLV0_CTRL       0x27        /**< iic slave0 ctrl register */
#define MPU6050_REG_I2C_SLV0_DO         0x63        /**< iic slave0 do register */
#define MPU6050_REG_I2C_SLV1_ADDR       0x28        /**< iic slave1 address register */
#define MPU6050_REG_I2C_SLV1_REG        0x29        /**< iic slave1 reg register */
#define MPU6050_REG_I2C_SLV1_CTRL       0x2A        /**< iic slave1 ctrl register */
#define MPU6050_REG_I2C_SLV1_DO         0x64        /**< iic slave1 do register */
#define MPU6050_REG_I2C_SLV2_ADDR       0x2B        /**< iic slave2 address register */
#define MPU6050_REG_I2C_SLV2_REG        0x2C        /**< iic slave2 reg register */
#define MPU6050_REG_I2C_SLV2_CTRL       0x2D        /**< iic slave2 ctrl register */
#define MPU6050_REG_I2C_SLV2_DO         0x65        /**< iic slave2 do register */
#define MPU6050_REG_I2C_SLV3_ADDR       0x2E        /**< iic slave3 address register */
#define MPU6050_REG_I2C_SLV3_REG        0x2F        /**< iic slave3 reg register */
#define MPU6050_REG_I2C_SLV3_CTRL       0x30        /**< iic slave3 ctrl register */
#define MPU6050_REG_I2C_SLV3_DO         0x66        /**< iic slave3 do register */
#define MPU6050_REG_I2C_SLV4_ADDR       0x31        /**< iic slave4 address register */
#define MPU6050_REG_I2C_SLV4_REG        0x32        /**< iic slave4 reg register */
#define MPU6050_REG_I2C_SLV4_CTRL       0x34        /**< iic slave4 ctrl register */
#define MPU6050_REG_I2C_SLV4_DO         0x33        /**< iic slave4 do register */
#define MPU6050_REG_I2C_SLV4_DI         0x35        /**< iic slave4 di register */
#define MPU6050_REG_EXT_SENS_DATA_00    0x49        /**< extern sensor data 00 register */
#define MPU6050_REG_INT_PIN_CFG         0x37        /**< interrupt pin configure register */
#define MPU6050_REG_INT_ENABLE          0x38        /**< interrupt enable register */
#define MPU6050_REG_INT_STATUS          0x3A        /**< interrupt status register */
#define MPU6050_REG_ACCEL_XOUT_H        0x3B        /**< accel xout high register */
#define MPU6050_REG_ACCEL_XOUT_L        0x3C        /**< accel xout low register */
#define MPU6050_REG_ACCEL_YOUT_H        0x3D        /**< accel yout high register */
#define MPU6050_REG_ACCEL_YOUT_L        0x3E        /**< accel yout low register */
#define MPU6050_REG_ACCEL_ZOUT_H        0x3F        /**< accel zout high register */
#define MPU6050_REG_ACCEL_ZOUT_L        0x40        /**< accel zout low register */
#define MPU6050_REG_TEMP_OUT_H          0x41        /**< temp high register */
#define MPU6050_REG_TEMP_OUT_L          0x42        /**< temp low register */
#define MPU6050_REG_GYRO_XOUT_H         0x43        /**< gyro xout high register */
#define MPU6050_REG_GYRO_XOUT_L         0x44        /**< gyro xout low register */
#define MPU6050_REG_GYRO_YOUT_H         0x45        /**< gyro yout high register */
#define MPU6050_REG_GYRO_YOUT_L         0x46        /**< gyro yout low register */
#define MPU6050_REG_GYRO_ZOUT_H         0x47        /**< gyro zout high register */
#define MPU6050_REG_GYRO_ZOUT_L         0x48        /**< gyro zout low register */
#define MPU6050_REG_SIGNAL_PATH_RESET   0x68        /**< signal path reset register */
#define MPU6050_REG_USER_CTRL           0x6A        /**< user ctrl register */
#define MPU6050_REG_PWR_MGMT_1          0x6B        /**< power management 1 register */
#define MPU6050_REG_PWR_MGMT_2          0x6C        /**< power management 2 register */
#define MPU6050_REG_BANK_SEL            0x6D        /**< bank sel register */
#define MPU6050_REG_MEM                 0x6F        /**< memory register */
#define MPU6050_REG_PROGRAM_START       0x70        /**< program start register */
#define MPU6050_REG_FIFO_COUNTH         0x72        /**< fifo count high threshold register */
#define MPU6050_REG_FIFO_COUNTL         0x73        /**< fifo count low threshold register */
#define MPU6050_REG_R_W                 0x74        /**< fifo read write data register */
#define MPU6050_REG_WHO_AM_I            0x75        /**< who am I register */



I2C_HandleTypeDef* mpu_handle;


uint8_t MPU6050_Init(I2C_HandleTypeDef *hi2c){
    uint8_t id;
    uint8_t data;

    mpu_handle = hi2c;

    
    data = 0x00; // dont sleep or reset
    HAL_I2C_Mem_Write(mpu_handle, MPU6050_ADDR, MPU6050_REG_PWR_MGMT_1, 1, &data, 1, i2c_timeout);

    osDelay(100);
    
    HAL_I2C_Mem_Read(mpu_handle, MPU6050_ADDR, MPU6050_REG_WHO_AM_I, 1, &id, 1, i2c_timeout);

    if(id != 0x68)return 1; // check to make sure id is correct, return 1 if wrong


    // osDelay(100);

    // data = 0b00000111;
    // HAL_I2C_Mem_Write(mpu_handle, MPU6050_ADDR, MPU6050_REG_SIGNAL_PATH_RESET, 1, &data, 1, i2c_timeout);



    // data = 0x07; //set data rate to 1kHz
    // HAL_I2C_Mem_Write(mpu_handle, MPU6050_ADDR, MPU6050_REG_SMPRT_DIV, 1, &data, 1, i2c_timeout);

    // osDelay(15);

    //  data = 0x03; //filter
    // HAL_I2C_Mem_Write(mpu_handle, MPU6050_ADDR, MPU6050_REG_CONFIG, 1, &data, 1, i2c_timeout);

    data = 0x00;
    HAL_I2C_Mem_Write(mpu_handle, MPU6050_ADDR, MPU6050_REG_ACCEL_CONFIG, 1, &data, 1, i2c_timeout);

    data = 0x00;
    HAL_I2C_Mem_Write(mpu_handle, MPU6050_ADDR, MPU6050_REG_GYRO_CONFIG, 1, &data, 1, i2c_timeout);



    return 0;
}


// void MPU6050_Read_Accel( MPU6050_t *data){

// }

// void MPU6050_Read_Gyro(MPU6050_t *data){

// }

// void MPU6050_Read_Temp(MPU6050_t *data){

// }

void MPU6050_Read_All(MPU6050_t *data){
    uint8_t raw_data[14];

    HAL_I2C_Mem_Read(mpu_handle, MPU6050_ADDR, MPU6050_REG_ACCEL_XOUT_H, 1, raw_data, 6, i2c_timeout);



    data->Ax = (int16_t)(raw_data[0] << 8 | raw_data[1])/(16384.0*0.5) + 0.15;
    data->Ay = (int16_t)(raw_data[2] << 8 | raw_data[3])/(16384.0*0.5) + 0.0;
    data->Az = (int16_t)(raw_data[4] << 8 | raw_data[5])/(16384.0*0.5) + 0.6;
    // data->Temp = ((int16_t)(raw_data[6] << 8 | raw_data[7]) / 340.0) + 36.53;
    // data->Gx = (int16_t)(raw_data[8] << 8 | raw_data[9])/131.0;
    // data->Gy = (int16_t)(raw_data[10] << 8 | raw_data[11])/131.0;
    // data->Gz = (int16_t)(raw_data[12] << 8 | raw_data[13])/131.0;



    
    // data->Ax = (int16_t)(raw_data[0] << 8 | raw_data[1]);
    // data->Ay = (int16_t)(raw_data[2] << 8 | raw_data[3]);
    // data->Az = (int16_t)(raw_data[4] << 8 | raw_data[5]);
    // data->Temp = ((int16_t)(raw_data[6] << 8 | raw_data[7]) / 340.0) + 36.53;
    // data->Gx = (int16_t)(raw_data[8] << 8 | raw_data[9]);
    // data->Gy = (int16_t)(raw_data[10] << 8 | raw_data[11]);
    // data->Gz = (int16_t)(raw_data[12] << 8 | raw_data[13]);

    

}