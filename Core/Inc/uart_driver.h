/*
 * uart_driver.h
 *
 *  Created on: 24 нояб. 2025 г.
 *      Author: georgijhabner
 *
 * Two UART driver modes:
 * 1. Interrupt mode (IT) - non-blocking with ring buffer
 * 2. Polling mode - non-blocking RX, no interrupts
 */

#ifndef INC_UART_DRIVER_H_
#define INC_UART_DRIVER_H_

#include <stdbool.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"

#define UART_RX_BUF_SIZE 64
#define UART_TX_BUF_SIZE 64

typedef struct {
    UART_HandleTypeDef *huart;

    /* Ring buffers for IT mode */
    volatile uint8_t rx_buf[UART_RX_BUF_SIZE];
    volatile uint16_t rx_head;
    volatile uint16_t rx_tail;

    volatile uint8_t tx_buf[UART_TX_BUF_SIZE];
    volatile uint16_t tx_head;
    volatile uint16_t tx_tail;
    volatile uint8_t tx_busy;

    /* Single byte buffer for HAL IT receive */
    uint8_t hal_rx_byte;

    /* Mode flag */
    volatile bool irq_enabled;
} UART;

/* ==================== Common ==================== */

/* Initialize UART driver instance */
void uart_init(UART *uart, UART_HandleTypeDef *huart);

/* Enable/disable interrupt mode. When disabled, polling mode is active */
void uart_set_irq_mode(UART *uart, bool enable);

/* Check if IRQ mode is enabled */
bool uart_is_irq_mode(UART *uart);

/* ==================== Interrupt Mode (non-blocking, buffered) ==================== */

/* Try to get a byte from RX buffer (non-blocking, returns false if empty) */
bool uart_it_try_get_byte(UART *uart, uint8_t *byte);

/* Put a byte into TX buffer (non-blocking, returns false if buffer full) */
bool uart_it_send_byte(UART *uart, uint8_t byte);

/* Send string via TX buffer (non-blocking, may truncate if buffer full) */
void uart_it_send_string(UART *uart, const char *str);

/* Check how many bytes available in RX buffer */
uint16_t uart_it_rx_available(UART *uart);

/* Check how many bytes free in TX buffer */
uint16_t uart_it_tx_free(UART *uart);

/* Callbacks - call from HAL_UART_RxCpltCallback / HAL_UART_TxCpltCallback */
void uart_rx_complete_callback(UART *uart);
void uart_tx_complete_callback(UART *uart);

/* ==================== Polling Mode (no interrupts) ==================== */

/* Try to get a byte by polling RXNE flag (non-blocking, returns false if no data) */
bool uart_poll_try_get_byte(UART *uart, uint8_t *byte);

/* Send a byte by polling TXE flag (blocking until sent) */
void uart_poll_send_byte(UART *uart, uint8_t byte);

/* Send string in polling mode (blocking) */
void uart_poll_send_string(UART *uart, const char *str);

#endif /* INC_UART_DRIVER_H_ */
