// Stm32l4xx_hal_uart_ext.h, ���������� �������� Stm32l4xx_hal_uart
// (�������� ���� ����������)
#ifndef	STM32XXXX_HAL_UART_EXT
#define	STM32XXXX_HAL_UART_EXT

#include "Stm32xxxx_hal_uart.h"			// ����������� �������
#include "ByteQueue.h"					// ByteQueue_t
#ifdef	USE_FREERTOS
#include "FreeRTOS.h"					// FreeRTOS
#include "Event_groups.h"				// FreeRTOS.EventGroups
#else
typedef uint32_t EventBits_t;
typedef uint32_t BaseType_t;
#endif	// USE_FREERTOS

typedef void ( *UART_Ext_Callback_t ) ( void );
typedef void ( *UART_Ext_IT_t ) ( UART_HandleTypeDef *huart );

typedef struct UART_Ext_IRQ_Handlers_struct
{
	UART_Ext_IT_t	xRxComplete;
	UART_Ext_IT_t	xRxIdle;
	UART_Ext_IT_t	xTxComplete;
	UART_Ext_IT_t	xTxEmpty;
} UART_Ext_IRQ_Handlers_t;

// ��������� �������� UART_Ext_HandleTypeDef.
// ��� ��������� �� ����������� API ����� ��������� ���������� ��� ������� ������� UART_HandleTypeDef.
// ���� ������� �������� � ������ ����������� API, �� ������� ��������������� � UART_Ext_HandleTypeDef.
// ??������-��, ������ ��� (���� ���-�� ��� ������������ ����� ������). ��� ����� ��������������� �������� �������??
typedef struct
{
	// ��������� UART �� ������� ���������� Stm32f4xx_hal_uart
	UART_HandleTypeDef	Common;
	// �������������� ����
	USART_TypeDef		*Instance;				// ������������ ���� Common.Instance - ��������� ������ �� ������������ ������������� ��������
	GPIO_TypeDef		*TXEN_GPIO;				// ���������� RS485.TxEn (��� �������������)
	uint16_t			TXEN_Pin;				// ���������� RS485.TxEn (��� �������������)
#ifdef	USE_FREERTOS
	EventGroupHandle_t	EventGroup;				// ������� ��� ������ � ����������� ��������� (����� ������� ��� �������������)
//#else
//	EventBits_t EventBitsResult;
#endif	// USE_FREERTOS
	EventBits_t			EventBitsMask;			// ����� �������, ������� ���������� �����������
	ByteQueue_t			*pByteQueueRx;			// ��������� ����� ��� ������ ����� DMA
	
	UART_Ext_Callback_t	TxCompleteCallback;		// ������������ ������� �� ����������� ���������� �� ���������� ��������
	UART_Ext_IRQ_Handlers_t IRQ_Handlers;		// ����������� ����������. �� STM32L4xx ��� ��� �������� � UART_HandleTypeDef �� ��� STM32F4xx �� ���
	// �������� �������?
	// �������� ������??
} UART_Ext_HandleTypeDef;

// ������� ��� ������ � UART_Ext
#define	EVENT_UART_EXT_TX_ERROR			( 1 << 0 )		// �������� ������ ��� �������� ������
#define	EVENT_UART_EXT_TX_COMPLETE		( 1 << 1 )		// �������� ������ ��������� ��������� (�� UART.TxC)
#define	EVENT_UART_EXT_TX_DMA_COMPLETE	( 1 << 2 )		// ����� DMA ������� � UART (�� DMA.TxC)
#define	EVENT_UART_EXT_TX_ALL			( EVENT_UART_EXT_TX_ERROR | EVENT_UART_EXT_TX_COMPLETE | EVENT_UART_EXT_TX_DMA_COMPLETE )
#define	EVENT_UART_EXT_RX_ERROR			( 1 << 3 )		// �������� ������ ��� ������ �����
#define	EVENT_UART_EXT_RX_IDLE			( 1 << 4 )		// �������� �������� ����� (����) ����� ������ ���������� �����
#define	EVENT_UART_EXT_RX_COMPLETE		( 1 << 5 )		// ������� ��������� ���������� ����
#define	EVENT_UART_EXT_RX_OVR			( 1 << 6 )		// ��������� ������������ ������
#define	EVENT_UART_EXT_RX_ALL			( EVENT_UART_EXT_RX_ERROR | EVENT_UART_EXT_RX_IDLE | EVENT_UART_EXT_RX_COMPLETE | EVENT_UART_EXT_RX_OVR )
#define	EVENT_UART_EXT_TIMEOUT			( 1 << 7 )		// ����� ������� �������� ���������� (���� ������ ������� ���������?)
//#define	EVENT_UART_EXT_ALL				( EVENT_UART_EXT_TX_ALL | EVENT_UART_EXT_RX_ALL | EVENT_UART_EXT_TIMEOUT | EVENT_UART_EXT_MUTEX | EVENT_UART_EXT_MODEM_INIT_COMPLETE )
#define	EVENT_UART_EXT_ALL				( EVENT_UART_EXT_TX_ALL | EVENT_UART_EXT_RX_ALL | EVENT_UART_EXT_TIMEOUT )
//#define	EVENT_UART_EXT_MUTEX			( 1 << 8 )		// !!����!!	������� - UART �������� ��� ����������� � ���� ���������� ���������
#define	EVENT_UART_EXT_MODEM_INIT_COMPLETE	( 1 << 13 )		// !!����!!	��������� ������������� ������������� ������������ (����., ������)
#define	EVENT_UART_EXT_INIT_COMPLETE		( 1 << 14 )		// !!����!! UART ���������������
#define	EVENT_UART_EXT_RX_ENABLED			( 1 << 15 )		// !!����!!�� �������! ���� � EventBitsMask ����������, ��� �� ������ ������� �������� ��� �������� �����. ����� ��������� ����� ����� ��������, ����� ���������� �������� ����� ����� ���� �������
#define	EVENT_UART_EXT_ACCESS_READY			( 1 << 12 )		// !!����!! �������: 1 - ���������� � �������, 0 - ��������

