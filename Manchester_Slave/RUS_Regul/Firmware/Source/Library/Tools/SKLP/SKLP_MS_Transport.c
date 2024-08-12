// SKLP_MS_Transport.h
// Протокол последовательной шины СКЛ, разбор пакетов на шине.
#include "ProjectConfig.h"		// конфиг платформы
#include "stm32xxxx_hal.h"		// дрова периферии
#include "platform_common.h"	// GPIO_Common_Write()
#include "common_gpio.h"		// GPIO_Common_Write()
#include "SKLP_MS_Transport.h"	// родной
#include "SKLP_MS_TransportInterface.h"	// родной
#include "SKLP_Service.h"
#include "Logger.h"
#include <stdio.h>
#include "RebootUtils.h"		// Использовать буфер ResetInfo.aLogMessage в качестве временного для вывода статитстики
#include "Utils.h"

// Проверка допустимости аргумента "интерфейс".
// ДлЯ внутренних функций можно не использовать, а длЯ внешних - желательно
static bool SKLP_InterfaceValidate( SKLP_Interface_t *pInterface )
{
	bool Result = false;
	do
	{
		if( NULL == pInterface )
			break;
		if( !HAL_UART_Ext_ValidateHdl( pInterface->pUART_Hdl ) )
			break;
		// !!! Пока прием из UART реализован в байтовую очередь, проверить исправность этой очереди!
		if( !ByteQueue_Validate( pInterface->pUART_Hdl->pByteQueueRx ) )
			break;
		// !!! Может, надо еще что-нибудь проверить из обЯзательных компонентов? Без проверки pByteQueueRx уже стрельнуло!
		Result = true;
	} while( 0 );
	return Result;
}

// Обслуживание таймера отслеживания паузы между байтами в пакете
// ********************************
// Перезапуск таймера
static void SKLP_TimerRx_Restart( SKLP_Interface_t *pInterface )
{
	UART_Ext_RxDMA_CheckActivity( pInterface->pUART_Hdl, &pInterface->RxBuffer_SavedState );	// Сохранить текущее состояние UART.Rx.DMA.NDTR, чтобы впоследствии можно было преверять активность приема
	__HAL_TIM_CLEAR_FLAG( pInterface->pTimerRxHdl, TIM_FLAG_UPDATE );
	__HAL_TIM_SET_COUNTER( pInterface->pTimerRxHdl, 0 );		// ранее было __HAL_TIM_SetCounter()
}

// Запуск и разрешение прерываний от таймера
static void SKLP_TimerRx_Start( SKLP_Interface_t *pInterface )
{
	SKLP_TimerRx_Restart( pInterface );
	__HAL_TIM_ENABLE_IT( pInterface->pTimerRxHdl, TIM_IT_UPDATE );
	__HAL_TIM_ENABLE( pInterface->pTimerRxHdl );
}

// Остановка таймера и запрещение прерываний
/*static*/ void SKLP_TimerRx_Stop( SKLP_Interface_t *pInterface )	
{
	__HAL_TIM_DISABLE( pInterface->pTimerRxHdl );
	__HAL_TIM_DISABLE_IT( pInterface->pTimerRxHdl, TIM_IT_UPDATE );
	__HAL_TIM_CLEAR_FLAG( pInterface->pTimerRxHdl, TIM_FLAG_UPDATE );
}

// Коллбек из IRQ.TIM.Ovf. Пауза на шине - порвать пакет или пересинхронизировать.
void SKLP_TimerElapsed( SKLP_Interface_t *pInterface )
{
	assert_param( SKLP_InterfaceValidate( pInterface ) );
	ByteQueueIndex_t PendingBytesCount = ByteQueue_GetSpaceFilled( pInterface->pUART_Hdl->pByteQueueRx );				// Узнать количество необработанных данных в приемном буфере UART
	bool RxDMA_NoActivity = !UART_Ext_RxDMA_CheckActivity( pInterface->pUART_Hdl, &pInterface->RxBuffer_SavedState );	// Проверить событие приема новых байт в буфер с момента последней проверки
	if( SKLP_STATE_WaitSync == pInterface->State )
	{	// Находимся в режиме ожидании синхронизации - в течении опред. времени на шине должна быть тишина
		if( RxDMA_NoActivity )
		{	// За время ожидания синхронизации не было получено байтов из UART по DMA
			pInterface->State = SKLP_STATE_WaitStart;		// Поймана синхро-пауза, теперь ждать новый пакет
		}
		else
		{	// За время ожидания синхронизации были получены байты из UART по DMA, синхронизация не удалась.
			// Приемник по-прежнему в состоянии ожидания синхро-паузы.
			// Когда принимаемый поток данных прекратится, будет вызван обработчик UART.RxIdle,
			// где будут отброшены все принятые байты, и ожидание синхро-паузы будет запущено заново.
		}
	}
	else
	{	// Находимся в режиме приема пакета. Таймер был запущен после короткой паузы внутри пакета.
		// Проверить активность на шине - возможно, прием данных уже продолжился!
		if( RxDMA_NoActivity )
		{	// С момента запуска таймера новых данных так и не пришло - пакет порван!
			assert_param( ByteQueue_RemoveFromTail( pInterface->pUART_Hdl->pByteQueueRx, NULL, PendingBytesCount ) );	// Отбросить ранее принятый фрагмент
			pInterface->State = SKLP_STATE_WaitStart;		// Перейти к ожиданию нового пакета
		}
		else
		{	// Продолжается прием данных, пакет не порван.
		}
	}
	// В любом случае, остановить таймер. Он будет снова запущен после приема очередного фрагмента
	SKLP_TimerRx_Stop( pInterface );
}

#ifdef	SKLP_SIZE_PACKET_LONG_MAX
#warning "!!! ТЕСТИРОВАНИЕ ПОДДЕРЖКИ ДЛИННЫХ ПАКЕТОВ SKLP !!!"
#endif

