// Driver_ADS1231.c
// ������� ��� ADS1231
#include "ProjectConfig.h"
#include "stm32xxxx_hal.h"		// ����� ���������
#include "Driver_ADS1231.h"		// ������
#include "Platform_Common.h"
#include "common_GPIO.h"
#include "FreeRTOS.h"
#include "task.h"

static SPI_HandleTypeDef ADS1231_SPI_hdl = { 0 };
static SPI_HandleTypeDef * const pADS1231_SPI_hdl = &ADS1231_SPI_hdl;

// ���������� ���� ������������ ��� ADS1231 �� 10/80 SPS
void ADS1231_SetSPS( ADS1231_SPS_t SPS )
{
	switch( SPS )
	{
	case ADS1231_SPS_10:	GPIO_Common_Write( iGPIO_ADC_SPD, GPIO_PIN_RESET );	break;
	case ADS1231_SPS_80:	GPIO_Common_Write( iGPIO_ADC_SPD, GPIO_PIN_SET );	break;
	default:				assert_param( 0 );
	}
}

// ���������� ������� ������ � ADS1231.nDRDY/DOUT
// ( bIRQ_En == false )		ADS1231.DOUT	SPI.MOSI	����� ������ ����� SPI � �������������� ������
// ( bIRQ_En != false )		ADS1231.nDRDY	EXTI		�������� ������� ������ ��� ������� ���������
void ADS1231_IRQ_Enable( bool bIRQ_En )
{
	GPIO_InitTypeDef  GPIO_InitStruct;
	GPIO_InitStruct.Pin			= ADS1231_SPI_MISO_PIN;
	GPIO_InitStruct.Pull		= GPIO_PULLDOWN;
	GPIO_InitStruct.Speed		= GPIO_SPEED_FREQ_HIGH;
	if( bIRQ_En )
	{	// ( bIRQ_En != false ) 	ADS1231.nDRDY	EXTI		�������� ������� ������ ��� ������� ���������
		// ���������� ����� ������� ���������� �� ������� ������
		GPIO_InitStruct.Mode		= GPIO_MODE_IT_FALLING;
		// ��� ������ HAL_GPIO_Init() ����� ��������� ���������� �� ����, �������������� ����� ������� ���� ������������ ��������
	}
	else
	{	// ( bIRQ_En == false )		ADS1231.DOUT	SPI.MOSI	����� ������ ����� SPI � �������������� ������
		// ���������� ����� SPI.MOSI
		GPIO_InitStruct.Mode		= GPIO_MODE_AF_PP;
		GPIO_InitStruct.Alternate	= ADS1231_SPI_MISO_AF;
		// ��� ������ HAL_GPIO_Init() ����������� �� EXTI �� ������������, ������� ���������� ������������� ������ ���������� �� ����
		EXTI->IMR1 &= ~ADS1231_SPI_MISO_PIN;
	}
	// ���������� �������������������� ����
	//taskENTER_CRITICAL( );
	HAL_GPIO_Init( ADS1231_SPI_MISO_GPIO, &GPIO_InitStruct );	// �������� HAL_GPIO_Init() �������� ����� ����������� �������� ���������������� ����� � �����
	//taskEXIT_CRITICAL( );
	// ��� ������������� ��������� ���������� EXTI#
	if( bIRQ_En )
	{
		HAL_NVIC_SetPriority( ADS1231_IRQn, ADS1231_IRQ_PREEMTPRIORITY, ADS1231_IRQ_SUBPRIORITY );
		HAL_NVIC_EnableIRQ( ADS1231_IRQn );
		// ��� ��������� ���������� ���������� �������������� ������ ������� ADS1231_EXTI_Callback(),
		// ������� ����� ������ �� ��������������� ����������� EXTI#.
	}
}

__weak void ADS1231_EXTI_Callback( void )
{
}

// ������������� ������ � ��� ADS1231
// !!! ��� ����������� �� ����� 687_01, �� ������� ����������� DCDC ��������������� �������.
// !!! ��� ��������� ��� ���������� �������� ���������������.
// !!! ���������� ���������� ������ ������������ �� ������ BKS_MainTask( ), ����� ��������� ��� �� ������������.
bool ADS1231_Init( void )
{
	bool bResult = false;
	do
	{
		pADS1231_SPI_hdl->Instance = ADS1231_SPI;
		
		SPI_InitTypeDef *pInit = &pADS1231_SPI_hdl->Init;
		pInit->Direction			= SPI_DIRECTION_2LINES_RXONLY;
		pInit->CLKPhase 			= SPI_PHASE_2EDGE;
		pInit->CLKPolarity			= SPI_POLARITY_HIGH;
		pInit->CRCCalculation		= SPI_CRCCALCULATION_DISABLE;
		pInit->CRCPolynomial		= 1;
		pInit->DataSize 			= SPI_DATASIZE_8BIT;
		pInit->FirstBit 			= SPI_FIRSTBIT_MSB;
		pInit->NSS					= SPI_NSS_SOFT;
		pInit->TIMode				= SPI_TIMODE_DISABLE;
		pInit->Mode 				= SPI_MODE_MASTER;
		pInit->BaudRatePrescaler	= SPI_BAUDRATEPRESCALER_32;		// 1 ��� ��� 2 ���
#warning "SPI Baudrate must set according to real CLK!"
		
		HAL_SPI_Init( pADS1231_SPI_hdl );
		
		// ��������� ������������ � �������� BKS_MainTask( );
//		ADS1231_PowerOn( true );
		// ���������� �������� ������������� ���
		ADS1231_SetSPS( ADS1231_SPS_80 );
		// ��������� ���, � �������� ��������� ADS1231.nDRDY/DOUT, � ����� EXTI
		ADS1231_IRQ_Enable( true );

		bResult = true;
	} while( 0 );
	return bResult;	
}

