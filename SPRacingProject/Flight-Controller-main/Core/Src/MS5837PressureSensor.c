

#include "MS5837PressureSensor.h"
#include "TelemetryStream.h"

// static const uint16_t fluiddensity = 1029;

volatile float pressure_offset = 28820.0f;
volatile float surface_pressure_pa = 0.0f;
volatile uint8_t baseline_set = 0;

float Callibrate_MS5837(MS5837_PressureSensor_t *Sensor, uint16_t Samples)
{
    float sum = 0.0f;

    for (uint8_t i = 0; i < 10; i++)
    {
        Read_MS5837(Sensor);
        HAL_Delay(50);
    }

    for (uint16_t i = 0; i < Samples; i++)
    {
        Read_MS5837(Sensor);
        sum += Sensor->Pressure_Pa;
        HAL_Delay(50);
    }

    float baseline = sum / Samples;

    printf("Surface baseline = %ld Pa\r\n", (int32_t)baseline);
	baseline_set = 1;
    return baseline;
}

static HAL_StatusTypeDef MS5837_WriteCommand(MS5837_PressureSensor_t *Sensor, uint8_t Command)
{
    return HAL_I2C_Master_Transmit(Sensor->hi2c, Sensor->Address, &Command, 1, 100);
}

static HAL_StatusTypeDef MS5837_ReadBytes(MS5837_PressureSensor_t *Sensor, uint8_t *Buffer, uint16_t Length)
{
    return HAL_I2C_Master_Receive(Sensor->hi2c, Sensor->Address, Buffer, Length, 100);
}


uint8_t Init_MS5837(MS5837_PressureSensor_t *Sensor,I2C_HandleTypeDef *hi2c){


   uint8_t buffer[2];

    Sensor -> hi2c = hi2c;
    Sensor -> Address = (MS5837_ADDR << 1); //stm32 requires one bit shift 


    MS5837_WriteCommand(Sensor, MS5837_RESET);

	HAL_Delay(20); //Maximum required command write period

	
	// Read calibration values
	for ( uint8_t i = 0 ; i < 7 ; i++ ) {

        // read command 
		MS5837_WriteCommand(Sensor, MS5837_PROM_READ+(i*2));
		HAL_Delay(20);

        // read and store 
        MS5837_ReadBytes(Sensor, buffer, 2);

		Sensor -> Coefficients[i] = ((uint16_t)buffer[0] << 8 )| buffer[1];
	}

    uint8_t crcRead = Sensor->Coefficients[0] >> 12;
    uint8_t crcCalculated = crc4(Sensor->Coefficients);
    for (uint8_t i = 0; i < 7; i++)
        {
            printf("C[%d] = %u\r\n", i, Sensor->Coefficients[i]);
        }

    printf("CRC read = %u, CRC calc = %u\r\n", crcRead, crcCalculated);
    printf("Model = %d\r\n", Sensor->Model);
	// Verify that data is correct with CRC

	if ( crcCalculated != crcRead ) {
		return 0; // CRC fail
	}

	// PROM Word 1 represents the sensor's pressure sensitivity calibration
	// Set _model according to the experimental pressure sensitivity thresholds

	if (Sensor -> Coefficients[1] < MS5837_30BA_MIN_SENSITIVITY ||Sensor -> Coefficients[1] > MS5837_02BA_MAX_SENSITIVITY)
	{
		Sensor -> Model = MS5837_UNRECOGNISED;
        return 0;
	}
	else if (Sensor -> Coefficients[1] > MS5837_02BA_30BA_SEPARATION)
	{
		Sensor -> Model = MS5837_02BA;
	}
	else
	{
		Sensor -> Model = MS5837_30BA;
	}
	return 1;
}

uint8_t  MS5837_GetModel(MS5837_PressureSensor_t *Sensor){
    return Sensor -> Model;
}


