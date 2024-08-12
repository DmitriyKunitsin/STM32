// SKLP_MS_TransportInterface.h
// Протокол последовательной шины СКЛ [Master/Slave], описание последовательного интерфейса и передаваемых пакетов.
#ifndef	SKLP_MS_TRANSPORT_INTERFACE_H
#define	SKLP_MS_TRANSPORT_INTERFACE_H

#include "ProjectConfig.h"		// GPIO_CommonIndex_t
#include "common_uart.h"		// UART_Ext_HandleTypeDef
#include "common_tim.h"			// TIM_HandleTypeDef
#include "FreeRTOS.h"
#include "Queue.h"

// ************************* Типы данных *************************
// Состояния автомата приема пакета
typedef enum SKLP_State_enum
{
	SKLP_STATE_WaitSync = 0,	// Ожидание синхронизации (паузы на шине). Первый принятый фрагмент будет отброшен, но произойдет синхронизация
	SKLP_STATE_WaitStart,		// Ожидание стартового байта
	SKLP_STATE_WaitSize,		// Ожидание размера пакета
	SKLP_STATE_WaitTail,		// Ожидание конца пакета
	SKLP_STATE_PacketProcess,	// Пакет адресован к этому модулю, и должен быть передан на дальнейшую обработку
	SKLP_STATE_PacketReject,	// Пакет должен быть отброшен (ошибка формата, потребуется пересинхронизация)
	SKLP_STATE_PacketSkip,		// Пакет должен быть пропущен (формат нормальный, но адресован не к этому модулю - пересинхронизация не требуется)
} SKLP_State_t;

// Статистика приема и обработки пакетов по последовательному интерфейсу
typedef struct SKLP_Statistic_struct
{
	uint32_t FragmentsRecieved;				// количество принятых фрагментов по UART.Rx.Idle (если пакеты не склеиваются и не расслаиваются, соответствует количеству принятых пакетов)
	uint32_t PacketsRejectedInFormat;		// количество отброшенных пакетов (нарушение формата пакета)
	uint32_t PacketsRejectedInBuzy;			// количество отброшенных пакетов (не завершена обработка предыдущего пакета)
	uint32_t PacketsSkipped;				// количество пропущенных пакетов (формат без CRC нормальный, но адресованы не к этому модулю)
	uint32_t PacketsProcessed;				// количество пакетов, переданных на обработку (формат без CRC нормальный, адресованы к этому модулю)
	uint32_t PacketsProcessedSuccessful;	// количество успешно обработанных пакетов (нормальный формат пакета и команды)
	uint32_t PacketsTransmitted;			// количество переданных пакетов
	uint32_t HeadersRecieved;				// количество принятых заголовков (START+SIZE) по UART.Rx.Idle
} SKLP_Statistic_t;

// Заголовок принимаемого пакета
#pragma pack( 1 )
typedef struct SKLP_PacketHeader_struct
{
	uint8_t Start;
	uint8_t Size;
	uint8_t Address;
} SKLP_PacketHeader_t;
#pragma pack( )

// Тип длЯ опционального коллбека из SKLP_ProcessPacket() по завершению передачи пакета
struct SKLP_Interface_struct;
typedef void ( *SKLP_InterfaceCB_t )( struct SKLP_Interface_struct *pInterface );

// СобытиЯ, отправлЯемые в очередь интрерфейса из разборщика пакетов
typedef uint8_t SKLP_InterfaceEvent_t;
#define	EVENT_SKLP_QUERY_2ME		( ( SKLP_InterfaceEvent_t ) ( 1 << 0 ) )	// Пришел запрос на мой адрес
#define	EVENT_SKLP_QUERY_2ALL		( ( SKLP_InterfaceEvent_t ) ( 1 << 1 ) )	// Пришел широковещательный запрос
#define	EVENT_SKLP_QUERY_2OTHER		( ( SKLP_InterfaceEvent_t ) ( 1 << 2 ) )	// Пришел запрос на чужой адрес
#define	EVENT_SKLP_QUERY_GATEWAY	( ( SKLP_InterfaceEvent_t ) ( 1 << 3 ) )	// Пришел запрос на адрес, попадающий в список шлюзования (дополняет _2OTHER и, возможно, _2ALL)
#define	EVENT_SKLP_ANSWER			( ( SKLP_InterfaceEvent_t ) ( 1 << 4 ) )	// Пришел ответ на запрос
#define	EVENT_SKLP_NONE				( ( SKLP_InterfaceEvent_t ) 0 )				// Нет событий
#define	EVENT_SKLP_ALL				( EVENT_SKLP_QUERY_2ME | EVENT_SKLP_QUERY_2ALL | EVENT_SKLP_QUERY_2OTHER | EVENT_SKLP_QUERY_GATEWAY | EVENT_SKLP_ANSWER )

