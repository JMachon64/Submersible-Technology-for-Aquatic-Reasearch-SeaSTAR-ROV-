
#include "stm32f3xx.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <tim.h>
#include <pwm.h>

// Boolean defines for TRUE, FALSE, SUCCESS and ERROR
#ifndef FALSE
#define FALSE ((int8_t) 0)
#define TRUE ((int8_t) 1)
#endif
#ifndef ERROR
#define ERROR ((int8_t) -1)
#define SUCCESS ((int8_t) 1)
#endif

#define NUM_CHANNELS 8 // number of pwm possible channels

 #define SERVO_CLOSE 1200
 #define SERVO_OPEN 1800

// User-level PWM channels for inits/updating duty cycle etc...
const PWM PWM_1 = {&htim3, TIM_CHANNEL_1, 0x1};
const PWM PWM_2 = {&htim3, TIM_CHANNEL_2, 0x2};
const PWM PWM_3 = {&htim4, TIM_CHANNEL_1, 0x4};
const PWM PWM_4 = {&htim4, TIM_CHANNEL_2, 0x8};
const PWM PWM_5 = {&htim4, TIM_CHANNEL_3, 0x10};
const PWM PWM_6 = {&htim4, TIM_CHANNEL_4, 0x20};
const PWM PWM_7 = {&htim2, TIM_CHANNEL_3, 0x40};
const PWM PWM_8 = {&htim2, TIM_CHANNEL_4, 0x80};


static unsigned int pwm_freq = 50; 
static uint32_t duty_cycles[NUM_CHANNELS]; // to store the duty cycles of each channel
static uint8_t init_status = FALSE;
static unsigned char pinsAdded = 0x00;


char PWM_Init(void) {
    if (init_status == FALSE) { // if PWM module has not been inited
        init_status = TRUE;
        HAL_TIM_PWM_Start(PWM_1.timer, PWM_1.channel);
        HAL_TIM_PWM_Start(PWM_2.timer, PWM_2.channel);
        HAL_TIM_PWM_Start(PWM_3.timer, PWM_3.channel);
        HAL_TIM_PWM_Start(PWM_4.timer, PWM_4.channel);
        HAL_TIM_PWM_Start(PWM_5.timer, PWM_5.channel);
        HAL_TIM_PWM_Start(PWM_6.timer, PWM_6.channel);
        HAL_TIM_PWM_Start(PWM_7.timer, PWM_7.channel);
        HAL_TIM_PWM_Start(PWM_8.timer, PWM_8.channel);
    }
    return SUCCESS;
}




char PWM_SetDutyCycle(PWM PWM_x, unsigned int Duty) {
    // if (init_status == FALSE) { // if pwm module has not been initialized
    //     printf("ERROR: PWM module has not yet been initialized!\r\n");
    //     return ERROR;
    // }
    // if ((pinsAdded & PWM_x.mask) == 0) { // if pin has not been added, add pin
    //     PWM_AddPin(PWM_x);
    // }
    if ((Duty < 0) || (Duty > 100)) { // if requested duty cycle is out of bounds
        printf("ERROR: pwm duty cycle must be between 0 and 100\r\n");
        return ERROR;
    }
    
    switch(PWM_x.mask) { // set capture compare register (CCR) to correct value and save duty cycle value
        case 0x1: // PWM_1
            TIM3->CCR1 = (uint32_t)((Duty/100.0)*(TIM3->ARR));
            // duty_cycles[0] = Duty;
            break;
        case 0x2: // PWM_2
            TIM3->CCR2 = (uint32_t)((Duty/100.0)*(TIM3->ARR));
            // duty_cycles[1] = Duty;
            break;
        case 0x4: // PWM_3
            TIM4->CCR1 = (uint32_t)((Duty/100.0)*(TIM4->ARR));
            // duty_cycles[2] = Duty;
            break;
        case 0x8: // PWM_4
            TIM4->CCR2 = (uint32_t)((Duty/100.0)*(TIM4->ARR));
            // duty_cycles[3] = Duty;
            break;
        case 0x10: // PWM_5
            TIM4->CCR3 = (uint32_t)((Duty/100.0)*(TIM4->ARR));
            // duty_cycles[4] = Duty;
            break;
        case 0x20: // PWM_6
            TIM4->CCR4 = (uint32_t)((Duty/100.0)*(TIM4->ARR));
            // duty_cycles[5] = Duty;
            break;
        case 0x40: // PWM_7
            TIM2->CCR3 = (uint32_t)((Duty/100.0)*(TIM2->ARR));
            duty_cycles[4] = Duty;
            break;
        case 0x80: // PWM_8
            TIM2->CCR4 = (uint32_t)((Duty/100.0)*(TIM2->ARR));
            // duty_cycles[5] = Duty;
            break;
    }

    return SUCCESS;
}




