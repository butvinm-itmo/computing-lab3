/*
 * drivers.c
 *
 *  Created on: 10 окт. 2025 г.
 *      Author: georgijhabner
 */

#include "drivers.h"

bool button_is_clicked(Button *btn) {
  uint32_t tick = HAL_GetTick();
  if (HAL_GPIO_ReadPin(btn->GPIOx, btn->GPIO_Pin) == GPIO_PIN_RESET) {
    btn->last_pressed_time = tick;
  } else {
    if (btn->last_pressed_time != 0 &&
        tick - btn->last_pressed_time > BUTTON_DEBOUNCE_INTERVAL) {
      btn->last_pressed_time = 0;
      return true;
    }
  }
  return false;
}

void led_activate(LED *led) {
  HAL_GPIO_WritePin(led->GPIOx, led->GPIO_Pin, GPIO_PIN_SET);
}

void led_deactivate(LED *led) {
  HAL_GPIO_WritePin(led->GPIOx, led->GPIO_Pin, GPIO_PIN_RESET);
}

void led_toggle(LED *led) { HAL_GPIO_TogglePin(led->GPIOx, led->GPIO_Pin); }