// Структура последовательного интерфейса для протокола СКЛ
typedef struct SKLP_Interface_struct
{
	char					*pName;					// Название
	UART_Ext_HandleTypeDef	*pUART_Hdl;				// Хендлер UART
	TIM_HandleTypeDef		*pTimerRxHdl;			// Хендлер таймера отслеживания паузы между байтами в пакете
	SKLP_Statistic_t		Statistic;				// Статистика приема и обработки пакетов

	// Ресурсы FreeRTOS
	QueueHandle_t			pMessageQueue;			// Очередь длЯ отправки событий от обработчиков прерываний UART
	SKLP_InterfaceEvent_t	EventsAllowed;			// Список ожидаемых событий
	TimerHandle_t			FastBaudTimerHandle;	// Таймер для автоматического возврата к низкой скорости UART при отсутствии запросов
	TickType_t				LastIncomingQueryTimeStamp;	// Отсечка времени поступлениЯ последнего входящего запроса
	TimerHandle_t			LedRxTimer;				// Таймер моргания светодиода по приему пакета
	GPIO_CommonIndex_t		LedRxPin;				// Ножка управлениЯ светодиодом

	// Служебные примочки парсера пакетов
	SKLP_State_t			State;					// Состояние автомата приема пакета
	SKLP_PacketHeader_t		PacketHeader;			// Копия заголовка обрабатываемого пакета
	uint32_t				RxBuffer_SavedState;	// Сохраненное состояние счетчика Rx.DMA.NDTR, используется для проверки активности приема через DMA, когда прерывание UART.Rx.Idle еще не возникло
	SKLP_InterfaceCB_t		xPacketTxCompleteCB;	// Опциональный коллбек из SKLP_ProcessPacket() по завершению передачи пакета
} SKLP_Interface_t;

// Сообщение, отправлЯемое в очередь интрерфейса из разборщика пакетов
typedef struct SKLP_Message_struct
{
	SKLP_Interface_t		*pInterface;	// интерфейс-отправитель (необходимо, если несколько интерфейсов работают на одну очередь)
	ByteQueueFragment_t		Packet;			// расположение принЯтого пакета в кольцевом буфере UART
	SKLP_InterfaceEvent_t	Event;		// тип возникшего событиЯ
	uint8_t mTxEventManchester;
	uint8_t mRxEventManchester;
} SKLP_Message_t;

// Формат формируемого пакета при передаче в последовательный интерфейс
typedef enum SKLP_SendPacketFormat_enum
{
	SKLP_SendPacketFormat_Query				= 0x01,	// запрос, стартоваЯ сигнатура SKLP_START_QUERY
	SKLP_SendPacketFormat_Answer			= 0x02,	// ответ, стартоваЯ сигнатура SKLP_START_ANSWER
	SKLP_SendPacketFormat_AnswerMemRead		= 0x03,	// ответ, стартоваЯ сигнатура SKLP_START_ANSWER, отсутствует поле [Size]
	SKLP_SendPacketFormat_MaskCommand		= 0x03,	// маска команды
	SKLP_SendPacketFormat_FlagPrepForAnswer	= 0x40,	// флаг: ждать завершениЯ передачи пакета, запрограммировать UART длЯ генерации событиЯ по получению ответа, но ответ не ждать
	SKLP_SendPacketFormat_FlagNoWait		= 0x80,	// флаг: не ожидать завершениЯ передачи пакета
} SKLP_SendPacketFormat_t;

