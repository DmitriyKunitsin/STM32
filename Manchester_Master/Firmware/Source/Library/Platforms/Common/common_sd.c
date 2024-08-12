// common_sd.c
// АдаптациЯ файла en.stm32cubef4.zip\STM32Cube_FW_F4_V1.15.0\Drivers\BSP\STM324xG_EVAL\stm324xg_eval_sd.c
/**
  ******************************************************************************
  * @file    stm324xg_eval_sd.c
  * @author  MCD Application Team
  * @version V3.0.0
  * @date    27-January-2017
  * @brief   This file includes the uSD card driver mounted on STM324xG-EVAL
  *          evaluation board.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */ 

/* File Info : -----------------------------------------------------------------
                                   User NOTES
1. How To use this driver:
--------------------------
   - This driver is used to drive the micro SD external card mounted on STM324xG-EVAL 
     evaluation board.
   - This driver does not need a specific component driver for the micro SD device
     to be included with.

2. Driver description:
---------------------
  + Initialization steps:
     o Initialize the micro SD card using the BSP_SD_Init() function. This 
       function includes the MSP layer hardware resources initialization and the
       SDIO interface configuration to interface with the external micro SD. It 
       also includes the micro SD initialization sequence.
     o To check the SD card presence you can use the function BSP_SD_IsDetected() which 
       returns the detection status 
     o If SD presence detection interrupt mode is desired, you must configure the 
       SD detection interrupt mode by calling the function BSP_SD_ITConfig(). The interrupt 
       is generated as an external interrupt whenever the micro SD card is 
       plugged/unplugged in/from the evaluation board. The SD detection interrupt
       is handeled by calling the function BSP_SD_DetectIT() which is called in the IRQ
       handler file, the user callback is implemented in the function BSP_SD_DetectCallback().
     o The function BSP_SD_GetCardInfo() is used to get the micro SD card information 
       which is stored in the structure "HAL_SD_CardInfoTypedef".
  
     + Micro SD card operations
        o The micro SD card can be accessed with read/write block(s) operations once 
          it is ready for access. The access can be performed whether using the polling
          mode by calling the functions BSP_SD_ReadBlocks()/BSP_SD_WriteBlocks(), or by DMA 
          transfer using the functions BSP_SD_ReadBlocks_DMA()/BSP_SD_WriteBlocks_DMA()
        o The DMA transfer complete is used with interrupt mode. Once the SD transfer
          is complete, the SD interrupt is handled using the function BSP_SD_IRQHandler(),
          the DMA Tx/Rx transfer complete are handled using the functions
          BSP_SD_DMA_Tx_IRQHandler()/BSP_SD_DMA_Rx_IRQHandler(). The corresponding user callbacks 
          are implemented by the user at application level. 
        o The SD erase block(s) is performed using the function BSP_SD_Erase() with specifying
          the number of blocks to erase.
        o The SD runtime status is returned when calling the function BSP_SD_GetCardState().
 
------------------------------------------------------------------------------*/ 