// Обслуживание UART
// ********************************
// Коллбек из IRQ.USART.RxIdle.
// Принять фрагмент пакета из буфера UART. При получении полного пакета передать сообщение в очередь.
// Тайминги обработки прерываниЯ длЯ 602_01 МПИ - STM32F405 @ 67 МГц:
// - отбросить кривой пакет - до 12 мкс
// - принЯть валидный пакет - до 20 мкс
void SKLP_ReceiveFragment( SKLP_Interface_t *pInterface )
{
	assert_param( SKLP_InterfaceValidate( pInterface ) );
	ByteQueue_t * const pByteQueueRx = pInterface->pUART_Hdl->pByteQueueRx;	// Приемный кольцевой буфер UART
	
	// Узнать количество необработанных данных в приемном буфере UART
	ByteQueueIndex_t PendingBytesCount = ByteQueue_GetSpaceFilled( pByteQueueRx );	// !! Критично, чтобы до обработки запроса (после USART.RxIdle) не был принят ни один байт!
	assert_param( 0 != PendingBytesCount );
	pInterface->Statistic.FragmentsRecieved++;

	if ( PendingBytesCount == pByteQueueRx->Size )
	{
		// Очередь скорее всего пуста! но почему тогда возникло прерывание?
		// Либо очередь заполена под завязку?
		// А давай просто очистим очередь, все равно уже не распарсить.
		// Продвинуть хвост в кольцевом буфере, не сохраняя удаляемые данные
		assert_param( ByteQueue_RemoveFromTail( pByteQueueRx, NULL, PendingBytesCount ) );
		// Запустить процедуру пересинхронизации
		SKLP_TimerRx_Start( pInterface );
		pInterface->State = SKLP_STATE_WaitSync;
		pInterface->Statistic.PacketsSkipped++;
	}

	bool bContinueParsing;				// Флаг необходимости повторной обработки пришедшего пакета
	do
	{	// Цикл для разбора склеенных пакетов
		bContinueParsing = false;
		SKLP_InterfaceEvent_t EventOccured;
		uint16_t PacketSizeExpected;

		// Проверить размер фрагмента
#ifndef	SKLP_SIZE_PACKET_LONG_MAX
		if( PendingBytesCount >= SKLP_SIZE_PACKET_MAX ) 		// запрещен прием "длинных" пакетов
#else
		if( PendingBytesCount > SKLP_SIZE_PACKET_LONG_MAX ) 	// пробуем принимать "длинные" пакеты
#endif
		{	// Отбросить пакет и переинициализировать прием
			pInterface->State = SKLP_STATE_WaitSync;
			// !!! ПоЯвилось в свЯзи с инцендентом зацикливаниЯ передачи на АКП(б)
		}

		// Первичный разбор фрагмента
		// Проверить состояние приема на момент возникновения UART.RxIdle
		switch( pInterface->State )
		{
		case SKLP_STATE_WaitSync:	// Фрагмент прилетел во время ожидания паузы на шине
			pInterface->State = SKLP_STATE_PacketReject;	// Отбросить фрагмент и продолжить ожидание паузы
			break;

		case SKLP_STATE_WaitStart:	// Фрагмент прилетел во время ожидания начала нового пакета
			pInterface->PacketHeader.Start = ByteQueue_Peek( pByteQueueRx, SKLP_OFFSET_START );
			if( ( SKLP_START_QUERY != pInterface->PacketHeader.Start ) && ( SKLP_START_ANSWER != pInterface->PacketHeader.Start ) )
			{	// Начало пакета неверное. Ждать следующий фрагмент
				pInterface->State = SKLP_STATE_PacketReject;
				break;
			}
			// Пришел Старт. Ждать продолжение пакета
			pInterface->State = SKLP_STATE_WaitSize;
			// Продолжить анализ фрагмента
	
		case SKLP_STATE_WaitSize:	// Фрагмент прилетел во время ожидания байта размера пакета
			if( PendingBytesCount <= SKLP_OFFSET_SIZE )
			{	// Размер принятого пакета меньше ожидаемого. Подожать еще.
				break;
			}
			pInterface->PacketHeader.Size = ByteQueue_Peek( pByteQueueRx, SKLP_OFFSET_SIZE );
#pragma	diag_suppress=Pa084		// Warning[Pa084]: pointless integer comparison, the result is always false
			if( ( pInterface->PacketHeader.Size < ( ( SKLP_START_QUERY == pInterface->PacketHeader.Start ) ? SKLP_SIZE_MIN_QUERY : SKLP_SIZE_MIN_ANSWER ) )
#ifndef	SKLP_SIZE_PACKET_LONG_MAX
				|| ( pInterface->PacketHeader.Size >= SKLP_SIZE_MAX )
#endif	//SKLP_SIZE_PACKET_LONG_MAX
				)
#pragma diag_default=Pe084		// Вернуть компиляТор в исходное
			{	// ПрниЯт недопустимый размер пакета. Ждать следующий пакет.
				pInterface->State = SKLP_STATE_PacketReject;
				break;
			}
			// Пришел Размер. Ждать окончание пакета
			pInterface->State = SKLP_STATE_WaitTail;
			// !!! Тест! Зафиксировать событие, что были принЯты два первых байта заголовка.
			// !!! ИспользуетсЯ в МПИ длЯ анализа состоЯниЯ шины при приеме ответов от модулей.
			pInterface->Statistic.HeadersRecieved++;
			// Продолжить анализ пакета
	
		case SKLP_STATE_WaitTail:	// Фрагмент прилетел во время ожидания завершениЯ пакета
			PacketSizeExpected = pInterface->PacketHeader.Size + ( uint16_t ) SKLP_SIZE_TRIM;	// определить ожидаемый размер пакета. в случае "длинного" пакета, будет (255+3)
			if( PendingBytesCount < PacketSizeExpected )
			{	// Размер принятого пакета меньше ожидаемого. Подожать еще.
				break;
			}
			if(
#ifdef	SKLP_SIZE_PACKET_LONG_MAX
				( PacketSizeExpected < SKLP_SIZE_PACKET_MAX ) &&	// разрешены "длинные" пакеты, но этот пакет не длинный
#endif	//SKLP_SIZE_PACKET_LONG_MAX
				( PendingBytesCount > PacketSizeExpected ) )
			{	// Размер принятого пакета больше ожидаемого. Отбросить этот пакет, но сначала проверить на особый случай при работе с SIG60
				if( ( pInterface->pUART_Hdl->EventBitsMask & EVENT_UART_EXT_MODEM_INIT_COMPLETE ) &&	// UART работает через модем SIG60
					( ( PendingBytesCount == ( PacketSizeExpected + 1 ) ) ||	// пакет от модема иногда увеличиваетсЯ на 1 байт при сбоЯх на линии
					  ( PendingBytesCount == ( PacketSizeExpected + 2 ) ) ) )	// совсем особый случай - пакет от глючного ГГП на команду [0x73] всегда добавлЯет лишний байт после CRC, а модем при сбое может добавить второй лишний байт
				{	// Произвести обработку глючного пакета. Лишние байты из хвоста отбросить (далее)
				}
				else
				{	// Размер принятого пакета больше ожидаемого. Пропустить этот пакет
					pInterface->State = SKLP_STATE_PacketSkip;
					// Вроде как д.б. SKLP_STATE_PacketReject, но см. дальнейшую обработку SKLP_STATE_PacketSkip:
					// Там при принЯтии "увеличенного" пакета отбрасываетсЯ только его начало по определенному размеру,
					// а оставшаЯсЯ часть снова обрабатываетсЯ на предмет соответствиЯ протоколу.
					// Зачем-то это было надо, хорошо бы проЯснить.
					break;
				}
			}
			// Размер принятого пакета равен ожидаемому. Проверить, кому адресован пакет,
			// и допускаетсЯ ли обработка пакета с таким адресом.
			EventOccured = EVENT_SKLP_NONE;
			switch( pInterface->PacketHeader.Start )
			{
			case SKLP_START_ANSWER:		// пришел ответ на запрос
				if( pInterface->EventsAllowed & EVENT_SKLP_ANSWER )
					EventOccured = EVENT_SKLP_ANSWER;
				break;
				
			case SKLP_START_QUERY:		// пришел запрос
				// Проверить, кому адресован запрос
				pInterface->PacketHeader.Address = ByteQueue_Peek( pByteQueueRx, SKLP_OFFSET_ADDRESS );
				switch( pInterface->PacketHeader.Address )
				{
				case SKLP_ADDRESS_BROADCAST:		// широковещательный запрос
					if( pInterface->EventsAllowed & EVENT_SKLP_QUERY_2ALL )
						EventOccured = EVENT_SKLP_QUERY_2ALL;
					break;
				case SKLP_ADDRESS_MYSELF:			// запрос адресован этому модулю
#ifdef	SKLP_ADDRESS_MYSELF_SECOND
				case SKLP_ADDRESS_MYSELF_SECOND:	// запрос адресован этому модулю (второй допустимый адрес)
#endif
#ifdef	SKLP_ADDRESS_MYSELF_THIRD
				case SKLP_ADDRESS_MYSELF_THIRD:		// запрос адресован этому модулю (третий допустимый адрес)
#endif
					if( pInterface->EventsAllowed & EVENT_SKLP_QUERY_2ME )
						EventOccured = EVENT_SKLP_QUERY_2ME;
					break;
				default:							// запрос адресован другому модулю
#ifdef	SKLP_ADDRESS_CUSTOM
					// Спец-обработка для адреса, получаемого в Run-Time
					extern uint8_t SKLP_CustomAddressGet( void );
					if( SKLP_CustomAddressGet() == pInterface->PacketHeader.Address )
						if( pInterface->EventsAllowed & EVENT_SKLP_QUERY_2ME )
						{
							EventOccured = EVENT_SKLP_QUERY_2ME;
							break;
						}
#endif
					if( pInterface->EventsAllowed & EVENT_SKLP_QUERY_2OTHER )
						EventOccured = EVENT_SKLP_QUERY_2OTHER;
					break;
				}
#ifdef	SKLP_GATEWAY_LIST
				if( pInterface->EventsAllowed & EVENT_SKLP_QUERY_GATEWAY )
				{	// Если разрешено шлюзование, проверить на соответствие адреса в запросе списку на шлюзование
					const uint8_t aGatewayAddresses[] = { SKLP_GATEWAY_LIST };
					for( int i = 0; i < SIZEOFARRAY( aGatewayAddresses ); i++ )
						if( aGatewayAddresses[i] == pInterface->PacketHeader.Address )
						{	// Выставить флаг принадлежности адреса шлюзуемым пакетам
							EventOccured |= EVENT_SKLP_QUERY_GATEWAY;
							break;
						}
				}
#endif	// SKLP_GATEWAY_LIST
				// Засечь время принЯтиЯ запроса. ТребуетсЯ длЯ МПИ при отслеживании активности другого мастера (ПК)
				pInterface->LastIncomingQueryTimeStamp = xTaskGetTickCount( );
				break;
			}
			
			if( EVENT_SKLP_NONE == EventOccured )
			{	// Пакет не вызывает интереса. Ждать следующий пакет.
				pInterface->State = SKLP_STATE_PacketSkip;
			}
			else
			{	// Пакет адресован к нам. Передать пакет на дальнейшую обработку, вне обработчика прерывания.
				pInterface->State = SKLP_STATE_PacketProcess;
			}
			break;
		
		// Состояния SKLP_STATE_PacketReject, SKLP_STATE_PacketSkip и SKLP_STATE_PacketProcess
		// могут появляться только внутри этого switch(),
		// и должны быть обработаны и завершены в следующем switch().
		default:
			assert_param( 0 );
		}
	
		// Проверить необходимость зажечь светодиод по приему пакета
		if( NULL != pInterface->LedRxTimer )
			switch( pInterface->State )
			{
/*			case SKLP_STATE_PacketSkip:
				if( bContinueParsing )
				{			// СитуациЯ возникает при получении "неправильного" фрагмента, который выгребаетсЯ из буфера
					break;	// в несколько этапов в этом цикле do {} while( bContinueParsing ).
				}			// В этом случае нет смысла несколько раз зажигать светодиод, но главное категорически нельзЯ вызывать xTimerResetFromISR() из-за возможноо переполнениЯ очереди.
*/
			case SKLP_STATE_PacketProcess:
				// По приему нормального пакета зажечь светодиод и запустить таймер, который его потом погасит
				assert_param( pdPASS == xTimerResetFromISR( pInterface->LedRxTimer, pdFALSE ) );
			}

		// Выделить ожидаемый размер пакета
		PacketSizeExpected = pInterface->PacketHeader.Size + ( uint16_t ) SKLP_SIZE_TRIM;
		// Продолжить обработку в зависимости от результата предыдущего switch()
		switch( pInterface->State )
		{
		case SKLP_STATE_PacketReject:	// В результате предыдущего анализа решено отбросить принятый пакет из-за ошибки формата
			// Продвинуть хвост в кольцевом буфере, не сохраняя удаляемые данные
			assert_param( ByteQueue_RemoveFromTail( pByteQueueRx, NULL, PendingBytesCount ) );
			// Запустить процедуру пересинхронизации
			SKLP_TimerRx_Start( pInterface );
			pInterface->State = SKLP_STATE_WaitSync;
			pInterface->Statistic.PacketsRejectedInFormat++;
			break;
		
		case SKLP_STATE_PacketSkip:		// В результате предыдущего анализа решено отбросить принятый пакет (несоответсвие адреса или слишком большой пакет)
			SKLP_TimerRx_Stop( pInterface );	// нет необходимости ловить длительность паузы после нормального пакета - она может быть произвольной
			// В случае получения фрагмента, склеенного из нескольких пакетов, пропустить только очередной пакет, и продолжить обработку оставшейся части фрагмента
			assert_param( PendingBytesCount >= PacketSizeExpected );
#ifdef	SKLP_SIZE_PACKET_LONG_MAX
			if( PacketSizeExpected < SKLP_SIZE_PACKET_MAX )
			{	// "короткий" пакет отбросить в обычном порЯдке
#endif	// SKLP_SIZE_PACKET_LONG_MAX
				assert_param( ByteQueue_RemoveFromTail( pByteQueueRx, NULL, PacketSizeExpected ) );	// продвинуть хвост в кольцевом буфере, не сохраняя удаляемые данные
				PendingBytesCount -= PacketSizeExpected;
				if( PendingBytesCount > 0 )
				{	// Из склеенного фрагмента вырезан сбойный пакет, а оставшийся фрагмент может быть нормальным пакетом - (как-то маловероЯтно?)
					bContinueParsing = true;	// продолжить обработку оставшегося фрагмента
				}
#ifdef	SKLP_SIZE_PACKET_LONG_MAX
			}
			else
			{	// "длинный" пакет отбросить полностью, без попытки склеиваниЯ
				assert_param( ByteQueue_RemoveFromTail( pByteQueueRx, NULL, PendingBytesCount ) ); // продвинуть хвост в кольцевом буфере, не сохраняя удаляемые данные
			}
#endif	// SKLP_SIZE_PACKET_LONG_MAX
			// Ожидать следующий пакет
			pInterface->State = SKLP_STATE_WaitStart;
			pInterface->Statistic.PacketsSkipped++;
			break;

		case SKLP_STATE_PacketProcess:	// В результате предыдущего анализа решено передать пакет на дальнейшую обработку.
			SKLP_TimerRx_Stop( pInterface );	// нет необходимости ловить длительность паузы после нормального пакета - она может быть произвольной
			// Зафиксирован потенциально валидный пакет в приемном буфере.
#ifdef	SKLP_SIZE_PACKET_LONG_MAX
			if( PacketSizeExpected < SKLP_SIZE_PACKET_MAX )
			{	// размер "короткого" пакета проверить в обычном порЯдке
#endif	// SKLP_SIZE_PACKET_LONG_MAX
				if( 0 == ( pInterface->pUART_Hdl->EventBitsMask & EVENT_UART_EXT_MODEM_INIT_COMPLETE ) )
				{	// Размер принЯтого пакета должен быть строго соответствовать полю "Размер" в пакете
					assert_param( PacketSizeExpected == PendingBytesCount );
				}
				else
				{	// В случае модема, допускаетсЯ получение пакета с "лишними" байтами - см. обработку switch() case SKLP_STATE_WaitTail
					assert_param( ( ( PendingBytesCount - PacketSizeExpected ) >= 0 ) && ( ( PendingBytesCount - PacketSizeExpected ) <= 2 ) );
				}
#ifdef	SKLP_SIZE_PACKET_LONG_MAX
			}
			else
			{	// Размер "длинного" пакета должен быть "длинным"
				assert_param( PacketSizeExpected <= PendingBytesCount );
			}
#endif	// SKLP_SIZE_PACKET_LONG_MAX
			// Передать активному процессу на интерфейсе сообщение с положением пакета в кольцевом буфере UART.Rx.
			{	// В сообщении указать "правильный" размер пакета. Если в конце фрагмента были лишние байты, они будут отброшены ниже в ByteQueue_RemoveFromTail()
				SKLP_Message_t Message = { pInterface, { pByteQueueRx->iTail, PacketSizeExpected }, EventOccured };
#ifdef	SKLP_SIZE_PACKET_LONG_MAX
				if( PacketSizeExpected >= SKLP_SIZE_PACKET_MAX )
				{	// "Длинный" пакет отправить на обработку целиком, т.к. его ожидаемый размер не известен
					Message.Packet.Size = PendingBytesCount;
				}
#endif	// SKLP_SIZE_PACKET_LONG_MAX
				BaseType_t xHigherPriorityTaskWoken = pdFALSE;
				assert_param( NULL != pInterface->pMessageQueue );
				if( pdPASS == xQueueSendFromISR( pInterface->pMessageQueue, &Message, &xHigherPriorityTaskWoken ) )
				{	// Сообщение помещено в очередь приемника
					portYIELD_FROM_ISR( xHigherPriorityTaskWoken ); // активировать переключение планировщика после выхода из прерываниЯ
				}
				else
				{	// Не удалось добавить пакет в очередь, видимо предыдущие пакеты еще не обработаны
					pInterface->Statistic.PacketsRejectedInBuzy++;	// ситуациЯ нехорошаЯ, но не на столько, чтобы ассертить
				}
			}

			// Пакет будет скопирован из кольцевого буфера по мере дальнейшей обработки отправленного сообщениЯ.
			// Продвинуть хвост в кольцевом буфере. Информация о положении "удаляемого" пакета уже сохранена и в дальнейшем будет использована при его извлечении.
			assert_param( ByteQueue_RemoveFromTail( pByteQueueRx, NULL, PendingBytesCount ) );
			// Перейти к ожиданию следующего фрагмента.
			pInterface->State = SKLP_STATE_WaitStart;
			break;

		default:
			// Принят только фрагмент пакета. Пока не ясно, нормальный пакет, либо битый
			SKLP_TimerRx_Start( pInterface );	// приступить к ожиданию следующей паузы
		}
	} while( bContinueParsing );
}

