// Common_sd_stm32l4.c
// РеализациЯ рЯда функций из common_sd.c под Ядро STM32L4
#include "ProjectConfig.h"
#include "common_sd.h"
#include "common_rcc.h"

#ifndef	STM32L4
#error "Only for STM32L4xx family!"
#endif

extern SD_HandleTypeDef uSdHandle;

//static HAL_StatusTypeDef SD_DMAConfigRx(SD_HandleTypeDef *hsd);
//static HAL_StatusTypeDef SD_DMAConfigTx(SD_HandleTypeDef *hsd);

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

	// uSD device interface configuration
	uSdHandle.Instance = SDMMC1;
	SDMMC_InitTypeDef *pInit = &uSdHandle.Init;
	pInit->ClockEdge			= SDMMC_CLOCK_EDGE_RISING;
	pInit->ClockBypass			= SDMMC_CLOCK_BYPASS_DISABLE;
	pInit->ClockPowerSave		= SDMMC_CLOCK_POWER_SAVE_DISABLE;
	pInit->BusWide				= SDMMC_BUS_WIDE_1B;
//	pInit->HardwareFlowControl	= SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
	pInit->HardwareFlowControl	= SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;		// ENABLE взЯто из BSP for STM32L496G_DISCOVERY
#ifdef	SDIO_CLKDIV
	pInit->ClockDiv				= SDIO_CLKDIV;				// Опциональное замеление частоты SDIO_CK
