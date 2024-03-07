/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "../../lis3dsh.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DEV_ADDR 0x68
//————————————————
#define ABS(x)         (x < 0) ? (-x) : x
//————————————————
#define LD_PORT GPIOD
#define LD3 GPIO_PIN_13 //ORANGE
#define LD4 GPIO_PIN_12 //GREEN
#define LD5 GPIO_PIN_14 //RED
#define LD6 GPIO_PIN_15 //BLUE
#define LD3_ON HAL_GPIO_WritePin(LD_PORT, LD3, GPIO_PIN_SET) //ORANGE
#define LD4_ON HAL_GPIO_WritePin(LD_PORT, LD4, GPIO_PIN_SET) //GREEN
#define LD5_ON HAL_GPIO_WritePin(LD_PORT, LD5, GPIO_PIN_SET) //RED
#define LD6_ON HAL_GPIO_WritePin(LD_PORT, LD6, GPIO_PIN_SET) //BLUE
#define LD3_OFF HAL_GPIO_WritePin(LD_PORT, LD3, GPIO_PIN_RESET) //ORANGE
#define LD4_OFF HAL_GPIO_WritePin(LD_PORT, LD4, GPIO_PIN_RESET) //GREEN
#define LD5_OFF HAL_GPIO_WritePin(LD_PORT, LD5, GPIO_PIN_RESET) //RED
#define LD6_OFF HAL_GPIO_WritePin(LD_PORT, LD6, GPIO_PIN_RESET) //BLUE
#define CS_GPIO_PORT GPIOE
#define CS_PIN GPIO_PIN_3
#define CS_ON HAL_GPIO_WritePin(CS_GPIO_PORT, CS_PIN, GPIO_PIN_RESET)
#define CS_OFF HAL_GPIO_WritePin(CS_GPIO_PORT, CS_PIN, GPIO_PIN_SET)
#define DUMMY_BYTE                        ((uint8_t)0x00)
//————————————————
#define LIS3DSH_WHO_AM_I_ADDR                0x0F
//————————————————
#define READWRITE_CMD                     ((uint8_t)0x80)
#define MULTIPLEBYTE_CMD                  ((uint8_t)0x40)
//————————————————
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t aRxBuffer[6]; // Buffer for input date
uint8_t aTxBuffer[6]; // Buffer for output date with accselerometr
extern SPI_HandleTypeDef hspi1;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
void Accel_Ini(void);
void Accel_ReadAcc(void);
static uint8_t SPIx_WriteRead(uint8_t Byte);
uint8_t Accel_IO_Read(uint8_t *pBuffer, uint8_t ReadAddr, uint16_t NumbByteToRead);
static void Error (void);
uint8_t Accel_IO_Read(uint8_t *pBuffer, uint8_t ReadAddr, uint16_t NumbByteToRead);
void Accel_IO_Write(uint16_t *pBuffer, uint8_t WriteAddr, uint8_t NumByteToWrite);
uint8_t Accel_ReadID(void);
void AccInit(uint16_t InitStruct);
void Accel_GetXYZ(uint16_t* pData);
void printXYZ(uint16_t *array);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//————————————

static void Error (void)

{
	LD5_ON;
}
//—————————————
uint8_t Accel_IO_Read(uint8_t *pBuffer, uint8_t ReadAddr, uint16_t NumbByteToRead)
{
		if(NumbByteToRead > 0x01) {
			ReadAddr |= (uint8_t)(READWRITE_CMD | MULTIPLEBYTE_CMD);
		} else {
			ReadAddr |= (uint8_t)READWRITE_CMD;
		}
			CS_ON;
		SPIx_WriteRead(ReadAddr);
		while(NumbByteToRead > 0x00) {
			*pBuffer = SPIx_WriteRead(DUMMY_BYTE);
			NumbByteToRead--;
			pBuffer++;
		}
			CS_OFF;
        return 0;
}

//—————————————