// ****************************************************************
// Обработка принЯтого пакета.
// ЗапускаетсЯ из задачи, обеспечивающей выполнение протокола, после получениЯ сообщениЯ из SKLP_ReceiveFragment().
// Задача SKLP_Task() обычно реализована в SKLP_MS_TransportLocal.c
// ****************************************************************

// Линейный буфер для обработки принимаемых пакетов и формирования ответа.
// ПредполагаетсЯ, что SKLP_ProcessPacket() вызываетсЯ из одного процесса, и параллельного использованиЯ буфера не будет.
// Тем не менее, контролировать доступ к буферу, если SKLP_ProcessPacket() будет вызван из разных процессов параллельно.
__no_init static uint8_t aQuery[ SKLP_SIZE_PACKET_MAX ];
static uint8_t bQueryBufferAccessTaken = false;

// Прием пакета-запроса из кольцевого буфера UART в линейный буфер, проверка формата и CRC.
// Передача управления на коллбек выполнениЯ принЯтой команды.
void SKLP_ProcessPacket( SKLP_Interface_t *pInterface, ByteQueueFragment_t PacketToRecive )
{
	assert_param( SKLP_InterfaceValidate( pInterface ) );
	
	// Получить доступ к линейному буферу
	ENTER_CRITICAL_SECTION( );
	assert_param( false == bQueryBufferAccessTaken );
	bQueryBufferAccessTaken = true;
	EXIT_CRITICAL_SECTION( );

	// Приступить к обработке пакета
	pInterface->Statistic.PacketsProcessed++;
	SKLP_CommandResult_t CommandResult = SKLP_COMMAND_RESULT_ERROR_FORMAT;

	// Остановить (если был запущен) таймер автовозврата к низкой скорости UART.
	// Остановка производитсЯ на случай длительной обработки команды (напр., при сбоЯх чтениЯ SD)
	// и на случай длительной передачи большого пакета через UART
	bool bFastBaudTimerActive = false;
	taskENTER_CRITICAL( );
	if( NULL != pInterface->FastBaudTimerHandle )
		if( xTimerIsTimerActive( pInterface->FastBaudTimerHandle ) )
		{
			bFastBaudTimerActive = true;
			assert_param( pdPASS == xTimerStop( pInterface->FastBaudTimerHandle, 0 ) );
		}
	taskEXIT_CRITICAL( );

	// Скопировать пакет из кольцевого буфера UART в линейный рабочий буфер, проверить формат
	bool ReceiveResult = SKLP_ReceivePacket( aQuery, sizeof( aQuery ), pInterface->pUART_Hdl->pByteQueueRx, PacketToRecive );
	// Выделить номер команды из пакета
	SKLP_Command_t Command = ( SKLP_Command_t ) aQuery[ SKLP_OFFSET_COMMAND ];

	// Произвести разбор и выполнение пакета
	if( ReceiveResult )
	{
GPIO_Common_Write( iGPIO_TestPinSKLP_Service, GPIO_PIN_SET );
		// Адрес буфера, куда коллбек поместит ответ.
		// По соглашению пост-фактум, через pAnswer в коллбек передаетсЯ адрес интерфейса.
		uint8_t *pAnswer = ( void * ) pInterface;
		CommandResult = SKLP_ProcessCommand_Common( aQuery, &pAnswer );
		// Коллбек выполнен успешно и требует вернуть ответ
		if( CommandResult >= SKLP_COMMAND_RESULT_RETURN )
		{	// Обработчик запроса требует вернуть ответ
			// Контролировать, что адрес буфера ответа был проинициализирован
			assert_param( ( pAnswer != ( void * ) pInterface ) && ( pAnswer != NULL ) );
			if( CommandResult > sizeof( aQuery ) )
				assert_param( pAnswer != aQuery );		// как правило, коллбек записывает ответ в буфер запроса. однако длЯ больших ответов должен использоватьсЯ другой буфер
			// Определить формат формируемого ответного пакета				
			SKLP_SendPacketFormat_t AnswerPacketFormat;
			uint16_t AnswerPacketSize;
			if( SKLP_COMMAND_MEMORY_READ != Command )
			{	// Формат пакета длЯ ответа на обычный запрос
				AnswerPacketFormat = SKLP_SendPacketFormat_Answer;
				AnswerPacketSize = CommandResult + SKLP_SIZE_TRIM;
			}
			else
			{	// Команда чтениЯ памЯти отличаетсЯ от прочих команд форматом ответного пакета
				AnswerPacketFormat = SKLP_SendPacketFormat_AnswerMemRead;
				AnswerPacketSize = CommandResult;
			}
			// Заполнить заголовок и CRC ответного пакета, передать пакет в последовательный интерфейс.
			assert_param( SKLP_SendPacket( pAnswer, AnswerPacketSize, AnswerPacketFormat, pInterface->pUART_Hdl ) );
			pInterface->Statistic.PacketsTransmitted++;
		}
		
		// Вызвать опциональный коллбек по завершению передачи пакета.
		// Коллбек может быть инициализирован в обработчике команды,
		// если обработчик требует дополнительной обработки после завершениЯ передачи -
		// например, освобождение передаваемого буфера.
		if( NULL != pInterface->xPacketTxCompleteCB )
		{
			pInterface->xPacketTxCompleteCB( pInterface );
			pInterface->xPacketTxCompleteCB = NULL;
		}
		
	}

	// Запустить (если ранее был запущен) остановленный таймер автовозврата к низкой скорости UART.
	if( bFastBaudTimerActive )
		assert_param( pdPASS == xTimerStart( pInterface->FastBaudTimerHandle, 0 ) );

	// Разобрать результат обработки поступившего пакета
	if( CommandResult >= SKLP_COMMAND_RESULT_NO_REPLY )
		pInterface->Statistic.PacketsProcessedSuccessful++;
	else
	{	// При попытке обработать запрос произошла ошибка. Оставить запись в лог
		static uint8_t FailCount = 10;		// Счетчик однотипных ошибок, чтобы не засирать лог
		if( FailCount )
		{
			// Добавить номер команды в код ошибки
			CommandResult &= 0xFFFF00FF;
			CommandResult |= ( ( uint32_t ) Command ) << 8;
			// Добавить номер UART в код ошибки
			CommandResult &= 0xFFF0FFFF;
			CommandResult |= UART_GET_NUMBER( pInterface->pUART_Hdl->Instance ) << 16;
			// Отправить код ошибки в лог
			assert_param( MemoryThread_SprintfMutexTake( 1 ) );
			char *pMsg = ( char * ) ResetInfo.aLogMessage;
			int MsgSize = sizeof( ResetInfo.aLogMessage );
			int MsgPos = 0;
			MsgPos += snprintf( pMsg + MsgPos, MsgSize - MsgPos, "[SKLP] Incoming packet processing failure: 0x%08lX", CommandResult );
//			MsgPos += snprintf( pMsg + MsgPos, MsgSize - MsgPos, "(UART=%u Cmd=0x%hh02X, )", ( uint8_t ) Command );
			if( 0 == --FailCount )
				MsgPos += snprintf( pMsg + MsgPos, MsgSize - MsgPos, " <Stop logging this error>" );
			MemoryThread_SprintfMutexGive( );
			assert_param( Logger_WriteRecord( pMsg, LOGGER_FLAGS_APPENDTIMESTAMP | LOGGER_FLAGS_STATICTEXT ) );
		}
	}
		
	// Освободить доступ к линейному буферу
	ATOMIC_WRITE( bQueryBufferAccessTaken, false );
GPIO_Common_Write( iGPIO_TestPinSKLP_Service, GPIO_PIN_RESET );
}