uint8_t Read_MS5837(MS5837_PressureSensor_t *Sensor)
{

    uint8_t buffer[3];

	// Request D1 conversion
    MS5837_WriteCommand(Sensor, MS5837_CONVERT_D1_8192);

    HAL_Delay(20); // Max conversion time per datasheet

    // Begin reading the bytes
    MS5837_WriteCommand(Sensor, MS5837_ADC_READ);

    MS5837_ReadBytes(Sensor, buffer, 3);

    Sensor -> D1_ADC_Pressure = ((uint32_t)buffer[0] << 16 | (uint32_t)buffer[1] << 8 | (uint32_t)buffer[2]);

    // Request D2 conversion

    MS5837_WriteCommand(Sensor, MS5837_CONVERT_D2_8192);
    
    HAL_Delay(20); // Max conversion time per datasheet
    
    MS5837_WriteCommand(Sensor, MS5837_ADC_READ);
    MS5837_ReadBytes(Sensor, buffer, 3);

    Sensor -> D2_ADC_Temperature = ((uint32_t)buffer[0] << 16 | (uint32_t)buffer[1] << 8 | (uint32_t)buffer[2]);


	Calculate_MS5837(Sensor);

	Sensor->Pressure_Pa = MS5837_GetPressure(Sensor, MS5837Pa);
	Sensor->Depth_m     = MS5837_GetDepth(Sensor, 1029.0f);


    return 1;
}



void Calculate_MS5837(MS5837_PressureSensor_t *Sensor) {

	// Given C1-C6 and D1, D2, calculated Temperature and Pressure
	// Do conversion first and then second order temp compensation

	int32_t dT = 0;
	int64_t SENS = 0;
	int64_t OFF = 0;
	int32_t SENSi = 0;
	int32_t OFFi = 0;
	int32_t Ti = 0;
	int64_t OFF2 = 0;
	int64_t SENS2 = 0;

	// Terms called
	dT = Sensor->D2_ADC_Temperature - (int64_t)(Sensor -> Coefficients[5]) * 256;

	if ( Sensor -> Model == MS5837_02BA ){
		SENS = (int64_t)(Sensor -> Coefficients[1]) * 65536+(int64_t)(Sensor -> Coefficients[3]) * dT / 128;
		OFF = (int64_t)(Sensor -> Coefficients[2]) * 131072+((int64_t)(Sensor -> Coefficients[4]) * dT) / 64;
		Sensor -> Pressure = (Sensor -> D1_ADC_Pressure * SENS / (2097152l) - OFF) / (32768);
	} else {
		SENS = (int64_t)(Sensor -> Coefficients[1]) * 32768 + (int64_t)(Sensor -> Coefficients[3])*dT / 256;
		OFF = (int64_t)(Sensor -> Coefficients[2]) * 65536 + (int64_t)(Sensor -> Coefficients[4])*dT / 128;
		Sensor -> Pressure = (Sensor -> D1_ADC_Pressure*SENS  / (2097152) - OFF)/(8192);
	}

	// Temp conversion
	Sensor -> Temperature = 2000 + (int64_t)(dT) * Sensor -> Coefficients[6] / 8388608;

	//Second order compensation
	if (Sensor -> Model == MS5837_02BA ) {
		if((Sensor -> Temperature / 100) < 20){         //Low temp
			Ti = (11 * (int64_t)(dT) * (int64_t)(dT)) / (34359738368LL);
			OFFi = (31 * (Sensor -> Temperature - 2000) * (Sensor -> Temperature - 2000)) / 8;
			SENSi = (63 * (Sensor -> Temperature - 2000) * (Sensor -> Temperature - 2000)) / 32;
		}
	} else {
		if((Sensor -> Temperature / 100) < 20){         //Low temp
			Ti = (3 * (int64_t)(dT) * (int64_t)(dT)) / (8589934592);
			OFFi = (3 * (Sensor -> Temperature - 2000)*(Sensor -> Temperature - 2000)) / 2;
			SENSi = (5 * (Sensor -> Temperature - 2000)*(Sensor -> Temperature - 2000)) / 8;
			if((Sensor -> Temperature / 100) < -15){    //Very low temp
				OFFi = OFFi + 7 * (Sensor -> Temperature + 1500l)*(Sensor -> Temperature + 1500);
				SENSi = SENSi + 4 * (Sensor -> Temperature + 1500l)*(Sensor -> Temperature + 1500);
			}
		}
		else if((Sensor -> Temperature / 100)>=20){    //High temp
			Ti = 2*(dT*dT)/(137438953472);
			OFFi = (1*(Sensor -> Temperature - 2000)*(Sensor -> Temperature -2000)) / 16;
			SENSi = 0;
		}

        
	}

	OFF2 = OFF-OFFi;           //Calculate pressure and temp second order
	SENS2 = SENS-SENSi;

	Sensor -> Temperature = (Sensor -> Temperature - Ti);

	if (Sensor -> Model == MS5837_02BA){
		Sensor -> Pressure = (((Sensor->D1_ADC_Pressure*SENS2) / 2097152-OFF2) / 32768);
	} else {
		Sensor -> Pressure = (((Sensor->D1_ADC_Pressure*SENS2) / 2097152-OFF2) / 8192);
	}

}

