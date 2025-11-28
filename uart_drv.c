#include "uart_drv.h"
#include <string.h>
#include <stdbool.h>
#include "play_queue.h"
#include <ctype.h>


#define UART_RX_BUF_SZ 128
#define UART_TX_BUF_SZ 128

static UART_HandleTypeDef *hUart = NULL;

/* RX ring buffer (used when IRQ mode active) */
static volatile uint8_t rx_buf[UART_RX_BUF_SZ];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;

/* TX ring buffer for non-blocking transmit via IT */
static volatile uint8_t tx_buf[UART_TX_BUF_SZ];
static volatile uint16_t tx_head = 0;
static volatile uint16_t tx_tail = 0;
static volatile uint8_t tx_busy = 0;

/* one-byte HAL rx buffer for Rx IT */
static uint8_t hal_rx_byte = 0;

/* state: IRQ enabled? */
static volatile bool irq_enabled = false;

/* флаг: запрос переключения от '+' (обрабатывает main) */
volatile uint8_t irq_toggle_request = 0;

/* Forward */
static void start_rx_it(void);
static void start_tx_from_buffer(void);

void UART_Drv_Init(UART_HandleTypeDef *huart) {
    hUart = huart;
    rx_head = rx_tail = 0;
    tx_head = tx_tail = 0;
    tx_busy = 0;
    irq_enabled = false;
}

/* Enable/disable NVIC/IT reception mode.
   If enabling — start HAL_UART_Receive_IT.
   If disabling — abort receive IT (so poll mode can read DR directly).
*/
void UART_Drv_EnableIRQ(bool on) {
    if (hUart == NULL) return;
    if (on && !irq_enabled) {
        /* enable NVIC for UART if needed */
    	IRQn_Type irqn;
    	if      (hUart->Instance == USART1) irqn = USART1_IRQn;
    	else if (hUart->Instance == USART2) irqn = USART2_IRQn;
    	else if (hUart->Instance == USART3) irqn = USART3_IRQn;
    	#ifdef UART4
    	else if (hUart->Instance == UART4)  irqn = UART4_IRQn;
    	#endif
    	#ifdef UART5
    	else if (hUart->Instance == UART5)  irqn = UART5_IRQn;
    	#endif
    	#ifdef USART6
    	else if (hUart->Instance == USART6) irqn = USART6_IRQn;
    	#endif
    	else return;

    	HAL_NVIC_SetPriority(irqn, 0, 0);
    	HAL_NVIC_EnableIRQ(irqn);
        irq_enabled = true;
        /* start single-byte receive IT */
        start_rx_it();
    } else if (!on && irq_enabled) {
        /* stop IT based reception */
        HAL_UART_AbortReceive_IT(hUart);
        irq_enabled = false;
        /* Optionally clear RX ring buffer? keep content */
    }
}

static void start_rx_it(void) {
    if (hUart != NULL) {
        HAL_UART_Receive_IT(hUart, &hal_rx_byte, 1);
    }
}

/* HAL callback entry points: we implement HAL callback handlers here.
   HAL will call these weak symbols from driver. */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart != hUart) return;

    uint8_t ch = hal_rx_byte;

    if (ch == '+') {
        irq_toggle_request = 1; // main проверит и переключит режим
    } else {
        // нормализуем букву в верхний регистр
        if (ch >= 'a' && ch <= 'z') ch = (uint8_t)(ch - 'a' + 'A');

        // ВЫВОД ЭХА В КОНСОЛЬ
        char echo_msg[32];
        snprintf(echo_msg, sizeof(echo_msg), " Echo from console:%c\r\n", ch);
        UART_Drv_SendString(echo_msg);

        // если это латинская буква — попытаемся положить в очередь воспроизведения
        if ((ch >= 'A' && ch <= 'Z')) {
            (void) play_queue_enqueue_from_isr((char)ch);
        }
    }

    /* restart receive if still in irq-enabled mode */
    if (irq_enabled) start_rx_it();
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart != hUart) return;
    tx_busy = 0;
    /* if more bytes in buffer — start next */
    if (tx_tail != tx_head) start_tx_from_buffer();
}

/* Non-blocking: try to get a byte.
   Behavior:
     - if IRQ enabled: read from rx ring buffer (filled by IRQ handler)
     - else: check RXNE flag and read DR directly (poll)
*/
bool UART_Drv_TryGetByte(uint8_t *b) {
    if (hUart == NULL) return false;
    if (irq_enabled) {
        if (rx_tail == rx_head) return false;
        *b = rx_buf[rx_tail];
        rx_tail = (rx_tail + 1) % UART_RX_BUF_SZ;
        return true;
    } else {
        /* polling: check RXNE */
        if (__HAL_UART_GET_FLAG(hUart, UART_FLAG_RXNE)) {
            uint8_t v = (uint8_t)(hUart->Instance->DR & 0xFF);
            *b = v;
            return true;
        }
        return false;
    }
}

/* TX: push to tx_buf and start IT if idle */
bool UART_Drv_SendByteNonBlocking(uint8_t b) {
    uint16_t next = (tx_head + 1) % UART_TX_BUF_SZ;
    if (next == tx_tail) return false; // full
    tx_buf[tx_head] = b;
    tx_head = next;
    if (!tx_busy) start_tx_from_buffer();
    return true;
}

static void start_tx_from_buffer(void) {
    if (tx_tail == tx_head) return;
    if (hUart == NULL) return;
    uint8_t c = tx_buf[tx_tail];
    tx_tail = (tx_tail + 1) % UART_TX_BUF_SZ;
    tx_busy = 1;
    HAL_UART_Transmit_IT(hUart, &c, 1);
}

/* Convenient string send (non-blocking, returns when buffer full or success) */
void UART_Drv_SendString(const char *s) {
    while (*s) {
        if (!UART_Drv_SendByteNonBlocking((uint8_t)*s)) {
            /* if buffer full, fallback to blocking transmit to avoid losing messages */
            HAL_UART_Transmit(hUart, (uint8_t*)s, 1, 50);
            s++;
        } else {
            s++;
        }
    }
}