char PWM_SetThrusterPeriods(uint16_t p1, uint16_t p2, uint16_t p3, uint16_t p4, uint16_t p5) { 
    if ((p1<1000 || p1>2000) || (p2<1000 || p2>2000) || (p3<1000 || p3>2000) || (p4<1000 || p4>2000) || (p5<1000 || p5>2000)) { 
        printf("period out of range\r\n");
        return ERROR;
    }
    TIM3->CCR1 = (uint32_t)(2*p1);
    TIM3->CCR2 = (uint32_t)(2*p2);
    TIM4->CCR1 = (uint32_t)(2*p3);
    TIM4->CCR2 = (uint32_t)(2*p4);
    TIM4->CCR3 = (uint32_t)(2*p5);

    return SUCCESS;
}

char PWM_SampleClose(){
    TIM4->CCR4 = (uint32_t)(2*SERVO_CLOSE);
    return SUCCESS;
}

char PWM_SampleOpen(){
    TIM4->CCR4 = (uint32_t)(2*SERVO_OPEN);
    return SUCCESS;
}
char PWM_SetPeriod(PWM PWM_x, unsigned int Period) {

    if ((Period < 1000) || (Period > 2000)) { // if requested duty cycle is out of bounds
        printf("ERROR: Period must be between 1000 and 2000 us\r\n");
        return ERROR;
    }
    
    switch(PWM_x.mask) { 
        case 0x1: // PWM_1
            TIM3->CCR1 = (uint32_t)(2*Period);
            // duty_cycles[0] = Duty;
            break;
        case 0x2: // PWM_2
            TIM3->CCR2 = (uint32_t)(2*Period);
            // duty_cycles[1] = Duty;
            break;
        case 0x4: // PWM_3
            TIM4->CCR1 = (uint32_t)(2*Period);
            // duty_cycles[2] = Duty;
            break;
        case 0x8: // PWM_4
            TIM4->CCR2 = (uint32_t)(2*Period);
            // duty_cycles[3] = Duty;
            break;
        case 0x10: // PWM_5
            TIM4->CCR3 = (uint32_t)(2*Period);
            // duty_cycles[4] = Duty;
            break;
        case 0x20: // PWM_6
            TIM4->CCR4 = (uint32_t)(2*Period);
            // duty_cycles[5] = Duty;
            break;
        case 0x40: // PWM_7
            TIM2->CCR3 = (uint32_t)(2*Period);
            // duty_cycles[4] = Duty;
            break;
        case 0x80: // PWM_8
            TIM2->CCR4 = (uint32_t)(2*Period);
            // duty_cycles[5] = Duty;
            break;
    }

    return SUCCESS;
}


// char PWM_Start(PWM PWM_x) {
//     if (init_status == FALSE) { // if pwm module has not been initialized
//         printf("ERROR: PWM module has not yet been initialized!\r\n");
//         return ERROR;
//     }
//     if ((pinsAdded & PWM_x.mask) == 0) { // if pin has not been added
//         printf("ERROR: PWM pin has not been added!\r\n");
//         return ERROR;
//     }
//     // start pwm channel
//     HAL_TIM_PWM_Start(PWM_x.timer, PWM_x.channel);
//     return SUCCESS;
// }

// char PWM_Stop(PWM PWM_x) {
//     if (init_status == FALSE) { // if pwm module has not been initialized
//         printf("ERROR: PWM module has not yet been initialized!\r\n");
//         return ERROR;
//     }
//     if ((pinsAdded & PWM_x.mask) == 0) { // if pin has not been added
//         printf("ERROR: PWM pin has not been added!\r\n");
//         return ERROR;
//     }
//     // start pwm channel
//     HAL_TIM_PWM_Stop(PWM_x.timer, PWM_x.channel);
//     return SUCCESS;
// }


// char PWM_End(void) {
//     if (init_status == FALSE) { // if pwm module has not been initialized
//         printf("ERROR: PWM module has not yet been initialized!\r\n");
//         return ERROR;
//     }
//     // stop all pwm channels
//     HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
//     HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
//     HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
//     HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);
//     HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);
//     HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_3);

//     //deinitialize timer peripherals
//     HAL_TIM_PWM_DeInit(&htim1);
//     HAL_TIM_PWM_DeInit(&htim4);
//     HAL_TIM_Base_DeInit(&htim1);
//     HAL_TIM_Base_DeInit(&htim4);
//     return SUCCESS;
// }
