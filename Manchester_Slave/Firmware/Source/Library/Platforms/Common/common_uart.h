// ���������-��������� ������������� �������� UART
// ��� ������������� ������ UART � ���������� ������� �/��� ��������� ���������� ����������:
//	#define COM#_USE						������������ UART# (USART#)
//		#define	COM#_TX_GPIO					���������� ���� UART#.Tx
//		#define	COM#_TX_PIN						���������� ��� UART#.Tx
//		#define	COM#_TX_AF						���������� �������������� ������� UART#.Tx
//		#define	COM#_RX_GPIO					���������� ���� UART#.Rx
//		#define	COM#_RX_PIN						���������� ��� UART#.Rx
//		#define	COM#_RX_AF						���������� �������������� ������� UART#.Rx
//	#define COM#_USE_TXEN					������������ �������������� ����� TxEn ��� ���������� ��������� RS-485
//		#define	COM#_TXEN_GPIO					���������� ���� UART#.TxEn
//		#define	COM#_TXEN_PIN					���������� ��� UART#.TxEn
//	#define COM#_USE_IRQ					������������ ���������� �� UART#
//		#define COM#_IRQ_PREEMTPRIORITY 		���������� ��������� ���������� UART#.IRQ
//		#define COM#_IRQ_SUBPRIORITY			���������� ������������ ���������� UART#.IRQ (������ 0)
// DMA ��� STM32F4xx:
//	#define COM#_USE_DMATX					������������ ����� DMA.Tx ��� UART#
//		#define COM#_DMATX_INSTANCE 			���������� ����� � ����� UART#.DMA.Tx
//		#define COM#_DMATX_CHANNEL				���������� ����� UART#.DMA.Tx
//		#define COM#_DMATX_IRQHandler			���������� ���������� ���������� UART#.DMA.Tx.IRQ
//		#define COM#_DMATX_PRIORITY 			���������� ��������� UART#.DMA.Tx
//		#define COM#_DMATX_IRQ_PREEMTPRIORITY	���������� ��������� ���������� UART.DMA.Tx.IRQ
//		#define COM#_DMATX_IRQ_SUBPRIORITY		���������� ������������ ���������� UART.DMA.Tx.IRQ (������ 0)
//	#define COM#_USE_DMARX					������������ ����� DMA.Rx ��� UART#
//		#define COM#_DMARX_INSTANCE 			���������� ����� � ����� UART#.DMA.Rx
//		#define COM#_DMARX_CHANNEL				���������� ����� UART#.DMA.Rx
//		#define COM#_DMARX_IRQHandler			���������� ���������� ���������� UART#.DMA.Rx.IRQ
//		#define COM#_DMARX_PRIORITY 			���������� ��������� UART#.DMA.Rx
//		#define COM#_DMARX_IRQ_PREEMTPRIORITY	���������� ��������� ���������� UART.DMA.Rx.IRQ
//		#define COM#_DMARX_IRQ_SUBPRIORITY		���������� ������������ ���������� UART.DMA.Rx.IRQ (������ 0)
// DMA ��� STM32L4xx:


#ifndef	COMMON_UART_H
#define	COMMON_UART_H

#include "stm32xxxx_hal_uart_ext.h"		// ����������� ������� UART

// ������� �������� COM# � UART#
#define	COM1			1
#define	COM1_UART		USART1
#define	COM2			2
#define	COM2_UART		USART2
#define	COM3			3
#define	COM3_UART		USART3
#define	COM4			4
#define	COM4_UART		UART4
#define	COM5			5
#define	COM5_UART		UART5
#define	COM6			6
#define	COM6_UART		USART6
//#define	COM7			7
//#define	COM7_UART		UART7
//#define	COM8			8
//#define	COM8_UART		UART8

// ***************** �������� ������������ � ������� ��������� UART **************
// COM#_UART_Ext_hdl	- ����������� �������
// COM#_UART_hdl		- �������������� ������������ �������� � ��������

#ifdef	COM1_USE
	extern UART_Ext_HandleTypeDef COM1_UART_Ext_hdl;
	#define COM1_UART_hdl	( *( UART_HandleTypeDef * ) &COM1_UART_Ext_hdl )
#endif	//COM1_USE

#ifdef	COM2_USE
	extern UART_Ext_HandleTypeDef COM2_UART_Ext_hdl;
	#define COM2_UART_hdl	( *( UART_HandleTypeDef * ) &COM2_UART_Ext_hdl )
#endif	//COM2_USE

#ifdef	COM3_USE
	extern UART_Ext_HandleTypeDef COM3_UART_Ext_hdl;
	#define COM3_UART_hdl	( *( UART_HandleTypeDef * ) &COM3_UART_Ext_hdl )
#endif	//COM3_USE

#ifdef	COM4_USE
	extern UART_Ext_HandleTypeDef COM4_UART_Ext_hdl;
	#define COM4_UART_hdl	( *( UART_HandleTypeDef * ) &COM4_UART_Ext_hdl )
#endif	//COM4_USE

#ifdef	COM5_USE
	extern UART_Ext_HandleTypeDef COM5_UART_Ext_hdl;
	#define COM5_UART_hdl	( *( UART_HandleTypeDef * ) &COM5_UART_Ext_hdl )
#endif	//COM5_USE

#ifdef	COM6_USE
	extern UART_Ext_HandleTypeDef COM6_UART_Ext_hdl;
	#define COM6_UART_hdl	( *( UART_HandleTypeDef * ) &COM6_UART_Ext_hdl )
#endif	//COM6_USE

#if defined ( COM7_USE ) || defined ( COM8_USE )
#error "�������� ��������!"
#endif


#define	__COM_INIT( __COM_N__ )															\
{																						\
	UART_HandleTypeDef *pUART_hdl = &( COM##__COM_N__##_UART_hdl );						\
	__HAL_UART_RESET_HANDLE_STATE( pUART_hdl );											\
	__HAL_UNLOCK( pUART_hdl );															\
	pUART_hdl->Instance = COM##__COM_N__##_UART;										\
	assert_param( HAL_OK == HAL_UART_Ext_Init( &( COM##__COM_N__##_UART_Ext_hdl ) ) );	\
}

#define	COM_INIT( __COM_N__ )	__COM_INIT( __COM_N__ )


#endif	// COMMON_UART_H