// Принять пакет из кольцевого буфера одного интерфейса и отправить в другой интерфейс
// MessageToGateway		- пакет, принятый через исходный интерфейс
// pInterface			- интерфейс, куда переправить пакет
void SKLP_ProcessPacketGatewayV2( SKLP_Message_t *pMessageToGateway, SKLP_Interface_t *pInterface )
{
GPIO_Common_Write( iGPIO_TestPinSKLP_Service, GPIO_PIN_SET );
	assert_param( NULL != pMessageToGateway );
	assert_param( SKLP_InterfaceValidate( pInterface ) );
	
	// Получить доступ к линейному буферу
	ENTER_CRITICAL_SECTION( );
	assert_param( false == bQueryBufferAccessTaken );
	bQueryBufferAccessTaken = true;
	EXIT_CRITICAL_SECTION( );

	do
	{
		UART_Ext_HandleTypeDef *pUART_Hdl = pInterface->pUART_Hdl;
		uint16_t PacketSize = pMessageToGateway->Packet.Size;
		if( PacketSize > sizeof( aQuery ) )
			break;		// размер пакета превышает размер буфера
		// Скопировать пакет из кольцевого буфера UART в линейный рабочий буфер, не проверять формат
		if( !ByteQueue_BufferCopyFragment( pMessageToGateway->pInterface->pUART_Hdl->pByteQueueRx, aQuery, pMessageToGateway->Packet ) )
			break;
		
		// Отправить пакет через UART, без модифицирования
		EventBits_t EventBitsMask = EVENT_UART_EXT_TX_ERROR | EVENT_UART_EXT_TX_COMPLETE;	// Ожидать событий о конце передачи или ошибке
		if( HAL_OK != HAL_UART_Ext_Transmit( pUART_Hdl, aQuery, PacketSize, EventBitsMask ) )
			break;
	
		// Ожидать событий о конце передачи или ошибке
		float TimeToTransmite = HAL_UART_Ext_CalcTimeToTransmite( pUART_Hdl, PacketSize );
		assert_param( TimeToTransmite > 0.0f );
		TickType_t PossibleTimout = 5 + pdMS_TO_TICKS( 1000.0f * 1.2f * TimeToTransmite );
		EventBits_t EventBitsResult = EventBitsMask & xEventGroupWaitBits( pUART_Hdl->EventGroup, EventBitsMask, pdTRUE, pdFALSE, PossibleTimout );
		if( EVENT_UART_EXT_TX_COMPLETE != EventBitsResult )
			break;
	}
	while( 0 );

	// Освободить доступ к линейному буферу
	ATOMIC_WRITE( bQueryBufferAccessTaken, false );
GPIO_Common_Write( iGPIO_TestPinSKLP_Service, GPIO_PIN_RESET );
}

