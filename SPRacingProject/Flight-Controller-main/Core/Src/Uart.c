#include "main.h"
#include "stm32f3xx_hal_dma.h"
#include "stm32f3xx_hal_uart.h"
#include "Uart.h"
#include <stdint.h>
#include <stdio.h>

static uint8_t uart3_dma_rx_buf[DMA_RX_BUF_SIZE];

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern DMA_HandleTypeDef hdma_usart3_rx;

static volatile CircularBuffer_t receive = {0};
static volatile CircularBuffer_t transmit = {0};

static uint8_t uart3_tx_byte = 0;

static volatile uint16_t dma_old_pos = 0;
static volatile uint8_t uart3_tx_busy = 0;

static void UART3_RestartRxDMA(void)
{
    HAL_UART_DMAStop(&huart3);

    __HAL_UART_CLEAR_OREFLAG(&huart3);
    __HAL_UART_CLEAR_FEFLAG(&huart3);
    __HAL_UART_CLEAR_NEFLAG(&huart3);
    __HAL_UART_CLEAR_PEFLAG(&huart3);

    receive.head = 0;
    receive.tail = 0;
    dma_old_pos = 0;

    HAL_UART_Receive_DMA(&huart3, uart3_dma_rx_buf, DMA_RX_BUF_SIZE);
}

int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

uint8_t GetChar(uint8_t *ch)
{
    if (receive.head == receive.tail) {
        return 0;
    }

    *ch = receive.buffer[receive.head];
    receive.head = (receive.head + 1) % BUFFERSIZE;
    return 1;
}

uint8_t PutChar(char ch)
{
    uint16_t next = (transmit.tail + 1) % BUFFERSIZE;

    if (next == transmit.head) {
        return 0;
    }

    transmit.buffer[transmit.tail] = (uint8_t)ch;
    transmit.tail = next;

    if (!uart3_tx_busy) {
        uart3_tx_busy = 1;
        uart3_tx_byte = transmit.buffer[transmit.head];
        transmit.head = (transmit.head + 1) % BUFFERSIZE;

        if (HAL_UART_Transmit_IT(&huart3, &uart3_tx_byte, 1) != HAL_OK) {
            uart3_tx_busy = 0;
            return 0;
        }
    }

    return 1;
}

void UART3_INIT_RxDMA(void)
{
    receive.head = 0;
    receive.tail = 0;

    transmit.head = 0;
    transmit.tail = 0;

    uart3_tx_busy = 0;
    uart3_tx_byte = 0;
    dma_old_pos = 0;

    if (HAL_UART_Receive_DMA(&huart3, uart3_dma_rx_buf, DMA_RX_BUF_SIZE) != HAL_OK) {
        Error_Handler();
    }
}

void UART3_Traverse_RxDMA(void)
{
    uint16_t dma_pos = DMA_RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart3_rx);

    while (dma_old_pos != dma_pos) {
        uint16_t next = (receive.tail + 1) % BUFFERSIZE;

        if (next != receive.head) {
            receive.buffer[receive.tail] = uart3_dma_rx_buf[dma_old_pos];
            receive.tail = next;
        }

        dma_old_pos++;
        if (dma_old_pos >= DMA_RX_BUF_SIZE) {
            dma_old_pos = 0;
        }
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3) {
        if (transmit.head != transmit.tail) {
            uart3_tx_byte = transmit.buffer[transmit.head];
            transmit.head = (transmit.head + 1) % BUFFERSIZE;

            if (HAL_UART_Transmit_IT(&huart3, &uart3_tx_byte, 1) != HAL_OK) {
                uart3_tx_busy = 0;
            }
        } else {
            uart3_tx_busy = 0;
        }
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3) {
        UART3_RestartRxDMA();
    }
}