#include "TelemetryStream.h"
#include "TSYS01TemperatureSensor.h"


static HAL_StatusTypeDef TSYS01_WriteCommand(TSY01_TemperatureSensor_t *Sensor, uint8_t Command)
{
    return HAL_I2C_Master_Transmit(Sensor->hi2c, Sensor->Address, &Command, 1, 100);
}

static HAL_StatusTypeDef TSYS01_ReadBytes(TSY01_TemperatureSensor_t *Sensor, uint8_t *Buffer, uint16_t Length)
{
    return HAL_I2C_Master_Receive(Sensor->hi2c, Sensor->Address, Buffer, Length, 100);
}


uint8_t Init_TSYS01(TSY01_TemperatureSensor_t *Sensor, I2C_HandleTypeDef *hi2c)
{
    uint8_t buffer[2] = {0};

    HAL_StatusTypeDef status;

    Sensor->hi2c = hi2c;
    Sensor->Address = (TSYS01_ADDR << 1);

    status = TSYS01_WriteCommand(Sensor, TSYS01_RESET);
    if (status != HAL_OK) {
        printf("TSYS01 reset failed, status=%d\r\n", status);
        return 0;
    }

    HAL_Delay(10);

    for (uint8_t i = 0; i < 8; i++) {
        status = TSYS01_WriteCommand(Sensor, TSYS01_PROM_READ + (i * 2));
        if (status != HAL_OK) {
            printf("TSYS01 PROM cmd failed at %u, status=%d\r\n", i, status);
            return 0;
        }

        status = TSYS01_ReadBytes(Sensor, buffer, 2);
        if (status != HAL_OK) {
            printf("TSYS01 PROM read failed at %u, status=%d\r\n", i, status);
            return 0;
        }

        Sensor->Coefficients[i] = ((uint16_t)buffer[0] << 8) | buffer[1];
        printf("C[%u] = %u\r\n", i, Sensor->Coefficients[i]);
    }

    printf("TSYS01 init done\r\n");
    return 1;
}

uint8_t Read_TSYS01(TSY01_TemperatureSensor_t *Sensor){

    uint8_t buffer[3];

    TSYS01_WriteCommand(Sensor, TSYS01_ADC_TEMP_CONV);

    HAL_Delay(10);

    TSYS01_WriteCommand(Sensor, TSYS01_ADC_READ);

    TSYS01_ReadBytes(Sensor, buffer, 3);
    
    Sensor -> ADC_Value = ((uint32_t)buffer[0] << 16 | (uint32_t)buffer[1] << 8 | (uint32_t)buffer[2]);
    printf("TEMP: %lu\r\n", Sensor -> ADC_Value);
    Calculate_TSYS01(Sensor);

    //store temp
    environmetal_telemetry_packet.temp = (int32_t)(Sensor->Temperature_C * 1000.0f);

    return 1;

}

void Calculate_TSYS01(TSY01_TemperatureSensor_t *Sensor){

    float ADC_Sample = (Sensor -> ADC_Value) / 256.0f;

    Sensor -> Temperature_C = (-2.0f) * ((float)Sensor -> Coefficients[1]) / 1000000000000000000000.0f * powf(ADC_Sample, 4) + 
                               (4.0f) * ((float)Sensor -> Coefficients[2]) / 10000000000000000.0f * powf(ADC_Sample, 3) +     
                              (-2.0f) * ((float)Sensor -> Coefficients[3]) / 100000000000.0f * powf(ADC_Sample, 2) + 
                               (1.0f) * ((float)Sensor -> Coefficients[4]) / 1000000.0f * ADC_Sample + 
                              (-1.5f) * ((float)Sensor -> Coefficients[5]) / 100.0f;

      float temp = Sensor->Temperature_C;
      
}



//I DID NOT MAKE SURE THIS WORKED 

// void readTestCase_TSYS01() {
// 	C[0] = 0;
// 	C[1] = 28446;  //0xA2 K4
// 	C[2] = 24926;  //0XA4 k3
// 	C[3] = 36016;  //0XA6 K2
// 	C[4] = 32791;  //0XA8 K1
// 	C[5] = 40781;  //0XAA K0
// 	C[6] = 0;
// 	C[7] = 0;

// 	D1 = 9378708.0f;
	
// 	adc = D1/256;

// 	calculate();
// }