// Прием пакета-запроса из кольцевого буфера UART в линейный буфер, проверка формата и CRC.
// Выполнение шлюзования
// pInterfaceMaster			интерфейс, откуда пришел запрос и куда вернуть ответ
// PacketToGateway			принятый пакет
// pGatewayHdl_UART			хэндлер UART, куда переправить запрос и откуда ждать ответ
// pGatewayHdl_Interface	интерфейс, куда переправить запрос и откуда ждать ответ
// !!! требуетсЯ использовать либо pGatewayHdl_UART, либо pGatewayHdl_Interface
// WaitAnswerTimeout_ms		таймаут на ожидание ответа, не считая времени передачи запроса (и максималоьного ответа?)
//							если 0 - ответ не ждать
void SKLP_ProcessPacketGateway( SKLP_Interface_t *pInterfaceMaster, ByteQueueFragment_t PacketToGateway, UART_Ext_HandleTypeDef *pGatewayHdl_UART, SKLP_Interface_t *pGatewayHdl_Interface, uint16_t WaitAnswerTimeout_ms )
{
	assert_param( SKLP_InterfaceValidate( pInterfaceMaster ) );
	static QueueHandle_t xInterfaceGatewayRxQueue = NULL;
	if( SKLP_InterfaceValidate( pGatewayHdl_Interface ) )
	{	// Работать через интерфейс SKLP
		pGatewayHdl_UART = pGatewayHdl_Interface->pUART_Hdl;
		if( NULL == xInterfaceGatewayRxQueue )
		{	// При первом вызове шлюза, создать очередь для приема сообщений
			assert_param ( NULL != ( xInterfaceGatewayRxQueue = xQueueCreate( 1, sizeof( SKLP_Message_t ) ) ) );
		}
	}
	else
		pGatewayHdl_Interface = NULL;
	assert_param( HAL_UART_Ext_ValidateHdl( pGatewayHdl_UART ) );
		
	// Получить доступ к линейному буферу, используемому в SKLP_ProcessPacket()
	ENTER_CRITICAL_SECTION( );
	assert_param( false == bQueryBufferAccessTaken );
	bQueryBufferAccessTaken = true;
	EXIT_CRITICAL_SECTION( );

	SKLP_CommandResult_t GatewayResult = SKLP_COMMAND_RESULT_ERROR;
	bool bGatewayUART_AccessTaken = false;
	do
	{
		EventBits_t EventBitsMask;
		EventBits_t EventBitsResult;

		// Приступить к обработке полученного пакета
		pInterfaceMaster->Statistic.PacketsProcessed++;
		// Скопировать пакет из кольцевого буфера UART в линейный рабочий буфер, проверить формат
		if( !SKLP_ReceivePacket( aQuery, sizeof( aQuery ), pInterfaceMaster->pUART_Hdl->pByteQueueRx, PacketToGateway ) )
			break;
		// Пакет принЯт, формат в порЯдке

		// Попытаться получить доступ к UART. Подождать, если прямо сейчас доступ захвачен.
		EventBitsMask = EVENT_UART_EXT_ACCESS_READY;
		EventBitsResult = EventBitsMask & xEventGroupWaitBits( pGatewayHdl_UART->EventGroup, EventBitsMask, pdTRUE, pdTRUE, pdMS_TO_TICKS( 50 ) );
		if( EventBitsMask != EventBitsResult )
		{	// Не удалось получить доступ (предположительно, работает основной обмен)
			GatewayResult = SKLP_COMMAND_RESULT_ERROR;
			break;
		}
		bGatewayUART_AccessTaken = true;

		// Очистить приемник интерфейса назначения перед отправкой запроса
		HAL_UART_Ext_ReceiveCyclicReset( pGatewayHdl_UART );
		EventBitsMask = 0;
		QueueHandle_t xInterfaceGatewayRxQueueSaved = NULL;
		if( NULL != pGatewayHdl_Interface )
		{	// Шлюзовать в интерфейс SKLP
			if( 0 != WaitAnswerTimeout_ms )
			{	// Переключить очередь принимаемых сообщений шлюза на этот процесс
				taskENTER_CRITICAL( );
				xInterfaceGatewayRxQueueSaved = pGatewayHdl_Interface->pMessageQueue;
				xQueueReset( xInterfaceGatewayRxQueueSaved );
				pGatewayHdl_Interface->pMessageQueue = xInterfaceGatewayRxQueue;
				taskEXIT_CRITICAL( );
			}
		}
		else
		{	// Шлюзовать в UART
			if( 0 == WaitAnswerTimeout_ms )
				EventBitsMask = EVENT_UART_EXT_TX_COMPLETE;								// ожидать событий о конце передачи
			else
				EventBitsMask = EVENT_UART_EXT_TX_COMPLETE | EVENT_UART_EXT_RX_IDLE;	// ожидать событий о конце передачи и приеме ответа
		}

		// Произвести отправку пакета в шлюз, без изменений (можно не использовать SKLP_SendPacket())
		if( HAL_OK != HAL_UART_Ext_Transmit( pGatewayHdl_UART, aQuery, PacketToGateway.Size, EventBitsMask ) )
			break;
		// Рассчитать времЯ передачи шлюзуемого пакета и приема максимального пакета в ответ
		float TimeToTransmite	= HAL_UART_Ext_CalcTimeToTransmite( pGatewayHdl_UART, PacketToGateway.Size );
		float TimeToReceive		= HAL_UART_Ext_CalcTimeToTransmite( pGatewayHdl_UART, SKLP_SIZE_PACKET_MAX );
		assert_param( TimeToTransmite > 0.0f );
		assert_param( TimeToReceive > 0.0f );
		if( 0 == WaitAnswerTimeout_ms )
		{	// Ждать ответ не требуется. Ждать завершение передачи пакета, и завершить шлюзование
			TickType_t PossibleTimout = pdMS_TO_TICKS( 1000.0f * TimeToTransmite + 2 );		// рассчетное времЯ передачи запроса
			EventBitsResult = EventBitsMask & xEventGroupWaitBits( pGatewayHdl_UART->EventGroup, EventBitsMask, pdTRUE, pdTRUE, PossibleTimout );
			if( EventBitsMask != EventBitsResult )
				GatewayResult = SKLP_COMMAND_RESULT_ERROR;			// Не удалось отправить пакет
			else
				GatewayResult = SKLP_COMMAND_RESULT_NO_REPLY;		// Пакет передан
			// Шлюзование завершено
			break;
		}
		
		// Ждать завершение передачи пакета и потом ответ
		ByteQueueFragment_t PacketToRecive = { 0 };
		ByteQueue_t *pByteQueueRx = pGatewayHdl_UART->pByteQueueRx;
		TickType_t PossibleTimout = pdMS_TO_TICKS( 1000.0f * TimeToTransmite + 2 + 1000.0f * TimeToReceive + WaitAnswerTimeout_ms );	// рассчетное времЯ передачи запроса и максимальный таймаут на ответ.
		if( NULL == pGatewayHdl_Interface )
		{	// Работа через UART - ждать событий завершениЯ передачи и приема пакетов
			EventBits_t EventBitsResult = EventBitsMask & xEventGroupWaitBits( pGatewayHdl_UART->EventGroup, EventBitsMask, pdTRUE, pdTRUE, PossibleTimout );
			if( EventBitsMask != EventBitsResult )
			{	// Не удалось отправить или получить пакет
				GatewayResult = SKLP_COMMAND_RESULT_ERROR_TM;
				break;
			}
			// Ответный пакет получен. Зафиксировать ответный пакет а приемном буфере
			PacketToRecive = ( ByteQueueFragment_t ) { pByteQueueRx->iTail, ByteQueue_GetSpaceFilled( pByteQueueRx ) };
			// Пакет зафиксирован, очистить буфер
			ByteQueue_RemoveFromTail( pByteQueueRx, NULL, PacketToRecive.Size );
		}
		else
		{	// Работа через интерфейс - ждать поЯвлениЯ сообщениЯ о приеме ответного пакета
			SKLP_Message_t Message;
			if( ( pdTRUE == xQueueReceive( xInterfaceGatewayRxQueue, &Message, PossibleTimout ) ) &&	// получен пакет SKLP
				( EVENT_SKLP_ANSWER == Message.Event ) )												// пакет с ответом
					PacketToRecive = Message.Packet;		// !!! надо бы подписатьсЯ на событиЯ EVENT_SKLP_ANSWER, вдруг не было подписки?
			else
			{	// Не удалось получить корректный ответный пакет
				GatewayResult = SKLP_COMMAND_RESULT_ERROR_TM;
				break;
			}
		}

		// Ответный пакет принЯт и находитсЯ в приемном буфере UART
		uint8_t *pAnswer = aQuery;		// использовать буфер исходного запроса SKLP.Slave длЯ передачи ответа от шлюза
		if( !SKLP_ReceivePacket( pAnswer, sizeof( aQuery ), pByteQueueRx, PacketToRecive ) ||
			( SKLP_START_ANSWER != pAnswer[SKLP_OFFSET_START] ) )
		{	// Нарушение формата полученного пакета
			GatewayResult = SKLP_COMMAND_RESULT_ERROR_FORMAT;
			break;
		}

		// Заполнить заголовок и CRC ответного пакета, передать пакет в последовательный интерфейс.
		assert_param( SKLP_SendPacket( pAnswer, PacketToRecive.Size, SKLP_SendPacketFormat_Answer, pInterfaceMaster->pUART_Hdl ) );
		pInterfaceMaster->Statistic.PacketsTransmitted++;
	} while( 0 );
		
	// Освободить доступ к линейному буферу
	ATOMIC_WRITE( bQueryBufferAccessTaken, false );
	if( bGatewayUART_AccessTaken )
	{	// Освободить доступ к UART
		( void ) xEventGroupSetBits( pGatewayHdl_UART->EventGroup, EVENT_UART_EXT_ACCESS_READY );
	}
}