void HAL_SPI_MspInit( SPI_HandleTypeDef *hspi )
{
	assert_param( hspi == pADS1231_SPI_hdl );

	GPIO_InitTypeDef  GPIO_InitStruct;
	
	/*##-1- Enable peripherals and GPIO Clocks #################################*/
	/* Enable GPIO TX/RX clock */
//	GPIO_CLK_ENABLE( ADS1231_SPI_MISO_GPIO );
	GPIO_CLK_ENABLE( ADS1231_SPI_MISO_GPIO );
	GPIO_CLK_ENABLE( ADS1231_SPI_SCK_GPIO );
	/* Enable SPI clock */
	SPI_CLK_ENABLE( ADS1231_SPI ); 
	
	/*##-2- Configure peripheral GPIO ##########################################*/	
	/* SPI SCK GPIO pin configuration  */
	GPIO_InitStruct.Pin			= ADS1231_SPI_SCK_PIN;
	GPIO_InitStruct.Mode		= GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull		= GPIO_PULLDOWN;
	GPIO_InitStruct.Speed		= GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate	= ADS1231_SPI_SCK_AF;
	HAL_GPIO_Init( ADS1231_SPI_SCK_GPIO, &GPIO_InitStruct );
	  
	/* SPI MISO GPIO pin configuration	*/
//	GPIO_InitStruct.Pin			= ADS1231_SPI_MISO_PIN;
//	GPIO_InitStruct.Alternate	= ADS1231_SPI_MISO_AF;
//	HAL_GPIO_Init( ADS1231_SPI_MISO_GPIO, &GPIO_InitStruct );
	
	/* SPI MOSI GPIO pin configuration	*/
	GPIO_InitStruct.Pin			= ADS1231_SPI_MISO_PIN;
	GPIO_InitStruct.Alternate	= ADS1231_SPI_MISO_AF;
	HAL_GPIO_Init( ADS1231_SPI_MISO_GPIO, &GPIO_InitStruct );
	
	/*##-3- Configure the NVIC for SPI #########################################*/
	/* NVIC for SPI */
//	HAL_NVIC_SetPriority(SPIx_IRQn, 0, 1);
//	HAL_NVIC_EnableIRQ(SPIx_IRQn);
}

// ��������� ������ �� �������� ��� �� SPI
// ���� �� ������� �������, ������� ����� ��������� ��������
// *pDataADC	- ��������� � �������� ��� 24 ���, �� ������
bool ADS1231_DataRead( int32_t *pDataADC )
{
	bool bResult = false;
	static volatile int32_t ADS1231_DataADC_Saved = 0;	// ����������� ���������� �������� ��� ��� �������� � ������ ������
	GPIO_Common_Write( iGPIO_TestPinADS1231, GPIO_PIN_RESET );		// �������� �������� ������, ����� ������������ ��� CS ��� �������� SPI � �����������
	do
	{
		// ��������� ���������� ������ � ��� (ADS1231.nDRDY/DOUT �������)
		if( GPIO_PIN_RESET != HAL_GPIO_ReadPin( ADS1231_SPI_MISO_GPIO, ADS1231_SPI_MISO_PIN ) )
			break;
		// ��������� ���, � �������� ��������� ADS1231.nDRDY/DOUT, � ����� SPI.MISO
		ADS1231_IRQ_Enable( false );
		// ���������� ������ ������ ����� SPI.
		// ������ ����� ��� ���������� 24 ����, �� ���������� ������� ���� �� 25 ���, ����� ADS1231.nDRDY/DOUT ����������� � 1.
		// ����� ��� ���������� ������������� ADS1231.nDRDY/DOUT ����� ������� ��� ������� ���������� ��� � ���������� ����� ������.
		// ��� ������ �� ������ 3-� ���� ������������ ���������� 32 SCLK.
		// ��� �������� SPI 1 ���, �� ��� �������� ������ �� 40 ���.
		uint32_t DataRaw = 0;
		HAL_StatusTypeDef ReceiveResult = HAL_SPI_Receive( pADS1231_SPI_hdl, ( uint8_t * ) &DataRaw, 3, 3 );
		// ��������� ���, � �������� ��������� ADS1231.nDRDY/DOUT, � ����� EXTI
		ADS1231_IRQ_Enable( true );
		if( HAL_OK != ReceiveResult )
			break;
		// �������������� ��������� ������: ���������� ������� ���������� � ������� �� 3-� ���� � 4-� � ����������� �����
		int32_t DataBin = ( ( int32_t ) __REV( DataRaw ) ) >> 8;
		if( ( DataBin > ADS1231_ADCMAX ) || ( DataBin < ADS1231_ADCMIN ) )
			break;
		ADS1231_DataADC_Saved = DataBin;
		bResult = true;
	} while( 0 );
	assert_param( NULL != pDataADC );
	*pDataADC = ADS1231_DataADC_Saved;
	GPIO_Common_Write( iGPIO_TestPinADS1231, GPIO_PIN_SET );
	return bResult;
}



