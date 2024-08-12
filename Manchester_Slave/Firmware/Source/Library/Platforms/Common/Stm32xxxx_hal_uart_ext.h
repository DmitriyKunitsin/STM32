// Stm32l4xx_hal_uart_ext.h, расширение драйвера Stm32l4xx_hal_uart
// (дописать суть расширений)
#ifndef	STM32XXXX_HAL_UART_EXT
#define	STM32XXXX_HAL_UART_EXT

#include "Stm32xxxx_hal_uart.h"			// Стандартный драйвер
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

// Структура хендлера UART_Ext_HandleTypeDef.
// При обращении со стандартным API адрес структуры передается как обычный хендлер UART_HandleTypeDef.
// Если коллбек приводит к вызову раширенного API, то хендлер разворачивается в UART_Ext_HandleTypeDef.
// ??Вообще-то, мелкий хак (если где-то еще используется такой подход). При каких обстоятельствах возможно влететь??
typedef struct
{
	// Структура UART из обычной библиотеки Stm32f4xx_hal_uart
	UART_HandleTypeDef	Common;
	// Дополнительные поля
	USART_TypeDef		*Instance;				// Дублирование поля Common.Instance - некоторая защита от неправльного использования драйвера
	GPIO_TypeDef		*TXEN_GPIO;				// Управление RS485.TxEn (при необходимости)
	uint16_t			TXEN_Pin;				// Управление RS485.TxEn (при необходимости)
#ifdef	USE_FREERTOS
	EventGroupHandle_t	EventGroup;				// События при работе с расширенным драйвером (можно удалить при необходимости)
//#else
//	EventBits_t EventBitsResult;
#endif	// USE_FREERTOS
	EventBits_t			EventBitsMask;			// Маска событий, которые необходимо формировать
	ByteQueue_t			*pByteQueueRx;			// Кольцевой буфер для приема через DMA
	
	UART_Ext_Callback_t	TxCompleteCallback;		// Опциональный коллбек из обработчика прерывания по завершению передачи
	UART_Ext_IRQ_Handlers_t IRQ_Handlers;		// Обработчики прерываний. Дл STM32L4xx они уже включены в UART_HandleTypeDef но длЯ STM32F4xx их нет
	// Добавить мьютекс?
	// Добавить таймер??
} UART_Ext_HandleTypeDef;

// События при работе с UART_Ext
#define	EVENT_UART_EXT_TX_ERROR			( 1 << 0 )		// Возникла ошибка при передаче пакета
#define	EVENT_UART_EXT_TX_COMPLETE		( 1 << 1 )		// Передача пакета полностью завершена (по UART.TxC)
#define	EVENT_UART_EXT_TX_DMA_COMPLETE	( 1 << 2 )		// Буфер DMA передан в UART (по DMA.TxC)
#define	EVENT_UART_EXT_TX_ALL			( EVENT_UART_EXT_TX_ERROR | EVENT_UART_EXT_TX_COMPLETE | EVENT_UART_EXT_TX_DMA_COMPLETE )
#define	EVENT_UART_EXT_RX_ERROR			( 1 << 3 )		// Возникла ошибка при приеме байта
#define	EVENT_UART_EXT_RX_IDLE			( 1 << 4 )		// Возникла короткая пауза (байт) после приема последнего байта
#define	EVENT_UART_EXT_RX_COMPLETE		( 1 << 5 )		// Принято указанное количество байт
#define	EVENT_UART_EXT_RX_OVR			( 1 << 6 )		// Произошло переполнение буфера
#define	EVENT_UART_EXT_RX_ALL			( EVENT_UART_EXT_RX_ERROR | EVENT_UART_EXT_RX_IDLE | EVENT_UART_EXT_RX_COMPLETE | EVENT_UART_EXT_RX_OVR )
#define	EVENT_UART_EXT_TIMEOUT			( 1 << 7 )		// Вышел таймаут ожидания завершения (пока только внешняя поддержка?)
//#define	EVENT_UART_EXT_ALL				( EVENT_UART_EXT_TX_ALL | EVENT_UART_EXT_RX_ALL | EVENT_UART_EXT_TIMEOUT | EVENT_UART_EXT_MUTEX | EVENT_UART_EXT_MODEM_INIT_COMPLETE )
#define	EVENT_UART_EXT_ALL				( EVENT_UART_EXT_TX_ALL | EVENT_UART_EXT_RX_ALL | EVENT_UART_EXT_TIMEOUT )
//#define	EVENT_UART_EXT_MUTEX			( 1 << 8 )		// !!тест!!	Мьютекс - UART свободен для подключения к нему очередного протокола
#define	EVENT_UART_EXT_MODEM_INIT_COMPLETE	( 1 << 13 )		// !!тест!!	Завершена инициализация подключенного оборудования (напр., модема)
#define	EVENT_UART_EXT_INIT_COMPLETE		( 1 << 14 )		// !!тест!! UART инициализирован
#define	EVENT_UART_EXT_RX_ENABLED			( 1 << 15 )		// !!тест!!не событие! флаг в EventBitsMask показывает, что на момент запуска передачи был разрешен прием. перед передачей прием будет запрещен, после завершениЯ передачи прием может быть включен
#define	EVENT_UART_EXT_ACCESS_READY			( 1 << 12 )		// !!тест!! мьютекс: 1 - готовность к захвату, 0 - захвачен

