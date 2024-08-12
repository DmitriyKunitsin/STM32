// Driver_SIG60.c
// ������� ������ SIG60
#include "ProjectConfig.h"
#include "stm32xxxx_hal.h"		// ����� ���������
#include "ProjectConfig.h"
#include "Driver_SIG60.h"		// ������
#include "Common_UART.h"
#include "Common_GPIO.h"
#include "Common_RCC.h"
#include "RebootUtils.h"		// ResetInfo

// ��������� ������������� FreeRTOS ��� ������ � �������
#ifdef	USE_FREERTOS
#define	SIG60_USE_RTOS
#endif	// USE_FREERTOS

#define	COM_SIG60					COM_SKLP_MODEM
#define	SIG60_TIMING_POWEROFF_ms	2		// [��]	����� ������������� ��� ������
#define	SIG60_TIMING_POWERON_ms		8		// [��]	����� ��������� ����� ������
#define	SIG60_TIMING_HDC_us			2		// [���]	�������� ��� ������������ ������� �������/������
#define	SIG60_TIMING_RESPONSE_ms	5		// [��]	����� ������� �� ������� �� UART

UART_Ext_HandleTypeDef * const xSIG60_UART_hdl = &COM_SKLP_MODEM_UART_EXT_HDL;

#ifndef	GPIO_PLC_HDC_POLARITTY_INVERTED
#define	SIG60_HDC_CONTROL( )		GPIO_Common_Write( iGPIO_SIG60_HDC, GPIO_PIN_RESET )
#define	SIG60_HDC_DATA( )			GPIO_Common_Write( iGPIO_SIG60_HDC, GPIO_PIN_SET )
#else
#define	SIG60_HDC_CONTROL( )		GPIO_Common_Write( iGPIO_SIG60_HDC, GPIO_PIN_SET )
#define	SIG60_HDC_DATA( )			GPIO_Common_Write( iGPIO_SIG60_HDC, GPIO_PIN_RESET )
#endif

#ifndef	GPIO_PLC_NRST_POLARITTY_INVERTED
#define	SIG60_RST_ON( )				GPIO_Common_Write( iGPIO_SIG60_nReset, GPIO_PIN_RESET )
#define	SIG60_RST_OFF( )			GPIO_Common_Write( iGPIO_SIG60_nReset, GPIO_PIN_SET )
#else
#define	SIG60_RST_ON( )				GPIO_Common_Write( iGPIO_SIG60_nReset, GPIO_PIN_SET )
#define	SIG60_RST_OFF( )			GPIO_Common_Write( iGPIO_SIG60_nReset, GPIO_PIN_RESET )
#endif

// ������ � ����������� �������
// [1..2 ms]
static bool SIG60_WriteControlReg( uint8_t Address, uint8_t Data )
{
	bool Result = false;
	SIG60_HDC_CONTROL( );
	Delay_us( SIG60_TIMING_HDC_us );
	do
	{
		Address &= ~SIG60_CMD_bm;
		Address |= SIG60_CMD_WRITEREG;
		uint8_t aBuffer[2] = { Address, Data };
#ifndef	SIG60_USE_RTOS
		if( HAL_OK != HAL_UART_Transmit( &xSIG60_UART_hdl->Common, aBuffer, sizeof( aBuffer ), 10 ) )
			break;
#else
		EventBits_t EventBitsMask = EVENT_UART_EXT_TX_ERROR | EVENT_UART_EXT_TX_COMPLETE;
		if( HAL_OK != HAL_UART_Ext_Transmit( xSIG60_UART_hdl, aBuffer,  sizeof( aBuffer ), EventBitsMask ) )
			break;
		// [1..2 ms]	������� ���������� �������� ������
		EventBits_t EventBitsResult = xEventGroupWaitBits( xSIG60_UART_hdl->EventGroup, EventBitsMask, pdTRUE, pdFALSE, pdMS_TO_TICKS( 2 ) );
		if( EVENT_UART_EXT_TX_COMPLETE != ( EventBitsResult & EventBitsMask ) )
			break;
#endif
		Result = true;
	} while( 0 );
	SIG60_HDC_DATA( );
	Delay_us( SIG60_TIMING_HDC_us );
	return Result;
}

