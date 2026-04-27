#include "Mux.h"


#define I2C_MUX_ADDR  0x70



void I2C_Mux_Disable_All(I2C_HandleTypeDef *hi2c)
{
    uint8_t data = 0x00;
    HAL_I2C_Master_Transmit(hi2c, 0x70 << 1, &data, 1, 100);
    HAL_Delay(2);
}

uint8_t I2C_Mux_Select(I2C_HandleTypeDef *hi2c, uint8_t channel)
{
    if (channel > 7) return 0;

    uint8_t data = (1 << channel);

    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
        hi2c,
        I2C_MUX_ADDR << 1,
        &data,
        1,
        100
    );

    if (status != HAL_OK) {
        printf("MUX select failed ch=%u status=%d\r\n", channel, status);
        return 0;
    }

    HAL_Delay(2);
    return 1;
}