#else
	pInit->ClockDiv				= SDMMC_TRANSFER_CLK_DIV;	// Частота SDIO_CK по-умолчанию
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
    if(HAL_SD_ConfigWideBusOperation(&uSdHandle, SDMMC_BUS_WIDE_4B) != HAL_OK)
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
void HAL_SD_MspInit(SD_HandleTypeDef *hsd)
{
	static DMA_HandleTypeDef dmaRxHandle;
	static DMA_HandleTypeDef dmaTxHandle;
	GPIO_InitTypeDef GPIO_Init_Structure;

	// Включить и настроить Programmable Voltage Detector длЯ определениЯ допустимости напрЯжениЯ питаниЯ SD
	// !! PVD отсутствует
	HAL_PWR_EnablePVD( );
	PWR_PVDTypeDef PWR_PVD;
	PWR_PVD.Mode		= PWR_PVD_MODE_NORMAL;		// режим длЯ поллинга, без прерываний и событий
//#if	( defined( STM32F401xB ) || defined( STM32F401xC ) || defined( STM32F405xx ) || defined( STM32F407xx ) )	// Пороги длЯ остальных чипов уточнить!
//#if	( defined( STM32F401xB ) || defined( STM32F401xC ) || defined( STM32F405xx ) || defined( STM32F407xx ) )	// Пороги длЯ остальных чипов уточнить!
	// !!Также проверить натурно!
	// !!Проверить уход по температуре
//	PWR_PVD.PVDLevel	= PWR_PVDLEVEL_5;			// /2.93V, \2.84V +-0.07V	в худшем случае включитсЯ на 2.99V или отключитсЯ на 2.65V
	PWR_PVD.PVDLevel	= PWR_PVDLEVEL_6;			// /3.03V, \2.93V +-0.07V	в худшем случае включитсЯ на 3.10V или отключитсЯ на 2.85V
//	PWR_PVD.PVDLevel	= PWR_PVDLEVEL_7;			// /3.14V, \3.03V +-0.07V	в худшем случае включитсЯ на 3.21V или отключитсЯ на 2.95V
//#endif
	HAL_PWR_ConfigPVD( &PWR_PVD );
	// !! надо как-то контролировать, что настройка PVD не будет изменена в других частЯх приложениЯ!

	// ******************
	// Тактирование SDMMC1 в серии STM32L4xx производитсЯ не от APB1 и APB2,
	// поэтому инициализациЯ тактированиЯ вынесена из SystemClock_Config()
	// Вообще говорЯ, такие особенности необходимо реализовывать в проектно-зависимой секции!
	// В проектах АКП(б) выбрано тактирование CLK48 от PLL.Q, и оно равно 32.768 МГц
#ifndef	PROJECTCONFIG_CLOCK_16M4_65M5
#error "SDMMC oscillator is hard tailored to 16.384 MHz design!"
#endif
	// ******************
/*	RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;
	HAL_RCCEx_GetPeriphCLKConfig( &RCC_PeriphClkInit );
	// Configure the SDMMC1 clock source. The clock is derived from the PLLSAI1
	// Hypothesis is that PLLSAI1 VCO input is 8Mhz
	RCC_PeriphClkInit.PeriphClockSelection		= RCC_PERIPHCLK_SDMMC1;
	RCC_PeriphClkInit.PLLSAI1.PLLSAI1N			= 24;
	RCC_PeriphClkInit.PLLSAI1.PLLSAI1Q			= 4;
	RCC_PeriphClkInit.PLLSAI1.PLLSAI1ClockOut	= RCC_PLLSAI1_48M2CLK;
	RCC_PeriphClkInit.Sdmmc1ClockSelection		= RCC_SDMMC1CLKSOURCE_PLLSAI1;
	assert_param( HAL_OK == HAL_RCCEx_PeriphCLKConfig( &RCC_PeriphClkInit) );
*/

/*	RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;
	RCC_PeriphClkInit.PeriphClockSelection	= RCC_PERIPHCLK_SDMMC1;
	RCC_PeriphClkInit.Sdmmc1ClockSelection	= RCC_SDMMC1CLKSOURCE_PLL;
	assert_param( HAL_OK == HAL_RCCEx_PeriphCLKConfig( &RCC_PeriphClkInit) );
*/

/*	// Пробую тактирование CLK48 от HSI48, т.к. тактирование от PLL.Q давало много сбоев (???)
	// Сначала запустить генератор HSI48
	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_OscInitStruct.OscillatorType		= RCC_OSCILLATORTYPE_HSI48;		// изменить конфигурацию только HSI48 
	RCC_OscInitStruct.HSI48State			= RCC_HSI48_ON;
	RCC_OscInitStruct.PLL.PLLState			= RCC_PLL_NONE;					// конфигурацию PLL не трогать
	assert_param( HAL_OK == HAL_RCC_OscConfig( &RCC_OscInitStruct ) );
	// Запустить тактирование CLK48 от HSI48
	RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;
	RCC_PeriphClkInit.PeriphClockSelection	= RCC_PERIPHCLK_SDMMC1;
	RCC_PeriphClkInit.Sdmmc1ClockSelection	= RCC_SDMMC1CLKSOURCE_HSI48;
	assert_param( HAL_OK == HAL_RCCEx_PeriphCLKConfig( &RCC_PeriphClkInit ) );
*/

	// Пробую тактирование CLK48 от MSI RC, т.к. тактирование от PLL.Q давало много сбоев (???)
	// Сначала запустить генератор MSI RC на частоте 32 МГц
	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_OscInitStruct.OscillatorType		= RCC_OSCILLATORTYPE_MSI;		// изменить конфигурацию только MSI 
	RCC_OscInitStruct.MSIState				= RCC_MSI_ON;
	RCC_OscInitStruct.MSICalibrationValue	= RCC_MSICALIBRATION_DEFAULT;	// ???
//	RCC_OscInitStruct.MSIClockRange			= RCC_MSIRANGE_10;				// 32 MHz
//	RCC_OscInitStruct.MSIClockRange			= RCC_MSIRANGE_8;				// 16 MHz
	RCC_OscInitStruct.MSIClockRange			= RCC_MSIRANGE_6;				// 4 MHz
	RCC_OscInitStruct.PLL.PLLState			= RCC_PLL_NONE;					// конфигурацию PLL не трогать
	assert_param( HAL_OK == HAL_RCC_OscConfig( &RCC_OscInitStruct ) );
	// Запустить тактирование CLK48 от MSI
	RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;
	RCC_PeriphClkInit.PeriphClockSelection	= RCC_PERIPHCLK_SDMMC1;
	RCC_PeriphClkInit.Sdmmc1ClockSelection	= RCC_SDMMC1CLKSOURCE_MSI;
	assert_param( HAL_OK == HAL_RCCEx_PeriphCLKConfig( &RCC_PeriphClkInit ) );


	/* Enable SDIO clock */
	__HAL_RCC_SDMMC1_CLK_ENABLE( );
  
	/* Enable DMA2 clocks */
	DMA_CLK_ENABLE( SD_DMAx_Rx_CHANNEL );	// __HAL_RCC_DMA2_CLK_ENABLE()
	// Оба канала DMA принадлежат одному модулю DMA, так что тактирование достаточно подать на любой из каналов
	
	/* Enable GPIOs clock */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
//	__SD_DETECT_GPIO_CLK_ENABLE();

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
  GPIO_Init_Structure.Speed     = GPIO_SPEED_FREQ_HIGH;		// GPIO_SPEED_FREQ_VERY_HIGH длЯ CLK48 >= 48 MHz
  GPIO_Init_Structure.Alternate = GPIO_AF12_SDMMC1;
  
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
  HAL_NVIC_SetPriority(SDMMC1_IRQn, SDIO_IRQ_PREEMTPRIORITY, SDIO_IRQ_SUBPRIORITY);
  HAL_NVIC_EnableIRQ(SDMMC1_IRQn);
    
#ifndef	SD_USE_SINGLE_DMA
	// Если длЯ реализации SDMMC1 использованы разные каналы DMA, сразу же их инициализировать
	assert_param( HAL_OK == SD_DMAConfigRx( hsd ) );
	assert_param( HAL_OK == SD_DMAConfigTx( hsd ) );
#else
	// Если длЯ реализации SDMMC1 использован один канал DMA, инициализацию придетсЯ производить каждый раз перед началом передачи или приема
#endif

}

