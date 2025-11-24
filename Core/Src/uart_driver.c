/*
 * uart_driver.c
 *
 *  Created on: 24 нояб. 2025 г.
 *      Author: georgijhabner
 *
 * Two UART driver modes:
 * 1. Interrupt mode (IT) - non-blocking with ring buffer
 * 2. Polling mode - non-blocking RX, no interrupts
 */

#include "uart_driver.h"
#include <string.h>

/* ==================== Internal helpers ==================== */

static void start_rx_it(UART *uart) {
    HAL_UART_Receive_IT(uart->huart, &uart->hal_rx_byte, 1);
}

static void start_tx_from_buffer(UART *uart) {
    if (uart->tx_tail == uart->tx_head) {
        return; /* buffer empty */
    }
    uint8_t byte = uart->tx_buf[uart->tx_tail];
    uart->tx_tail = (uart->tx_tail + 1) % UART_TX_BUF_SIZE;
    uart->tx_busy = 1;
    HAL_UART_Transmit_IT(uart->huart, &byte, 1);
}

/* ==================== Common ==================== */

void uart_init(UART *uart, UART_HandleTypeDef *huart) {
    uart->huart = huart;
    uart->rx_head = 0;
    uart->rx_tail = 0;
    uart->tx_head = 0;
    uart->tx_tail = 0;
    uart->tx_busy = 0;
    uart->hal_rx_byte = 0;
    uart->irq_enabled = false;
}

void uart_set_irq_mode(UART *uart, bool enable) {
    if (uart->huart == NULL) return;

    if (enable && !uart->irq_enabled) {
        /* Enable IRQ mode */
        HAL_NVIC_SetPriority(USART6_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART6_IRQn);
        uart->irq_enabled = true;

        /* Start receiving */
        start_rx_it(uart);

    } else if (!enable && uart->irq_enabled) {
        /* Disable IRQ mode */
        HAL_UART_AbortReceive_IT(uart->huart);
        HAL_UART_AbortTransmit_IT(uart->huart);
        HAL_NVIC_DisableIRQ(USART6_IRQn);
        uart->irq_enabled = false;
        uart->tx_busy = 0;
    }
}

bool uart_is_irq_mode(UART *uart) {
    return uart->irq_enabled;
}

/* ==================== Interrupt Mode (non-blocking, buffered) ==================== */

bool uart_it_try_get_byte(UART *uart, uint8_t *byte) {
    if (uart->rx_tail == uart->rx_head) {
        return false; /* buffer empty */
    }
    *byte = uart->rx_buf[uart->rx_tail];
    uart->rx_tail = (uart->rx_tail + 1) % UART_RX_BUF_SIZE;
    return true;
}

bool uart_it_send_byte(UART *uart, uint8_t byte) {
    uint16_t next_head = (uart->tx_head + 1) % UART_TX_BUF_SIZE;
    if (next_head == uart->tx_tail) {
        return false; /* buffer full */
    }
    uart->tx_buf[uart->tx_head] = byte;
    uart->tx_head = next_head;

    /* Start transmission if not already busy */
    if (!uart->tx_busy) {
        start_tx_from_buffer(uart);
    }
    return true;
}

void uart_it_send_string(UART *uart, const char *str) {
    while (*str) {
        if (!uart_it_send_byte(uart, (uint8_t)*str)) {
            break; /* buffer full, stop */
        }
        str++;
    }
}

uint16_t uart_it_rx_available(UART *uart) {
    if (uart->rx_head >= uart->rx_tail) {
        return uart->rx_head - uart->rx_tail;
    }
    return UART_RX_BUF_SIZE - uart->rx_tail + uart->rx_head;
}

uint16_t uart_it_tx_free(UART *uart) {
    if (uart->tx_head >= uart->tx_tail) {
        return UART_TX_BUF_SIZE - 1 - (uart->tx_head - uart->tx_tail);
    }
    return uart->tx_tail - uart->tx_head - 1;
}

/* HAL Callbacks */

void uart_rx_complete_callback(UART *uart) {
    /* Put received byte into ring buffer */
    uint16_t next_head = (uart->rx_head + 1) % UART_RX_BUF_SIZE;
    if (next_head != uart->rx_tail) {
        /* Buffer not full */
        uart->rx_buf[uart->rx_head] = uart->hal_rx_byte;
        uart->rx_head = next_head;
    }
    /* else: buffer full, byte is lost */

    /* Restart receive if still in IRQ mode */
    if (uart->irq_enabled) {
        start_rx_it(uart);
    }
}

void uart_tx_complete_callback(UART *uart) {
    uart->tx_busy = 0;
    /* Continue transmitting if more data in buffer */
    if (uart->tx_tail != uart->tx_head) {
        start_tx_from_buffer(uart);
    }
}

/* ==================== Polling Mode (no interrupts) ==================== */

bool uart_poll_try_get_byte(UART *uart, uint8_t *byte) {
    /* Check RXNE flag directly */
    if (__HAL_UART_GET_FLAG(uart->huart, UART_FLAG_RXNE)) {
        *byte = (uint8_t)(uart->huart->Instance->DR & 0xFF);
        return true;
    }
    return false;
}

void uart_poll_send_byte(UART *uart, uint8_t byte) {
    /* Wait for TXE flag (blocking) */
    while (!__HAL_UART_GET_FLAG(uart->huart, UART_FLAG_TXE));
    uart->huart->Instance->DR = byte;
}

void uart_poll_send_string(UART *uart, const char *str) {
    while (*str) {
        uart_poll_send_byte(uart, (uint8_t)*str);
        str++;
    }
    /* Wait for TC (transmit complete) */
    while (!__HAL_UART_GET_FLAG(uart->huart, UART_FLAG_TC));
}
