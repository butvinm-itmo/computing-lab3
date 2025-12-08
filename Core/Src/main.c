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
#include "musical_keyboard.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Musical keyboard state */
static uint8_t current_octave = 4;
static uint16_t note_duration_ms = 1000;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief Simple UART transmit function
 */
static void uart_send(const char* str) {
    HAL_UART_Transmit(&huart6, (uint8_t*)str, strlen(str), 100);
}

/**
 * @brief Send integer as string
 */
static void uart_send_int(int value) {
    char buffer[12];
    snprintf(buffer, sizeof(buffer), "%d", value);
    uart_send(buffer);
}

/**
 * @brief Send duration in format X.Xs (e.g., "1.0s")
 */
static void uart_send_duration(uint16_t duration_ms) {
    char buffer[16];
    int seconds = duration_ms / 1000;
    int tenths = (duration_ms % 1000) / 100;
    snprintf(buffer, sizeof(buffer), "%d.%ds", seconds, tenths);
    uart_send(buffer);
}

/**
 * @brief Process single character command from UART
 */
static void process_uart_char(char c) {
    if (c >= '1' && c <= '7') {
        /* Play note Do-Si (0-6) */
        uint8_t note = c - '1';
        play_note(note, current_octave, note_duration_ms);
        uart_send("Playing: ");
        uart_send(note_names[note]);
        uart_send(", octave ");
        uart_send_int(current_octave);
        uart_send(", duration ");
        uart_send_duration(note_duration_ms);
        uart_send("\r\n");
    }
    else if (c == '+') {
        /* Increase octave */
        if (current_octave < MAX_OCTAVE) {
            current_octave++;
        }
        uart_send("Settings: octave ");
        uart_send_int(current_octave);
        uart_send(", duration ");
        uart_send_duration(note_duration_ms);
        uart_send("\r\n");
    }
    else if (c == '-') {
        /* Decrease octave */
        if (current_octave > MIN_OCTAVE) {
            current_octave--;
        }
        uart_send("Settings: octave ");
        uart_send_int(current_octave);
        uart_send(", duration ");
        uart_send_duration(note_duration_ms);
        uart_send("\r\n");
    }
    else if (c == 'A') {
        /* Increase duration */
        if (note_duration_ms < MAX_DURATION_MS) {
            note_duration_ms += DURATION_STEP_MS;
        }
        uart_send("Settings: octave ");
        uart_send_int(current_octave);
        uart_send(", duration ");
        uart_send_duration(note_duration_ms);
        uart_send("\r\n");
    }
    else if (c == 'a') {
        /* Decrease duration */
        if (note_duration_ms > MIN_DURATION_MS) {
            note_duration_ms -= DURATION_STEP_MS;
        }
        uart_send("Settings: octave ");
        uart_send_int(current_octave);
        uart_send(", duration ");
        uart_send_duration(note_duration_ms);
        uart_send("\r\n");
    }
    else if (c == '\r' || c == '\n') {
        /* Play scale */
        play_scale(current_octave, note_duration_ms);
        uart_send("Playing scale: octave ");
        uart_send_int(current_octave);
        uart_send(", duration ");
        uart_send_duration(note_duration_ms);
        uart_send("\r\n");
    }
    else {
        /* Invalid character */
        uart_send("Invalid character: ");
        uart_send_int((int)c);
        uart_send("\r\n");
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

  /* Initialize musical keyboard */
  musical_keyboard_init();

  /* Start timers */
  HAL_TIM_Base_Start_IT(&htim6);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      /* Update musical keyboard state (non-blocking) */
      musical_keyboard_update();

      /* Check for UART input */
      uint8_t c;
      if (HAL_UART_Receive(&huart6, &c, 1, 0) == HAL_OK) {
          /* Process the received character */
          process_uart_char(c);
      }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 15;
  RCC_OscInitStruct.PLL.PLLN = 144;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
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
