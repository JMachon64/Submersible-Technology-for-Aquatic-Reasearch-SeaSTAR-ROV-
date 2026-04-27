#include "HMC5883L.h"
#include <i2c.h>


#define HMC5883L_ADDR (0x1E << 1)

#define HMC5883L_REG_CONFIG_A 0x00
#define HMC5883L_REG_CONFIG_B 0x01
#define HMC5883L_REG_MODE 0x02
#define HMC5883L_REG_X_MSB 0x03
#define HMC5883L_REG_X_LSB 0x04
#define HMC5883L_REG_Y_MSB 0x05
#define HMC5883L_REG_Y_LSB 0x06
#define HMC5883L_REG_Z_MSB 0x07
#define HMC5883L_REG_Z_LSB 0x08
#define HMC5883L_REG_STATUS 0x09
#define HMC5883L_REG_ID_A 0x0A
#define HMC5883L_REG_ID_B 0x0B
#define HMC5883L_REG_ID_C 0x0C

I2C_HandleTypeDef* hmc_handle;



uint8_t HMC5883L_Init(I2C_HandleTypeDef *hi2c){
    uint8_t id[3];
    uint8_t data;

    hmc_handle = hi2c;
    HAL_I2C_Mem_Read(hmc_handle, HMC5883L_ADDR, HMC5883L_REG_ID_A, 1, id, 3, i2c_timeout);

    if(id[0] != 0x48 || id[1] != 0x34 || id[2] != 0x33)return 1;




    return 0;
}