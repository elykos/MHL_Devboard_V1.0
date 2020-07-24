/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "i2c_comm.h"
#include "usbd_cdc_if.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define EXP_OUTPUTS_ADDR (0x21)<<1
#define EXP_INPUTS_ADDR  (0x20)<<1

#define BUF_LENGTH 4
#define NUM_OF_COMMANDS 10

#define I2C_BUS_STATUS_LED 1
#define USB_STATUS_LED	   2
#define ERROR_LED		   3

#define ON	1
#define OFF 0
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim7;
TIM_HandleTypeDef htim14;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

uint8_t is_USB_Connected;
uint8_t transmit_inputs;

uint8_t data_buffer[BUF_LENGTH];
uint8_t data_available;

uint8_t encoded_command_byte_2[NUM_OF_COMMANDS] = {0xA1,0xA2,0xA3,0xA4,0xA5, \
												   0xA6,0xA7,0xA8,0xA9,0xDC };

uint16_t inputs_read;

uint32_t timer6_counter;
uint32_t timer7_counter;
uint32_t timer14_counter;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM6_Init(void);
static void MX_TIM7_Init(void);
static void MX_TIM14_Init(void);
/* USER CODE BEGIN PFP */
void Reset_Expanders(void);
void flush_data_buffer(void);
void enable_interrupt_handlers(void);
int I2C_Peripherals_available();
void status_led_control(int led, int state);
void LED_Quick_Flash(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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

  is_USB_Connected = 0;
  transmit_inputs = 0;
  flush_data_buffer();

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_USB_DEVICE_Init();
  MX_TIM6_Init();
  MX_TIM7_Init();
  MX_TIM14_Init();
  /* USER CODE BEGIN 2 */
  LED_Quick_Flash();

  HAL_Delay(1000);

  Reset_Expanders();

  I2C_Peripherals_available();

  initialize_expanders(hi2c1);

  /*Test code here*/
  //configure_pupd_enable_registers(hi2c1, EXP_INPUTS_ADDR, 0xff);
  //configure_pupd_selection_registers(hi2c1, EXP_INPUTS_ADDR, 0x00);
  /*Test code above*/

  enable_interrupt_handlers();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  // For debugging info used with STMStudio
	  timer6_counter = TIM6->CNT;
	  timer7_counter = TIM7->CNT;
	  timer14_counter = TIM14->CNT;


	  if(data_available == 1)
	  {
		  if(data_buffer[0] == 0x00 || data_buffer[0] == 0xCB)
		  {
			  // Command 0xA1 -> Configure Outputs
			  if(data_buffer[1] == encoded_command_byte_2[0])
			  {
				  uint16_t tmp = ((data_buffer[2] <<8)| data_buffer[3]);
				  configure_outputs(hi2c1, tmp);
			  }

			  // Command 0xA2 -> Read Outputs State
			  else if(data_buffer[1] == encoded_command_byte_2[1])
			  {
				  uint16_t output_states;
				  output_states = read_all_outputs(hi2c1);

				  uint8_t output_states_buffer[2];

				  /* To identify that the transmitted data are outputs
				   * we send as first byte the byte 0xB2
				   */
				  output_states_buffer[0] = 0xB2;

				  output_states_buffer[1] = ((output_states)>>8);	//High Byte
				  output_states_buffer[2] = output_states;			//Low Byte

				  CDC_Transmit_FS(output_states_buffer, 3);
			  }

			  // Command 0xA3 -> Read Inputs State
			  else if(data_buffer[1] == encoded_command_byte_2[2])
			  {
				  uint16_t inputs_states;
				  inputs_states = read_all_inputs(hi2c1);

				  uint8_t inputs_states_buffer[3];

				  /* To identify that the transmitted data are inputs
				   * we send as first byte the byte 0xB1
				   */
				  inputs_states_buffer[0] = 0xB1;

				  inputs_states_buffer[1] = ((inputs_states)>>8); //High byte
				  inputs_states_buffer[2] =   inputs_states;	  //Low  byte
				  CDC_Transmit_FS(inputs_states_buffer, 3);
			  }

			  // Command 0xA4 -> Generate clock pulse
			  else if(data_buffer[1] == encoded_command_byte_2[3])
			  {
				  /* Start the countdown timer */
				  HAL_GPIO_WritePin(GEN_GPIO_Port, GEN_Pin, GPIO_PIN_SET);
				  HAL_TIM_Base_Start(&htim6);
			  }

			  // Command 0xA5 -> Establish Connection
			  else if(data_buffer[1] == encoded_command_byte_2[4])
			  {
				  if(data_buffer[2] == 0x75 && data_buffer[3] == 0x31)
				  {
					/* Refresh the "Lost connection with PC" timer */
					TIM14->CNT = 0;

					  is_USB_Connected = 1;
					  status_led_control(USB_STATUS_LED, ON);
				  }
			  }

			  // Command 0xA6 -> Connection OK Report Message
			  else if(data_buffer[1] == encoded_command_byte_2[5])
			  {
				  TIM14->CNT = 0;
				  is_USB_Connected = 1;
			  }

			  // Command 0xA7 -> Disconnect Dev Board
			  else if(data_buffer[1] == encoded_command_byte_2[6])
			  {
				  is_USB_Connected = 0;
				  status_led_control(USB_STATUS_LED, OFF);
			  }

			  else if(data_buffer[1] == encoded_command_byte_2[7])
			  {

			  }

			  else if(data_buffer[1] == encoded_command_byte_2[8])
			  {


			  }
			  else if(data_buffer[1] == encoded_command_byte_2[9])
			  {
				  //DEBUG

			  }
			  else
			  {

			  }

		  }


		  data_available = 0;
		  flush_data_buffer();
	  }


	  if(transmit_inputs == 1 && is_USB_Connected==1)
	  {
	  	  uint16_t inputs_states;
	  	  inputs_states = read_all_inputs(hi2c1);

	  	  uint8_t inputs_states_buffer[3];

	  	  /* To identify we send back the state of the inputs
	  	   * we send the byte 0xB1
	  	   */
	  	  inputs_states_buffer[0] = 0xB1;

	  	  inputs_states_buffer[1] = ((inputs_states)>>8); //High byte
	  	  inputs_states_buffer[2] =   inputs_states;	  //Low  byte
	  	  CDC_Transmit_FS(inputs_states_buffer, 3);

	  	  transmit_inputs = 0;

	  	  /* Debounce time when input is unstable
	  	   * to avoid system congestion (by 
         * flooding the communication channel) */
	  	  HAL_Delay(20);
	  }

	  //DBG
    //HAL_GPIO_WritePin(LD5_GPIO_Port,LD5_Pin,HAL_GPIO_ReadPin(INT_1_GPIO_Port,INT_1_Pin));

	  /*else
	  {
	  	  //__NOP();
		  uint8_t tmp_buf[4] = {0x57,0x4F,0x4C,0x46};
	  	  CDC_Transmit_FS(tmp_buf, 4);
	  	  HAL_Delay(2000);
	  }*/


	  /*
	  GPIO_PinState tmp_read;
	  tmp_read = HAL_GPIO_ReadPin(INT_1_GPIO_Port, INT_1_Pin);


	  HAL_GPIO_WritePin(LD5_GPIO_Port, LD5_Pin, tmp_read);


	  if(HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_SET)
	  {
		  while(HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) != GPIO_PIN_RESET)
		  {

		  }
		  inputs_read = read_all_inputs(hi2c1);
	  }
	  */



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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  RCC_CRSInitTypeDef RCC_CRSInitStruct = {0};

  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB|RCC_PERIPHCLK_USART2
                              |RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  /** Enable the SYSCFG APB clock 
  */
  __HAL_RCC_CRS_CLK_ENABLE();
  /** Configures CRS 
  */
  RCC_CRSInitStruct.Prescaler = RCC_CRS_SYNC_DIV1;
  RCC_CRSInitStruct.Source = RCC_CRS_SYNC_SOURCE_USB;
  RCC_CRSInitStruct.Polarity = RCC_CRS_SYNC_POLARITY_RISING;
  RCC_CRSInitStruct.ReloadValue = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000,1000);
  RCC_CRSInitStruct.ErrorLimitValue = 34;
  RCC_CRSInitStruct.HSI48CalibrationValue = 32;

  HAL_RCCEx_CRSConfig(&RCC_CRSInitStruct);
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x0000020B;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Analogue filter 
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Digital filter 
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  //Prescaler value : 100-1
  //Period value : 480-1

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 100-1;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 480-1;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  if (HAL_TIM_Base_Start_IT(&htim6) != HAL_OK)
  {
  	  /* Starting Error */
  	  Error_Handler();
  }

  /* Initially we want the CLK Generation timer
   * to be deactivated. It's activated when the
   * generation command arrives from the PC.
   */
  HAL_TIM_Base_Stop(&htim6);

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief TIM7 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM7_Init(void)
{

  /* USER CODE BEGIN TIM7_Init 0 */

  /* USER CODE END TIM7_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM7_Init 1 */

  //We configure Timer 7 (TIM7) to have a frequency
  	//of 1000Hz (T=50ms) used create a 1ms timebase
  	//so it transmits the output states every one ms

  //Prescaler value : 500-1
    //Period value : 4800-1

  /* USER CODE END TIM7_Init 1 */
  htim7.Instance = TIM7;
  htim7.Init.Prescaler = 500-1;
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period = 4800-1;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM7_Init 2 */

  if (HAL_TIM_Base_Start_IT(&htim7) != HAL_OK)
  {
  	  /* Starting Error */
  	  Error_Handler();
  }

  /* USER CODE END TIM7_Init 2 */

}

