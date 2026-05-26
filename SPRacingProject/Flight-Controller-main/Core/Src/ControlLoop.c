/* 
 * File:   UartProtocol.h
 * Author: Jose Machon
 *
 * Created on April 1, 2026, 9:24 AM
 */
 
#include "ControlLoop.h"

volatile control_command_t thruster_command = {0};

uint16_t joystick_periods[5];

volatile uint8_t thrusters_disabled = 1;

uint8_t Control_Update_Command(int16_t x1, int16_t y1, int16_t x2, int16_t y2, 
    int16_t trigger)
{
 
    if(thrusters_disabled){return 0;}

    thruster_command.joystick1x = x1;
    thruster_command.joystick1y = y1;
    thruster_command.joystick2x = x2;
    thruster_command.joystick2y = y2;
    thruster_command.trigger = trigger;


    joystick_periods[0] = (x1/2.0)+1500;
    joystick_periods[1] = (y1/2.0)+1500;
    joystick_periods[2] = (x2/2.0)+1500;
    joystick_periods[3] = (y2/2.0)+1500;
    joystick_periods[4] = (trigger/2.0)+1500;
 
    return 1;
}


void NavigationLights_On(void){HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);}

void NavigationLights_Off(void){HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);}