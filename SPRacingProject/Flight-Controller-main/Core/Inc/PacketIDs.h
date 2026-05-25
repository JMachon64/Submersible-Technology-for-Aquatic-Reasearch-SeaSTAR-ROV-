/* 
 * File:   PacketIDs.h
 * Author: Jose Machon
 *
 * Created on April 1, 2026, 9:24 AM
 */

#ifndef PACKETIDS_H
#define PACKETIDS_H

typedef enum{

    //BASIC DEBUG MESSAGES 
    ID_INVALID                 = 0, // DEBUG MESSAGE IDS
    ID_DEBUG                   = 128,
    ID_PACKET_ACKNOWLEGDED     = 0x99,
    ID_WATER_SAMPLE_COMPLETE   = 0x55,

    //HANDSHAKE PROTOCOL
    ID_PI_HELLO                = 0x10,
    ID_STM32_HELLO             = 0x11,

    //HEARTBEAT DEBUG MESSAGES 
    ID_PING                    = 0x01, // HEARTBEAT ECHO IDS
    ID_PONG                    = 0x02,

    //CONTROLLER COMMAND MAPPING 
    ID_THRUSTER_INPUT          = 0x03,

    //SYSTEM MODE COMMANDS 
    ID_START_MISSION           = 0x4,
    ID_END_MISSION             = 0x5,
    ID_PI_RECOVERY             = 0x19,

    //MISSION SETTINGS
    ID_SET_CONTROL_LOOP_GAINS  = 0x18,
    ID_SET_SAMPLING_RATES      = 0x29,
    ID_SET_MAXIMUM_SPEED       = 0x39,
    
    ///MISSION COMMANDS

    ID_COLLECT_WATER_SAMPLE    = 0x21,
    ID_NAVIGATION_LIGHTS_ON    = 0x67,
    ID_NAVIGATION_LIGHTS_OFF   = 0x68,

    //TELEMETRY 
    ID_ENVIRONMENTAL_TELEMETRY = 0x8,
    ID_POSITIONAL_TELEMETRY    = 0x9,
    ID_POWER_STATUS_TELEMETRY  = 0x69,

   //FAILURE DETECTION 
    ID_LEAK_DETECTED           = 0x6,
    ID_CURRENT_SPIKE_DETECTED  = 0x7,
    ID_PI_FAILURE              = 0x22,


}PacketIDs_t;


#endif  /* PACKETIDS_H */

