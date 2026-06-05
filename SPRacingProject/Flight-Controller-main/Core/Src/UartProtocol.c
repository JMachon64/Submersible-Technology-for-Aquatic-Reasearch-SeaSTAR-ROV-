/* 
 * File:   UartProtocol.c
 * Author: Jose Machon
 *
 * Created on March 10, 2026, 9:43 AM
 *
 * Most of this code was taken from my experience in embedded system design ECE 121. 
 * The entire UartProtocol was part of a lab but a crc check replaced the standard checksum for 
 * robustness. The packet parser is also customized to suite the application needs of the SeaSTAR.
 * 
 */

#include "UartProtocol.h"

typedef struct {
    uint8_t len;
    uint8_t id;
    uint8_t payload[MAXPAYLOADLENGTH];
} TxQueuedPacket_t;

volatile uint8_t heartbeat_enabled = false;
volatile uint8_t uart_connected    = false;
volatile uint8_t comm_failure      = false;

uint16_t CalculateCyclicalRedundancyCheck(const uint8_t *data, uint16_t length)
 {
    uint16_t crc = 0xFFFF;

    for (uint16_t i = 0; i < length; i++)
    {
        crc ^= ((uint16_t)data[i] << 8);

        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = (crc << 1);
            }
        }
    }
    return crc;
}

uint8_t Protocol_SendPacket(uint8_t len, uint8_t ID, void *Payload)
{
    uint8_t crcBuf[MAXPAYLOADLENGTH + 2];
    uint16_t crc;

    if (len > MAXPAYLOADLENGTH) {
        return 0;
    }
    
    crcBuf[0] = len;
    crcBuf[1] = ID;
    
    for (int i = 0; i < len; i++) {
        crcBuf[2 + i] = ((uint8_t *)Payload)[i];
    }
    crc = CalculateCyclicalRedundancyCheck(crcBuf, len + 2);

    PutChar(HEAD);
    PutChar(len);
    PutChar(ID);

    for (int i = 0; i < len; i++) {
        PutChar(((uint8_t *)Payload)[i]);
    }

    PutChar((uint8_t)(crc & 0xFF));
    PutChar((uint8_t)((crc >> 8) & 0xFF));
    return 1;
}

static Built_Packet_t builtpacket = WAIT_START;
static uint8_t crc_low = 0;
static uint8_t index   = 0;


uint8_t BuildRxPacket(Packet_Sent_t *rxPacket, unsigned char reset)
{
    uint8_t val;
    
    if (reset)
    {
        builtpacket = WAIT_START;
        index   = 0;
        crc_low = 0;
        return 0;
    }
    
    while (GetChar(&val))
    {

        switch (builtpacket)
        {
            case WAIT_START:
                if (val == HEAD)
                {
                    rxPacket->start = val;
                    builtpacket = WAIT_LEN;
                }
                break;

            case WAIT_LEN:
                if (val <= MAXPAYLOADLENGTH)
                {
                    rxPacket->len = val;
                    builtpacket = WAIT_ID;
                }
                else
                {
                    builtpacket = WAIT_START;
                }
                break;

            case WAIT_ID:
                rxPacket->ID = val;
                index = 0;
                builtpacket = (rxPacket->len == 0) ? WAIT_CRC_L : WAIT_PAYLOAD;
                break;

            case WAIT_PAYLOAD:
                rxPacket->payLoad[index++] = val;
                if (index >= rxPacket->len)
                {
                    builtpacket = WAIT_CRC_L;
                }
                break;

            case WAIT_CRC_L:
                crc_low = val;
                builtpacket = WAIT_CRC_H;
                break;

            case WAIT_CRC_H:
            {
                rxPacket->crc = (uint16_t)crc_low | ((uint16_t)val << 8);

                uint8_t crcBuf[MAXPAYLOADLENGTH + 2];
                crcBuf[0] = rxPacket->len;
                crcBuf[1] = rxPacket->ID;

                for (uint8_t i = 0; i < rxPacket->len; i++) {
                    crcBuf[2 + i] = rxPacket->payLoad[i];
                }

                uint16_t computed_crc =
                    CalculateCyclicalRedundancyCheck(crcBuf, rxPacket->len + 2);

                builtpacket = WAIT_START;

                if (computed_crc == rxPacket->crc) {
                    return 1;
                }
                break;
            }

            default:
                builtpacket = WAIT_START;
                break;
        }
    }

    return 0;
}

volatile uint32_t last_heartbeat = false;

