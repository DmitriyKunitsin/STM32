// Common_sd_stm32l4.c
// ���������� ���� ������� �� common_sd.c ��� ���� STM32L4
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

	// ����� �������������� ���������� �������� ������� SDIO, ���� �� ����� ��� ��� ���������������.
	// ��-��������, ��� ���� ������ � BSP_SD_DeInit(), ������ FatFS ����� ������ ���������������� ���������� ��� ������������,
	// � ������������������ ��� ��������������� �� �����.
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
	pInit->HardwareFlowControl	= SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;		// ENABLE ����� �� BSP for STM32L496G_DISCOVERY
#ifdef	SDIO_CLKDIV
	pInit->ClockDiv				= SDIO_CLKDIV;				// ������������ ��������� ������� SDIO_CK
#else
	pInit->ClockDiv				= SDMMC_TRANSFER_CLK_DIV;	// ������� SDIO_CK ��-���������
#endif	// SDIO_CLKDIV
  
  /* Check if the SD card is plugged in the slot */
  if(BSP_SD_IsDetected() != SD_PRESENT)
  {
    return MSD_ERROR;
  }
  
  /* Msp SD initialization */
//  BSP_SD_MspInit(&uSdHandle, NULL);
  // ���������, ��� ���������� ������� SD-����� � �����
  if( __HAL_PWR_GET_FLAG( PWR_FLAG_PVDO ) )
	  return MSD_ERROR;

  if(HAL_SD_Init(&uSdHandle) != HAL_OK)
  {
    SD_state = MSD_ERROR;
  }

  // ���������, ��� ���������� ������� SD-����� � �����
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
������� ��������� SD-����� (Power Cycle)
1. ���������� �������:
  - �� VDD ���������� ����� 0.5 �;
  - DAT, CMD, SCK � Z-��������� ��� ��������� � 0;
  - ����� ��� ������ �� ����� 1 ��.
2. ��������� �������:
  - ���������� ������� ���������� �� 2.7 �� 3.6 �;
  - ����� ���������� ���������� �� �������� - �� 0.1 �� �� 35 ��;
  - ���������� ��-����������� �������;
  - ����������� �������� SCK (?);
  - �� ������ ������ ������� ���������� �������� �� ����� 74 SCK ��� ���������� ����� CMD.

���� ������� ��������� ������� �� ������ SD:
  - SD �������� ����� P-��������� MOSFET, ����������� ����� SD_POWEREN_GPIO_PORT.SD_POWEREN_PIN
  - �� ������� SD ���������� ����������� 2.2 ��� (����������� ���������) � �������� 2.4 ��� (������ ������� ����� ����������)
  - ����� ������� ������� ����� �������� �� 0.5 � ���������� ������� 12 �� (2RC) - �.�. �� ��� ����� (� �����) ���������� ��������� ������� SD ��� ������

�������� �� ��������� ����� ���� 516_03 (��� 3.3 ���):
- ��� ��������� ���������� ��������� �� 5 ���
- ��� ���������� ���������� ������� �� 1.2 � �� 3 ��, �� 0.5 � �� 20 ��
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

	// �������� � ��������� Programmable Voltage Detector ��� ����������� ������������ ���������� ������� SD
	// !! PVD �����������
	HAL_PWR_EnablePVD( );
	PWR_PVDTypeDef PWR_PVD;
	PWR_PVD.Mode		= PWR_PVD_MODE_NORMAL;		// ����� ��� ��������, ��� ���������� � �������
//#if	( defined( STM32F401xB ) || defined( STM32F401xC ) || defined( STM32F405xx ) || defined( STM32F407xx ) )	// ������ ��� ��������� ����� ��������!
//#if	( defined( STM32F401xB ) || defined( STM32F401xC ) || defined( STM32F405xx ) || defined( STM32F407xx ) )	// ������ ��� ��������� ����� ��������!
	// !!����� ��������� �������!
	// !!��������� ���� �� �����������
