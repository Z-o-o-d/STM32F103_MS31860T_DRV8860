/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define SOFT_SPI_DELAY_CYCLES 30U
#define MS31860T_CHIPS 4U
#define MS31860T_CHANNELS_PER_CHIP 8U
#define MS31860T_TOTAL_CHANNELS (MS31860T_CHIPS * MS31860T_CHANNELS_PER_CHIP)
#define MS31860T_CH_ON_PATTERN 0x0002U
#define MS31860T_FAULT_ACTIVE_LEVEL GPIO_PIN_RESET

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */
static void SoftSPI_Init(void);
static void SoftSPI_CS(uint8_t level);
static uint8_t SoftSPI_TransferByte(uint8_t tx);
static void MS31860T_SetEnable(uint8_t enable);
static uint8_t MS31860T_IsFaultActive(void);
static void MS31860T_TransferFrame(const uint16_t *tx_data, uint16_t *rx_data, uint8_t chips);
static void MS31860T_SetSingleChannel(uint8_t ch, uint16_t *tx_data, uint8_t chips);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void SoftSPI_Delay(void)
{
  volatile uint32_t i;

  for (i = 0; i < SOFT_SPI_DELAY_CYCLES; i++)
  {
    __NOP();
  }
}

static void SoftSPI_Init(void)
{
  HAL_GPIO_WritePin(CSN_GPIO_Port, CSN_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(SCLK_GPIO_Port, SCLK_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(MOSI_GPIO_Port, MOSI_Pin, GPIO_PIN_RESET);
}

static void SoftSPI_CS(uint8_t level)
{
  HAL_GPIO_WritePin(CSN_GPIO_Port, CSN_Pin, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static uint8_t SoftSPI_TransferByte(uint8_t tx)
{
  uint8_t rx = 0;
  uint8_t i;

  for (i = 0; i < 8; i++)
  {
    HAL_GPIO_WritePin(MOSI_GPIO_Port, MOSI_Pin, (tx & 0x80U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    tx <<= 1;

    SoftSPI_Delay();
    HAL_GPIO_WritePin(SCLK_GPIO_Port, SCLK_Pin, GPIO_PIN_SET);
    SoftSPI_Delay(); /* Wait for chip to output data on rising edge */

    rx <<= 1;
    if (HAL_GPIO_ReadPin(MISO_GPIO_Port, MISO_Pin) == GPIO_PIN_SET)
    {
      rx |= 0x01U;
    }
    
    HAL_GPIO_WritePin(SCLK_GPIO_Port, SCLK_Pin, GPIO_PIN_RESET); /* Chip latches SI on falling edge */
    SoftSPI_Delay();
  }

  return rx;
}

static void MS31860T_SetEnable(uint8_t enable)
{
  HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, enable ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static uint8_t MS31860T_IsFaultActive(void)
{
  return (HAL_GPIO_ReadPin(FAULT_GPIO_Port, FAULT_Pin) == MS31860T_FAULT_ACTIVE_LEVEL) ? 1U : 0U;
}

static void MS31860T_TransferFrame(const uint16_t *tx_data, uint16_t *rx_data, uint8_t chips)
{
  uint8_t i;

  SoftSPI_CS(0);
  SoftSPI_Delay();

  for (i = 0; i < chips; i++)
  {
    uint16_t rx_word = (uint16_t)SoftSPI_TransferByte((uint8_t)(tx_data[i] >> 8)) << 8;
    rx_word |= SoftSPI_TransferByte((uint8_t)(tx_data[i] & 0xFFU));
    if (rx_data != NULL)
    {
      rx_data[i] = rx_word;
    }
  }

  SoftSPI_Delay();
  SoftSPI_CS(1);
}

static void MS31860T_SetSingleChannel(uint8_t ch, uint16_t *tx_data, uint8_t chips)
{
  uint8_t i;
  uint8_t target_chip;
  uint8_t target_channel;

  for (i = 0; i < chips; i++)
  {
    tx_data[i] = 0U;
  }

  target_chip = (uint8_t)(ch / MS31860T_CHANNELS_PER_CHIP);
  target_channel = (uint8_t)(ch % MS31860T_CHANNELS_PER_CHIP);

  if (target_chip < chips)
  {
    /* Daisy-chain order: tx_data[chips-1] reaches first chip, tx_data[0] reaches last chip. */
    tx_data[(chips - 1U) - target_chip] = (uint16_t)(MS31860T_CH_ON_PATTERN << (target_channel * 2U));
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
  /* USER CODE BEGIN 2 */
  SoftSPI_Init();
  MS31860T_SetEnable(0U);
  HAL_Delay(10);
  MS31860T_SetEnable(1U);
  HAL_Delay(2);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint8_t ch = 0;
  while (1)
  {
    uint16_t tx_data[MS31860T_CHIPS];
    uint16_t rx_data[MS31860T_CHIPS];
    uint8_t fault_active;

    MS31860T_SetSingleChannel(ch, tx_data, MS31860T_CHIPS);
    MS31860T_TransferFrame(tx_data, rx_data, MS31860T_CHIPS);

    fault_active = MS31860T_IsFaultActive();
    HAL_GPIO_WritePin(LED_ERR_GPIO_Port, LED_ERR_Pin, fault_active ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_TogglePin(LED_RUN_GPIO_Port, LED_RUN_Pin);

    ch++;
    if (ch >= MS31860T_TOTAL_CHANNELS)
    {
      ch = 0;
    }

    HAL_Delay(500);

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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, CSN_Pin|SCLK_Pin|MOSI_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, LED_RUN_Pin|LED_ERR_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : CSN_Pin SCLK_Pin MOSI_Pin */
  GPIO_InitStruct.Pin = CSN_Pin|SCLK_Pin|MOSI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : MISO_Pin */
  GPIO_InitStruct.Pin = MISO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MISO_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : FAULT_Pin */
  GPIO_InitStruct.Pin = FAULT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(FAULT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ENABLE_Pin */
  GPIO_InitStruct.Pin = ENABLE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ENABLE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_RUN_Pin LED_ERR_Pin */
  GPIO_InitStruct.Pin = LED_RUN_Pin|LED_ERR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
#ifdef USE_FULL_ASSERT
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
