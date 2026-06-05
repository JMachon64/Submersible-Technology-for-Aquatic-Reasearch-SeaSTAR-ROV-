/* 
 * File:   PacketIDs.h
 * Author: Jose Machon
 *
 * Created on April 1, 2026, 9:24 AM
 *
 * Contains all the packet identifiers for the UART communication. They are assinged 
 * arbitrary unique hex codes that are the same on the RaspberryPi 4.
 *
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
    
    ///MISSION COMMANDS

    ID_COLLECT_WATER_SAMPLE    = 0x21,
    ID_NAVIGATION_LIGHTS_ON    = 0x67,
    ID_NAVIGATION_LIGHTS_OFF   = 0x68,

    //TELEMETRY 
    ID_ENVIRONMENTAL_TELEMETRY = 0x8,
    ID_POSITIONAL_TELEMETRY    = 0x9,

   //FAILURE DETECTION 
    ID_LEAK_DETECTED           = 0x6,
    ID_PI_FAILURE              = 0x22,


}PacketIDs_t;


#endif  /* PACKETIDS_H */