float MS5837_GetPressure(MS5837_PressureSensor_t *Sensor, float conversion) {

	if (Sensor -> Model == MS5837_02BA ) {
		return  (Sensor->Pressure * conversion/100.0f); 
	}
	else {
		return  (Sensor->Pressure * conversion/10.0f);
	}

}

float MS5837_GetTemp(MS5837_PressureSensor_t *Sensor) {
	return Sensor->Temperature / 100.0f;
}

// // The pressure sensor measures absolute pressure, so it will measure the atmospheric pressure + water pressure
// // We subtract the atmospheric pressure to calculate the depth with only the water pressure
// // The average atmospheric pressure of 101300 pascal is used for the calcuation, but atmospheric pressure varies
// // If the atmospheric pressure is not 101300 at the time of reading, the depth reported will be offset
// // In order to calculate the correct depth, the actual atmospheric pressure should be measured once in air, and
// // that value should subtracted for subsequent depth calculations.
float MS5837_GetDepth(MS5837_PressureSensor_t *Sensor, float fluiddensity) {


	float depth = (Sensor -> Pressure_Pa - surface_pressure_pa) / (fluiddensity * 9.80665);


	return depth;

}

float MS5837_GetAltitude(MS5837_PressureSensor_t *Sensor) {

    float pressure_mbar = MS5837_GetPressure(Sensor, MS5837mbar);

	Sensor -> Pressure_mbar = pressure_mbar;
	Sensor -> Pressure_bar = pressure_mbar / 1000.0f;

	return (1-pow((pressure_mbar/1013.25), .190284)) * 145366.45 * .3048;

}


uint8_t crc4(uint16_t *Coefficient) {
	uint16_t n_rem = 0;

	Coefficient[0] = ((Coefficient[0]) & 0x0FFF);
	Coefficient[7] = 0;

	for ( uint8_t i = 0 ; i < 16; i++ ) {
		if ( i%2 == 1 ) {
			n_rem ^= (uint16_t)((Coefficient[i>>1]) & 0x00FF);
		} else {
			n_rem ^= (uint16_t)(Coefficient[i>>1] >> 8);
		}
		for ( uint8_t n_bit = 8 ; n_bit > 0 ; n_bit-- ) {
			if ( n_rem & 0x8000 ) {
				n_rem = (n_rem << 1) ^ 0x3000;
			} else {
				n_rem = (n_rem << 1);
			}
		}
	}

	n_rem = ((n_rem >> 12) & 0x000F);

	return n_rem ^ 0x00;
}


