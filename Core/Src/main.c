/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "gpio_driver.h"
#include "uart_driver.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    STATE_RED,
    STATE_GREEN,
    STATE_GREEN_BLINKING,
    STATE_YELLOW
} TrafficState;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DEFAULT_TIMEOUT_SEC 4
#define GREEN_DURATION_MS 4000
#define GREEN_BLINK_DURATION_MS 4000
#define GREEN_BLINK_TOGGLE_MS 400
#define YELLOW_DURATION_MS 4000
#define CMD_BUF_SIZE 64
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static UART uart6;

/* Traffic light state */
static TrafficState traffic_state = STATE_RED;
static uint32_t state_start_time = 0;
static uint32_t last_blink_time = 0;
static uint32_t red_timeout_ms = DEFAULT_TIMEOUT_SEC * 1000;
static uint8_t button_mode = 1;  /* 1 = button enabled, 2 = button ignored */
static bool red_shortened = false;  /* flag: next red timeout will be /4 */

/* Command buffer */
static char cmd_buf[CMD_BUF_SIZE];
static uint8_t cmd_len = 0;

/* LED and button instances */
static Button btn;
static LED red_led, green_led, yellow_led;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART6) {
		uart_rx_complete_callback(&uart6);
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART6) {
		uart_tx_complete_callback(&uart6);
	}
}

/* Send string (auto-selects IT or poll mode) */
static void send_str(const char *s) {
    if (uart_is_irq_mode(&uart6)) {
        uart_it_send_string(&uart6, s);
    } else {
        uart_poll_send_string(&uart6, s);
    }
}

/* Send single char */
static void send_char(char c) {
    if (uart_is_irq_mode(&uart6)) {
        uart_it_send_byte(&uart6, (uint8_t)c);
    } else {
        uart_poll_send_byte(&uart6, (uint8_t)c);
    }
}

/* Get state name */
static const char* get_state_name(void) {
    switch (traffic_state) {
        case STATE_RED: return "red";
        case STATE_GREEN: return "green";
        case STATE_GREEN_BLINKING: return "blinking green";
        case STATE_YELLOW: return "yellow";
        default: return "unknown";
    }
}

/* Simple string compare */
static int str_eq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a++ != *b++) return 0;
    }
    return *a == *b;
}

/* Check if string starts with prefix */
static int str_starts(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s++ != *prefix++) return 0;
    }
    return 1;
}

/* Parse integer from string */
static int parse_int(const char *s) {
    int val = 0;
    while (*s >= '0' && *s <= '9') {
        val = val * 10 + (*s - '0');
        s++;
    }
    return val;
}

/* Process command */
static void process_command(void) {
    cmd_buf[cmd_len] = '\0';

    if (str_eq(cmd_buf, "?")) {
        /* Status query */
        uint32_t effective_timeout = red_shortened ? (red_timeout_ms / 4) : red_timeout_ms;
        int timeout_sec = effective_timeout / 1000;
        char mode_char = uart_is_irq_mode(&uart6) ? 'I' : 'P';

        /* Build response manually */
        send_str(get_state_name());
        send_str(" mode ");
        send_char('0' + button_mode);
        send_str(" timeout ");
        if (timeout_sec >= 100) send_char('0' + (timeout_sec / 100) % 10);
        if (timeout_sec >= 10) send_char('0' + (timeout_sec / 10) % 10);
        send_char('0' + (timeout_sec % 10));
        send_str(" ");
        send_char(mode_char);
        send_str("\r\n");

    } else if (str_eq(cmd_buf, "set mode 1")) {
        button_mode = 1;
        send_str("OK\r\n");

    } else if (str_eq(cmd_buf, "set mode 2")) {
        button_mode = 2;
        send_str("OK\r\n");

    } else if (str_starts(cmd_buf, "set timeout ")) {
        int val = parse_int(cmd_buf + 12);
        if (val > 0) {
            red_timeout_ms = val * 1000;
            send_str("OK\r\n");
        } else {
            send_str("ERROR: invalid timeout\r\n");
        }

    } else if (str_eq(cmd_buf, "set interrupts on")) {
        uart_set_irq_mode(&uart6, true);
        send_str("OK\r\n");

    } else if (str_eq(cmd_buf, "set interrupts off")) {
        uart_set_irq_mode(&uart6, false);
        send_str("OK\r\n");

    } else if (cmd_len > 0) {
        send_str("ERROR: unknown command\r\n");
    }

    cmd_len = 0;
}

