#include "main.h"
#include <stdio.h>



void I2C_Mux_Disable_All(I2C_HandleTypeDef *hi2c);

uint8_t I2C_Mux_Select(I2C_HandleTypeDef *hi2c, uint8_t channel);