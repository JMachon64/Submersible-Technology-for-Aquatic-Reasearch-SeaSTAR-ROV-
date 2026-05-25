#include "hmc5883l.h"

static I2C_HandleTypeDef *hmc5883l_i2c;
extern UART_HandleTypeDef huart2;

static int16_t x_offset = 0;
static int16_t y_offset = 0;
static int16_t z_offset = 0;

HMC5883L_Status HMC5883L_GetHeading(float *heading) {
    int16_t x_data, y_data;
    if (HMC5883L_GetCalibratedXData(&x_data) != HMC5883L_OK) return HMC5883L_ERROR;
    if (HMC5883L_GetCalibratedYData(&y_data) != HMC5883L_OK) return HMC5883L_ERROR;

    float heading_rad = atan2(y_data, x_data);
    *heading = heading_rad * 180.0 / M_PI;

    if (*heading < 0) {
        *heading += 360.0;
    }

    uint8_t mode = MODE_CONTINUOUS;
    if (HAL_I2C_Mem_Write(hmc5883l_i2c, HMC5883L_I2C_ADDR, MODE_REGISTER, 1, &mode, 1, 100) != HAL_OK) return HMC5883L_ERROR;

    return HMC5883L_OK;
}

HMC5883L_Status HMC5883L_SelfTest() {
    uint8_t config_a_backup, config_b_backup, mode_backup;
    if (HAL_I2C_Mem_Read(hmc5883l_i2c, HMC5883L_I2C_ADDR, CONFIG_REGISTER_A, 1, &config_a_backup, 1, 100) != HAL_OK) return HMC5883L_ERROR;
    if (HAL_I2C_Mem_Read(hmc5883l_i2c, HMC5883L_I2C_ADDR, CONFIG_REGISTER_B, 1, &config_b_backup, 1, 100) != HAL_OK) return HMC5883L_ERROR;
    if (HAL_I2C_Mem_Read(hmc5883l_i2c, HMC5883L_I2C_ADDR, MODE_REGISTER, 1, &mode_backup, 1, 100) != HAL_OK) return HMC5883L_ERROR;

    uint8_t config_a = (SAMPLE_8 << 5) | (RATE_15 << 2) | NORMAL;
    if (HAL_I2C_Mem_Write(hmc5883l_i2c, HMC5883L_I2C_ADDR, CONFIG_REGISTER_A, 1, &config_a, 1, 100) != HAL_OK) return HMC5883L_ERROR;

    uint8_t config_b = GAIN_330;
    if (HAL_I2C_Mem_Write(hmc5883l_i2c, HMC5883L_I2C_ADDR, CONFIG_REGISTER_B, 1, &config_b, 1, 100) != HAL_OK) return HMC5883L_ERROR;

    uint8_t mode = MODE_CONTINUOUS;
    if (HAL_I2C_Mem_Write(hmc5883l_i2c, HMC5883L_I2C_ADDR, MODE_REGISTER, 1, &mode, 1, 100) != HAL_OK) return HMC5883L_ERROR;

    HAL_Delay(6);

    uint16_t buffer[3];
    HMC5883L_ReadRawXData(&buffer[0]);
    HMC5883L_ReadRawYData(&buffer[1]);
    HMC5883L_ReadRawZData(&buffer[2]);

    if (HAL_I2C_Mem_Write(hmc5883l_i2c, HMC5883L_I2C_ADDR, CONFIG_REGISTER_A, 1, &config_a_backup, 1, 100) != HAL_OK) return HMC5883L_ERROR;
    if (HAL_I2C_Mem_Write(hmc5883l_i2c, HMC5883L_I2C_ADDR, CONFIG_REGISTER_B, 1, &config_b_backup, 1, 100) != HAL_OK) return HMC5883L_ERROR;
    if (HAL_I2C_Mem_Write(hmc5883l_i2c, HMC5883L_I2C_ADDR, MODE_REGISTER, 1, &mode_backup, 1, 100) != HAL_OK) return HMC5883L_ERROR;

    if (243 <= buffer[0] && buffer[0] <= 575 && 243 <= buffer[1] && buffer[1] <= 575 && 243 <= buffer[2] && buffer[2] <= 575) {
        return HMC5883L_OK;
    }

    return HMC5883L_ERROR;
}