void Accel_IO_Write(uint16_t *pBuffer, uint8_t WriteAddr, uint8_t NumByteToWrite)
{
				CS_OFF;
		if(NumByteToWrite > 0x01) {
			WriteAddr |= (uint8_t)MULTIPLEBYTE_CMD;
		}
				CS_ON;
		SPIx_WriteRead(WriteAddr);
		while(NumByteToWrite >= 0x01) {
			SPIx_WriteRead(*pBuffer);
			NumByteToWrite--;
			pBuffer++;
		}
				CS_OFF;
}
//—————————————
uint8_t Accel_ReadID(void)
{  
  uint8_t ctrl = 0;
	Accel_IO_Read(&ctrl, LIS3DSH_WHO_AM_I_ADDR, 1);
        return ctrl;

}
//—————————————
void Accel_AccFilterConfig(uint8_t FilterStruct)
{

 

}
//—————————————
void AccInit(uint16_t InitStruct)
{
	uint16_t ctrl = 0x00;
	ctrl = (uint8_t)(InitStruct);
		Accel_IO_Write(&ctrl, LIS3DSH_CTRL_REG4_ADDR, 1);
	ctrl = (uint8_t)(InitStruct >> 8);
		Accel_IO_Write(&ctrl, LIS3DSH_CTRL_REG5_ADDR, 1);
}
//—————————————
void Accel_GetXYZ(uint16_t* pData)
{
	uint8_t buffer[6];
	uint8_t ctrl, i = 0x00;
	float sensitivity = LIS3DSH_SENSITIVITY_0_06G;
	float valueinfloat = 0;
	Accel_IO_Read(&ctrl, LIS3DSH_CTRL_REG5_ADDR, 1);
	Accel_IO_Read((uint8_t*)&buffer[0], LIS3DSH_OUT_X_L_ADDR, 1);
	Accel_IO_Read((uint8_t*)&buffer[1], LIS3DSH_OUT_X_H_ADDR, 1);
	Accel_IO_Read((uint8_t*)&buffer[2], LIS3DSH_OUT_Y_L_ADDR, 1);
	Accel_IO_Read((uint8_t*)&buffer[3], LIS3DSH_OUT_Y_H_ADDR, 1);
	Accel_IO_Read((uint8_t*)&buffer[4], LIS3DSH_OUT_Z_L_ADDR, 1);
	Accel_IO_Read((uint8_t*)&buffer[5], LIS3DSH_OUT_Z_H_ADDR, 1);
	switch(ctrl & LIS3DSH__FULLSCALE_SELECTION) {
		case LIS3DSH_FULLSCALE_2:
			sensitivity = LIS3DSH_SENSITIVITY_0_06G;
		break;
		case LIS3DSH_FULLSCALE_4:
			sensitivity = LIS3DSH_SENSITIVITY_0_12G;
		break;
		case LIS3DSH_FULLSCALE_6:
			sensitivity = LIS3DSH_SENSITIVITY_0_18G;
		break;
		case LIS3DSH_FULLSCALE_8:
			sensitivity = LIS3DSH_SENSITIVITY_0_24G;
		break;
		case LIS3DSH_FULLSCALE_16:
			sensitivity = LIS3DSH_SENSITIVITY_0_24G;
		break;
		default:
			break;
	}
	for(int i = 0; i < 3; i++) {
		valueinfloat = ((buffer[2 * i + 1] << 8) + buffer[2 * i] );// * sensitivity;
		pData[i] = (uint16_t)valueinfloat;
	}
}

