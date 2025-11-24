/*
 * gpio_driver.h
 *
 *  Created on: 24 нояб. 2025 г.
 *      Author: georgijhabner
 */

#ifndef INC_GPIO_DRIVER_H_
#define INC_GPIO_DRIVER_H_

#include <stdbool.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"

/* ---------------------- Button ---------------------- */

#define BUTTON_DEBOUNCE_INTERVAL 10

typedef struct {
    GPIO_TypeDef *GPIOx;
    uint16_t GPIO_Pin;
    uint32_t last_pressed_time;
} Button;

bool button_is_clicked(Button *btn);

/* ---------------------- LED ---------------------- */

typedef struct {
    GPIO_TypeDef *GPIOx;
    uint16_t GPIO_Pin;
} LED;

void led_activate(LED *led);
void led_deactivate(LED *led);
void led_toggle(LED *led);

#endif /* INC_GPIO_DRIVER_H_ */
