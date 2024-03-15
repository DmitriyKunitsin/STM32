/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32l4xx_it.c
  * @brief   Interrupt Service Routines.
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
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"
#include "stm32l4xx_it.h"
#include "string.h"
#include "../../Register_UART.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */
	#define UART_BUFFER_SIZE 9
	#define CRC_Polynom 0x3C
	typedef struct {
		uint8_t adress;
		uint8_t size;
		uint8_t command;
		uint8_t CRC8;
		uint8_t data[UART_BUFFER_SIZE];
	} Packet;
/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
char test[20];
char output[20];
uint8_t testUint;

	/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
void My_Data_Processing_Function(uint8_t* data_buffer, Packet* packet);
void CRC_com(Packet* packet);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim15;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
/* USER CODE BEGIN EV */
/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32L4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32l4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles TIM1 break interrupt and TIM15 global interrupt.
  */
void TIM1_BRK_TIM15_IRQHandler(void)
{
  /* USER CODE BEGIN TIM1_BRK_TIM15_IRQn 0 */

  /* USER CODE END TIM1_BRK_TIM15_IRQn 0 */
  HAL_TIM_IRQHandler(&htim15);
  /* USER CODE BEGIN TIM1_BRK_TIM15_IRQn 1 */

  /* USER CODE END TIM1_BRK_TIM15_IRQn 1 */
}

/**
  * @brief This function handles USART1 global interrupt.
  */
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */
	Packet packet;
	
  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */
	HAL_UART_Transmit(&huart2, (uint8_t*)"Myyyyyyyyy\r\n", strlen("Myyyyyyyyy\r\n"), HAL_MAX_DELAY);
	while((huart1.Instance->ISR & USART_ISR_RXNE) == 0) {} // Проверка на наличие данных в приемнике
    
        if(huart1.Instance->ISR & USART_ISR_ORE) // Проверка на переполнение буфера приемника
        {
            // Обработка ошибки переполнения буфера приемника
            huart1.Instance->ISR |= USART_ICR_ORECF; // Сброс флага переполнения буфера
        }
        else if(huart1.Instance->ISR & USART_ISR_FE) // Проверка на ошибку кадра
        {
            // Обработка ошибки кадра
            huart1.Instance->ISR |= USART_ICR_FECF; // Сброс флага ошибки кадра
        }
        else
        {	
            rx_data = huart1.Instance->RDR; // Чтение данных из приемника
					
//						while((huart2.Instance->ISR & USART_ISR_TXE) == 0) {}
//						huart2.Instance->TDR = rx_data;
							//HAL_UART_Receive_IT(&huart1, &rx_data, UART_BUFFER_SIZE);
					//HAL_UART_Transmit(&huart2, (uint8_t*)"My", strlen("My"), HAL_MAX_DELAY);
					My_Data_Processing_Function(&rx_data, &packet);
        
			}
    
	
	if((GPIOB->ODR & (1 << 6)) == 0) {
		GPIOB->ODR |= (1 << 6);
	} else {
		GPIOB->ODR &= ~(1 << 6);
	}
  /* USER CODE END USART1_IRQn 1 */
}

/**
  * @brief This function handles USART2 global interrupt.
  */
void USART2_IRQHandler(void)
{
  /* USER CODE BEGIN USART2_IRQn 0 */

  /* USER CODE END USART2_IRQn 0 */
  HAL_UART_IRQHandler(&huart2);
  /* USER CODE BEGIN USART2_IRQn 1 */

  /* USER CODE END USART2_IRQn 1 */
}

/* USER CODE BEGIN 1 */
void My_Data_Processing_Function(uint8_t* data_buffer, Packet* packet) {
		
		HAL_UART_Transmit(&huart2, (uint8_t*)"My", strlen("My"), HAL_MAX_DELAY);
	
		packet->command = data_buffer[3];
		testUint = packet->command;
		sprintf(output, "command : %d\r\n", testUint);
		HAL_UART_Transmit(&huart2, (uint8_t*)output, strlen(output), HAL_MAX_DELAY);
		
		packet->adress = data_buffer[0];
		testUint = packet->adress;
		sprintf(output, "adress : %d\r\n", testUint);
		HAL_UART_Transmit(&huart2, (uint8_t*)output, strlen(output), HAL_MAX_DELAY);
	
		packet->size = data_buffer[1];
		testUint = packet->size;
		sprintf(output, "size : %d\r\n", testUint);
		HAL_UART_Transmit(&huart2, (uint8_t*)output, strlen(output), HAL_MAX_DELAY);
			if(packet->adress == 0x40) {
				
				switch(packet->command) {
					case 0x11:
						strcpy(test, "0x11"); 
						HAL_UART_Transmit(&huart2, (uint8_t*)test, strlen(test), HAL_MAX_DELAY);
							if(HAL_UART_GetState(&huart2) == HAL_UART_STATE_READY) {
								strcpy(test, "suc\r\n");
								HAL_UART_Transmit(&huart2, (uint8_t*)test, strlen(test), HAL_MAX_DELAY);
							}
						CRC_com(packet);
						if(packet->CRC8 == packet->data[8]) {
							//HAL_UART_Transmit(&huart2, test, 1, HAL_MAX_DELAY);
						}
						break;
					case 0x13:
						//HAL_UART_Transmit(&huart2, test, 1, HAL_MAX_DELAY);
						CRC_com(packet);
						if(packet->CRC8 == packet->data[4]) {
							//HAL_UART_Transmit(&huart2, test, 1, HAL_MAX_DELAY);
						}
						break;
					default:
						
						break;
				}
			}
}

void CRC_com(Packet* packet) {
	packet->CRC8 = 0x00;
	for(int i =0; i < packet->size; i++) {
		packet->CRC8 ^= packet->data[i];
		if(packet->CRC8 & (1 << 0)) {
			packet->CRC8 >>= 1;
			packet->CRC8 |= (1 << 7);
			packet->CRC8 ^= CRC_Polynom;
		} else {
			packet->CRC8 >>=1;
		}
	}
}
/* USER CODE END 1 */