void printXYZ(uint16_t *array) {
	uint16_t xval = array[0];
	uint16_t yval = array[1];
	uint16_t zval = array[2];
	
	char str[50];
	
	sprintf(str, "X: %u ; Y: %u ; Z: %u\r\n", xval, yval, zval);
	HAL_UART_Transmit(&huart2,(uint8_t*)str,strlen(str),0xFFFF);
	//uint8_t test[] = "TEST\r\n";
	//HAL_UART_Transmit(&huart2, test, 16, 0xFFFF);
}
//—————————————
void Accel_ReadAcc(void)
{
	uint16_t buffer[3] = {0};
	int16_t xval, yval, zval = 0x00;
	Accel_GetXYZ(buffer);
	printXYZ(buffer);
	xval = buffer[0];
	yval = buffer[1];
	zval = buffer[2];
	
	  if((ABS(xval))>(ABS(yval))) {
		if (xval > 2000) {
			LD5_ON;
		} else if(xval < -2000) {
			LD4_ON;
		}
	} else {
		if(yval > 2000) {
			LD3_ON;
		} else if(yval < -2000) {
			LD6_ON;
		}
	}
	HAL_Delay(20);
	LD3_OFF;
	LD4_OFF;
	LD5_OFF;
	LD6_OFF;
}
//—————————————
void Accel_Ini(void)
{
	uint16_t ctrl = 0x0000;
	HAL_Delay(1000);
	if(Accel_ReadID() == 0x3F) LD4_ON;
	else Error();
	/* Configure MEMS: power mode(ODR) and axes enable */
	ctrl = (uint16_t)(LIS3DSH_DATARATE_100 | LIS3DSH_XYZ_ENABLE);
	/* Configure MEMS: full scale and self test */
	ctrl |= (uint16_t)((LIS3DSH_SERIALINTERFACE_4WIRE |
                         LIS3DSH_SELFTEST_NORMAL   |
                         LIS3DSH_FULLSCALE_2  |
                         LIS3DSH_FILTER_BW_800) << 8);
	AccInit(ctrl);
		LD6_ON;// BLUE
}
//—————————————
static uint8_t SPIx_WriteRead(uint8_t Byte) {
	uint8_t receivedbyte = 0;
	if(HAL_SPI_TransmitReceive(&hspi1,(uint8_t*)&Byte, (uint8_t*)&receivedbyte, 1, 0x1000) != HAL_OK) {
		Error();
	}
	return receivedbyte;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	//uint8_t str[]="Hello, wolrd!\r\n";

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
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
	Accel_Ini();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		// 
    
			
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		Accel_ReadAcc();
		HAL_Delay(100);
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 50;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

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
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
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
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3|GPIO_PIN_0|GPIO_PIN_1, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OTG_FS_PowerSwitchOn_GPIO_Port, OTG_FS_PowerSwitchOn_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, LD4_Pin|LD3_Pin|LD5_Pin|LD6_Pin
                          |Audio_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PE3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = OTG_FS_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(OTG_FS_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PDM_OUT_Pin */
  GPIO_InitStruct.Pin = PDM_OUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(PDM_OUT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : I2S3_WS_Pin */
  GPIO_InitStruct.Pin = I2S3_WS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
  HAL_GPIO_Init(I2S3_WS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BOOT1_Pin */
  GPIO_InitStruct.Pin = BOOT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BOOT1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : CLK_IN_Pin */
  GPIO_InitStruct.Pin = CLK_IN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(CLK_IN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD4_Pin LD3_Pin LD5_Pin LD6_Pin
                           Audio_RST_Pin */
  GPIO_InitStruct.Pin = LD4_Pin|LD3_Pin|LD5_Pin|LD6_Pin
                          |Audio_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : I2S3_MCK_Pin I2S3_SCK_Pin I2S3_SD_Pin */
  GPIO_InitStruct.Pin = I2S3_MCK_Pin|I2S3_SCK_Pin|I2S3_SD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : VBUS_FS_Pin */
  GPIO_InitStruct.Pin = VBUS_FS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(VBUS_FS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : OTG_FS_ID_Pin OTG_FS_DM_Pin OTG_FS_DP_Pin */
  GPIO_InitStruct.Pin = OTG_FS_ID_Pin|OTG_FS_DM_Pin|OTG_FS_DP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_OverCurrent_Pin */
  GPIO_InitStruct.Pin = OTG_FS_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : Audio_SCL_Pin Audio_SDA_Pin */
  GPIO_InitStruct.Pin = Audio_SCL_Pin|Audio_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PE0 PE1 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

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

