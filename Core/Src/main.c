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
    RED,
    GREEN,
    GREEN_BLINKING,
    YELLOW
} Colors;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DEFAULT_DURATION 4000
#define RED_DURATION (4*DEFAULT_DURATION)
#define GREEN_DURATION DEFAULT_DURATION
#define GREEN_BLINKING_DURATION DEFAULT_DURATION
#define GREEN_BLINKING_TOGGLE_DURATION (GREEN_BLINKING_DURATION / 10)
#define YELLOW_DURATION DEFAULT_DURATION
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static UART uart6;
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
  /* USER CODE BEGIN 2 */
  uart_init(&uart6, &huart6);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  // uint32_t HAL_GetTick();
  
  Button btn = {
      .GPIOx = GPIOC,
      .GPIO_Pin = GPIO_PIN_15,
      .last_pressed_time = 0
  };
  LED red_led = {
      .GPIOx = GPIOD,
      .GPIO_Pin = GPIO_PIN_15
  };
  LED green_led = {
      .GPIOx = GPIOD,
      .GPIO_Pin = GPIO_PIN_13
  };
  LED yellow_led = {
      .GPIOx = GPIOD,
      .GPIO_Pin = GPIO_PIN_14
  };

  Colors current_color = RED;
  uint32_t current_red_duration = RED_DURATION;
  uint32_t color_activated_time = HAL_GetTick();
  uint32_t last_green_blinking_toggle_time = 0;

  led_activate(&red_led);

  char hello_msg[] = "Hello, world!\r\n";
  char irq_on_msg[] = "[IRQ mode ON]\r\n";
  char irq_off_msg[] = "[Polling mode ON]\r\n";
  uint8_t c;

  /* Start in IRQ mode */
  uart_set_irq_mode(&uart6, true);
  uart_it_send_string(&uart6, irq_on_msg);

  while (1)
  {
      /* Button toggles between IRQ and Polling mode */
      if (button_is_clicked(&btn)) {
          if (uart_is_irq_mode(&uart6)) {
              uart_set_irq_mode(&uart6, false);
              uart_poll_send_string(&uart6, irq_off_msg);
          } else {
              uart_set_irq_mode(&uart6, true);
              uart_it_send_string(&uart6, irq_on_msg);
          }
      }

      if (uart_is_irq_mode(&uart6)) {
          if (uart_it_try_get_byte(&uart6, &c)) {
              switch (c) {
              case '!':
                  uart_it_send_string(&uart6, hello_msg);
                  break;
              default:
                  uart_it_send_byte(&uart6, c);
                  break;
              }
          }
      } else {
          if (uart_poll_try_get_byte(&uart6, &c)) {
              switch (c) {
              case '!':
                  uart_poll_send_string(&uart6, hello_msg);
                  break;
              default:
                  uart_poll_send_byte(&uart6, c);
                  break;
              }
          }
      }

	  // blocking
	  // HAL_UART_Transmit(&huart6, (uint8_t*)s, sizeof(s), uart_timeout);
      // HAL_Delay(1000);

//    uint32_t current_time = HAL_GetTick();
//
//    if (button_is_clicked(&btn)) {
//      if (current_color == RED && current_red_duration < RED_DURATION) {
//        current_color = GREEN;
//        color_activated_time = current_time;
//        led_deactivate(&red_led);
//        led_activate(&green_led);
//      } else if (current_color != GREEN) {
//        current_red_duration = RED_DURATION / 4;
//      }
//    }
//
//    switch (current_color)
//    {
//    case RED:
//      if (current_time - color_activated_time >= current_red_duration) {
//        current_color = GREEN;
//        color_activated_time = current_time;
//        current_red_duration = RED_DURATION;
//        led_deactivate(&red_led);
//        led_activate(&green_led);
//      }
//      break;
//    case GREEN:
//      if (current_time - color_activated_time >= GREEN_DURATION) {
//        current_color = GREEN_BLINKING;
//        color_activated_time = current_time;
//        last_green_blinking_toggle_time = current_time;
//        led_deactivate(&green_led);
//      }
//      break;
//    case GREEN_BLINKING:
//      // if (current_time - color_activated_time >= GREEN_BLINKING_DURATION) {
//      //   current_color = YELLOW;
//      //   color_activated_time = current_time;
//      //   led_deactivate(&green_led);
//      //   led_activate(&yellow_led);
//      // } else {
//      //   led_toggle(&green_led);
//      // }
//      if (current_time - color_activated_time >= GREEN_BLINKING_DURATION) {
//        current_color = YELLOW;
//        color_activated_time = current_time;
//        led_deactivate(&green_led);
//        led_activate(&yellow_led);
//      } else if (current_time - last_green_blinking_toggle_time >= GREEN_BLINKING_TOGGLE_DURATION) {
//        last_green_blinking_toggle_time = current_time;
//        led_toggle(&green_led);
//      }
//
//      break;
//    case YELLOW:
//      if (current_time - color_activated_time >= YELLOW_DURATION) {
//        current_color = RED;
//        color_activated_time = current_time;
//        led_deactivate(&yellow_led);
//        led_activate(&red_led);
//      }
//      break;
//    }
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
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