// Скопировать полученный пакет из кольцевого в линейный буфер, проверить заголовок и контрольную сумму
// pPacketDest		- линейный буфер, куда скопировать пакет
// PacketSizeMax	- размер линейного буфера
// pByteQueueRx		- адрес кольцевого буфера UART
// PacketToRecive	- позиция и количество байт, которые необходимо вычитать из кольцевого буфера
// return			- true, если пакет скопирован, и его формат (START, SIZE, CRC8) не нарушен.
bool SKLP_ReceivePacket( uint8_t *pPacketDest, ByteQueueIndex_t PacketSizeMax, ByteQueue_t *pByteQueueRx, ByteQueueFragment_t PacketToRecive )
{
	bool Result = false;

	do
	{	// Проверить аргументы
		if( ( NULL == pPacketDest ) || ( NULL == pByteQueueRx ) )
			break;

		// Проверить размер пакета и сравнить с допустимым размером линейного буфера
		if( PacketToRecive.Size > PacketSizeMax )
			break;

		// Скопировать пакет из кольцевого буфера UART в линейный рабочий буфер, голову и хвост буфера не перемещать
		if( !ByteQueue_BufferCopyFragment( pByteQueueRx, pPacketDest, PacketToRecive ) )
			break;

		// Проверить заголовок (поле Старт)
		uint8_t ByteStart = pPacketDest[SKLP_OFFSET_START];
		if( ( ByteStart != SKLP_START_QUERY ) && ( ByteStart != SKLP_START_ANSWER ) )
			break;

		// Проверить заголовок (поле Размер)
		uint8_t ByteSize = pPacketDest[SKLP_OFFSET_SIZE];
#ifdef	SKLP_SIZE_PACKET_LONG_MAX
		if( PacketToRecive.Size < SKLP_SIZE_PACKET_MAX )
		{	// ДлЯ "коротких" пакетов проверЯть размер в обычном порЯдке
#endif	// SKLP_SIZE_PACKET_LONG_MAX
			if( ( PacketToRecive.Size > SKLP_SIZE_PACKET_MAX ) || ( ( PacketToRecive.Size - SKLP_SIZE_TRIM ) != ByteSize ) )
				break;
#ifdef	SKLP_SIZE_PACKET_LONG_MAX
		}
		else
		{	// ДлЯ "длинных" пакетов размер должен быть "длинным"
			if( SKLP_SIZE_MAX != ByteSize )
				break;
		}
#endif	// SKLP_SIZE_PACKET_LONG_MAX
		
		// Поля Адрес, Команда, Данные - не проверяются

		// Проверить CRC8
		if( CalcCRC8SKLP( pPacketDest, PacketToRecive.Size - 1 ) != pPacketDest[ PacketToRecive.Size - 1 ] )
			break;

		// Пакет получен и скопирован в линейный буфер, все возможные проверки формата проведены
		Result = true;
	}
	while( 0 );

	return Result;
}