// !!! ����!
// �������� ������������ ����� �� ����� HDI �� ����� Reset � ����� �������
// !!! ��������������, ��� ��������� SIG60.Reset, ��� �� ��������������. ��� ���� ��� ������������� ��������� SIG60.HDC
// !!! �������� UART ����� ���� ����������� � ��������� ��������
static bool SIG60_WriteDummyWhileReset( uint8_t *pBuffer, uint16_t BufferSize )
{
	bool Result = false;
	do
	{
#ifndef	SIG60_USE_RTOS
		if( HAL_OK != HAL_UART_Transmit( &xSIG60_UART_hdl->Common, pBuffer, BufferSize, 100 ) )
			break;
#else
		EventBits_t EventBitsMask = EVENT_UART_EXT_TX_ERROR | EVENT_UART_EXT_TX_COMPLETE;
		if( HAL_OK != HAL_UART_Ext_Transmit( xSIG60_UART_hdl, pBuffer, BufferSize, EventBitsMask ) )
			break;
		// [5..6 ms]	������� ���������� �������� ������
		EventBits_t EventBitsResult = xEventGroupWaitBits( xSIG60_UART_hdl->EventGroup, EventBitsMask, pdTRUE, pdFALSE, pdMS_TO_TICKS( 6 ) );
		if( EVENT_UART_EXT_TX_COMPLETE != ( EventBitsResult & EventBitsMask ) )
			break;
#endif
		Result = true;
	} while( 0 );
	return Result;
}

// ���������� ���������� �� UART.
// �� UART.RxNotEmpty �������� ���� � ������� � ��������� �����
volatile static uint8_t RecByte, RecFlag;
static void SIG60_UART_ReceiveCallback( UART_HandleTypeDef *huart )
{
	assert_param( &xSIG60_UART_hdl->Common == huart );
	uint8_t RecByteNew = ( uint8_t )( UART_INSTANCE_RDR( huart->Instance ) & 0x00FF );		// ������� ��������, ����� �������� ���� ����������
	if( !RecFlag )
	{	// ��������� ������ ������ ����. ���� ����� ������ - ������������
		RecByte = RecByteNew;
		RecFlag = 1;

#ifdef	SIG60_USE_RTOS
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		assert_param( pdPASS == xEventGroupSetBitsFromISR( ( ( UART_Ext_HandleTypeDef * ) huart )->EventGroup, EVENT_UART_EXT_RX_COMPLETE, &xHigherPriorityTaskWoken ) );
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
#endif	// SIG60_USE_RTOS
	}
}

// ������ �� ������������ ��������
// [1..5 ms]
static bool SIG60_ReadControlReg( uint8_t Address, uint8_t *pData )
{
	bool Result = false;
//	GPIO_Common_Write( iGPIO_SIG60_nHDC, GPIO_PIN_SET );
	SIG60_HDC_CONTROL( );
	Delay_us( SIG60_TIMING_HDC_us );
	do
	{
		Address &= ~SIG60_CMD_bm;
		Address |= SIG60_CMD_READREG;
		// ������������� � ������ ������
		RecFlag = 0;
		if( HAL_OK != HAL_UART_Ext_ReceiveRawStart( xSIG60_UART_hdl, SIG60_UART_ReceiveCallback, NULL ) )
			break;
#ifndef	SIG60_USE_RTOS
		// ��������� ����� ��������
		if( HAL_OK != HAL_UART_Transmit( &xSIG60_UART_hdl->Common, &Address, sizeof( Address ), 10 ) )
			break;
		// ��������� ������ ������
		uint32_t TickStart = HAL_GetTick( );
		while( !RecFlag )
			if( ( HAL_GetTick( ) - TickStart ) > 5 )		// ��������� �������, ���� ���� �������� ������ �����
				break;
#else
		EventBits_t EventBitsMask = EVENT_UART_EXT_TX_COMPLETE | EVENT_UART_EXT_RX_COMPLETE;
		if( HAL_OK != HAL_UART_Ext_Transmit( xSIG60_UART_hdl, &Address, sizeof( Address ), EventBitsMask ) )
			break;
		// [1..5 ms]	������� ���������� �������� ������ � ������ ������
		EventBits_t EventBitsResult = EventBitsMask & xEventGroupWaitBits( xSIG60_UART_hdl->EventGroup, EventBitsMask, pdTRUE, pdTRUE, pdMS_TO_TICKS( SIG60_TIMING_RESPONSE_ms ) );
		if( EventBitsResult != EventBitsMask )
			break;
#endif
		// ��������� �����
		if( !RecFlag )
			break;		// ������� �����, ����� �� ������
		// ������ �����, ��������� �������� ����
		*pData = RecByte;
		Result = true;
	} while( 0 );
	// ��������� �����
	assert_param( HAL_OK == HAL_UART_Ext_ReceiveRawStop( xSIG60_UART_hdl ) );
//	GPIO_Common_Write( iGPIO_SIG60_nHDC, GPIO_PIN_RESET );
	SIG60_HDC_DATA( );
	Delay_us( SIG60_TIMING_HDC_us );
	return Result;
}

