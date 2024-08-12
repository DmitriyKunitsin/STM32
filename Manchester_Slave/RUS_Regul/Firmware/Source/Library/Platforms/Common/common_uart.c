
// Платформо-зависимый инициализатор драйвера UART
#include "ProjectConfig.h"			// конфиг платформы, конфиг задачи.
#include "stm32xxxx_hal.h"			// дрова периферии, в т.ч. 
#include "common_uart.h"
#include "platform_common.h"

// ***************** Определение используемых в проекте хендлеров UART и DMA **************
// COM#_UART_Ext_hdl
// COM#_DMATX_hdl
// COM#_DMARX_hdl
#ifdef	COM1_USE
	__no_init UART_Ext_HandleTypeDef COM1_UART_Ext_hdl __PLACE_AT_RAM_CCM__;
	#ifdef	COM1_USE_DMATX
		__no_init DMA_HandleTypeDef COM1_DMATX_hdl __PLACE_AT_RAM_CCM__;
	#endif	// COM1_DMATX_INSTANCE
	#ifdef	COM1_USE_DMARX
		__no_init DMA_HandleTypeDef COM1_DMARX_hdl __PLACE_AT_RAM_CCM__;
	#endif	// COM1_DMARX_INSTANCE
#endif	//COM1_USE

#ifdef	COM2_USE
	__no_init UART_Ext_HandleTypeDef COM2_UART_Ext_hdl __PLACE_AT_RAM_CCM__;
	#ifdef	COM2_USE_DMATX
		__no_init DMA_HandleTypeDef COM2_DMATX_hdl __PLACE_AT_RAM_CCM__;
	#endif	// COM2_DMATX_INSTANCE
	#ifdef	COM2_USE_DMARX
		__no_init DMA_HandleTypeDef COM2_DMARX_hdl __PLACE_AT_RAM_CCM__;
	#endif	// COM2_DMARX_INSTANCE
#endif	//COM2_USE

#ifdef	COM3_USE
	__no_init UART_Ext_HandleTypeDef COM3_UART_Ext_hdl __PLACE_AT_RAM_CCM__;
	#ifdef	COM3_USE_DMATX
		__no_init DMA_HandleTypeDef COM3_DMATX_hdl __PLACE_AT_RAM_CCM__;
	#endif	// COM3_DMATX_INSTANCE
	#ifdef	COM3_USE_DMARX
		__no_init DMA_HandleTypeDef COM3_DMARX_hdl __PLACE_AT_RAM_CCM__;
	#endif	// COM3_DMARX_INSTANCE
#endif	//COM3_USE

#ifdef	COM4_USE
	__no_init UART_Ext_HandleTypeDef COM4_UART_Ext_hdl __PLACE_AT_RAM_CCM__;
	#ifdef	COM4_USE_DMATX
		__no_init DMA_HandleTypeDef COM4_DMATX_hdl __PLACE_AT_RAM_CCM__;
	#endif	// COM4_DMATX_INSTANCE
	#ifdef	COM4_USE_DMARX
		__no_init DMA_HandleTypeDef COM4_DMARX_hdl __PLACE_AT_RAM_CCM__;
	#endif	// COM4_DMARX_INSTANCE
#endif	//COM4_USE

#ifdef	COM5_USE
	__no_init UART_Ext_HandleTypeDef COM5_UART_Ext_hdl __PLACE_AT_RAM_CCM__;
	#ifdef	COM5_USE_DMATX
		__no_init DMA_HandleTypeDef COM5_DMATX_hdl __PLACE_AT_RAM_CCM__;
	#endif	// COM5_DMATX_INSTANCE
	#ifdef	COM5_USE_DMARX
		__no_init DMA_HandleTypeDef COM5_DMARX_hdl __PLACE_AT_RAM_CCM__;
	#endif	// COM5_DMARX_INSTANCE
#endif	//COM5_USE

