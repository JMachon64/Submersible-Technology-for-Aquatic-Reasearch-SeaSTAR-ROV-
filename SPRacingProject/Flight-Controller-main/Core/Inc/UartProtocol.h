/* 
 * File:   UartProtocol.h
 * Author: Jose Machon
 *
 * Created on April 1, 2026, 9:24 AM
 */

#ifndef UARTPROTOCOL_H
#define UARTPROTOCOL_H

#define HEAD 204

#define PACKETBUFFERSIZE 64
#define MAXPAYLOADLENGTH 64

#define BAUDRATEBYTES 11520

typedef enum {
    WAIT_START,
    WAIT_LEN,
    WAIT_ID,
    WAIT_PAYLOAD,
    WAIT_CRC_L,
    WAIT_CRC_H,
    COMPLETE
} Built_Packet_t;

typedef struct {
    uint8_t start;
    uint8_t ID;
    uint8_t len;
    uint16_t crc;
    unsigned char payLoad[MAXPAYLOADLENGTH];
} Packet_Sent_t;

uint16_t CalculateCyclicalRedundancyCheck(const uint8_t *data, uint16_t length);

uint8_t Protocol_SendDebugMessage(char *Message);

unsigned char CyclicalRedundancyCheck(unsigned char charIn, unsigned char curChecksum);

uint8_t Protocol_SendDebugMessage(char *Message);

uint8_t Protocol_ParsePacket(Packet_Sent_t *packet);

uint8_t BuildRxPacket(Packet_Sent_t *rxPacket, unsigned char reset);

uint8_t CalculateThroughPut(void);

uint8_t Protocol_SendPacket(uint8_t len, uint8_t ID, void *Payload);

void Protocol_UpdateThroughput(void);
void Protocol_ProcessTxQueue(void);
uint8_t Protocol_QueuePacket(uint8_t len, uint8_t ID, void *Payload);

#endif