// ��������� ��������� �������� ��� ���������������� SIG60
static void SIG60_Delay_ms( uint16_t Delay_ms )
{
#ifdef	SIG60_USE_RTOS
		vTaskDelay( 1 + pdMS_TO_TICKS( Delay_ms ) );
#else
		Delay_us( Delay_ms * 1000 );
#endif
}

/*	������� ������������� SIG60
 *	0. ���������������, ��� UART ��� ��������������� � SKLP_InterfacePeripheryInit()
 *	1. �������� ������� ��� ���������� - ��������� �� ����� ?? ��
 *	2. ������ ������� ��� ����� ����� - ��������� �� ����� 6 ��
 *	3. ������������������� UART �� �������� 19200
 *	4. ������� HDC
 *	5. �������� HDC, �������� CR0, ������� HDC
 *	6. �������� HDC, �������� CR1, ������� HDC
 *	7. ��������� ��� ���������������� �������� � �������� �� ���������� � ��������
 *
 *	���������, ���������� �������������:
 *	1. SKLP_SlaveTask(), ��������� �������������
 *	2. SKLP_SlaveTask(), ���� � ������� ������� ������� �� ���� �������� �������
 *	3. MPI_OperationTask(), ���� � ������� ���������� ����� ����� �� ������������ ����� SIG60 ��������� �� �������
 */
