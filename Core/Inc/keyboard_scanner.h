#ifndef __KEYBOARD_SCANNER_H__
#define __KEYBOARD_SCANNER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

#define KB_I2C_ADDRESS          0xE2
#define KB_I2C_READ_ADDRESS     ((KB_I2C_ADDRESS) | 1)
#define KB_I2C_WRITE_ADDRESS    ((KB_I2C_ADDRESS) & ~1)
#define KB_INPUT_REG            0x0
#define KB_OUTPUT_REG           0x1
#define KB_CONFIG_REG           0x3
#define KB_DEBOUNCE_TIME_MS     50

#define SIDE_BUTTON_PORT        GPIOC
#define SIDE_BUTTON_PIN         GPIO_PIN_15
#define SIDE_BUTTON_DEBOUNCE_MS 100

typedef enum {
    BUTTON_NOT_PRESSED,
    BUTTON_IN_PROGRESS,
    BUTTON_PRESSED
} button_state_t;

void keyboard_scanner_init(void);
int keyboard_scanner_get_button(void);
button_state_t keyboard_scanner_check_side_button(void);

#ifdef __cplusplus
}
#endif

#endif /* __KEYBOARD_SCANNER_H__ */