// Результат выполнения запроса (команды) для мастера
typedef enum SKLPM_Result_enum
{
	SKLPM_ResultOK = 0,				// Команда выполнена успешно
	SKLPM_ResultOK_Skip,			// Опрос был пропущен в свЯзи с логикой работы драйвера модулЯ
	SKLPM_ResultOK_Restart,			// Необходимо перезапустить цикл опроса
	SKLPM_ResultOK_Finish,			// Необходимо завершить цикл опроса
	SKLPM_ResultFail,				// Ошибка (неидентифицированная)
	SKLPM_ResultFail_Internal,		// Ошибка внутренняя (???)
	SKLPM_ResultFail_Parameters, 	// Ошибка аргументов функции
	SKLPM_ResultFail_ChannelBuzy,	// Не удалось подключиться к последовательному каналу
	SKLPM_ResultFail_TxErr,			// Возникли ошибки при передаче запроса в UART
	SKLPM_ResultFail_TxTimeout, 	// Вышел таймаут при ожидании конца передачи запроса
	SKLPM_ResultFail_RxTimeout,		// Вышел таймаут на ожидание ответа (за отведенное времЯ не принЯт пакет с ответом)
	SKLPM_ResultFail_RxIdleTimeout,	// Вышел таймаут на ожидание ответа (за отведенное времЯ не было активности в канале приема)
	SKLPM_ResultFail_RxErr, 		// Возникли ошибки при приеме ответа из UART
	SKLPM_ResultFail_RxOwr,			// Переполнение буфера приемника
	SKLPM_ResultFail_RxEcho,		// В ответ на запрос принято эхо
//	SKLPM_ResultFail_RxFormat, 		// Ошибка формата принятого ответа (Старт, Размер, CRC)
	SKLPM_ResultFail_RxFormatSKLP,	// Ошибка формата принятого ответа по несущему протоколу (Старт, Размер, CRC)
	SKLPM_ResultFail_RxFormatPayload,	// Ошибка формата содержимого принятого ответа (Размер, аргументы и т.д.)
} SKLPM_Result_t;

// Коллбеки из обработчиков прерываний таймера и UART
void SKLP_TimerElapsed( SKLP_Interface_t *pInterface );		// Коллбек из TIM.Ovf. Пауза на шине - порвать пакет или пересинхронизировать
void SKLP_ReceiveFragment( SKLP_Interface_t *pInterface );	// Коллбек из USART.RxIdle. Принять фрагмент пакета из буфера UART. При получении полного пакета отправить на дальнейшую обработку

// Скопировать полученный пакет из кольцевого в линейный буфер, проверить заголовок и контрольную сумму
bool SKLP_ReceivePacket( uint8_t *pPacket, ByteQueueIndex_t PacketSizeMax, ByteQueue_t *pByteQueueRx, ByteQueueFragment_t PacketToRecive );
// ПринЯть пакет из буфера UART и запустить его обработку
void SKLP_ProcessPacket( SKLP_Interface_t *pInterface, ByteQueueFragment_t PacketToRecive );
// ПринЯть пакет из буфера UART и выполнить шлюзование
void SKLP_ProcessPacketGateway( SKLP_Interface_t *pInterfaceMaster, ByteQueueFragment_t PacketToGateway, UART_Ext_HandleTypeDef *pGatewayHdl_UART, SKLP_Interface_t *pGatewayHdl_Interface, uint16_t WaitAnswerTimeout_ms );
void SKLP_ProcessPacketGatewayV2( SKLP_Message_t *pMessageToGateway, SKLP_Interface_t *pInterface );
// Оформить пакет с заполненным полем данных, и отправить в UART
bool SKLP_SendPacket( uint8_t *pPacket, uint16_t PacketSize, SKLP_SendPacketFormat_t Format, UART_Ext_HandleTypeDef *pUART_Hdl );
// Запрос в интерфейс и ожидание ответа
SKLPM_Result_t SKLPM_Query( SKLP_Interface_t	*pInterface, uint8_t *pQuery, uint16_t QuerySize, uint8_t *pAnswer, uint16_t *pAnswerSize, TickType_t Timeout );
SKLPM_Result_t SKLPM_Query_RetMsg( SKLP_Interface_t	*pInterface, uint8_t *pQuery, uint16_t QuerySize, SKLP_Message_t *pReturnRxMessage, TickType_t Timeout );
// Распечатать статистику по работе последовательного канала в лог
void SKLP_PrintStatistics( SKLP_Interface_t *pInterface );
// Установить коллбек, однократно вызываемый из SKLP_ProcessPacket() после завершениЯ передачи пакета
void SKLP_SetTxCompleteCB( SKLP_Interface_t *pInterface, SKLP_InterfaceCB_t xPacketTxCompleteCB );

#endif	// SKLP_MS_TRANSPORT_INTERFACE_H