//	PWR_PVD.PVDLevel	= PWR_PVDLEVEL_5;			// /2.93V, \2.84V +-0.07V	� ������ ������ ��������� �� 2.99V ��� ���������� �� 2.65V
	PWR_PVD.PVDLevel	= PWR_PVDLEVEL_6;			// /3.03V, \2.93V +-0.07V	� ������ ������ ��������� �� 3.10V ��� ���������� �� 2.85V
//	PWR_PVD.PVDLevel	= PWR_PVDLEVEL_7;			// /3.14V, \3.03V +-0.07V	� ������ ������ ��������� �� 3.21V ��� ���������� �� 2.95V
//#endif
	HAL_PWR_ConfigPVD( &PWR_PVD );
	// !! ���� ���-�� ��������������, ��� ��������� PVD �� ����� �������� � ������ ������ ����������!

	// ******************
	// ������������ SDMMC1 � ����� STM32L4xx ������������ �� �� APB1 � APB2,
	// ������� ������������� ������������ �������� �� SystemClock_Config()
	// ������ ������, ����� ����������� ���������� ������������� � ��������-��������� ������!
	// � �������� ���(�) ������� ������������ CLK48 �� PLL.Q, � ��� ����� 32.768 ���
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

/*	// ������ ������������ CLK48 �� HSI48, �.�. ������������ �� PLL.Q ������ ����� ����� (???)
	// ������� ��������� ��������� HSI48
	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_OscInitStruct.OscillatorType		= RCC_OSCILLATORTYPE_HSI48;		// �������� ������������ ������ HSI48 
	RCC_OscInitStruct.HSI48State			= RCC_HSI48_ON;
	RCC_OscInitStruct.PLL.PLLState			= RCC_PLL_NONE;					// ������������ PLL �� �������
	assert_param( HAL_OK == HAL_RCC_OscConfig( &RCC_OscInitStruct ) );
	// ��������� ������������ CLK48 �� HSI48
	RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;
	RCC_PeriphClkInit.PeriphClockSelection	= RCC_PERIPHCLK_SDMMC1;
	RCC_PeriphClkInit.Sdmmc1ClockSelection	= RCC_SDMMC1CLKSOURCE_HSI48;
	assert_param( HAL_OK == HAL_RCCEx_PeriphCLKConfig( &RCC_PeriphClkInit ) );
*/

	// ������ ������������ CLK48 �� MSI RC, �.�. ������������ �� PLL.Q ������ ����� ����� (???)
	// ������� ��������� ��������� MSI RC �� ������� 32 ���
	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_OscInitStruct.OscillatorType		= RCC_OSCILLATORTYPE_MSI;		// �������� ������������ ������ MSI 
	RCC_OscInitStruct.MSIState				= RCC_MSI_ON;
	RCC_OscInitStruct.MSICalibrationValue	= RCC_MSICALIBRATION_DEFAULT;	// ???
//	RCC_OscInitStruct.MSIClockRange			= RCC_MSIRANGE_10;				// 32 MHz
//	RCC_OscInitStruct.MSIClockRange			= RCC_MSIRANGE_8;				// 16 MHz
	RCC_OscInitStruct.MSIClockRange			= RCC_MSIRANGE_6;				// 4 MHz
	RCC_OscInitStruct.PLL.PLLState			= RCC_PLL_NONE;					// ������������ PLL �� �������
	assert_param( HAL_OK == HAL_RCC_OscConfig( &RCC_OscInitStruct ) );
	// ��������� ������������ CLK48 �� MSI
	RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;
	RCC_PeriphClkInit.PeriphClockSelection	= RCC_PERIPHCLK_SDMMC1;
	RCC_PeriphClkInit.Sdmmc1ClockSelection	= RCC_SDMMC1CLKSOURCE_MSI;
	assert_param( HAL_OK == HAL_RCCEx_PeriphCLKConfig( &RCC_PeriphClkInit ) );


	/* Enable SDIO clock */
	__HAL_RCC_SDMMC1_CLK_ENABLE( );
  
	/* Enable DMA2 clocks */
	DMA_CLK_ENABLE( SD_DMAx_Rx_CHANNEL );	// __HAL_RCC_DMA2_CLK_ENABLE()
	// ��� ������ DMA ����������� ������ ������ DMA, ��� ��� ������������ ���������� ������ �� ����� �� �������
	
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
	// ��������� �������
	HAL_GPIO_WritePin( SD_POWEREN_GPIO_PORT, SD_POWEREN_PIN, GPIO_PIN_SET );
	HAL_GPIO_Init(SD_POWEREN_GPIO_PORT, &GPIO_Init_Structure);
	// �������� ������� ����� �����
	HAL_Delay( 20 );		// !!! ����� ������� ������ �� ������� ������� ������������ 2.2 ��� ���������� 2.4 ���! ��� ������ ����� �������, ����� ����� ���� ������!
	HAL_GPIO_WritePin( SD_POWEREN_GPIO_PORT, SD_POWEREN_PIN, GPIO_PIN_RESET );
	HAL_Delay( 1 );