// Оформить пакет с заполненным полем данных, отправить в UART,
// *** и дождатьсЯ завершениЯ отправки. ***
// Стандартная передача в UART через DMA не может быть больше 65535 байт из-за ограничения DMA.
// При необходимости передачи пакетов более 65535 байт переделать функцию HAL_UART_Ext_Transmit()
// pPacket		- адрес передаваемого пакета в ОЗУ
// PacketSize	- размер передаваемого пакета
// Format		- формат формированиЯ пакета
// pUART_Hdl	- последовательный интерфейс, в который переслать пакет
bool SKLP_SendPacket( uint8_t *pPacket, uint16_t PacketSize, SKLP_SendPacketFormat_t Format, UART_Ext_HandleTypeDef *pUART_Hdl )
{
	bool ReturnValue = false;
	do
	{
		// Проверить аргументы
		if( ( NULL == pPacket ) || ( NULL == pUART_Hdl ) )
			break;

		// Выделить и удалить флаг NoWait из формата
		bool bWaitForTransferFinish = !( Format & SKLP_SendPacketFormat_FlagNoWait );
		bool bPrepareForAnswer		= ( Format & SKLP_SendPacketFormat_FlagPrepForAnswer );
		Format &= SKLP_SendPacketFormat_MaskCommand;

		// Заполнить стартовую сигнатуру
		uint8_t Start = 0;
		switch( Format )
		{
		case SKLP_SendPacketFormat_Query:
			Start = SKLP_START_QUERY;
			break;
		case SKLP_SendPacketFormat_Answer:
		case SKLP_SendPacketFormat_AnswerMemRead:
			Start = SKLP_START_ANSWER;
			break;
		}
		if( 0 == Start )
			break;		// неверный аргумент Format
		pPacket[SKLP_OFFSET_START] = Start;

		// Заполнить размер и (возможно) контрольную сумму
		bool bCalcCRCLater = true;
		switch( Format )
		{
		case SKLP_SendPacketFormat_Query:
		case SKLP_SendPacketFormat_Answer:
			assert_param( PacketSize >= SKLP_SIZE_TRIM );
			// Заполнить поле размера
			if( PacketSize <= ( 0xFF + SKLP_SIZE_TRIM ) )
			{	
				pPacket[SKLP_OFFSET_SIZE] = ( uint8_t ) ( PacketSize - SKLP_SIZE_TRIM );
				// ДлЯ маленьких пакетов CRC лучше рассчитать сразу же
				pPacket[ PacketSize - 1 ] = CalcCRC8SKLP( pPacket, PacketSize - 1 );
				bCalcCRCLater = false;
			}
			else
			{	// В случае длинного пакета отложить времязатратный расчет CRC. Расчет будет произведен уже в процессе передачи пакета.
				pPacket[SKLP_OFFSET_SIZE] = 0xFF;
			}
			break;
		case SKLP_SendPacketFormat_AnswerMemRead:
			// Поле размера в пакете отсутствет, не заполнЯть. Пакет заведомо большой, контрольную сумму рассчитать позже
			assert_param( PacketSize > SKLP_MEMSECTORSIZE );
			break;
		default:
			assert_param( 0 );
		}
		
		// Отправить пакет через UART
		EventBits_t EventBitsMask = EVENT_UART_EXT_TX_ERROR | EVENT_UART_EXT_TX_COMPLETE;	// Ожидать событий о конце передачи или ошибке
		if( HAL_OK != HAL_UART_Ext_Transmit( pUART_Hdl, pPacket, PacketSize, EventBitsMask | ( bPrepareForAnswer ? EVENT_UART_EXT_RX_IDLE : 0 ) ) )
			break;
		// Для длинных пакетов, рассчитать CRC прЯмо во времЯ передачи пакета
		if( bCalcCRCLater )
			pPacket[ PacketSize - 1 ] = CalcCRC8SKLP( pPacket, PacketSize - 1 );

		if( bWaitForTransferFinish )
		{	// Ожидать событий о конце передачи или ошибке
			float TimeToTransmite = HAL_UART_Ext_CalcTimeToTransmite( pUART_Hdl, PacketSize );
			assert_param( TimeToTransmite > 0.0f );
			TickType_t PossibleTimout = 5 + pdMS_TO_TICKS( 1000.0f * 1.2f * TimeToTransmite );
			EventBits_t EventBitsResult = EventBitsMask & xEventGroupWaitBits( pUART_Hdl->EventGroup, EventBitsMask, pdTRUE, pdFALSE, PossibleTimout );
			// !!! Этот ассерт однажды выстрелил в МПИ. Предположительно, вызов производился из таймслота обмена с подчиненным модулем,
			// !!! вызов произошел поздно, время таймслота закончилось раньше и задача была принудительно разбужена таймером-формирователем таймслотов.
			// !!! Если предположение верно, то ситуация не на столько плохая, чтобы перезагружаться.
			// assert_param( 0 != EventBitsResult );
			if( EVENT_UART_EXT_TX_COMPLETE != EventBitsResult )
				break;
		}
		
		// Пакет сформирован и полностью отправлен в интерфейс, либо начал отправлЯтьсЯ -
		// в зависимости от SKLP_SendPacketFormat_FlagNoWait
		ReturnValue = true;
	} while( 0 );

	return ReturnValue;
}

