

#include "UartProtocol.h"

#define TX_QUEUE_SIZE 24

typedef struct {
    uint8_t len;
    uint8_t id;
    uint8_t payload[MAXPAYLOADLENGTH];
} TxQueuedPacket_t;

static TxQueuedPacket_t txQueue[TX_QUEUE_SIZE];
static volatile uint8_t txHead = 0;
static volatile uint8_t txTail = 0;




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

uint8_t Protocol_SendDebugMessage(char *Message) {
    uint8_t len = 0;
    while (len < MAXPAYLOADLENGTH && Message[len] != '\0') {len++;}
    if (len >= MAXPAYLOADLENGTH) {return 0;}
    return Protocol_SendPacket(len, ID_DEBUG, Message);

}

static Built_Packet_t builtpacket = WAIT_START;
static uint8_t crc_low = 0;
static uint8_t index = 0;


volatile uint32_t Packet_Bytes = 0;
volatile uint32_t Control_Packets = 0;
volatile uint32_t Ping_Packets = 0;

void Protocol_UpdateThroughput(void)
{
    static uint32_t last_time = 0;
    uint32_t now = HAL_GetTick();

    if ((now - last_time) >= 1000)
    {
        uint32_t bytes = Packet_Bytes;
        uint32_t ctrl = Control_Packets;
        uint32_t ping = Ping_Packets;
        uint32_t throughput = (bytes * 100) / BAUDRATEBYTES;

        //printf("Bytes/sec: %lu, CTRL/sec: %lu, PING/sec: %lu, Throughput: %lu%%\r\n", bytes, ctrl, ping, throughput);

        Packet_Bytes = 0;
        Control_Packets = 0;
        Ping_Packets = 0;
        last_time = now;

    }
}

uint8_t BuildRxPacket(Packet_Sent_t *rxPacket, unsigned char reset)
{
    uint8_t val;
    
    if (reset)
    {
        builtpacket = WAIT_START;
        index = 0;
        crc_low = 0;
        return 0;
    }
    
    while (GetChar(&val))
    {

        Packet_Bytes++;

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

uint8_t Protocol_ParsePacket(Packet_Sent_t *packet)
{
    switch (packet->ID)
    {

        //BOOT PACKETS
        case ID_PI_HELLO:
        {
            uint32_t acknowlegdment_timestamp_ms = HAL_GetTick();
            printf("HELLO...\n\n");
            Protocol_SendPacket(4, ID_STM32_HELLO, &acknowlegdment_timestamp_ms);
            break;
        }

        // HEARTBEAT PACKETS 
        case ID_PING:
        {
            Ping_Packets++;

            if (packet->len != 4)
            {
                printf("Bad ping packet length: %u\r\n", packet->len);
                break;
            }

            Protocol_SendPacket(4, ID_PONG, packet->payLoad);
            break;
        }

        // THRUSTER CONTROL 
    case ID_THRUSTER_INPUT:
    {
        Control_Packets++;

        if (packet->len != 10)
        {
            printf("INVALID thruster control packet length: %u\r\n", packet->len);
            break;
        }

        if(leakLatched){

            //printf("Blocking control packets due to a leak...\n");
            break;

        }

        int16_t x1 = (int16_t)(((uint16_t)packet->payLoad[0]) | ((uint16_t)packet->payLoad[1] << 8));
        int16_t y1 = (int16_t)(((uint16_t)packet->payLoad[2]) | ((uint16_t)packet->payLoad[3] << 8));
        int16_t x2 = (int16_t)(((uint16_t)packet->payLoad[4]) | ((uint16_t)packet->payLoad[5] << 8));
        int16_t y2 = (int16_t)(((uint16_t)packet->payLoad[6]) | ((uint16_t)packet->payLoad[7] << 8));
        int16_t trigger = (int16_t)(((uint16_t)packet->payLoad[8]) | ((uint16_t)packet->payLoad[9] << 8));


        uint32_t seq = 0;

        // Apply command
        Control_Update_Command(x1, y1, x2, y2, trigger, seq);
        break;
    }

        //STATE MACHINE COMMANDS 

        case ID_START_MISSION:
        {

            Protocol_SendPacket(4, ID_PACKET_ACKNOWLEGDED, packet->payLoad);
            printf("START MISSION\n");
            FSM_PostEvent(FSM_EVENT_START_MISSION);
            break;
        }

        case ID_END_MISSION:
        {

            printf("END MISSION\n");

            Protocol_SendPacket(4, ID_PACKET_ACKNOWLEGDED, packet->payLoad);
            FSM_PostEvent(FSM_EVENT_END_MISSION);
        
            break;
        }

        // SYSTEM SETTING COMMANDS
        //IMPLEMENT THESE LATER 

        case ID_COLLECT_WATER_SAMPLE:
        {

            FSM_PostEvent(FSM_EVENT_COLLECT_WATER_SAMPLE);
            
            printf("COLLECTING WATER SAMPLE...\n");
            
            Protocol_SendPacket(4, ID_PACKET_ACKNOWLEGDED, packet->payLoad);

            break;
        }
        case ID_NAVIGATION_LIGHTS_ON:
        {
            
            printf("LIGHTS ON...\n");
            NavigationLights_On();            
            break;
        }  
        case ID_NAVIGATION_LIGHTS_OFF:
        {
            printf("LIGHTS OFF...\n");
            NavigationLights_Off();
            
            break;
        }  
        default:
        {
            printf("Unknown packet ID: 0x%02X\r\n", packet->ID);
            return 0;
            break;
        }
    }

    return 1;
}
