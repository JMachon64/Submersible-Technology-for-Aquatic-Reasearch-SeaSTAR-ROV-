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

/* 
 * 
 * 
 * 
 */




/* 
 * 
 * 
 * 
 */

typedef struct {
    int16_t j1x;
    int16_t j1y;
    int16_t j2x;
    int16_t j2y;
    uint8_t trigger;
}control_packet_t;

typedef struct {
    int16_t joystick1x;
    int16_t joystick1y;
    int16_t joystick2x;
    int16_t joystick2y;
    int16_t trigger;

    uint32_t last_update_ms;
    uint32_t seq;
}control_command_t;

/* 
 * 
 * 
 * 
 */

extern volatile control_command_t thruster_command;

void Control_Update_Command(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t trigger, uint32_t seq);

void Set_Servo(uint16_t angle);

#endif