uint8_t Protocol_ParsePacket(Packet_Sent_t *packet)
{


    switch (packet->ID)
    {
        //BOOT PACKETS HAS THE HANDSHAKE PROTOCOL 
        case ID_PI_HELLO:
        {
            if (packet->len != 4)
            {
                break;
            }

            uint32_t acknowlegdment_timestamp_ms = HAL_GetTick();


            if(propulsion_initialized && baseline_set && !comm_failure){

                last_heartbeat = HAL_GetTick(); 
                heartbeat_enabled = true;
                comm_failure = false;
                FSM_PostEvent(FSM_EVENT_BOOT_DONE);
                Protocol_SendPacket(4, ID_STM32_HELLO, &acknowlegdment_timestamp_ms);

                break;
            }

            if(comm_failure){

            
                heartbeat_enabled = true;
                comm_failure = false;
                last_heartbeat = HAL_GetTick();
                Protocol_SendPacket(4, ID_STM32_HELLO, &acknowlegdment_timestamp_ms);
                FSM_PostEvent(FSM_EVENT_COMMUNICATION_RECONNECTION_SUCCESS); 
                break;
            }
            break;

        }

        //HEARTBEAT PACKETS 
        case ID_PING:
        {

            // printf("PING %d \n", heartbeat_enabled);

            if(comm_failure ||!heartbeat_enabled ){
                break;
            }

            //printf("PONG\n");
            if (packet->len != 4)
            {
                break;
            }
       
            Protocol_SendPacket(4, ID_PONG, packet->payLoad);
            last_heartbeat = HAL_GetTick();

            break;
        }

         // THRUSTER CONTROL 
        case ID_THRUSTER_INPUT:
        {

            if (packet->len != 10)
            {
                break;
            }

            if(thrusters_disabled){break;}

            // PARSE THE THRUSTER COMMANDS FROM THE GAME CONTROLLER 
            int16_t x1      = (int16_t)(((uint16_t)packet->payLoad[0]) | ((uint16_t)packet->payLoad[1] << 8));
            int16_t y1      = (int16_t)(((uint16_t)packet->payLoad[2]) | ((uint16_t)packet->payLoad[3] << 8));
            int16_t x2      = (int16_t)(((uint16_t)packet->payLoad[4]) | ((uint16_t)packet->payLoad[5] << 8));
            int16_t y2      = (int16_t)(((uint16_t)packet->payLoad[6]) | ((uint16_t)packet->payLoad[7] << 8));
            int16_t trigger = (int16_t)(((uint16_t)packet->payLoad[8]) | ((uint16_t)packet->payLoad[9] << 8));

            //Apply command
            Control_Update_Command(x1, y1, x2, y2, trigger);
            break;
        }

        //STATE MACHINE COMMANDS 

        case ID_START_MISSION:
        {
            Protocol_SendPacket(4, ID_PACKET_ACKNOWLEGDED, packet->payLoad);
            FSM_PostEvent(FSM_EVENT_START_MISSION);
            break;
        }

        case ID_END_MISSION:
        {

            Protocol_SendPacket(4, ID_PACKET_ACKNOWLEGDED, packet->payLoad);
            FSM_PostEvent(FSM_EVENT_END_MISSION);
        
            break;
        }

        case ID_PI_FAILURE:
        {
            if(!heartbeat_enabled){
                return 0;
            }

            pi_healthy = false;
    
            Protocol_SendPacket(4, ID_PACKET_ACKNOWLEGDED, packet->payLoad);
            FSM_PostEvent(FSM_EVENT_PI_FAILURE);
            break; 

        }
        case ID_PI_RECOVERY:
        {
            pi_healthy = 1;

            Protocol_SendPacket(4, ID_PACKET_ACKNOWLEGDED, packet->payLoad);
            FSM_PostEvent(FSM_EVENT_PI_RECOVERY);
            break;

        }

        //SYSTEM SETTING COMMANDS
        //ROV COMMANDS
        case ID_COLLECT_WATER_SAMPLE:
        {

            thrusters_disabled = 1;
            FSM_PostEvent(FSM_EVENT_COLLECT_WATER_SAMPLE);
            Protocol_SendPacket(4, ID_PACKET_ACKNOWLEGDED, packet->payLoad);
            Control_Update_Command(0,0,0,0,0);
            break;
        }

        case ID_NAVIGATION_LIGHTS_ON:
        {
            NavigationLights_On();  
            Protocol_SendPacket(4, ID_PACKET_ACKNOWLEGDED, packet->payLoad);
            break;
        }  

        case ID_NAVIGATION_LIGHTS_OFF:
        {
            NavigationLights_Off();
            Protocol_SendPacket(4, ID_PACKET_ACKNOWLEGDED, packet->payLoad);
            break;
        }  

        default:
        {
            printf("Unknown packet ID: 0x%02X\r\n", packet->ID);
            break;
        }
    }

    return 1;
}


void communication_status(){

    if(!heartbeat_enabled){return;} //Either the stm32 is still booting and unresponsive or in failure.

    uint32_t current_time = HAL_GetTick();

    if(current_time - last_heartbeat >= COMMUNICATION_TIMEOUT){
        FSM_PostEvent(FSM_EVENT_COMMUNICATION_CONNECTION_FAILURE);
        heartbeat_enabled = false;
       }

}