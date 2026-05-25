#ifndef PWM_H
#define	PWM_H

#include <stdint.h>
#include "main.h"
#include "tim.h"

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

// TIM_HandleTypeDef htim2;
// TIM_HandleTypeDef htim3;
// TIM_HandleTypeDef htim4;

// pwm channel struct with timer and channel attributes + bit mask for book keeping
typedef struct PWM {
    TIM_HandleTypeDef* timer;
    unsigned int channel;
    unsigned char mask;
} PWM;  

// user-level PWM channels
extern const PWM PWM_1;
extern const PWM PWM_2;
extern const PWM PWM_3;
extern const PWM PWM_4;
extern const PWM PWM_5;
extern const PWM PWM_6;
extern const PWM PWM_7;
extern const PWM PWM_8;

char PWM_Init(void);


// char PWM_SetFrequency(unsigned int NewFrequency);


// unsigned int PWM_GetFrequency(void);


// char PWM_AddPin(PWM PWM_x);


char PWM_SetDutyCycle(PWM PWM_x, unsigned int Duty);

char PWM_SetPeriod(PWM PWM_x, unsigned int Period);

uint16_t PWM_GetPeriod(PWM PWM_x);

char PWM_SetThrusterPeriods(uint16_t p1, uint16_t p2, uint16_t p3, uint16_t p4, uint16_t p5);

char PWM_SetServoPeriod(uint16_t p6);

char PWM_SampleClosedPosition();

char PWM_SampleOpenPosition();

// char PWM_Start(PWM PWM_x);


// char PWM_Stop(PWM PWM_x);


// char PWM_End(void);


#endif