#if		defined( STM32F4 )
	#define	UART_INSTANCE_TDR( __INSTANCE__ )	( ( __INSTANCE__ )->DR )
	#define UART_INSTANCE_RDR( __INSTANCE__ )	( ( __INSTANCE__ )->DR )
#elif	defined( STM32L4 ) || defined( STM32F3 )
	#define UART_INSTANCE_TDR( __INSTANCE__ )	( ( __INSTANCE__ )->TDR )
	#define UART_INSTANCE_RDR( __INSTANCE__ )	( ( __INSTANCE__ )->RDR )
#else
#error "Select Target Family!"
#endif

// ��������� �������
HAL_StatusTypeDef HAL_UART_Ext_Init( UART_Ext_HandleTypeDef *huart );				// ������������� ������������ ������� UART
HAL_StatusTypeDef HAL_UART_Ext_DeInit( UART_Ext_HandleTypeDef *huart );			// ��������������� ������������ ������� UART
HAL_StatusTypeDef HAL_UART_Ext_Transmit( UART_Ext_HandleTypeDef *huart_ext, uint8_t *pData, uint16_t Size, EventBits_t EventBitsMask );			// �������� ����� � UART ����� DMA.
HAL_StatusTypeDef HAL_UART_Ext_ReceiveCyclicStart( UART_Ext_HandleTypeDef *huart_ext, ByteQueue_t *pByteQueueRx, EventBits_t EventBitsMask );	// ��������� ����� �� UART �� DMA � ����� pByteQueueRx ����������
HAL_StatusTypeDef HAL_UART_Ext_ReceiveCyclicStop( UART_Ext_HandleTypeDef *huart_ext );															// ��������� ����� �� UART �� DMA � ����������� �����
HAL_StatusTypeDef HAL_UART_Ext_ReceiveCyclicReset( UART_Ext_HandleTypeDef *huart_ext );															// �������� �������� �����
HAL_StatusTypeDef HAL_UART_Ext_ReceiveStart( UART_Ext_HandleTypeDef *huart_ext, uint8_t *pData, uint16_t Size, EventBits_t EventBitsMask );		// ��������� ����� �� UART �� DMA � ����� �������������� ������� (�� ���������� ������ ��� Idle). ��� �������������� ���������� �������� ����?
HAL_StatusTypeDef HAL_UART_Ext_ReceiveStop( UART_Ext_HandleTypeDef *huart_ext );																// ��������� ����� �� UART �� DMA � �����
bool UART_Ext_RxDMA_CheckActivity( UART_Ext_HandleTypeDef *huart_ext, uint32_t *pPrevState );	// �������� ���������� ��� ������ � ����������� ����� �� DMA - ��������� �������� DMA.NDTR � �����������, ������������ � *pPrevState
void HAL_UART_Ext_IRQHandler( UART_HandleTypeDef *huart );						// ���������� ���������� UART ��� ������������ ��������
void HAL_UART_RxIdleCallback( UART_HandleTypeDef *huart );						// ������� �� ���������� UART.RxIdle
void HAL_UART_Ext_ResetBaudrate( UART_HandleTypeDef *huart );					// �������� �������� UART ��� ������ �����������������
uint32_t HAL_UART_Ext_CalcByteRate( UART_Ext_HandleTypeDef *huart_ext );		// [����/�]	���������� �������� �������� ����������
float HAL_UART_Ext_CalcTimeToTransmite( UART_Ext_HandleTypeDef *huart_ext, uint16_t PacketSize );		// [�]	���������� ����� �� �������� ������
HAL_StatusTypeDef HAL_UART_Ext_ReceiveRawStart( UART_Ext_HandleTypeDef *huart_ext, UART_Ext_IT_t xRxIT, UART_Ext_IT_t xRxIdleIT );	// ������ UART �� �����, ����������� ���������� ����������� � ����������.
HAL_StatusTypeDef HAL_UART_Ext_ReceiveRawStop( UART_Ext_HandleTypeDef *huart_ext );												// ��������� ����� �� UART, ������� � HAL_UART_Ext_ReceiveRawStart()
UART_Ext_Callback_t UART_Ext_SetTransferCompleteCallback( UART_Ext_HandleTypeDef *huart_ext, UART_Ext_Callback_t xCallback );		// ���������� ������� ��� ���������� �������� ������ ����� UART
BaseType_t HAL_UART_Ext_ValidateHdl( UART_Ext_HandleTypeDef *huart );			// ��������, ��� ������� huart �������� ����������� ������� ������������ ��������

#endif	// STM32XXXX_HAL_UART_EXT