/* Includes ------------------------------------------------------------------*/
//#include "stm324xg_eval_sd.h"
#include "ProjectConfig.h"
#include "common_sd.h"
#include "common_rcc.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM324xG_EVAL
  * @{
  */ 
  
/** @defgroup STM324xG_EVAL_SD STM324xG EVAL SD
  * @{
  */ 

/** @defgroup STM324xG_EVAL_SD_Private_Variables STM324xG EVAL SD Private Variables
  * @{
  */       
SD_HandleTypeDef uSdHandle;

/**
  * @}
  */ 

/** @defgroup STM324xG_EVAL_SD_Private_Functions STM324xG EVAL SD Private Functions
  * @{
  */

// Проверить выравнивание адреса буфера длЯ приема-передачи и настройки размера элементарной посылки модулЯ DMA
HAL_StatusTypeDef DMA_ValidateBufferAlign( DMA_HandleTypeDef *pDMA, uint8_t *pBuffer )
{
	assert_param( NULL != pDMA );
	assert_param( NULL != pDMA->Instance );
	HAL_StatusTypeDef Result = HAL_ERROR;
	switch( pDMA->Init.MemDataAlignment )
	{
	case DMA_MDATAALIGN_BYTE:		Result = HAL_OK;	break;
	case DMA_MDATAALIGN_HALFWORD:	if( VALIDATE_ALIGN( pBuffer, sizeof( uint16_t ) ) )	Result = HAL_OK;	break;
	case DMA_MDATAALIGN_WORD:		if( VALIDATE_ALIGN( pBuffer, sizeof( uint32_t ) ) )	Result = HAL_OK;	break;
	default:						assert_param( 0 );
	}
	return Result;
}

// Платформенно-зависимые функции BSP_SD_Init() перенесены в Common_SD_STMF4xx.c
#if 0
/**
  * @brief  Initializes the SD card device.
  * @retval SD status.
  */
uint8_t BSP_SD_Init(void)
{ 
  uint8_t SD_state = MSD_OK;

	// Перед инициализацией необходимо сбросить драйвер SDIO, если он ранее уже был инициализирован.
	// По-хорошему, это надо делать в BSP_SD_DeInit(), однако FatFS умеет только инициализировать устройства при монтировании,
	// а деинициализировать при размонтировании не умеет.
	if( HAL_SD_STATE_RESET != uSdHandle.State )
		HAL_SD_DeInit( &uSdHandle );
  
  /* uSD device interface configuration */
  uSdHandle.Instance = SDIO;

  uSdHandle.Init.ClockEdge           = SDIO_CLOCK_EDGE_RISING;
  uSdHandle.Init.ClockBypass         = SDIO_CLOCK_BYPASS_DISABLE;
  uSdHandle.Init.ClockPowerSave      = SDIO_CLOCK_POWER_SAVE_DISABLE;
  uSdHandle.Init.BusWide             = SDIO_BUS_WIDE_1B;
  uSdHandle.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
#ifdef	SDIO_CLKDIV
	uSdHandle.Init.ClockDiv			= SDIO_CLKDIV;				// Опциональное замеление частоты SDIO_CK
#else
	uSdHandle.Init.ClockDiv			= SDIO_TRANSFER_CLK_DIV;	// Частота SDIO_CK по-умолчанию
#endif	// SDIO_CLKDIV
  
  /* Check if the SD card is plugged in the slot */
  if(BSP_SD_IsDetected() != SD_PRESENT)
  {
    return MSD_ERROR;
  }
  
  /* Msp SD initialization */
//  BSP_SD_MspInit(&uSdHandle, NULL);
  // Проверить, что напрЯжение питаниЯ SD-карты в норме
  if( __HAL_PWR_GET_FLAG( PWR_FLAG_PVDO ) )
	  return MSD_ERROR;

  if(HAL_SD_Init(&uSdHandle) != HAL_OK)
  {
    SD_state = MSD_ERROR;
  }

  // Проверить, что напрЯжение питаниЯ SD-карты в норме
  if( __HAL_PWR_GET_FLAG( PWR_FLAG_PVDO ) )
  {
	SD_state = MSD_ERROR;
  }
  
  /* Configure SD Bus width */
  if(SD_state == MSD_OK)
  {
    /* Enable wide operation */
    if(HAL_SD_ConfigWideBusOperation(&uSdHandle, SDIO_BUS_WIDE_4B) != HAL_OK)
    {
      SD_state = MSD_ERROR;
    }
    else
    {
      SD_state = MSD_OK;
    }
  }
  
  return  SD_state;
}
#endif

#ifdef	SD_DETECT_PIN
/**
  * @brief  Configures Interrupt mode for SD detection pin.
  * @retval Returns 0
  */
uint8_t BSP_SD_ITConfig(void)
{ 
  GPIO_InitTypeDef GPIO_Init_Structure;
  
  /* Configure Interrupt mode for SD detection pin */ 
  GPIO_Init_Structure.Mode      = GPIO_MODE_IT_RISING_FALLING;
  GPIO_Init_Structure.Pull      = GPIO_PULLUP;
  GPIO_Init_Structure.Speed     = GPIO_SPEED_HIGH;
  GPIO_Init_Structure.Pin       = SD_DETECT_PIN;
  HAL_GPIO_Init(SD_DETECT_GPIO_PORT, &GPIO_Init_Structure);
    
  /* NVIC configuration for SDIO interrupts */
  HAL_NVIC_SetPriority(SD_DETECT_IRQn, 0x0E, 0);
  HAL_NVIC_EnableIRQ(SD_DETECT_IRQn);
  
  return 0;
}
#endif

/**
  * @brief  Detects if SD card is correctly plugged in the memory slot or not.
  * @retval Returns if SD is detected or not
  */
uint8_t BSP_SD_IsDetected(void)
{
  __IO uint8_t status = SD_PRESENT;

#ifdef	SD_DETECT_PIN
  /* Check SD card detect pin */
  if(HAL_GPIO_ReadPin(SD_DETECT_GPIO_PORT, SD_DETECT_PIN) != GPIO_PIN_RESET) 
  {
    status = SD_NOT_PRESENT;
  }
#endif
  
  return status;
}

#ifdef	SD_DETECT_PIN
/**
  * @brief  SD detect IT treatment
  */
void BSP_SD_DetectIT(void)
{
  /* SD detect IT callback */
  BSP_SD_DetectCallback();
  
}
#endif

/** 
  * @brief  SD detect IT detection callback
  */
__weak void BSP_SD_DetectCallback(void)
{
  /* NOTE: This function Should not be modified, when the callback is needed,
  the BSP_SD_DetectCallback could be implemented in the user file
  */ 
  
}

/**
  * @brief  Reads block(s) from a specified address in an SD card, in polling mode.
  * @param  pData: Pointer to the buffer that will contain the data to transmit
  * @param  ReadAddr: Address from where data is to be read
  * @param  NumOfBlocks: Number of SD blocks to read
  * @param  Timeout: Timeout for read operation
  * @retval SD status
  */
uint8_t BSP_SD_ReadBlocks(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks, uint32_t Timeout)
{
	assert_param( MSD_OK == HAL_OK );
	// Проверить, что напрЯжение питаниЯ SD-карты в норме
	if( __HAL_PWR_GET_FLAG( PWR_FLAG_PVDO ) )
		return HAL_ERROR;
	return ( uint8_t ) HAL_SD_ReadBlocks(&uSdHandle, (uint8_t *)pData, ReadAddr, NumOfBlocks, Timeout);
}

/**
  * @brief  Writes block(s) to a specified address in an SD card, in polling mode. 
  * @param  pData: Pointer to the buffer that will contain the data to transmit
  * @param  WriteAddr: Address from where data is to be written
  * @param  NumOfBlocks: Number of SD blocks to write
  * @param  Timeout: Timeout for write operation
  * @retval SD status
  */
uint8_t BSP_SD_WriteBlocks(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks, uint32_t Timeout)
{
	assert_param( MSD_OK == HAL_OK );
	// Проверить, что напрЯжение питаниЯ SD-карты в норме
	if( __HAL_PWR_GET_FLAG( PWR_FLAG_PVDO ) )
		return HAL_ERROR;
	return ( uint8_t ) HAL_SD_WriteBlocks(&uSdHandle, (uint8_t *)pData, WriteAddr, NumOfBlocks, Timeout);
}

/**
  * @brief  Reads block(s) from a specified address in an SD card, in DMA mode.
  * @param  pData: Pointer to the buffer that will contain the data to transmit
  * @param  ReadAddr: Address from where data is to be read
  * @param  NumOfBlocks: Number of SD blocks to read 
  * @retval SD status
  */
uint8_t BSP_SD_ReadBlocks_DMA(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks)
{  
	assert_param( MSD_OK == HAL_OK );
	// Проверить, что напрЯжение питаниЯ SD-карты в норме
	if( __HAL_PWR_GET_FLAG( PWR_FLAG_PVDO ) )
		return HAL_ERROR;
#ifdef	SD_USE_SINGLE_DMA
	// Если длЯ реализации SDMMC1 использованы разные каналы DMA, каждый раз инициализировать DMA на прием
	assert_param( HAL_OK == SD_DMAConfigRx( &uSdHandle ) );
#endif
	
	// Проверить выравнивание буфера
	if( NULL == uSdHandle.hdmarx )
		return HAL_ERROR;	// были прецеденты, когда указатель на канал DMA обнулЯлсЯ в результате прерываниЯ по ошибке DMA
	assert_param( HAL_OK == DMA_ValidateBufferAlign( uSdHandle.hdmarx, ( uint8_t * ) pData ) );
	return ( uint8_t ) HAL_SD_ReadBlocks_DMA(&uSdHandle, (uint8_t *)pData, ReadAddr, NumOfBlocks);
}

/**
  * @brief  Writes block(s) to a specified address in an SD card, in DMA mode.
  * @param  pData: Pointer to the buffer that will contain the data to transmit
  * @param  WriteAddr: Address from where data is to be written
  * @param  NumOfBlocks: Number of SD blocks to write 
  * @retval SD status
  */
uint8_t BSP_SD_WriteBlocks_DMA(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks)
{ 
  assert_param( MSD_OK == HAL_OK );
  // Проверить, что напрЯжение питаниЯ SD-карты в норме
//  if( __HAL_PWR_GET_FLAG( PWR_FLAG_PVDO ) )
//	  return HAL_ERROR;
#ifdef	SD_USE_SINGLE_DMA
	// Если длЯ реализации SDMMC1 использованы разные каналы DMA, каждый раз инициализировать DMA на передачу
	assert_param( HAL_OK == SD_DMAConfigTx( &uSdHandle ) );
#endif
  // Проверить выравнивание буфера
  if( NULL == uSdHandle.hdmatx )
	  return HAL_ERROR;   // были прецеденты, когда указатель на канал DMA обнулЯлсЯ в результате прерываниЯ по ошибке DMA
  assert_param( HAL_OK == DMA_ValidateBufferAlign( uSdHandle.hdmatx, ( uint8_t * ) pData ) );
  return ( uint8_t ) HAL_SD_WriteBlocks_DMA(&uSdHandle, (uint8_t *)pData, WriteAddr, NumOfBlocks);
}

/**
  * @brief  Erases the specified memory area of the given SD card. 
  * @param  StartAddr: Start byte address
  * @param  EndAddr: End byte address
  * @retval SD status
  */
uint8_t BSP_SD_Erase(uint32_t StartAddr, uint32_t EndAddr)
{
	assert_param( MSD_OK == HAL_OK );
	// Проверить, что напрЯжение питаниЯ SD-карты в норме
	if( __HAL_PWR_GET_FLAG( PWR_FLAG_PVDO ) )
		return HAL_ERROR;
	return ( uint8_t ) HAL_SD_Erase(&uSdHandle, StartAddr, EndAddr);
}

// Прервать текущую транзакцию
uint8_t BSP_SD_Abort( void )
{ 
  return ( uint8_t ) HAL_SD_Abort( &uSdHandle );
}

#if 0
/*********************************************************************
Порядок включения SD-карты (Power Cycle)
1. Отключение питания:
  - на VDD напряжение менее 0.5 В;
  - DAT, CMD, SCK в Z-состоянии или подтянуты к 0;
  - время для сброса не менее 1 мс.
2. Включение питания:
  - допустимое рабочее напрЯжение от 2.7 до 3.6 В;
  - время нарастания напряжения до номинала - от 0.1 мс до 35 мс;
  - нарастание по-возможности плавное;
  - допускается подавать SCK (?);
  - до подачи первой команды необходимо выдавить не менее 74 SCK при подтянутом вверх CMD.

Пока принято следующее решение по сбросу SD:
  - SD запитана через P-канальный MOSFET, управляемый через SD_POWEREN_GPIO_PORT.SD_POWEREN_PIN
  - на питании SD установлен конденсатор 2.2 мкФ (сглаживание включения) и резистор 2.4 кОм (разряд емкости после отключения)
  - время разряда емкости через резистор до 0.5 В составляет порядка 12 мс (2RC) - т.е. на это время (и более) необходимо отключать питание SD при сбросе

Измерено на макетоной плате ИНГК 516_03 (там 3.3 кОм):
- при включении напрЯжение нарастает за 5 мкс
- при отключении напрЯжение спадает до 1.2 В за 3 мс, до 0.5 В за 20 мс
*********************************************************************/

/**
  * @brief  Initializes the SD MSP.
  * @param  hsd: SD handle
  * @param  Params : pointer on additional configuration parameters, can be NULL.
  */
//__weak void BSP_SD_MspInit(SD_HandleTypeDef *hsd, void *Params)
void HAL_SD_MspInit(SD_HandleTypeDef *hsd)
{
  static DMA_HandleTypeDef dmaRxHandle __PLACE_AT_RAM_CCM__;
  static DMA_HandleTypeDef dmaTxHandle __PLACE_AT_RAM_CCM__;
  GPIO_InitTypeDef GPIO_Init_Structure;

	// Включить и настроить Programmable Voltage Detector длЯ определениЯ допустимости напрЯжениЯ питаниЯ SD
	// !! PVD отсутствует
	HAL_PWR_EnablePVD( );
	PWR_PVDTypeDef PWR_PVD;
	PWR_PVD.Mode		= PWR_PVD_MODE_NORMAL;		// режим длЯ поллинга, без прерываний и событий
#if	( defined( STM32F401xB ) || defined( STM32F401xC ) || defined( STM32F405xx ) || defined( STM32F407xx ) )	// Пороги длЯ остальных чипов уточнить!
	// !!Также проверить натурно!
	// !!Проверить уход по температуре
//	PWR_PVD.PVDLevel	= PWR_PVDLEVEL_5;			// /2.93V, \2.84V +-0.07V	в худшем случае включитсЯ на 2.99V или отключитсЯ на 2.65V
	PWR_PVD.PVDLevel	= PWR_PVDLEVEL_6;			// /3.03V, \2.93V +-0.07V	в худшем случае включитсЯ на 3.10V или отключитсЯ на 2.85V
//	PWR_PVD.PVDLevel	= PWR_PVDLEVEL_7;			// /3.14V, \3.03V +-0.07V	в худшем случае включитсЯ на 3.21V или отключитсЯ на 2.95V
#endif
	HAL_PWR_ConfigPVD( &PWR_PVD );
	// !! надо как-то контролировать, что настройка PVD не будет изменена в других частЯх приложениЯ!

  /* Enable SDIO clock */
  __HAL_RCC_SDIO_CLK_ENABLE();
  
  /* Enable DMA2 clocks */
  __DMAx_TxRx_CLK_ENABLE();

  /* Enable GPIOs clock */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __SD_DETECT_GPIO_CLK_ENABLE();

#ifdef	SD_POWEREN_GPIO_PORT
	__SD_POWEREN_GPIO_CLK_ENABLE( );
	/* SD Card Power Enable pin configuration */
	GPIO_Init_Structure.Mode	  = GPIO_MODE_OUTPUT_PP;
	GPIO_Init_Structure.Pull	  = GPIO_PULLUP;
	GPIO_Init_Structure.Speed	  = GPIO_SPEED_FREQ_LOW;
	GPIO_Init_Structure.Pin 	  = SD_POWEREN_PIN;
	// Отключить питание
	HAL_GPIO_WritePin( SD_POWEREN_GPIO_PORT, SD_POWEREN_PIN, GPIO_PIN_SET );
	HAL_GPIO_Init(SD_POWEREN_GPIO_PORT, &GPIO_Init_Structure);
	// Включить питание после паузы
	HAL_Delay( 20 );		// !!! Пауза выбрана исходЯ из времени разрЯда конденсатора 2.2 мкФ резистором 2.4 кОм! ДлЯ другой схемы питаниЯ, пауза может быть другой!
	HAL_GPIO_WritePin( SD_POWEREN_GPIO_PORT, SD_POWEREN_PIN, GPIO_PIN_RESET );
	HAL_Delay( 1 );
#endif	// SD_POWEREN_GPIO_PORT
  
  /* Common GPIO configuration */
  GPIO_Init_Structure.Mode      = GPIO_MODE_AF_PP;
  GPIO_Init_Structure.Pull      = GPIO_PULLUP;
  GPIO_Init_Structure.Speed     = GPIO_SPEED_FREQ_HIGH;
  GPIO_Init_Structure.Alternate = GPIO_AF12_SDIO;
  
  /* GPIOC configuration */
  GPIO_Init_Structure.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
   
  HAL_GPIO_Init(GPIOC, &GPIO_Init_Structure);

  /* GPIOD configuration */
  GPIO_Init_Structure.Pin = GPIO_PIN_2;
  HAL_GPIO_Init(GPIOD, &GPIO_Init_Structure);

#ifdef	SD_DETECT_PIN
  /* SD Card detect pin configuration */
  GPIO_Init_Structure.Mode      = GPIO_MODE_INPUT;
  GPIO_Init_Structure.Pull      = GPIO_PULLUP;
  GPIO_Init_Structure.Speed     = GPIO_SPEED_FREQ_HIGH;
  GPIO_Init_Structure.Pin       = SD_DETECT_PIN;
  HAL_GPIO_Init(SD_DETECT_GPIO_PORT, &GPIO_Init_Structure);
#endif
    
  /* NVIC configuration for SDIO interrupts */
//  HAL_NVIC_SetPriority(SDIO_IRQn, 0x0E, 0);
  HAL_NVIC_SetPriority(SDIO_IRQn, SDIO_IRQ_PREEMTPRIORITY, SDIO_IRQ_SUBPRIORITY);
  HAL_NVIC_EnableIRQ(SDIO_IRQn);
    
  /* Configure DMA Rx parameters */
  dmaRxHandle.Init.Channel             = SD_DMAx_Rx_CHANNEL;
  dmaRxHandle.Init.Direction           = DMA_PERIPH_TO_MEMORY;
  dmaRxHandle.Init.PeriphInc           = DMA_PINC_DISABLE;
  dmaRxHandle.Init.MemInc              = DMA_MINC_ENABLE;
  dmaRxHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
//  dmaRxHandle.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
  dmaRxHandle.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
  dmaRxHandle.Init.Mode                = DMA_PFCTRL;
//  dmaRxHandle.Init.Priority            = DMA_PRIORITY_VERY_HIGH;
  dmaRxHandle.Init.Priority            = SD_DMAx_Rx_DMA_PRIORITY;
  dmaRxHandle.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
  dmaRxHandle.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
  dmaRxHandle.Init.MemBurst            = DMA_MBURST_INC4;
  dmaRxHandle.Init.PeriphBurst         = DMA_PBURST_INC4;
  // !! Важно!!
  // !! По-умолчанию в примере stm324xg_eval_sd.c была настройка ( MemDataAlignment = DMA_MDATAALIGN_WORD ).
  // !! Словами работать быстрее, но необходимо, чтобы пакеты данных всегда были выровнены на 4 байта.
  // !! FatFS в некоторых случаЯх (уточнить!) выравниваниЯ не обеспечивает, поэтому здесь установлено ( MemDataAlignment = DMA_MDATAALIGN_BYTE ).
  // !! Далее, у STM32F ограничение на количество транзакий DMA до 65535. При этом FatFS при обращении на чтение/запись, работает с блоками от 1 до 128 секторов.
  // !! 128*512 == 65536, т.е. такой запрос не пролезет через DMA.
  // !! Однако в том же примере в DMA задействован FIFO на 4 операции: ( MemBurst = DMA_MBURST_INC4 ), ( PeriphBurst = DMA_MBURST_INC4 ).
  // !! Этот FIFO необходим, т.к. библиотечные функции HAL_SD_ReadBlocks_DMA() и HAL_SD_WriteBlocks_DMA() уже на него рассчитаны,
  // !! и при запросе операции на 65536 байт реально запрашивают у DMA 65536/4 транзакции.
  
  dmaRxHandle.Instance = SD_DMAx_Rx_STREAM;
  
  /* Associate the DMA handle */
  __HAL_LINKDMA(hsd, hdmarx, dmaRxHandle);
  
  /* Deinitialize the stream for new transfer */
  HAL_DMA_DeInit(&dmaRxHandle);
  
  /* Configure the DMA stream */
  HAL_DMA_Init(&dmaRxHandle);
  
  /* Configure DMA Tx parameters */
  dmaTxHandle.Init.Channel             = SD_DMAx_Tx_CHANNEL;
  dmaTxHandle.Init.Direction           = DMA_MEMORY_TO_PERIPH;
  dmaTxHandle.Init.PeriphInc           = DMA_PINC_DISABLE;
  dmaTxHandle.Init.MemInc              = DMA_MINC_ENABLE;
  dmaTxHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
//  dmaTxHandle.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
  dmaTxHandle.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
  dmaTxHandle.Init.Mode                = DMA_PFCTRL;
//  dmaTxHandle.Init.Priority            = DMA_PRIORITY_VERY_HIGH;
  dmaTxHandle.Init.Priority            = SD_DMAx_Tx_DMA_PRIORITY;
  dmaTxHandle.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
  dmaTxHandle.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
  dmaTxHandle.Init.MemBurst            = DMA_MBURST_INC4;
  dmaTxHandle.Init.PeriphBurst         = DMA_PBURST_INC4;
  
  dmaTxHandle.Instance = SD_DMAx_Tx_STREAM;
  
  /* Associate the DMA handle */
  __HAL_LINKDMA(hsd, hdmatx, dmaTxHandle);
  
  /* Deinitialize the stream for new transfer */
  HAL_DMA_DeInit(&dmaTxHandle);
  
  /* Configure the DMA stream */
  HAL_DMA_Init(&dmaTxHandle); 
  
  /* NVIC configuration for DMA transfer complete interrupt */
//  HAL_NVIC_SetPriority(SD_DMAx_Rx_IRQn, 0x0F, 0);
  HAL_NVIC_SetPriority(SD_DMAx_Rx_IRQn, SD_DMAx_Rx_IRQ_PREEMTPRIORITY, SD_DMAx_Rx_IRQ_SUBPRIORITY);
  HAL_NVIC_EnableIRQ(SD_DMAx_Rx_IRQn);
  
  /* NVIC configuration for DMA transfer complete interrupt */
//  HAL_NVIC_SetPriority(SD_DMAx_Tx_IRQn, 0x0F, 0);
  HAL_NVIC_SetPriority(SD_DMAx_Tx_IRQn, SD_DMAx_Tx_IRQ_PREEMTPRIORITY, SD_DMAx_Tx_IRQ_SUBPRIORITY);
  HAL_NVIC_EnableIRQ(SD_DMAx_Tx_IRQn);
}

void HAL_SD_MspDeInit(SD_HandleTypeDef *hsd)
{
	assert_param( &uSdHandle == hsd );
	HAL_SD_Abort( hsd );
/*/	SDMMC_CmdStopTransfer(hsd->Instance);
	__HAL_SD_DISABLE(hsd); 
	
	SDIO_PowerState_OFF(hsd->Instance);
*/
	__HAL_RCC_SDIO_CLK_DISABLE( );
	Delay_us( 2 );
	__HAL_RCC_SDIO_FORCE_RESET( );
	Delay_us( 2 );
	__HAL_RCC_SDIO_RELEASE_RESET( );

	HAL_GPIO_WritePin( SD_POWEREN_GPIO_PORT, SD_POWEREN_PIN, GPIO_PIN_SET );
}
#endif

/**
  * @brief  Gets the current SD card data status.
  * @retval Data transfer state.
  *          This value can be one of the following values:
  *            @arg  SD_TRANSFER_OK: No data transfer is acting
  *            @arg  SD_TRANSFER_BUSY: Data transfer is acting
  */
uint8_t BSP_SD_GetCardState(void)
{
//  return((HAL_SD_GetCardState(&uSdHandle) == HAL_SD_CARD_TRANSFER ) ? SD_TRANSFER_OK : SD_TRANSFER_BUSY);

	static volatile HAL_SD_CardStateTypedef SD_State = HAL_SD_CARD_READY;

	// НельЯ сразу использовать HAL_SD_GetCardState(), т.к. там нет проверки, что модуль SDIO не знанЯт выполнением операции
	if( HAL_SD_STATE_READY == HAL_SD_GetState( &uSdHandle ) )
  		if( HAL_SD_CARD_TRANSFER == ( SD_State = HAL_SD_GetCardState( &uSdHandle ) ) )
  			return SD_TRANSFER_OK;
	return SD_TRANSFER_BUSY;
}
  

/**
  * @brief  Get SD information about specific SD card.
  * @param  CardInfo: Pointer to HAL_SD_CardInfoTypedef structure
  * @retval None 
  */
void BSP_SD_GetCardInfo(HAL_SD_CardInfoTypeDef *CardInfo)
{
  /* Get SD card Information */
  HAL_SD_GetCardInfo(&uSdHandle, CardInfo);
}

void BSP_SD_GetCardCID( void )
{
	assert_param( 16 == sizeof( SD_CID ) );
	( ( uint32_t * ) &SD_CID )[0] = uSdHandle.CID[3];
	( ( uint32_t * ) &SD_CID )[1] = uSdHandle.CID[2];
	( ( uint32_t * ) &SD_CID )[2] = uSdHandle.CID[1];
	( ( uint32_t * ) &SD_CID )[3] = uSdHandle.CID[0];
}


/**
  * @brief SD Abort callbacks
  * @param hsd: SD handle
  * @retval None
  */
void HAL_SD_AbortCallback(SD_HandleTypeDef *hsd)
{
  BSP_SD_AbortCallback();
}

/**
  * @brief Tx Transfer completed callbacks
  * @param hsd: SD handle
  * @retval None
  */
#include "TaskConfig.h"
#include "common_gpio.h"
extern volatile uint32_t TimestampSDDMA;
void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
//	TimestampSDDMA = DWT_TimerGet( );
	BSP_SD_WriteCpltCallback();

	GPIO_Common_Write( iGPIO_TestPinSD, GPIO_PIN_RESET );
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	assert_param( pdPASS == xEventGroupSetBitsFromISR( EventGroup_System, EVENTSYSTEM_FS_DMA_WRITE_COMPLETE, &xHigherPriorityTaskWoken ) );
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/**
  * @brief Rx Transfer completed callbacks
  * @param hsd: SD handle
  * @retval None
  */
void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
{
//	TimestampSDDMA = DWT_TimerGet( );
	BSP_SD_ReadCpltCallback();

	GPIO_Common_Write( iGPIO_TestPinSD, GPIO_PIN_RESET );
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	assert_param( pdPASS == xEventGroupSetBitsFromISR( EventGroup_System, EVENTSYSTEM_FS_DMA_READ_COMPLETE, &xHigherPriorityTaskWoken ) );
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/**
  * @brief SD error callbacks
  * @param hsd: Pointer SD handle
  * @retval None
  */
void HAL_SD_ErrorCallback(SD_HandleTypeDef *hsd)
{
//	BSP_SD_ReadCpltCallback();

	GPIO_Common_Write( iGPIO_TestPinSD, GPIO_PIN_RESET );
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	assert_param( pdPASS == xEventGroupSetBitsFromISR( EventGroup_System, EVENTSYSTEM_FS_DMA_ERROR, &xHigherPriorityTaskWoken ) );
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}


/**
  * @brief BSP SD Abort callbacks
  * @retval None
  */
__weak void BSP_SD_AbortCallback(void)
{

}

/**
  * @brief BSP Tx Transfer completed callbacks
  * @retval None
  */
__weak void BSP_SD_WriteCpltCallback(void)
{

}

/**
  * @brief BSP Rx Transfer completed callbacks
  * @retval None
  */
__weak void BSP_SD_ReadCpltCallback(void)
{

}

// Обработчики прерываний взЯты из en.stm32cubef4.zip\STM32Cube_FW_F4_V1.15.0\Projects\STM324xG_EVAL\Applications\FatFs\FatFs_uSD\Src\stm32f4xx_it.c
void SDIO_IRQHandler(void)
{
  HAL_SD_IRQHandler(&uSdHandle);
}

#ifndef	SD_USE_SINGLE_DMA
void SD_DMAx_Tx_IRQHandler(void)
{
  HAL_DMA_IRQHandler(uSdHandle.hdmatx); 
}

void SD_DMAx_Rx_IRQHandler(void)
{
  HAL_DMA_IRQHandler(uSdHandle.hdmarx);
}

#else	// SD_USE_SINGLE_DMA
// Использован один канал DMA длЯ приема и длЯ передачи.
// Необходимо определЯть, в каком режиме (Perif->Mem или Mem->Perif) находитсЯ DMA,
void SD_DMAx_Tx_IRQHandler(void)
{
	if( ( SD_DMAx_Tx_CHANNEL == uSdHandle.hdmatx->Instance ) &&
		( DMA_MEMORY_TO_PERIPH == ( uSdHandle.hdmatx->Instance->CCR & DMA_CCR_DIR ) ) )
		HAL_DMA_IRQHandler( uSdHandle.hdmatx );
	else if( ( SD_DMAx_Rx_CHANNEL == uSdHandle.hdmarx->Instance ) &&
		( DMA_PERIPH_TO_MEMORY == ( uSdHandle.hdmarx->Instance->CCR & DMA_CCR_DIR ) ) )
		HAL_DMA_IRQHandler( uSdHandle.hdmarx );
	else
		assert_param( 0 );
}
#endif	// SD_USE_SINGLE_DMA

/**
  * @}
  */  

/**
  * @}
  */ 

/**
  * @}
  */ 

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
