// ѕлатформо-зависимый инициализатор драйвера UART
// ƒл€ инициализации канала UART в заголовках проекта и/или платформы необходимо определить:
//	#define COM#_USE						использовать UART# (USART#)
//		#define	COM#_TX_GPIO					определить порт UART#.Tx
//		#define	COM#_TX_PIN						определить пин UART#.Tx
//		#define	COM#_TX_AF						определить альтернативную функцию UART#.Tx
//		#define	COM#_RX_GPIO					определить порт UART#.Rx
//		#define	COM#_RX_PIN						определить пин UART#.Rx
//		#define	COM#_RX_AF						определить альтернативную функцию UART#.Rx
//	#define COM#_USE_TXEN					использовать дополнительный вывод TxEn дл€ управлени€ драйвером RS-485
//		#define	COM#_TXEN_GPIO					определить порт UART#.TxEn
//		#define	COM#_TXEN_PIN					определить пин UART#.TxEn
//	#define COM#_USE_IRQ					использовать прерывание от UART#
//		#define COM#_IRQ_PREEMTPRIORITY 		определить приоритет прерывани€ UART#.IRQ
//		#define COM#_IRQ_SUBPRIORITY			определить подприоритет прерывани€ UART#.IRQ (обычно 0)
// DMA для STM32F4xx:
//	#define COM#_USE_DMATX					использовать канал DMA.Tx дл€ UART#
//		#define COM#_DMATX_INSTANCE 			определить номер и поток UART#.DMA.Tx
//		#define COM#_DMATX_CHANNEL				определить канал UART#.DMA.Tx
//		#define COM#_DMATX_IRQHandler			определить обработчик прерывани€ UART#.DMA.Tx.IRQ
//		#define COM#_DMATX_PRIORITY 			определить приоритет UART#.DMA.Tx
//		#define COM#_DMATX_IRQ_PREEMTPRIORITY	определить приоритет прерывани€ UART.DMA.Tx.IRQ
//		#define COM#_DMATX_IRQ_SUBPRIORITY		определить подприоритет прерывани€ UART.DMA.Tx.IRQ (обычно 0)
//	#define COM#_USE_DMARX					использовать канал DMA.Rx дл€ UART#
//		#define COM#_DMARX_INSTANCE 			определить номер и поток UART#.DMA.Rx
//		#define COM#_DMARX_CHANNEL				определить канал UART#.DMA.Rx
//		#define COM#_DMARX_IRQHandler			определить обработчик прерывани€ UART#.DMA.Rx.IRQ
//		#define COM#_DMARX_PRIORITY 			определить приоритет UART#.DMA.Rx
//		#define COM#_DMARX_IRQ_PREEMTPRIORITY	определить приоритет прерывани€ UART.DMA.Rx.IRQ
//		#define COM#_DMARX_IRQ_SUBPRIORITY		определить подприоритет прерывани€ UART.DMA.Rx.IRQ (обычно 0)
// DMA для STM32L4xx:


#ifndef	COMMON_UART_H
#define	COMMON_UART_H

#include "stm32xxxx_hal_uart_ext.h"		// расширенный драйвер UART

// ∆естка€ прив€зка COM# к UART#
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

// ***************** ќписание используемых в проекте хендлеров UART **************
// COM#_UART_Ext_hdl	- расширенный хендлер
// COM#_UART_hdl		- преобразование расширенного хендлера к обычному

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
#error "ƒописать исходник!"
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

