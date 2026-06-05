/* 
 * File:   ControlLoop.c
 * Author: Jose Machon
 *
 * Created on April 1, 2026, 9:24 AM
 *
 * The purpose of this script is to take in the incoming 
 * values from the xbox controller through the UART parser
 * and store them into a struct for the control mixing
 * done by the propulsion subsystem code.
 * 
 * Navigation lights are also part of the control loop 
 * so small helper functions are included here to toggle them.
 *
 */
 

#include "ControlLoop.h"

volatile control_command_t thruster_command = {0};

// control mixing array 
uint16_t joystick_periods[5];

//start off disabling thrusters until mission is active 
volatile uint8_t thrusters_disabled = 1;


/* 
 * This functions continously updates the controller input recieved from the pi
 * the packet parser calls this function and places each input into a struct for 
 * propulsion control mixing. Returns 0 if thrusters are disabled.
 * 
 */

uint8_t Control_Update_Command(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t trigger)
{
 
    if(thrusters_disabled){return 0;} // thrusters disabled case.

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


/* 
 * Helper functions that toggle the lights on or off.
 * Calls the gpio pin the light circuit is connected to.
 */

void NavigationLights_On(void){HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);}

void NavigationLights_Off(void){HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);}