/* 
 * File:   ControlLoop.h
 * Author: Jose Machon
 *
 * Created on April 1, 2026, 9:24 AM
 */

#ifndef CONTROLLOOP_H
#define CONTROLLOOP_H

#include "main.h"
#include <stdint.h>
#include <stdio.h>
#include "UartProtocol.h"
#include "pwm.h"

/* 
 * Below are the structs used to pack the raw controller values
 * followed by the processed and control mixed values. 
 */

typedef struct {
    int16_t j1x;
    int16_t j1y;
    int16_t j2x;
    int16_t j2y;
    uint8_t trigger;
}control_packet_t; // raw control packet values 

typedef struct {
    int16_t joystick1x;
    int16_t joystick1y;
    int16_t joystick2x;
    int16_t joystick2y;
    int16_t trigger;

    uint32_t last_update_ms;
    uint32_t seq;
}control_command_t; // turns into command when processed

/* 
 * 
 * 
 * 
 */

extern volatile control_command_t thruster_command;

extern volatile uint8_t thrusters_disabled; // thruster control flag

/* 
 * This function takes in raw control values from the UART packet parser.
 */  

uint8_t Control_Update_Command(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t trigger);

/* 
 * Helper function for the navigation light toggling.
 */  

void NavigationLights_On(void);

void NavigationLights_Off(void);

#endif