#ifdef	COM6_USE
	__no_init UART_Ext_HandleTypeDef COM6_UART_Ext_hdl __PLACE_AT_RAM_CCM__;
	#ifdef	COM6_USE_DMATX
		__no_init DMA_HandleTypeDef COM6_DMATX_hdl __PLACE_AT_RAM_CCM__;
	#endif	// COM6_DMATX_INSTANCE
	#ifdef	COM6_USE_DMARX
		__no_init DMA_HandleTypeDef COM6_DMARX_hdl __PLACE_AT_RAM_CCM__;
	#endif	// COM6_DMARX_INSTANCE
#endif	//COM6_USE

// ******** Макросы для инициализации UART и DMA из функции HAL_UART_MspInit() ********************

// Вспомогательный макрос для COM_INIT_GPIO()
#define	COM_INIT_GPIO_RXTX( __COM_N__, __RX_TX__ )							\
{																			\
	if( huart->Init.Mode & UART_MODE_##__RX_TX__ )							\
	{																		\
		GPIO_CLK_ENABLE( COM##__COM_N__##_##__RX_TX__##_GPIO );				\
		GPIO_InitStruct.Pin = COM##__COM_N__##_##__RX_TX__##_PIN;			\
		GPIO_InitStruct.Alternate = COM##__COM_N__##_##__RX_TX__##_AF; 		\
		HAL_GPIO_Init( COM##__COM_N__##_##__RX_TX__##_GPIO, &GPIO_InitStruct );	\
	}																		\
}

// Инициализация пинов Rx и Tx на работу в режиме UART
#define	COM_INIT_GPIO( __COM_N__ )											\
{																			\
	GPIO_InitStruct.Mode	= GPIO_MODE_AF_PP;								\
	GPIO_InitStruct.Pull	= GPIO_NOPULL;									\
	GPIO_InitStruct.Speed	= GPIO_SPEED_FREQ_LOW;							\
	COM_INIT_GPIO_RXTX( __COM_N__, RX );									\
	COM_INIT_GPIO_RXTX( __COM_N__, TX );									\
}

// Основная инициализация - тактирование на UART и пины Rx&Tx
#define	COM_INIT_COMMON( __COM_N__ )										\
{																			\
	UART_CLK_ENABLE( COM##__COM_N__##_UART );				 				\
	COM_INIT_GPIO( __COM_N__ );								 				\
	UART_Ext_HandleTypeDef *pUART_hdl = &( COM##__COM_N__##_UART_Ext_hdl );	\
	pUART_hdl->Common.hdmatx = NULL;										\
	pUART_hdl->Common.hdmarx = NULL;										\
}

// Опциональная инициализация пина TxEn для управления драйвером RS-485
#define	COM_INIT_GPIO_TXEN( __COM_N__ )										\
{																			\
	UART_Ext_HandleTypeDef *huart_ext = ( UART_Ext_HandleTypeDef * ) huart;	\
	if( huart->Instance == huart_ext->Instance )							\
	{																		\
		GPIO_CLK_ENABLE( COM##__COM_N__##_TXEN_GPIO );						\
		GPIO_InitStruct.Pin = COM##__COM_N__##_TXEN_PIN;					\
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;							\
		GPIO_InitStruct.Pull = GPIO_PULLDOWN;								\
		HAL_GPIO_Init( COM##__COM_N__##_TXEN_GPIO, &GPIO_InitStruct);		\
		huart_ext->TXEN_GPIO = COM##__COM_N__##_TXEN_GPIO;					\
		huart_ext->TXEN_Pin = COM##__COM_N__##_TXEN_PIN;					\
	}																		\
}

// Опциональная инициализация прерывания
#define	COM_INIT_IRQ( __COM_N__ )											\
{																			\
	HAL_NVIC_SetPriority(	UART_IRQn( COM##__COM_N__##_UART ),				\
							COM##__COM_N__##_IRQ_PREEMTPRIORITY,			\
							COM##__COM_N__##_IRQ_SUBPRIORITY );				\
	HAL_NVIC_EnableIRQ( UART_IRQn( COM##__COM_N__##_UART ) );				\
}

// Опциональная инициализация канала DMA.Rx или DMA.Tx
#if		defined( STM32F4 )
#define	COM_INIT_DMA( __COM_N__, __RX_TX__, __DIR__, _HDMA_ )							\
{																						\
	DMA_CLK_ENABLE( COM##__COM_N__##_DMA##__RX_TX__##_INSTANCE );						\
	DMA_HandleTypeDef *pDMA_hdl = &( COM##__COM_N__##_DMA##__RX_TX__##_hdl );			\
	__HAL_DMA_RESET_HANDLE_STATE( pDMA_hdl );											\
	__HAL_UNLOCK( pDMA_hdl );															\
	pDMA_hdl->Instance					= COM##__COM_N__##_DMA##__RX_TX__##_INSTANCE;	\
	pDMA_hdl->Init.Channel				= COM##__COM_N__##_DMA##__RX_TX__##_CHANNEL;	\
	pDMA_hdl->Init.Direction			= __DIR__;										\
	pDMA_hdl->Init.PeriphInc			= DMA_PINC_DISABLE;								\
	pDMA_hdl->Init.MemInc				= DMA_MINC_ENABLE;								\
	pDMA_hdl->Init.PeriphDataAlignment	= DMA_PDATAALIGN_BYTE;							\
	pDMA_hdl->Init.MemDataAlignment		= DMA_MDATAALIGN_BYTE;							\
	pDMA_hdl->Init.Mode					= DMA_NORMAL;									\
	pDMA_hdl->Init.Priority				= COM##__COM_N__##_DMA##__RX_TX__##_PRIORITY;	\
	pDMA_hdl->Init.FIFOMode				= DMA_FIFOMODE_DISABLE;							\
	assert_param( HAL_OK == HAL_DMA_Init( pDMA_hdl ) );									\
	__HAL_LINKDMA( huart, _HDMA_, COM##__COM_N__##_DMA##__RX_TX__##_hdl );				\
																						\
	HAL_NVIC_SetPriority(	DMA_IRQn( COM##__COM_N__##_DMA##__RX_TX__##_INSTANCE ),		\
							COM##__COM_N__##_DMA##__RX_TX__##_IRQ_PREEMTPRIORITY,		\
							COM##__COM_N__##_DMA##__RX_TX__##_IRQ_SUBPRIORITY ); 		\
	HAL_NVIC_EnableIRQ( DMA_IRQn( COM##__COM_N__##_DMA##__RX_TX__##_INSTANCE ) );		\
}
#elif	defined( STM32L4 )
#define	COM_INIT_DMA( __COM_N__, __RX_TX__, __DIR__, _HDMA_ )							\
{																						\
	DMA_CLK_ENABLE( COM##__COM_N__##_DMA##__RX_TX__##_CHANNEL );						\
	DMA_HandleTypeDef *pDMA_hdl = &( COM##__COM_N__##_DMA##__RX_TX__##_hdl );			\
	__HAL_DMA_RESET_HANDLE_STATE( pDMA_hdl );											\
	__HAL_UNLOCK( pDMA_hdl );															\
	pDMA_hdl->Instance					= COM##__COM_N__##_DMA##__RX_TX__##_CHANNEL;	\
	pDMA_hdl->Init.Request				= COM##__COM_N__##_DMA##__RX_TX__##_REQUEST;	\
	pDMA_hdl->Init.Direction			= __DIR__;										\
	pDMA_hdl->Init.PeriphInc			= DMA_PINC_DISABLE;								\
	pDMA_hdl->Init.MemInc				= DMA_MINC_ENABLE;								\
	pDMA_hdl->Init.PeriphDataAlignment	= DMA_PDATAALIGN_BYTE;							\
	pDMA_hdl->Init.MemDataAlignment		= DMA_MDATAALIGN_BYTE;							\
	pDMA_hdl->Init.Mode					= DMA_NORMAL;									\
	pDMA_hdl->Init.Priority				= COM##__COM_N__##_DMA##__RX_TX__##_PRIORITY;	\
	assert_param( HAL_OK == HAL_DMA_Init( pDMA_hdl ) );									\
	__HAL_LINKDMA( huart, _HDMA_, COM##__COM_N__##_DMA##__RX_TX__##_hdl );				\
																						\
	HAL_NVIC_SetPriority(	DMA_IRQn( COM##__COM_N__##_DMA##__RX_TX__##_CHANNEL ),		\
							COM##__COM_N__##_DMA##__RX_TX__##_IRQ_PREEMTPRIORITY,		\
							COM##__COM_N__##_DMA##__RX_TX__##_IRQ_SUBPRIORITY ); 		\
	HAL_NVIC_EnableIRQ( DMA_IRQn( COM##__COM_N__##_DMA##__RX_TX__##_CHANNEL ) );		\
}
#elif	defined( STM32F3 )
// в STM32F3 не используетсЯ Channel/Request, хз как без них
#define	COM_INIT_DMA( __COM_N__, __RX_TX__, __DIR__, _HDMA_ )							\
{																						\
	DMA_CLK_ENABLE( COM##__COM_N__##_DMA##__RX_TX__##_CHANNEL );						\
	DMA_HandleTypeDef *pDMA_hdl = &( COM##__COM_N__##_DMA##__RX_TX__##_hdl );			\
	__HAL_DMA_RESET_HANDLE_STATE( pDMA_hdl );											\
	__HAL_UNLOCK( pDMA_hdl );															\
	pDMA_hdl->Instance					= COM##__COM_N__##_DMA##__RX_TX__##_CHANNEL;	\
	pDMA_hdl->Init.Direction			= __DIR__;										\
	pDMA_hdl->Init.PeriphInc			= DMA_PINC_DISABLE;								\
	pDMA_hdl->Init.MemInc				= DMA_MINC_ENABLE;								\
	pDMA_hdl->Init.PeriphDataAlignment	= DMA_PDATAALIGN_BYTE;							\
	pDMA_hdl->Init.MemDataAlignment		= DMA_MDATAALIGN_BYTE;							\
	pDMA_hdl->Init.Mode					= DMA_NORMAL;									\
	pDMA_hdl->Init.Priority				= COM##__COM_N__##_DMA##__RX_TX__##_PRIORITY;	\
	assert_param( HAL_OK == HAL_DMA_Init( pDMA_hdl ) );									\
	__HAL_LINKDMA( huart, _HDMA_, COM##__COM_N__##_DMA##__RX_TX__##_hdl );				\
																						\
	HAL_NVIC_SetPriority(	DMA_IRQn( COM##__COM_N__##_DMA##__RX_TX__##_CHANNEL ),		\
							COM##__COM_N__##_DMA##__RX_TX__##_IRQ_PREEMTPRIORITY,		\
							COM##__COM_N__##_DMA##__RX_TX__##_IRQ_SUBPRIORITY ); 		\
	HAL_NVIC_EnableIRQ( DMA_IRQn( COM##__COM_N__##_DMA##__RX_TX__##_CHANNEL ) );		\
}
#else
#error "Select Target Family!"
#endif	// STM32xx


// Низкоуровневый (плтаформо-зависимый) инициализатор UART, вызывается из HAL_UART_Init() и т.п.
void HAL_UART_MspInit( UART_HandleTypeDef* huart )
{
	GPIO_InitTypeDef GPIO_InitStruct;

#ifdef	COM1_USE
	if( COM1_UART == huart->Instance )
	{
		COM_INIT_COMMON( 1 );
	#ifdef	COM1_USE_TXEN
		COM_INIT_GPIO_TXEN( 1 );
	#endif	// COM#_USE_TXEN
	#ifdef	COM1_USE_IRQ
		COM_INIT_IRQ( 1 );
	#endif	// COM#_USE_IRQ
	#ifdef	COM1_USE_DMATX
		COM_INIT_DMA( 1, TX, DMA_MEMORY_TO_PERIPH, hdmatx );
	#endif	// COM#_USE_DMATX
	#ifdef	COM1_USE_DMARX
		COM_INIT_DMA( 1, RX, DMA_PERIPH_TO_MEMORY, hdmarx );
	#endif	// COM#_USE_DMATX
	}
#endif	// COM1_USE

#ifdef	COM2_USE
	if( COM2_UART == huart->Instance )
	{
		COM_INIT_COMMON( 2 );
	#ifdef	COM2_USE_TXEN
		COM_INIT_GPIO_TXEN( 2 );
	#endif	// COM#_USE_TXEN
	#ifdef	COM2_USE_IRQ
		COM_INIT_IRQ( 2 );
	#endif	// COM#_USE_IRQ
	#ifdef	COM2_USE_DMATX
		COM_INIT_DMA( 2, TX, DMA_MEMORY_TO_PERIPH, hdmatx );
	#endif	// COM#_USE_DMATX
	#ifdef	COM2_USE_DMARX
		COM_INIT_DMA( 2, RX, DMA_PERIPH_TO_MEMORY, hdmarx );
	#endif	// COM#_USE_DMATX
	}
#endif	// COM2_USE

#ifdef	COM3_USE
	if( COM3_UART == huart->Instance )
	{
		COM_INIT_COMMON( 3 );
	#ifdef	COM3_USE_TXEN
		COM_INIT_GPIO_TXEN( 3 );
	#endif	// COM#_USE_TXEN
	#ifdef	COM3_USE_IRQ
		COM_INIT_IRQ( 3 );
	#endif	// COM#_USE_IRQ
	#ifdef	COM3_USE_DMATX
		COM_INIT_DMA( 3, TX, DMA_MEMORY_TO_PERIPH, hdmatx );
	#endif	// COM#_USE_DMATX
	#ifdef	COM3_USE_DMARX
		COM_INIT_DMA( 3, RX, DMA_PERIPH_TO_MEMORY, hdmarx );
	#endif	// COM#_USE_DMATX
	}
#endif	// COM3_USE

#ifdef	COM4_USE
	if( COM4_UART == huart->Instance )
	{
		COM_INIT_COMMON( 4 );
	#ifdef	COM4_USE_TXEN
		COM_INIT_GPIO_TXEN( 4 );
	#endif	// COM#_USE_TXEN
	#ifdef	COM4_USE_IRQ
		COM_INIT_IRQ( 4 );
	#endif	// COM#_USE_IRQ
	#ifdef	COM4_USE_DMATX
		COM_INIT_DMA( 4, TX, DMA_MEMORY_TO_PERIPH, hdmatx );
	#endif	// COM#_USE_DMATX
	#ifdef	COM4_USE_DMARX
		COM_INIT_DMA( 4, RX, DMA_PERIPH_TO_MEMORY, hdmarx );
	#endif	// COM#_USE_DMATX
	}
#endif	// COM4_USE

#ifdef	COM5_USE
	if( COM5_UART == huart->Instance )
	{
		COM_INIT_COMMON( 5 );
	#ifdef	COM5_USE_TXEN
		COM_INIT_GPIO_TXEN( 5 );
	#endif	// COM#_USE_TXEN
	#ifdef	COM5_USE_IRQ
		COM_INIT_IRQ( 5 );
	#endif	// COM#_USE_IRQ
	#ifdef	COM5_USE_DMATX
		COM_INIT_DMA( 5, TX, DMA_MEMORY_TO_PERIPH, hdmatx );
	#endif	// COM#_USE_DMATX
	#ifdef	COM5_USE_DMARX
		COM_INIT_DMA( 5, RX, DMA_PERIPH_TO_MEMORY, hdmarx );
	#endif	// COM#_USE_DMATX
	}
#endif	// COM5_USE

#ifdef	COM6_USE
	if( COM6_UART == huart->Instance )
	{
		COM_INIT_COMMON( 6 );
	#ifdef	COM6_USE_TXEN
		COM_INIT_GPIO_TXEN( 6 );
	#endif	// COM#_USE_TXEN
	#ifdef	COM6_USE_IRQ
		COM_INIT_IRQ( 6 );
	#endif	// COM#_USE_IRQ
	#ifdef	COM6_USE_DMATX
		COM_INIT_DMA( 6, TX, DMA_MEMORY_TO_PERIPH, hdmatx );
	#endif	// COM#_USE_DMATX
	#ifdef	COM6_USE_DMARX
		COM_INIT_DMA( 6, RX, DMA_PERIPH_TO_MEMORY, hdmarx );
	#endif	// COM#_USE_DMATX
	}
#endif	// COM6_USE

}

/* ************ Определение обработчиков прерывания от каждого UART, которые вызывают общую функцию HAL_UART_Ext_IRQHandler() ***************
Не понятно, как аккуратно разделять обработчики UART и USART для разных uC.
Обязательно контролировать при переходе на другой uC!!!
STM32F40x, 41x:
USART1
USART2
USART3
UART4
UART5
USART6
STM32F42x, 43x:
+UART7
+UART8
STM32L4x5, 4x6
USART1
USART2
USART3
UART4
UART5
LPUART1
*/

#define	UARTx_IRQHandler( __COM__ )		void UART##__COM__##_IRQHandler( void )		{	HAL_UART_Ext_IRQHandler( &COM##__COM__##_UART_hdl );	}
#define	USARTx_IRQHandler( __COM__ )	void USART##__COM__##_IRQHandler( void )	{	HAL_UART_Ext_IRQHandler( &COM##__COM__##_UART_hdl );	}
#define	LPUARTx_IRQHandler( __COM__ )	void LPUART##__COM__##_IRQHandler( void )	{	HAL_UART_Ext_IRQHandler( &COM##__COM__##_UART_hdl );	}

#ifdef	COM1_USE_IRQ
USARTx_IRQHandler( 1 );
#endif
#ifdef	COM2_USE_IRQ
USARTx_IRQHandler( 2 );
#endif
#ifdef	COM3_USE_IRQ
USARTx_IRQHandler( 3 );
#endif
#ifdef	COM4_USE_IRQ
UARTx_IRQHandler( 4 );
#endif
#ifdef	COM5_USE_IRQ
UARTx_IRQHandler( 5 );
#endif
#ifdef	COM6_USE_IRQ
USARTx_IRQHandler( 6 );
#endif
#ifdef	COM7_USE_IRQ
UARTx_IRQHandler( 7 );
#endif
#ifdef	COM8_USE_IRQ
UARTx_IRQHandler( 8 );
#endif


// ************ Определение обработчиков прерывания от каждого UART.DMA, которые вызывают общую функцию HAL_DMA_IRQHandler() ***************
#ifdef	COM1_USE_DMATX
void COM1_DMATX_IRQHandler( void )	{	HAL_DMA_IRQHandler( &COM1_DMATX_hdl );	}
#endif

#ifdef	COM1_USE_DMARX
void COM1_DMARX_IRQHandler( void )	{	HAL_DMA_IRQHandler( &COM1_DMARX_hdl );	}
#endif

#ifdef	COM2_USE_DMATX
void COM2_DMATX_IRQHandler( void )	{	HAL_DMA_IRQHandler( &COM2_DMATX_hdl );	}
#endif

#ifdef	COM2_USE_DMARX
void COM2_DMARX_IRQHandler( void )	{	HAL_DMA_IRQHandler( &COM2_DMARX_hdl );	}
#endif

#ifdef	COM3_USE_DMATX
void COM3_DMATX_IRQHandler( void )	{	HAL_DMA_IRQHandler( &COM3_DMATX_hdl );	}
#endif

#ifdef	COM3_USE_DMARX
void COM3_DMARX_IRQHandler( void )	{	HAL_DMA_IRQHandler( &COM3_DMARX_hdl );	}
#endif

#ifdef	COM4_USE_DMATX
void COM4_DMATX_IRQHandler( void )	{	HAL_DMA_IRQHandler( &COM4_DMATX_hdl );	}
#endif

#ifdef	COM4_USE_DMARX
void COM4_DMARX_IRQHandler( void )	{	HAL_DMA_IRQHandler( &COM4_DMARX_hdl );	}
#endif

#ifdef	COM5_USE_DMATX
void COM5_DMATX_IRQHandler( void )	{	HAL_DMA_IRQHandler( &COM5_DMATX_hdl );	}
#endif

#ifdef	COM5_USE_DMARX
void COM5_DMARX_IRQHandler( void )	{	HAL_DMA_IRQHandler( &COM5_DMARX_hdl );	}
#endif

#ifdef	COM6_USE_DMATX
void COM6_DMATX_IRQHandler( void )	{	HAL_DMA_IRQHandler( &COM6_DMATX_hdl );	}
#endif

#ifdef	COM6_USE_DMARX
void COM6_DMARX_IRQHandler( void )	{	HAL_DMA_IRQHandler( &COM6_DMARX_hdl );	}
#endif