void HAL_SD_MspDeInit(SD_HandleTypeDef *hsd)
{
	assert_param( &uSdHandle == hsd );
	HAL_SD_Abort( hsd );
/*/	SDMMC_CmdStopTransfer(hsd->Instance);
	__HAL_SD_DISABLE(hsd); 
	
	SDIO_PowerState_OFF(hsd->Instance);
*/
	__HAL_RCC_SDMMC1_CLK_DISABLE( );
	Delay_us( 2 );
	__HAL_RCC_SDMMC1_FORCE_RESET( );
	Delay_us( 2 );
	__HAL_RCC_SDMMC1_RELEASE_RESET( );

	// Отключить пины SDIO, чтобы через них не подсасывалось питание
	GPIO_InitTypeDef GPIO_Init_Structure;
	GPIO_Init_Structure.Mode	= GPIO_MODE_INPUT;
	GPIO_Init_Structure.Pull	= GPIO_PULLDOWN;
	GPIO_Init_Structure.Speed	= GPIO_SPEED_FREQ_LOW;
	GPIO_Init_Structure.Pin		= GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
	HAL_GPIO_Init( GPIOC, &GPIO_Init_Structure );
	GPIO_Init_Structure.Pin		= GPIO_PIN_2;
	HAL_GPIO_Init( GPIOD, &GPIO_Init_Structure );

	// СнЯть питание с SD-карты
	HAL_GPIO_WritePin( SD_POWEREN_GPIO_PORT, SD_POWEREN_PIN, GPIO_PIN_SET );
}

// !! Важно!!
// !! По-умолчанию в примере stm324xg_eval_sd.c была настройка ( MemDataAlignment = DMA_MDATAALIGN_WORD ).
// !! Словами работать быстрее, но необходимо, чтобы пакеты данных всегда были выровнены на 4 байта.
// !! FatFS в некоторых случаЯх (уточнить!) выравниваниЯ не обеспечивает, поэтому здесь установлено ( MemDataAlignment = DMA_MDATAALIGN_BYTE ).
// !! Далее, у STM32F ограничение на количество транзакий DMA до 65535. При этом FatFS при обращении на чтение/запись, работает с блоками от 1 до 128 секторов.
// !! 128*512 == 65536, т.е. такой запрос не пролезет через DMA.
// !! Однако в том же примере в DMA задействован FIFO на 4 операции: ( MemBurst = DMA_MBURST_INC4 ), ( PeriphBurst = DMA_MBURST_INC4 ).
// !! Этот FIFO необходим, т.к. библиотечные функции HAL_SD_ReadBlocks_DMA() и HAL_SD_WriteBlocks_DMA() уже на него рассчитаны,
// !! и при запросе операции на 65536 байт реально запрашивают у DMA 65536/4 транзакции.