HMC5883L_Status HMC5883L_Configure() {
    uint8_t config_a = (SAMPLE_8 << 5) | (RATE_75 << 2) | NORMAL;
    if (HAL_I2C_Mem_Write(hmc5883l_i2c, HMC5883L_I2C_ADDR, CONFIG_REGISTER_A, 1, &config_a, 1, 100) != HAL_OK) return HMC5883L_ERROR;

    uint8_t config_b = GAIN_1370;
    if (HAL_I2C_Mem_Write(hmc5883l_i2c, HMC5883L_I2C_ADDR, CONFIG_REGISTER_B, 1, &config_b, 1, 100) != HAL_OK) return HMC5883L_ERROR;

    uint8_t mode = MODE_CONTINUOUS;
    if (HAL_I2C_Mem_Write(hmc5883l_i2c, HMC5883L_I2C_ADDR, MODE_REGISTER, 1, &mode, 1, 100) != HAL_OK) return HMC5883L_ERROR;

    HAL_Delay(6);
    return HMC5883L_OK;
}

HMC5883L_Status HMC5883L_HardIronCalibrate(uint32_t calibration_duration_ms) {
    int16_t x_max = -32768, x_min = 32767;
    int16_t y_max = -32768, y_min = 32767;
    int16_t z_max = -32768, z_min = 32767;

    uint32_t start_time = HAL_GetTick();
    while (HAL_GetTick() - start_time < calibration_duration_ms) {
        uint16_t x_raw, y_raw, z_raw;
        if (HMC5883L_ReadRawXData(&x_raw) == HMC5883L_OK &&
            HMC5883L_ReadRawYData(&y_raw) == HMC5883L_OK &&
            HMC5883L_ReadRawZData(&z_raw) == HMC5883L_OK) {

            if ((int16_t)x_raw > x_max) x_max = (int16_t)x_raw;
            if ((int16_t)x_raw < x_min) x_min = (int16_t)x_raw;

            if ((int16_t)y_raw > y_max) y_max = (int16_t)y_raw;
            if ((int16_t)y_raw < y_min) y_min = (int16_t)y_raw;

            if ((int16_t)z_raw > z_max) z_max = (int16_t)z_raw;
            if ((int16_t)z_raw < z_min) z_min = (int16_t)z_raw;
        }
        HAL_Delay(50);
    }

    x_offset = (x_max + x_min) / 2;
    y_offset = (y_max + y_min) / 2;
    z_offset = (z_max + z_min) / 2;

    return HMC5883L_OK;
}

HMC5883L_Status HMC5883L_GetCalibratedXData(int16_t *buffer) {
    uint16_t raw_x;
    if (HMC5883L_ReadRawXData(&raw_x) != HMC5883L_OK) return HMC5883L_ERROR;
    *buffer = ((int16_t)raw_x - x_offset);
    return HMC5883L_OK;
}

HMC5883L_Status HMC5883L_GetCalibratedYData(int16_t *buffer) {
    uint16_t raw_y;
    if (HMC5883L_ReadRawYData(&raw_y) != HMC5883L_OK) return HMC5883L_ERROR;
    *buffer = ((int16_t)raw_y - y_offset);
    return HMC5883L_OK;
}

HMC5883L_Status HMC5883L_GetCalibratedZData(int16_t *buffer) {
    uint16_t raw_z;
    if (HMC5883L_ReadRawZData(&raw_z) != HMC5883L_OK) return HMC5883L_ERROR;
    *buffer = ((int16_t)raw_z - z_offset);
    return HMC5883L_OK;
}

HMC5883L_Status HMC5883L_ReadRawXData(uint16_t *buffer) {
    uint8_t status;
    uint32_t start_time = HAL_GetTick();
    do {
        if (HAL_I2C_Mem_Read(hmc5883l_i2c, HMC5883L_I2C_ADDR, STATUS_REGISTER, 1, &status, 1, 100) != HAL_OK) return HMC5883L_ERROR;
        if (HAL_GetTick() - start_time > 100) return HMC5883L_TIMEOUT;
    } while (!(status & 0x01));

    uint8_t data[2];
    if (HAL_I2C_Mem_Read(hmc5883l_i2c, HMC5883L_I2C_ADDR, DO_X_MSB_REGISTER, 1, data, 2, 500) != HAL_OK) return HMC5883L_ERROR;

    *buffer = (data[0] << 8) | data[1];
    return HMC5883L_OK;
}

HMC5883L_Status HMC5883L_ReadRawYData(uint16_t *buffer) {
    uint8_t status;
    uint32_t start_time = HAL_GetTick();
    do {
        if (HAL_I2C_Mem_Read(hmc5883l_i2c, HMC5883L_I2C_ADDR, STATUS_REGISTER, 1, &status, 1, 100) != HAL_OK) return HMC5883L_ERROR;
        if (HAL_GetTick() - start_time > 100) return HMC5883L_TIMEOUT; 
    } while (!(status & 0x01));

    uint8_t data[2];
    if (HAL_I2C_Mem_Read(hmc5883l_i2c, HMC5883L_I2C_ADDR, DO_Y_MSB_REGISTER, 1, data, 2, 500) != HAL_OK) return HMC5883L_ERROR;

    *buffer = (data[0] << 8) | data[1];
    return HMC5883L_OK;
}

