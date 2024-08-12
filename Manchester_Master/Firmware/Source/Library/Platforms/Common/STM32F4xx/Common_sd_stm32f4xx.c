// Common_sd_stm32f4.c
// ���������� ���� ������� �� common_sd.c ��� ���� STM32F4
#include "ProjectConfig.h"
#include "common_sd.h"
#include "common_rcc.h"

#ifndef	STM32F4
#error "Only for STM32F4xx family!"
#endif

extern SD_HandleTypeDef uSdHandle;

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
  
  /* uSD device interface configuration */
  uSdHandle.Instance = SDIO;

  uSdHandle.Init.ClockEdge           = SDIO_CLOCK_EDGE_RISING;
  uSdHandle.Init.ClockBypass         = SDIO_CLOCK_BYPASS_DISABLE;
  uSdHandle.Init.ClockPowerSave      = SDIO_CLOCK_POWER_SAVE_DISABLE;
  uSdHandle.Init.BusWide             = SDIO_BUS_WIDE_1B;
  uSdHandle.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
#ifdef	SDIO_CLKDIV
	uSdHandle.Init.ClockDiv			= SDIO_CLKDIV;				// ������������ ��������� ������� SDIO_CK
#else
	uSdHandle.Init.ClockDiv			= SDIO_TRANSFER_CLK_DIV;	// ������� SDIO_CK ��-���������
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
    if(HAL_SD_ConfigWideBusOperation(&uSdHandle, SDIO_BUS_WIDE_4B) != HAL_OK)
    {
      SD_state = MSD_ERROR;
    }
    else
    {
      SD_state = MSD_OK;
    }
  }

//static volatile HAL_SD_CardCIDTypeDef SD_CardCID;
//HAL_SD_GetCardCID(&uSdHandle, ( HAL_SD_CardCIDTypeDef *) &SD_CardCID);

  
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
//__weak void BSP_SD_MspInit(SD_HandleTypeDef *hsd, void *Params)
void HAL_SD_MspInit(SD_HandleTypeDef *hsd)
{
  static DMA_HandleTypeDef dmaRxHandle __PLACE_AT_RAM_CCM__;
  static DMA_HandleTypeDef dmaTxHandle __PLACE_AT_RAM_CCM__;
  GPIO_InitTypeDef GPIO_Init_Structure;

	// �������� � ��������� Programmable Voltage Detector ��� ����������� ������������ ���������� ������� SD
	// !! PVD �����������
	HAL_PWR_EnablePVD( );
	PWR_PVDTypeDef PWR_PVD;
	PWR_PVD.Mode		= PWR_PVD_MODE_NORMAL;		// ����� ��� ��������, ��� ���������� � �������
#if	( defined( STM32F401xB ) || defined( STM32F401xC ) || defined( STM32F405xx ) || defined( STM32F407xx ) )	// ������ ��� ��������� ����� ��������!
	// !!����� ��������� �������!
	// !!��������� ���� �� �����������
//	PWR_PVD.PVDLevel	= PWR_PVDLEVEL_5;			// /2.93V, \2.84V +-0.07V	� ������ ������ ��������� �� 2.99V ��� ���������� �� 2.65V
	PWR_PVD.PVDLevel	= PWR_PVDLEVEL_6;			// /3.03V, \2.93V +-0.07V	� ������ ������ ��������� �� 3.10V ��� ���������� �� 2.85V
//	PWR_PVD.PVDLevel	= PWR_PVDLEVEL_7;			// /3.14V, \3.03V +-0.07V	� ������ ������ ��������� �� 3.21V ��� ���������� �� 2.95V
#endif
	HAL_PWR_ConfigPVD( &PWR_PVD );
	// !! ���� ���-�� ��������������, ��� ��������� PVD �� ����� �������� � ������ ������ ����������!

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
  // !! �����!!
  // !! ��-��������� � ������� stm324xg_eval_sd.c ���� ��������� ( MemDataAlignment = DMA_MDATAALIGN_WORD ).
  // !! ������� �������� �������, �� ����������, ����� ������ ������ ������ ���� ��������� �� 4 �����.
  // !! FatFS � ��������� ������� (��������!) ������������ �� ������������, ������� ����� ����������� ( MemDataAlignment = DMA_MDATAALIGN_BYTE ).
  // !! �����, � STM32F ����������� �� ���������� ��������� DMA �� 65535. ��� ���� FatFS ��� ��������� �� ������/������, �������� � ������� �� 1 �� 128 ��������.
  // !! 128*512 == 65536, �.�. ����� ������ �� �������� ����� DMA.
  // !! ������ � ��� �� ������� � DMA ������������ FIFO �� 4 ��������: ( MemBurst = DMA_MBURST_INC4 ), ( PeriphBurst = DMA_MBURST_INC4 ).
  // !! ���� FIFO ���������, �.�. ������������ ������� HAL_SD_ReadBlocks_DMA() � HAL_SD_WriteBlocks_DMA() ��� �� ���� ����������,
  // !! � ��� ������� �������� �� 65536 ���� ������� ����������� � DMA 65536/4 ����������.
  
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