/**
  * @brief TIM14 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM14_Init(void)
{

  /* USER CODE BEGIN TIM14_Init 0 */

  /* USER CODE END TIM14_Init 0 */

  /* USER CODE BEGIN TIM14_Init 1 */

	//We configure Timer 14 (TIM14) to have a frequency
	//of 0.2Hz (T=5sec) used to check if the USB connection
	//is still alive

	//Presc Value : 10000
	//Period Value : 24000

  /* USER CODE END TIM14_Init 1 */
  htim14.Instance = TIM14;
  htim14.Init.Prescaler = 10000-1;
  htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim14.Init.Period = 24000-1;
  htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim14) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM14_Init 2 */

  if (HAL_TIM_Base_Start_IT(&htim14) != HAL_OK)
  {
  	  /* Starting Error */
  	  Error_Handler();
  }

  /* USER CODE END TIM14_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 0x9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_AUTOBAUDRATE_INIT;
  huart2.AdvancedInit.AutoBaudRateEnable = UART_ADVFEATURE_AUTOBAUDRATE_ENABLE;
  huart2.AdvancedInit.AutoBaudRateMode = UART_ADVFEATURE_AUTOBAUDRATE_ONSTARTBIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, LED1_Pin|LED2_Pin|LED3_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GEN_GPIO_Port, GEN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, ERST_1_Pin|ERST_2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, DBG_1_Pin|DBG_2_Pin|DBG_3_Pin|DBG_4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PC13 PC14 PC15 PC4 
                           PC6 PC7 PC8 PC9 
                           PC10 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_4 
                          |GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9 
                          |GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PF1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : LED1_Pin LED2_Pin LED3_Pin ERST_1_Pin 
                           ERST_2_Pin */
  GPIO_InitStruct.Pin = LED1_Pin|LED2_Pin|LED3_Pin|ERST_1_Pin 
                          |ERST_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : INT_1_Pin INT_2_Pin */
  GPIO_InitStruct.Pin = INT_1_Pin|INT_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA1 PA5 PA6 
                           PA7 PA8 PA9 PA10 
                           PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_5|GPIO_PIN_6 
                          |GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10 
                          |GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : GEN_Pin */
  GPIO_InitStruct.Pin = GEN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GEN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB2 PB3 
                           PB4 PB5 PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3 
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : I2C2_SCL_Pin I2C2_SDA_Pin */
  GPIO_InitStruct.Pin = I2C2_SCL_Pin|I2C2_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF1_I2C2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : DBG_1_Pin DBG_2_Pin DBG_3_Pin DBG_4_Pin */
  GPIO_InitStruct.Pin = DBG_1_Pin|DBG_2_Pin|DBG_3_Pin|DBG_4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PD2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI2_3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);

  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

}

