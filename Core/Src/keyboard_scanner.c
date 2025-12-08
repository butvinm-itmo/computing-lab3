#include "keyboard_scanner.h"
#include "i2c.h"

static uint32_t last_pressing_time = 0;
static int last_pressed_btn_index = -1;
static int row_counts[4] = {0, 0, 0, 0};
static int col_counts[3] = {0, 0, 0};

void keyboard_scanner_init(void) {
    last_pressing_time = 0;
    last_pressed_btn_index = -1;
}

int keyboard_scanner_get_button(void) {
    const uint32_t current_time = HAL_GetTick();

    if (current_time - last_pressing_time < KB_DEBOUNCE_TIME_MS) {
        return -1;
    }

    int index = -1;
    uint8_t reg_buffer = ~0;
    uint8_t tmp = 0;
    int pressed_count = 0;

    HAL_I2C_Mem_Write(&hi2c1, KB_I2C_WRITE_ADDRESS, KB_OUTPUT_REG, 1, &tmp, 1, KB_DEBOUNCE_TIME_MS);

    for (int row = 0; row < 4; row++) {
        uint8_t config_byte = ~((uint8_t)(1 << row));
        HAL_I2C_Mem_Write(&hi2c1, KB_I2C_WRITE_ADDRESS, KB_CONFIG_REG, 1, &config_byte, 1, KB_DEBOUNCE_TIME_MS);
        HAL_Delay(10);

        HAL_I2C_Mem_Read(&hi2c1, KB_I2C_READ_ADDRESS, KB_INPUT_REG, 1, &reg_buffer, 1, KB_DEBOUNCE_TIME_MS);

        uint8_t col_data = reg_buffer >> 4;
        switch (col_data) {
            case 6:
                if (pressed_count == 0) {
                    index = row * 3 + 1;
                } else {
                    index = -1;
                }
                pressed_count++;
                row_counts[row]++;
                col_counts[0]++;
                break;

            case 5:
                if (pressed_count == 0) {
                    index = row * 3 + 2;
                } else {
                    index = -1;
                }
                pressed_count++;
                row_counts[row]++;
                col_counts[1]++;
                break;

            case 3:
                if (pressed_count == 0) {
                    index = row * 3 + 3;
                } else {
                    index = -1;
                }
                pressed_count++;
                row_counts[row]++;
                col_counts[2]++;
                break;

            default:
                break;
        }
    }

    int sum_rows = 0;
    int sum_cols = 0;

    for (int i = 0; i < 4; i++) {
        sum_rows += row_counts[i];
        row_counts[i] = 0;
    }

    for (int i = 0; i < 3; i++) {
        sum_cols += col_counts[i];
        col_counts[i] = 0;
    }

    if (sum_rows != 1 || sum_cols != 1) {
        index = -1;
    }

    if (index == -1) {
        last_pressed_btn_index = -1;
        return -1;
    }

    last_pressing_time = current_time;

    if (index == last_pressed_btn_index) {
        return -1;
    }

    last_pressed_btn_index = index;
    return index;
}

button_state_t keyboard_scanner_check_side_button(void) {
    static uint8_t was_pressed = 0;
    static uint32_t start_time = 0;

    if (HAL_GPIO_ReadPin(SIDE_BUTTON_PORT, SIDE_BUTTON_PIN) == GPIO_PIN_RESET) {
        if (!was_pressed) {
            was_pressed = 1;
            start_time = HAL_GetTick();
        }
        return BUTTON_IN_PROGRESS;
    } else {
        if (!was_pressed) {
            return BUTTON_NOT_PRESSED;
        }

        was_pressed = 0;

        uint32_t press_duration = HAL_GetTick() - start_time;
        if (press_duration >= SIDE_BUTTON_DEBOUNCE_MS) {
            return BUTTON_PRESSED;
        }
    }

    return BUTTON_NOT_PRESSED;
}
