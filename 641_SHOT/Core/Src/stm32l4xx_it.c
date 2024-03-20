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
#include "main.h"
#include "stm32l4xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"
#include "stm32l4xx_it.h"
#include "string.h"
#include "math.h"
#include "../../Register_UART.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */
	#define UART_BUFFER_SIZE 13
	#define CRC_Polynom 0x3C
	#define PGS ((10000 + 1) / 80e6f) * 1000000
	typedef struct {
		uint16_t comparator;
		uint8_t size;
		uint8_t CRC8;
		uint8_t count;
		uint16_t timer;
		uint8_t data[UART_BUFFER_SIZE];
		uint8_t answer[UART_BUFFER_SIZE];
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
uint8_t data_index = 0;
uint8_t flag = 0;
uint16_t counter = 0;
uint8_t timer_flag = 0;
int COUNTcheck = 0;
uint16_t time_per_tick = PGS;
uint32_t timer_start ;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
void My_Data_Processing_Function(uint8_t* data_buffer, Packet* packet);
void CRC_com(Packet** packet);
uint16_t My_Start_Timer(uint8_t flag);
uint16_t Get_Comparator(Packet* packet_7_8);
void Set_answer_Size_2(Packet** packet_full);
void Set_answer_Size_12(Packet** packet_full);
void Set_answer_uart(Packet* packet_answer, uint8_t lengh);
void Split_Comparator(uint16_t value, uint8_t* valueOne, uint8_t* valueTwo);
void clearPacket(Packet** packet);
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
  * @brief This function handles EXTI line0 interrupt.
  */
void EXTI0_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI0_IRQn 0 */
	if(GPIOA->IDR & (1 << 0)) {
		GPIOB->BSRR = (1 << 6);
	} else {
		GPIOB->BSRR = (1 << (6 + 16));
	}
	counter++;
  /* USER CODE END EXTI0_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
  /* USER CODE BEGIN EXTI0_IRQn 1 */

  /* USER CODE END EXTI0_IRQn 1 */
}

/**
  * @brief This function handles EXTI line1 interrupt.
  */
void EXTI1_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI1_IRQn 0 */
	
  /* USER CODE END EXTI1_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
  /* USER CODE BEGIN EXTI1_IRQn 1 */

  /* USER CODE END EXTI1_IRQn 1 */
}

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
    if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE)) // Проверка на наличие данных в приемнике
    {	
        static uint8_t data_buffer[UART_BUFFER_SIZE];
        
        if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_ORE)) // Проверка на переполнение буфера приемника
        {
            // Обработка ошибки переполнения буфера приемника
            __HAL_UART_CLEAR_FLAG(&huart1, UART_CLEAR_OREF);
        }
        else if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_FE)) // Проверка на ошибку кадра
        {
            // Обработка ошибки кадра
            __HAL_UART_CLEAR_FLAG(&huart1, UART_CLEAR_FEF);
        }
        else
        { 
            uint8_t rx_data = huart1.Instance->RDR; // Чтение данных из приемника
            if(data_index < UART_BUFFER_SIZE) {
                data_buffer[data_index] = rx_data;
                data_index++;
						} else {
                data_index = 0;
            }
            if(data_index == 12 && data_buffer[1] == 9) {
							data_index = 0;
              My_Data_Processing_Function(data_buffer, &packet);
              memset(data_buffer, 0, UART_BUFFER_SIZE);
            }
						if(data_index == 5 && data_buffer[1] == 2) {
							packet.timer  =	My_Start_Timer(timer_flag);
							data_index = 0;
              My_Data_Processing_Function(data_buffer, &packet);
              memset(data_buffer, 0, UART_BUFFER_SIZE);   
            }
        }
    }
  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */
	HAL_UART_Receive_IT(&huart1, &rx_data, UART_BUFFER_SIZE);
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
	
		packet->size = data_buffer[1];
		if(packet->size == 9) {
			for(int i = 0; i < 12; i++) // большой пакет запроса
			{
				packet->data[i] = data_buffer[i];
			}
			CRC_com(&packet);
			if(packet->CRC8 == 0) {
				variable_for_DAC = Get_Comparator(packet);
				Set_answer_Size_12(&packet);
				Set_answer_uart(packet, 4);
			}
			clearPacket(&packet);
		} else {
			for(int i = 0; i < 5; i++) { // малый пакет запроса
				packet->data[i] = data_buffer[i];
			}
			CRC_com(&packet);
			if(packet->CRC8 == 0) {
				Set_answer_Size_2(&packet);
				Set_answer_uart(packet, 12);
				counter = 0;
			}
			clearPacket(&packet);
	}
	data_index = 0;
}