/* USER CODE BEGIN 4 */

/*
 * @brief Flashes all the LEDs 3 times at a rate of 400ms
 *
 */
void LED_Quick_Flash()
{
	for(int i=0;i<3;i++)
	{
		status_led_control(ERROR_LED, ON);
		status_led_control(I2C_BUS_STATUS_LED, ON);
		status_led_control(USB_STATUS_LED, ON);

		HAL_Delay(400);

		status_led_control(ERROR_LED, OFF);
		status_led_control(I2C_BUS_STATUS_LED, OFF);
		status_led_control(USB_STATUS_LED, OFF);

		HAL_Delay(400);
	}
}


/*
 * @brief Checks that the I/O Expanders are up and reachable
 * 		  If the I/O Expanders are ready and listening
 * 		  the function turns on the I2C_BUS_STATUS_LED
 */
int I2C_Peripherals_available()
{
	if(HAL_I2C_IsDeviceReady(&hi2c1,EXP_INPUTS_ADDR,10,1000) == HAL_OK \
			&& HAL_I2C_IsDeviceReady(&hi2c1,EXP_OUTPUTS_ADDR,10,1000) == HAL_OK)
	{
		status_led_control(I2C_BUS_STATUS_LED, ON);

		//FOR DEBUG PURPOSES
		//HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, GPIO_PIN_SET);
	}

	return 0;
}