// [14..26 ms]
bool SIG60_Init( uint8_t CR0, uint8_t CR1 )
{
#ifdef	SIG60_USE_RTOS
	assert_param( taskSCHEDULER_RUNNING == xTaskGetSchedulerState( ) );
	// ���������, ��� UART ��� ��������������� (� �.�. �� ������� ��������)
	assert_param( EVENT_UART_EXT_INIT_COMPLETE & xEventGroupGetBits( xSIG60_UART_hdl->EventGroup ) );
#else
	assert_param( EVENT_UART_EXT_INIT_COMPLETE & xSIG60_UART_hdl->EventBitsMask );
#endif

	// ���������� ��������� �������� UART ����� �������������
	uint32_t BaudRateNew;
	switch( CR1 & SIG60_CR1_BitRate_bm )
	{
	case SIG60_CR1_BitRate_9600_115200_gc:
		if( SIG60_CR1_Freq_10M5_13M0_gc == ( CR0 & SIG60_CR1_Freq_bm ) )
			BaudRateNew = 115200;
		else
			BaudRateNew = 9600;
		break;
	case SIG60_CR1_BitRate_19200_gc:	BaudRateNew = 19200;	break;
	case SIG60_CR1_BitRate_38400_gc:	BaudRateNew = 38400;	break;
	case SIG60_CR1_BitRate_57600_gc:	BaudRateNew = 57600;	break;
	default:							BaudRateNew = 0;		break;
	}

	// ���������������, ��� ������������� ����� ���� ������� � �������� �������� ������,
	// �.�. ������� �������� UART � ��������� �������� SIG60 ������ ���������
	assert_param( BaudRateNew == xSIG60_UART_hdl->Common.Init.BaudRate );	// ������������� ��������� - ������ ����������, � �� ������

	// �������� �����, ��� ��������������������
//	xEventGroupClearBits( xSIG60_UART_hdl->EventGroup, EVENT_UART_EXT_MODEM_INIT_COMPLETE );
	ENTER_CRITICAL_SECTION( );
	xSIG60_UART_hdl->EventBitsMask &= ~EVENT_UART_EXT_MODEM_INIT_COMPLETE;
	EXIT_CRITICAL_SECTION( );

	// [10..12 ms] ���������� ����� ������
//	GPIO_Common_Write( iGPIO_SIG60_Reset, GPIO_PIN_RESET );
	SIG60_RST_ON( );
	SIG60_Delay_ms( SIG60_TIMING_POWEROFF_ms );

/*/ �������� ���������� � ����� ������������ assert() �� ����� ����������������� SIG60
xSIG60_UART_hdl->Common.Init.BaudRate = 1000000;
HAL_UART_Ext_ResetBaudrate( &xSIG60_UART_hdl->Common );
SIG60_WriteDummyWhileReset( ( uint8_t * ) ResetInfo.aLogMessage, strlen( ( char * ) ResetInfo.aLogMessage ) );
*/	
	// GPIO_Common_Write( iGPIO_SIG60_Reset, GPIO_PIN_SET );
	SIG60_RST_OFF( );
	SIG60_Delay_ms( SIG60_TIMING_POWERON_ms );

	// �������� �������� UART �� �������� SIG60 ��-��������� (19200)
	xSIG60_UART_hdl->Common.Init.BaudRate = SIG60_BITRATE_DEFAULT;
	HAL_UART_Ext_ResetBaudrate( &xSIG60_UART_hdl->Common );
	xSIG60_UART_hdl->Common.Init.BaudRate = BaudRateNew;		// ����� �� ������������� � �������������� ���������� ��������

	bool Result = false;
	do
	{
		uint8_t RegData;
		// [2..4 ms]	�������� CR0 � CR1
		if( !SIG60_WriteControlReg( SIG16_ADDRESS_CR0, CR0 ) || !SIG60_WriteControlReg( SIG16_ADDRESS_CR1, CR1 ) )
		{	// ������ ��������� �� �������
			HAL_UART_Ext_ResetBaudrate( &xSIG60_UART_hdl->Common );		// ������������ �������� UART
			break;
		}
		HAL_UART_Ext_ResetBaudrate( &xSIG60_UART_hdl->Common );			// ������������ �������� UART
		
		// [1..5 ms]	 ��������� CR0
		if( !SIG60_ReadControlReg( SIG16_ADDRESS_CR0, &RegData ) || ( RegData != CR0 ) )
			break;
		// [1..5 ms]	 ��������� CR1
		if( !SIG60_ReadControlReg( SIG16_ADDRESS_CR1, &RegData ) || ( RegData != CR1 ) )
			break;

		// �������� �����, ��� ������������������
//		xEventGroupSetBits( xSIG60_UART_hdl->EventGroup, EVENT_UART_EXT_MODEM_INIT_COMPLETE );
		ENTER_CRITICAL_SECTION( );
		xSIG60_UART_hdl->EventBitsMask |= EVENT_UART_EXT_MODEM_INIT_COMPLETE;
		EXIT_CRITICAL_SECTION( );
		Result = true;
	} while( 0 );

	return Result;
}

// ��������� SIG60 ����� ������ Reset
// ������������ ��� �������� ����������� ������ �����, ����� � ������ ��� ����������
void SIG60_PowerOff( void )
{
//	GPIO_Common_Write( iGPIO_SIG60_Reset, GPIO_PIN_RESET );
	SIG60_RST_ON( );
}