uint16_t Get_Comparator(Packet* packet_7_8) {
	uint8_t valueOne = packet_7_8->data[7];
	uint8_t valueTwo = packet_7_8->data[8];
	
	uint16_t result = (valueOne << 8) | valueTwo;// Объединяем два значения в одно 16-битное значение
	// используя побитовый сдвиг на 8 бит влево для первого значения и побитовое ИЛИ для объединения двух значений
	return result;
}

void Split_Comparator(uint16_t value, uint8_t* valueOne, uint8_t* valueTwo) {
		*valueOne = (value >> 8) & 0xFF;// Получаю старший байт путем сдвига на 8 бит вправо и применения маски
		
		*valueTwo = value & 0xFF;	// Получаю младший байт путем применения маски
}

void Set_answer_Size_2(Packet** packet_full) {
		Packet* packet = *packet_full;
		uint8_t valOne, valTwo;
	
		packet->answer[0] = 35;
		packet->answer[1] =	10;
		packet->answer[2] =	1;
		packet->answer[3] =	0;
		Split_Comparator(packet->timer, &valOne, &valTwo);
		packet->answer[4] =	valOne;
		packet->answer[5] =	valTwo;
		Split_Comparator(counter, &valOne, &valTwo);
		packet->answer[6] =	valOne;
		packet->answer[7] =	valTwo;
		packet->answer[8] =	0;
		packet->answer[9] =	0;
		packet->answer[10] =	0;
		packet->answer[11] =	0;
		packet->answer[12]	=	packet->data[4];
}

void Set_answer_Size_12(Packet** packet_full) {
		Packet* packet = *packet_full;
	
		packet->answer[0] = 35;
		packet->answer[1] = 1;
		packet->answer[2] = 1;
		packet->answer[3] =	packet->data[11];
}

void Set_answer_uart(Packet* packet_answer, uint8_t lengh) {
		while(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TXE)){}
		HAL_UART_Transmit(&huart1, (uint8_t*)packet_answer->answer, lengh, HAL_MAX_DELAY);
}

void CRC_com(Packet** packet_ptr) {
	Packet* packet = *packet_ptr;// Разыменовываем указатель на указатель для доступа к структуре Packet
	packet->CRC8 = 0x00;
	int lenght = 0;
	if(packet->size == 9) {
		lenght = 12;
	} else {
		lenght = 5;
	}
	for(int i =0; i < lenght; i++) {
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

uint16_t My_Start_Timer(uint8_t flag) {
			
			static uint32_t timer_end ;
			uint16_t timer_elapsed = 0;
	
			timer_end = TIM15->CNT; // записываю показания окончания таймера
			uint16_t elapsed_ticks = (timer_end > timer_start) ? (timer_end - timer_start): (timer_start - timer_end);
	
	//		uint16_t time_per_tick = ((10000 + 1) / 80e6f) * 1000000; // Время на один тик в МКС // примерно 0.000125 секунды или 125 микросекунды
	//														10000 + 1 - это зачение преддделителя таймера
			timer_elapsed = elapsed_ticks * time_per_tick;//расчитываю время работы таймера в МКС
	//				elapsed_ticks - кол-во тиков    time_per_tick - время на один тик в наносекундах
			uint16_t result =	timer_elapsed; // Конверт секунд в микросекунды
	
			timer_start = 0;// 8e7f частота тактирования МК 80 MHz
			timer_end = 0;
			timer_start = TIM15->CNT;

	return result;
}
void clearPacket(Packet** packet_clear) {
		Packet* packet = *packet_clear;
    packet->comparator = 0;
    packet->size = 0;
    packet->CRC8 = 0;
    packet->count = 0;
    packet->timer = 0;

    // Очистка массивов data и answer
    for (int i = 0; i < UART_BUFFER_SIZE; i++) {
        packet->data[i] = 0;
        packet->answer[i] = 0;
    }
}
/* USER CODE END 1 */