/*
 * @brief Clears the data (buffer) received from the USB
 * 		  Usually called after a successful reception
 */
void flush_data_buffer()
{
	for(int j=0;j<BUF_LENGTH;j++)
	{
		data_buffer[j] = 0x00;
	}
}

/*
 * @brief Enable the interrupt handlers
 */
void enable_interrupt_handlers()
{
	HAL_NVIC_SetPriority(EXTI2_3_IRQn, 2, 0);
	HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);
}


/*
 * @brief Function that is called upon an External Interrupt
 * 		  When the INT_1 pin becomes LOW, we set a global flag
 * 		  to notify that there's been an update on the Dev Board
 *
 * @param GPIO_Pin : The source (External GPIO pin) of the Interrupt
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{


	if(GPIO_Pin == INT_1_Pin)
	{
		transmit_inputs = 1;
	}

	/* Wait 4ms for debouncing*/
	//HAL_Delay(4);


    /* Toggle LED3 */
	/* Debug Purposes*/
	//HAL_GPIO_TogglePin(LD5_GPIO_Port, LD5_Pin);

}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	// Create a 1msec pulse
	if(htim->Instance == TIM6)
	{
		HAL_TIM_Base_Stop(&htim6);
		HAL_GPIO_WritePin(GEN_GPIO_Port, GEN_Pin, GPIO_PIN_RESET);
	}

	// Set the flag to transmit inputs status
	else if(htim->Instance == TIM7)
	{
		transmit_inputs = 1;
	}

	/* If counter reaches ARR value (5sec)
	*  the connection with the computer
	*  is considered lost
	* */
	else if(htim->Instance == TIM14)
	{
		is_USB_Connected = 0;
		status_led_control(USB_STATUS_LED, OFF);
	}
}

/*
 * @brief Switch ON/OFF the Dev Board LEDs
 * 		  The Board has 3 available LEDs
 * 		  LED1 = Red LED
 * 		  LED2 = Orange LED
 * 		  LED3 = Blue LED
 * @param  The led to be switched
 * @param  The state to be switched which
 * 		   can be either ON or OFF
 * @retval None
 */
void status_led_control(int led, int state)
{
	/* For open-drain outputs a "0" in the Output register
	 * activates the N-MOS (and current is flowing to ground)
	 * whereas a "1" in the Output register leaves the port
	 * in Hi-Z mode. So to activate the LED's we need to set
	 * a "0" (GPIO_PIN_RESET) on the Output register
	 */
	if(led == ERROR_LED)
	{
		if(state == ON)
		{
			HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);

			/* Debug purposes */
			//HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
		}
		else if(state == OFF)
		{
			HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);

			/* Debug purposes */
			//HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
		}
	}

	else if(led == I2C_BUS_STATUS_LED)
	{
		if(state == ON)
		{
			HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);

			/* Debug purposes */
			//HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, GPIO_PIN_SET);
		}
		else if(state == OFF)
		{
			HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);

			/* Debug purposes */
			//HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, GPIO_PIN_RESET);
		}
	}

	else if(led == USB_STATUS_LED)
	{
		if(state == ON)
		{
			HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET);

			/* Debug purposes */
			//HAL_GPIO_WritePin(LD6_GPIO_Port, LD6_Pin, GPIO_PIN_SET);
		}
		else if(state == OFF)
		{
			HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);

			/* Debug purposes */
			//HAL_GPIO_WritePin(LD6_GPIO_Port, LD6_Pin, GPIO_PIN_RESET);
		}
	}
}

/*
 * @brief Pull down the external RESET line
 * of the I/O expanders
 */
void Reset_Expanders()
{
	HAL_GPIO_WritePin(ERST_1_GPIO_Port, ERST_1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(ERST_2_GPIO_Port, ERST_2_Pin, GPIO_PIN_RESET);

	/*
	 *  Expander Reset signal is connected to HIGH through a high-value
	 *  pull-up resistor (100kOhm) so the reset signal is slow
	 *  We handle the slow signal by adding a small delay to ensure
	 *  the expanders are actually Reset
	 */
	HAL_Delay(1000);

	HAL_GPIO_WritePin(ERST_1_GPIO_Port, ERST_1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(ERST_2_GPIO_Port, ERST_2_Pin, GPIO_PIN_SET);
}


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
	status_led_control(ERROR_LED, ON);
  //HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
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
void assert_failed(char *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