//HAL_StatusTypeDef SD_DMAConfigRx(SD_HandleTypeDef *hsd);
//HAL_StatusTypeDef SD_DMAConfigTx(SD_HandleTypeDef *hsd);

/**
  * @brief Configure the DMA to receive data from the SD card
  * @retval
  *  HAL_ERROR or HAL_OK
  */
/*static*/ HAL_StatusTypeDef SD_DMAConfigRx(SD_HandleTypeDef *hsd)
{
  static DMA_HandleTypeDef hdma_rx;
  HAL_StatusTypeDef status = HAL_ERROR;

  /* Configure DMA Rx parameters */
  hdma_rx.Init.Request             = SD_DMAx_Rx_REQUEST;		// DMA_REQUEST_7
  hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
  hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
  hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
  hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
//  hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;		// DMA_MDATAALIGN_WORD
  hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;		// DMA_MDATAALIGN_WORD
  hdma_rx.Init.Priority            = SD_DMA_PRIORITY;			// DMA_PRIORITY_VERY_HIGH

  hdma_rx.Instance = SD_DMAx_Rx_CHANNEL;

  /* Associate the DMA handle */
  __HAL_LINKDMA(hsd, hdmarx, hdma_rx);

  /* Stop any ongoing transfer and reset the state*/
  HAL_DMA_Abort(&hdma_rx);
//  assert_param( HAL_OK == HAL_DMA_Abort(&hdma_rx) );

  /* Deinitialize the Channel for new transfer */
//  HAL_DMA_DeInit(&hdma_rx);
  assert_param( HAL_OK == HAL_DMA_DeInit(&hdma_rx));

  /* Configure the DMA Channel */
  status = HAL_DMA_Init(&hdma_rx);

  /* NVIC configuration for DMA transfer complete interrupt */
  HAL_NVIC_SetPriority(SD_DMAx_Rx_IRQn, SD_DMAx_Rx_IRQ_PREEMTPRIORITY, SD_DMAx_Rx_IRQ_SUBPRIORITY);
  HAL_NVIC_EnableIRQ(SD_DMAx_Rx_IRQn);

  return (status);
}

/**
  * @brief Configure the DMA to transmit data to the SD card
  * @retval
  *  HAL_ERROR or HAL_OK
  */
/*static*/ HAL_StatusTypeDef SD_DMAConfigTx(SD_HandleTypeDef *hsd)
{
  static DMA_HandleTypeDef hdma_tx;
  HAL_StatusTypeDef status;

  /* Configure DMA Tx parameters */
  hdma_tx.Init.Request             = SD_DMAx_Tx_REQUEST;		// DMA_REQUEST_7
  hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
  hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
  hdma_tx.Init.MemInc              = DMA_MINC_ENABLE;
  hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
//  hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;		// DMA_MDATAALIGN_WORD
  hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;		// DMA_MDATAALIGN_WORD
  hdma_tx.Init.Priority            = SD_DMA_PRIORITY;			// DMA_PRIORITY_VERY_HIGH

  hdma_tx.Instance = SD_DMAx_Tx_CHANNEL;

  /* Associate the DMA handle */
  __HAL_LINKDMA(hsd, hdmatx, hdma_tx);

  /* Stop any ongoing transfer and reset the state*/
  HAL_DMA_Abort(&hdma_tx);
//  assert_param( HAL_OK == HAL_DMA_Abort(&hdma_tx));

  /* Deinitialize the Channel for new transfer */
//  HAL_DMA_DeInit(&hdma_tx);
  assert_param( HAL_OK == HAL_DMA_DeInit(&hdma_tx));

  /* Configure the DMA Channel */
  status = HAL_DMA_Init(&hdma_tx);

  /* NVIC configuration for DMA transfer complete interrupt */
  HAL_NVIC_SetPriority(SD_DMAx_Tx_IRQn, SD_DMAx_Tx_IRQ_PREEMTPRIORITY, SD_DMAx_Tx_IRQ_SUBPRIORITY);
  HAL_NVIC_EnableIRQ(SD_DMAx_Tx_IRQn);

  return (status);
}