#endif	// SD_POWEREN_GPIO_PORT
  
  /* Common GPIO configuration */
  GPIO_Init_Structure.Mode      = GPIO_MODE_AF_PP;
  GPIO_Init_Structure.Pull      = GPIO_PULLUP;
  GPIO_Init_Structure.Speed     = GPIO_SPEED_FREQ_HIGH;		// GPIO_SPEED_FREQ_VERY_HIGH ��� CLK48 >= 48 MHz
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
	// ���� ��� ���������� SDMMC1 ������������ ������ ������ DMA, ����� �� �� ����������������
	assert_param( HAL_OK == SD_DMAConfigRx( hsd ) );
	assert_param( HAL_OK == SD_DMAConfigTx( hsd ) );
#else
	// ���� ��� ���������� SDMMC1 ����������� ���� ����� DMA, ������������� �������� ����������� ������ ��� ����� ������� �������� ��� ������
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

	// ��������� ���� SDIO, ����� ����� ��� �� ������������� �������
	GPIO_InitTypeDef GPIO_Init_Structure;
	GPIO_Init_Structure.Mode	= GPIO_MODE_INPUT;
	GPIO_Init_Structure.Pull	= GPIO_PULLDOWN;
	GPIO_Init_Structure.Speed	= GPIO_SPEED_FREQ_LOW;
	GPIO_Init_Structure.Pin		= GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
	HAL_GPIO_Init( GPIOC, &GPIO_Init_Structure );
	GPIO_Init_Structure.Pin		= GPIO_PIN_2;
	HAL_GPIO_Init( GPIOD, &GPIO_Init_Structure );

	// ����� ������� � SD-�����
	HAL_GPIO_WritePin( SD_POWEREN_GPIO_PORT, SD_POWEREN_PIN, GPIO_PIN_SET );
}

// !! �����!!
// !! ��-��������� � ������� stm324xg_eval_sd.c ���� ��������� ( MemDataAlignment = DMA_MDATAALIGN_WORD ).
// !! ������� �������� �������, �� ����������, ����� ������ ������ ������ ���� ��������� �� 4 �����.
// !! FatFS � ��������� ������� (��������!) ������������ �� ������������, ������� ����� ����������� ( MemDataAlignment = DMA_MDATAALIGN_BYTE ).
// !! �����, � STM32F ����������� �� ���������� ��������� DMA �� 65535. ��� ���� FatFS ��� ��������� �� ������/������, �������� � ������� �� 1 �� 128 ��������.
// !! 128*512 == 65536, �.�. ����� ������ �� �������� ����� DMA.
// !! ������ � ��� �� ������� � DMA ������������ FIFO �� 4 ��������: ( MemBurst = DMA_MBURST_INC4 ), ( PeriphBurst = DMA_MBURST_INC4 ).
// !! ���� FIFO ���������, �.�. ������������ ������� HAL_SD_ReadBlocks_DMA() � HAL_SD_WriteBlocks_DMA() ��� �� ���� ����������,
// !! � ��� ������� �������� �� 65536 ���� ������� ����������� � DMA 65536/4 ����������.

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

