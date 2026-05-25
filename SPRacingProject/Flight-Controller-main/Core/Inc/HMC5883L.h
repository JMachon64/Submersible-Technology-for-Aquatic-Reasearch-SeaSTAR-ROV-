#ifndef __HMC5883L_H__
#define __HMC5883L_H__

#include "main.h"
#include <math.h>

#define HMC5883L_I2C_ADDR (0x1E << 1)

#define CONFIG_REGISTER_A 0x00
#define CONFIG_REGISTER_B 0x01
#define MODE_REGISTER     0x02
#define DO_X_MSB_REGISTER 0x03
#define DO_X_LSB_REGISTER 0x04
#define DO_Y_MSB_REGISTER 0x05
#define DO_Y_LSB_REGISTER 0x06
#define DO_Z_MSB_REGISTER 0x07
#define DO_Z_LSB_REGISTER 0x08
#define STATUS_REGISTER   0x09
#define ID_REGISTER_A     0x0A
#define ID_REGISTER_B     0x0B
#define ID_REGISTER_C     0x0C

typedef enum {
    GAIN_1370 = (0x00 << 5),
    GAIN_1090 = (0x01 << 5),
    GAIN_820 = (0x02 << 5),
    GAIN_660 = (0x03 << 5),
    GAIN_440 = (0x04 << 5),
    GAIN_390 = (0x05 << 5),
    GAIN_330 = (0x06 << 5),
    GAIN_230 = (0x07 << 5),
} Gain;

typedef enum {
    RATE_0_75 = 0x00,
    RATE_1_5 = 0x01,
    RATE_3 = 0x02,
    RATE_7_5 = 0x03,
    RATE_15 = 0x04,
    RATE_30 = 0x05,
    RATE_75 = 0x06
} DataOutputRate;

typedef enum {
    SAMPLE_1 = 0x00,
    SAMPLE_2 = 0x01,
    SAMPLE_4 = 0x02,
    SAMPLE_8 = 0x03
} Samples;

typedef enum {
    NORMAL = 0x00,
    POSITIVE_BIAS = 0x01,
    NEGATIVE_BIAS = 0x02
} MeasurementConfig;

typedef enum {
    MODE_CONTINUOUS = 0x00,
    MODE_SINGLE = 0x01,
    MODE_IDLE = 0x02
} MeasurementMode;

typedef enum {
    HMC5883L_ERROR,
    HMC5883L_OK,
    HMC5883L_TIMEOUT
} HMC5883L_Status;

HMC5883L_Status HMC5883L_GetHeading(float *heading);
HMC5883L_Status HMC5883L_SelfTest();
HMC5883L_Status HMC5883L_Configure();
HMC5883L_Status HMC5883L_HardIronCalibrate(uint32_t calibration_duration_ms);
HMC5883L_Status HMC5883L_GetCalibratedXData(int16_t *buffer);
HMC5883L_Status HMC5883L_GetCalibratedYData(int16_t *buffer);
HMC5883L_Status HMC5883L_GetCalibratedZData(int16_t *buffer);
HMC5883L_Status HMC5883L_ReadRawXData(uint16_t *buffer);
HMC5883L_Status HMC5883L_ReadRawYData(uint16_t *buffer);
HMC5883L_Status HMC5883L_ReadRawZData(uint16_t *buffer);
HMC5883L_Status HMC5883L_SetMeasurementConfig(MeasurementConfig config);
HMC5883L_Status HMC5883L_SetMeasurementMode(MeasurementMode mode);
HMC5883L_Status HMC5883L_SetDataOutputRate(DataOutputRate rate);
HMC5883L_Status HMC5883L_SetSamples(Samples samples);
HMC5883L_Status HMC5883L_SetGain(Gain gain);
HMC5883L_Status HMC5883L_Init(I2C_HandleTypeDef *hi2c1);

#endif