#if		defined( STM32F4 )
	#define	UART_INSTANCE_TDR( __INSTANCE__ )	( ( __INSTANCE__ )->DR )
	#define UART_INSTANCE_RDR( __INSTANCE__ )	( ( __INSTANCE__ )->DR )
#elif	defined( STM32L4 ) || defined( STM32F3 )
	#define UART_INSTANCE_TDR( __INSTANCE__ )	( ( __INSTANCE__ )->TDR )
	#define UART_INSTANCE_RDR( __INSTANCE__ )	( ( __INSTANCE__ )->RDR )
#else
#error "Select Target Family!"
#endif

// Прототипы функций
HAL_StatusTypeDef HAL_UART_Ext_Init( UART_Ext_HandleTypeDef *huart );				// Инициализация расширенного хедлера UART
HAL_StatusTypeDef HAL_UART_Ext_DeInit( UART_Ext_HandleTypeDef *huart );			// Деинициализация расширенного хедлера UART
HAL_StatusTypeDef HAL_UART_Ext_Transmit( UART_Ext_HandleTypeDef *huart_ext, uint8_t *pData, uint16_t Size, EventBits_t EventBitsMask );			// Передать буфер в UART через DMA.
HAL_StatusTypeDef HAL_UART_Ext_ReceiveCyclicStart( UART_Ext_HandleTypeDef *huart_ext, ByteQueue_t *pByteQueueRx, EventBits_t EventBitsMask );	// Запустить прием из UART по DMA в буфер pByteQueueRx циклически
HAL_StatusTypeDef HAL_UART_Ext_ReceiveCyclicStop( UART_Ext_HandleTypeDef *huart_ext );															// Закончить прием из UART по DMA в циклический буфер
HAL_StatusTypeDef HAL_UART_Ext_ReceiveCyclicReset( UART_Ext_HandleTypeDef *huart_ext );															// Очистить приемный буфер
HAL_StatusTypeDef HAL_UART_Ext_ReceiveStart( UART_Ext_HandleTypeDef *huart_ext, uint8_t *pData, uint16_t Size, EventBits_t EventBitsMask );		// Запустить прием из UART по DMA в буфер фиксированного размера (до заполнения буфера или Idle). Как контролировать количество принятых байт?
HAL_StatusTypeDef HAL_UART_Ext_ReceiveStop( UART_Ext_HandleTypeDef *huart_ext );																// Закончить прием из UART по DMA в буфер
bool UART_Ext_RxDMA_CheckActivity( UART_Ext_HandleTypeDef *huart_ext, uint32_t *pPrevState );	// Проверка активности при приеме в циклический буфер по DMA - сравнение текущего DMA.NDTR и предыдущего, сохраненного в *pPrevState
void HAL_UART_Ext_IRQHandler( UART_HandleTypeDef *huart );						// Обработчик прерывания UART для расширенного драйвера
void HAL_UART_RxIdleCallback( UART_HandleTypeDef *huart );						// Коллбек по прерыванию UART.RxIdle
void HAL_UART_Ext_ResetBaudrate( UART_HandleTypeDef *huart );					// Изменить скорость UART без прочей переинициализации
uint32_t HAL_UART_Ext_CalcByteRate( UART_Ext_HandleTypeDef *huart_ext );		// [байт/с]	Рассчитать байтовую скорость интерфейса
float HAL_UART_Ext_CalcTimeToTransmite( UART_Ext_HandleTypeDef *huart_ext, uint16_t PacketSize );		// [с]	Рассчитать времЯ на передачу пакета
HAL_StatusTypeDef HAL_UART_Ext_ReceiveRawStart( UART_Ext_HandleTypeDef *huart_ext, UART_Ext_IT_t xRxIT, UART_Ext_IT_t xRxIdleIT );	// Запуск UART на прием, обработчики прерываний реализованы в приложении.
HAL_StatusTypeDef HAL_UART_Ext_ReceiveRawStop( UART_Ext_HandleTypeDef *huart_ext );												// Закончить прием из UART, начатый в HAL_UART_Ext_ReceiveRawStart()
UART_Ext_Callback_t UART_Ext_SetTransferCompleteCallback( UART_Ext_HandleTypeDef *huart_ext, UART_Ext_Callback_t xCallback );		// Установить коллбек при завершении передачи пакета через UART
BaseType_t HAL_UART_Ext_ValidateHdl( UART_Ext_HandleTypeDef *huart );			// Проверка, что хендлер huart является расширенной версией стандартного хендлера

#endif	// STM32XXXX_HAL_UART_EXT

