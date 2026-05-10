/* 
 * File:   Uart.h
 * Author: Jose Machon
 *
 * Created on April 1, 2026, 9:24 AM
 */

#ifndef UART_H
#define UART_H


#include "main.h"
#include "stm32f3xx_hal_dma.h"
#include "stm32f3xx_hal_uart.h"
#include <stdint.h>
#include <stdio.h>

#define BUFFERSIZE 64

#define DMA_RX_BUF_SIZE 64


typedef struct{
    uint8_t buffer[BUFFERSIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} CircularBuffer_t ;


uint8_t PutChar(char ch);

uint8_t GetChar(uint8_t *ch);

void UART3_INIT_RxDMA(void);

void UART3_Traverse_RxDMA(void);

#endif/* UART.H */