// Запрос в интерфейс и ожидание ответа
// Завершение происходит:
// - при получении ответа
// - по выходу таймаута на получение ответа
// - по принудительному пробуждению задачи
// pInterface			интерфейс обмена
// pQuery				буфер запроса. должно быть заполнено основное содержание (в т.ч. команда и адрес), а транспорт (Старт, Размер, CRC) будут заполнены здесь.
// QuerySize			полный размер запроса (от старта до CRC)
// pAnswer				буфер для приема ответа. NULL, если не ждать ответа.
// pAnswerSize			при вызове - размер буфера под ответ. при возврате - актуальный размер ответа
// Timeout				таймаут на ожидание принятия ответного пакета (отсчет начинается с момента завершения отправки запроса)
SKLPM_Result_t SKLPM_Query( SKLP_Interface_t	*pInterface, uint8_t *pQuery, uint16_t QuerySize, uint8_t *pAnswer, uint16_t *pAnswerSize, TickType_t Timeout )
{
	SKLPM_Result_t Result = SKLPM_ResultFail;
	do
	{
		SKLP_Message_t *pRxMessage, RxMessage;
		// Проверка аргументов
		if( NULL == pAnswer )
			pRxMessage = NULL;
		else
		{
			if( ( NULL == pAnswerSize ) || ( *pAnswerSize < SKLP_SIZE_PACKET_MIN_ANSWER ) )
			{
				Result = SKLPM_ResultFail_Parameters;
				break;
			}
			pRxMessage = &RxMessage;
		}

		// Отправить пакет. При необходимости, дождаться ответа
		if( SKLPM_ResultOK != ( Result = SKLPM_Query_RetMsg( pInterface, pQuery, QuerySize, pRxMessage, Timeout ) ) )
			break;
		if( NULL == pAnswer )
			break;		// ответ принимать не требуется
		
		// Принять ответ
		if( pRxMessage->Packet.Size > *pAnswerSize )
		{	// Полученный ответ не влазит в отведенный буфер
			Result = SKLPM_ResultFail_RxFormatPayload;
			break;
		}
		// Скопировать пакет из кольцевого буфера UART в линейный рабочий буфер, проверить формат
		if( !SKLP_ReceivePacket( pAnswer, *pAnswerSize,  pInterface->pUART_Hdl->pByteQueueRx, pRxMessage->Packet ) )
		{	// Не удалось принять пакет в буфер
			Result = SKLPM_ResultFail_RxFormatSKLP;		// !!! обычно ошибка по CRC, но может быть что-то с аргументами - видимо надо менять возврат из SKLP_ReceivePacket()...
			break;
		}

		// Принят пакет ожидаемого формата, не превышающий отведенный размер
		*pAnswerSize = pRxMessage->Packet.Size;
		Result = SKLPM_ResultOK;
	} while( 0 );
	return Result;
}

// Запрос в интерфейс и ожидание ответа
// Завершение происходит:
// - при получении ответа
// - по выходу таймаута на получение ответа
// - по принудительному пробуждению задачи
// pInterface			интерфейс обмена
// pQuery				буфер запроса. должно быть заполнено основное содержание (в т.ч. команда и адрес), а транспорт (Старт, Размер, CRC) будут заполнены здесь.
// QuerySize			полный размер запроса (от старта до CRC)
// pReturnRxMessage		если не NULL -ожидать ответ, и вернуть сообщение с пакетом (не вычитывая)
// Timeout				таймаут на ожидание принятия ответного пакета (отсчет начинается с момента завершения отправки запроса)
SKLPM_Result_t SKLPM_Query_RetMsg( SKLP_Interface_t	*pInterface, uint8_t *pQuery, uint16_t QuerySize, SKLP_Message_t *pReturnRxMessage, TickType_t Timeout )
{
	SKLP_InterfaceValidate( pInterface );

	SKLPM_Result_t Result = SKLPM_ResultFail;
	do
	{
		// Проверка аргументов
		if( ( NULL == pQuery ) || ( QuerySize < SKLP_SIZE_PACKET_MIN_QUERY ) )
		{
			Result = SKLPM_ResultFail_Parameters;
			break;
		}

		// Очистить приемную очередь
		xQueueReset( pInterface->pMessageQueue );

		// Отправить пакет через UART
		TickType_t TimestampStart = xTaskGetTickCount();
		if( !SKLP_SendPacket( pQuery, QuerySize, SKLP_SendPacketFormat_Query, pInterface->pUART_Hdl ) )
		{	// Не удалось отправить запрос
			Result = SKLPM_ResultFail_TxErr;
			break;
		}
		// Отправка пакета завершена

		if( NULL == pReturnRxMessage )
		{	// Если не требуется ожидать ответ - завершить
			Result = SKLPM_ResultOK;
			break;
		}
			
		// Контроль таймаута					
		TickType_t TimeElapsed = xTaskGetTickCount() - TimestampStart;
		if( TimeElapsed >= Timeout )
		{	// Вышел таймаут
			Result = SKLPM_ResultFail_RxTimeout;
			break;
		}

		// Ожидать событие от последовательных интерфейсов
		// Принять событие из очереди
		SKLP_Message_t Message;
		if( pdFALSE == xQueueReceive( pInterface->pMessageQueue, &Message, Timeout - TimeElapsed ) )
		{	// Ожидание прервано по таймауту, или произошло пробуждение про команде от другого процесса
			Result = SKLPM_ResultFail_RxTimeout;
			break;
		}

		// Проверить событие
		if( Message.Event != EVENT_SKLP_ANSWER )
		{	// На шине другой мастер?
			Result = SKLPM_ResultFail_RxErr;
			break;
		}

		// Ответ принят. Вернуть сообщение с принятым пакетом, без обработки
		*pReturnRxMessage = Message;
		Result = SKLPM_ResultOK;
	} while( 0 );
	return Result;
}

// Установить коллбек, однократно вызываемый из SKLP_ProcessPacket() после завершениЯ передачи пакета.
// ВызываетсЯ обычно из коллбека-обработчика пакета, чтобы продолжить исполнение
// после передачи последнего байта в последовательный канал - например,
// длЯ освобождениЯ буфера, из которого велась передача.
// !! После вызова коллбека SKLP_ProcessPacket(), он обнулЯетсЯ!
void SKLP_SetTxCompleteCB( SKLP_Interface_t *pInterface, SKLP_InterfaceCB_t xPacketTxCompleteCB )
{
	assert_param( SKLP_InterfaceValidate( pInterface ) );
	pInterface->xPacketTxCompleteCB = xPacketTxCompleteCB;
}

// Распечатать статистику по работе протокола в лог
void SKLP_PrintStatistics( SKLP_Interface_t *pInterface )
{
	SKLP_Statistic_t Statistic;
	ATOMIC_WRITE( Statistic, pInterface->Statistic );
	assert_param( MemoryThread_SprintfMutexTake( 5 ) );
	snprintf( ( char * ) ResetInfo.aLogMessage, sizeof( ResetInfo.aLogMessage ),
//		"[SKLP] %s channel statistics:\r\nTaskResetCounter = %lu\r\nFragmentsRecieved = %lu\r\nPacketsRejectedInFormat = %lu\r\nPacketsRejectedInBuzy = %lu\r\nPacketsSkipped = %lu\r\nPacketsProcessed = %lu\r\nPacketsProcessedSuccessful = %lu\r\nPacketsTransmitted = %lu\r\n",
//		pInterface->pName, Statistic.TaskResetCounter, Statistic.FragmentsRecieved, Statistic.PacketsRejectedInFormat, Statistic.PacketsRejectedInBuzy, Statistic.PacketsSkipped, Statistic.PacketsProcessed, Statistic.PacketsProcessedSuccessful, Statistic.PacketsTransmitted );
		"[SKLP] %s channel statistics:\r\nFragmentsRecieved = %lu\r\nPacketsRejectedInFormat = %lu\r\nPacketsRejectedInBuzy = %lu\r\nPacketsSkipped = %lu\r\nPacketsProcessed = %lu\r\nPacketsProcessedSuccessful = %lu\r\nPacketsTransmitted = %lu\r\n",
		pInterface->pName, Statistic.FragmentsRecieved, Statistic.PacketsRejectedInFormat, Statistic.PacketsRejectedInBuzy, Statistic.PacketsSkipped, Statistic.PacketsProcessed, Statistic.PacketsProcessedSuccessful, Statistic.PacketsTransmitted );
	MemoryThread_SprintfMutexGive( );
	assert_param( Logger_WriteRecord( ( char * ) ResetInfo.aLogMessage, LOGGER_FLAGS_STATICTEXT ) );
}