HMC5883L_Status HMC5883L_ReadRawZData(uint16_t *buffer) {
    uint8_t status;
    uint32_t start_time = HAL_GetTick();
    do {
        if (HAL_I2C_Mem_Read(hmc5883l_i2c, HMC5883L_I2C_ADDR, STATUS_REGISTER, 1, &status, 1, 100) != HAL_OK) return HMC5883L_ERROR;
        if (HAL_GetTick() - start_time > 100) return HMC5883L_TIMEOUT;
    } while (!(status & 0x01));

    uint8_t data[2];
    if (HAL_I2C_Mem_Read(hmc5883l_i2c, HMC5883L_I2C_ADDR, DO_Z_MSB_REGISTER, 1, data, 2, 500) != HAL_OK) return HMC5883L_ERROR;

    *buffer = (data[0] << 8) | data[1];
    return HMC5883L_OK;
}

HMC5883L_Status HMC5883L_SetMeasurementConfig(MeasurementConfig measurementConfig) {
    uint8_t config = 0;
    if (HAL_I2C_Mem_Read(hmc5883l_i2c, HMC5883L_I2C_ADDR, CONFIG_REGISTER_A, 1, &config, 1, 100) != HAL_OK) return HMC5883L_ERROR;

    config &= 0xFC;
    config |= measurementConfig;

    if (HAL_I2C_Mem_Write(hmc5883l_i2c, HMC5883L_I2C_ADDR, CONFIG_REGISTER_A, 1, &config, 1, 100) != HAL_OK) return HMC5883L_ERROR;
    
    return HMC5883L_OK;
}

HMC5883L_Status HMC5883L_SetMeasurementMode(MeasurementMode mode) {
    if (HAL_I2C_Mem_Write(hmc5883l_i2c, HMC5883L_I2C_ADDR, MODE_REGISTER, 1, (uint8_t*) &mode, 1, 100) != HAL_OK) return HMC5883L_ERROR;
    return HMC5883L_OK;
}

HMC5883L_Status HMC5883L_SetDataOutputRate(DataOutputRate rate) {
    uint8_t config = 0;
    if (HAL_I2C_Mem_Read(hmc5883l_i2c, HMC5883L_I2C_ADDR, CONFIG_REGISTER_A, 1, &config, 1, 100) != HAL_OK) return HMC5883L_ERROR;

    config &= 0xE3;
    config |= (rate << 2);

    if (HAL_I2C_Mem_Write(hmc5883l_i2c, HMC5883L_I2C_ADDR, CONFIG_REGISTER_A, 1, &config, 1, 100) != HAL_OK) return HMC5883L_ERROR;
    
    return HMC5883L_OK;
}

HMC5883L_Status HMC5883L_SetSamples(Samples samples) {
    uint8_t config = 0;
    if (HAL_I2C_Mem_Read(hmc5883l_i2c, HMC5883L_I2C_ADDR, CONFIG_REGISTER_A, 1, &config, 1, 100) != HAL_OK) return HMC5883L_ERROR;

    config &= 0x9F;
    config |= (samples << 5);

    if (HAL_I2C_Mem_Write(hmc5883l_i2c, HMC5883L_I2C_ADDR, CONFIG_REGISTER_A, 1, &config, 1, 100) != HAL_OK) return HMC5883L_ERROR;
    
    return HMC5883L_OK;
}

HMC5883L_Status HMC5883L_SetGain(Gain gain) {
    uint8_t config = 0;
    if (HAL_I2C_Mem_Read(hmc5883l_i2c, HMC5883L_I2C_ADDR, CONFIG_REGISTER_B, 1, &config, 1, 100) != HAL_OK) return HMC5883L_ERROR;

    config &= 0x1F;
    config |= gain;

    if (HAL_I2C_Mem_Write(hmc5883l_i2c, HMC5883L_I2C_ADDR, CONFIG_REGISTER_B, 1, &config, 1, 100) != HAL_OK) return HMC5883L_ERROR;
    
    return HMC5883L_OK;
}

HMC5883L_Status HMC5883L_Init(I2C_HandleTypeDef *hi2c) {
    hmc5883l_i2c = hi2c;

    uint8_t id[3];
    if (HAL_I2C_Mem_Read(hmc5883l_i2c, HMC5883L_I2C_ADDR, ID_REGISTER_A, 1, id, 3, 100) != HAL_OK) {
        return HMC5883L_ERROR;
    }

    if (id[0] != 'H' || id[1] != '4' || id[2] != '3') {
        return HMC5883L_ERROR;
    }

    if (HMC5883L_SelfTest() != HMC5883L_OK) return HMC5883L_ERROR;

    return HMC5883L_OK;
}