/* Set traffic light state */
static void set_state(TrafficState new_state) {
    /* Turn off all LEDs */
    led_deactivate(&red_led);
    led_deactivate(&green_led);
    led_deactivate(&yellow_led);

    traffic_state = new_state;
    state_start_time = HAL_GetTick();
    last_blink_time = state_start_time;

    /* Turn on appropriate LED */
    switch (new_state) {
        case STATE_RED:    led_activate(&red_led); break;
        case STATE_GREEN:  led_activate(&green_led); break;
        case STATE_YELLOW: led_activate(&yellow_led); break;
        default: break;
    }
}

/* Update traffic light state machine */
static void update_traffic_light(void) {
    uint32_t now = HAL_GetTick();
    uint32_t elapsed = now - state_start_time;

    /* Calculate effective red timeout (shortened if flag set) */
    uint32_t effective_red_timeout = red_shortened ? (red_timeout_ms / 4) : red_timeout_ms;

    switch (traffic_state) {
    case STATE_RED:
        if (elapsed >= effective_red_timeout) {
            red_shortened = false;  /* reset after red phase completes */
            set_state(STATE_GREEN);
        }
        break;

    case STATE_GREEN:
        if (elapsed >= GREEN_DURATION_MS) {
            set_state(STATE_GREEN_BLINKING);
        }
        break;

    case STATE_GREEN_BLINKING:
        if (elapsed >= GREEN_BLINK_DURATION_MS) {
            set_state(STATE_YELLOW);
        } else if (now - last_blink_time >= GREEN_BLINK_TOGGLE_MS) {
            led_toggle(&green_led);
            last_blink_time = now;
        }
        break;

    case STATE_YELLOW:
        if (elapsed >= YELLOW_DURATION_MS) {
            set_state(STATE_RED);
        }
        break;
    }
}

/* Handle button press (mode 1 only) */
static void handle_button(void) {
    if (button_mode != 1) return;

    if (button_is_clicked(&btn)) {
        if (traffic_state == STATE_GREEN_BLINKING || traffic_state == STATE_YELLOW) {
            /* First click during blink/yellow: shorten next red */
            red_shortened = true;
        } else if (traffic_state == STATE_RED) {
            if (red_shortened) {
                /* Second click during red: skip to green, reset */
                red_shortened = false;
                set_state(STATE_GREEN);
            } else {
                /* First click during red: shorten current red */
                red_shortened = true;
            }
        }
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART6_UART_Init();
  MX_TIM6_Init();
  MX_TIM4_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
//  uart_init(&uart6, &huart6);

  /* Initialize button and LEDs */
//  btn.GPIOx = GPIOC;
//  btn.GPIO_Pin = GPIO_PIN_15;
//  btn.last_pressed_time = 0;
//
//  red_led.GPIOx = GPIOD;
//  red_led.GPIO_Pin = GPIO_PIN_15;
//
//  green_led.GPIOx = GPIOD;
//  green_led.GPIO_Pin = GPIO_PIN_13;
//
//  yellow_led.GPIOx = GPIOD;
//  yellow_led.GPIO_Pin = GPIO_PIN_14;

  /* Start in IRQ mode */
//  uart_set_irq_mode(&uart6, true);

  /* Initialize traffic light */
//  set_state(STATE_RED);
//  send_str("Traffic light ready\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint8_t c;

  HAL_TIM_Base_Start_IT(&htim6);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  while (1)
  {


	  /* Update traffic light state machine */
//      update_traffic_light();
//
//      /* Handle button (mode 1 only) */
//      handle_button();
//
//      /* Process UART input */
//      bool got_char = false;
//      if (uart_is_irq_mode(&uart6)) {
//          got_char = uart_it_try_get_byte(&uart6, &c);
//      } else {
//          got_char = uart_poll_try_get_byte(&uart6, &c);
//      }
//
//      if (got_char) {
//          /* Echo the character */
//          send_char(c);
//
//          if (c == '\r' || c == '\n') {
//              /* End of command - process it */
//              send_str("\r\n");
//              process_command();
//          } else if (c == 127 || c == 8) {
//              /* Backspace */
//              if (cmd_len > 0) {
//                  cmd_len--;
//                  send_str("\b \b");
//              }
//          } else if (cmd_len < CMD_BUF_SIZE - 1) {
//              /* Add to buffer */
//              cmd_buf[cmd_len++] = c;
//          }
//      }
  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  // }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 15;
  RCC_OscInitStruct.PLL.PLLN = 216;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
