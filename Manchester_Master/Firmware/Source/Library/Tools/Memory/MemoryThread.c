// MemoryThread.c
#include "ProjectConfig.h"		// конфиг платформы, конфиг задачи.
#include "stm32xxxx_hal.h"		// дрова периферии
#include "MemoryThread.h"
#include "TaskConfig.h"
#include "Queue.h"
//#include "FileSystem.h"			// FileSystem_CheckFSReady()

static QueueHandle_t pMemoryThread_QueueHdl = NULL;
static QueueHandle_t pMemoryThread_QueueDelayedHdl = NULL;
static EventGroupHandle_t pMemoryThread_EventGroupHdl = NULL;

// Маска свободных битов длЯ использованиЯ в качестве событий при ожидании завершении обработки коллбеков
//static EventBits_t MemoryThread_EventGroupFreeBits = 0x00FFFFFF;	// 1 соответствует свободному (наиспользуемому) биту
#define MEMORYTHREAD_EVENTFREEBITS_bm		( ( uint16_t ) 0xFFFF )
static uint16_t MemoryThread_EventGroupFreeBits = 0;	// 1 соответствует свободному (наиспользуемому) биту

// Контроль максимальной заполненности буфера сообщений
uint32_t MemoryThread_MinQueueEmptySpace		= MEMORYTHREAD_MESSAGE_COUNT;
uint32_t MemoryThread_MinQueueDelayedEmptySpace	= MEMORYTHREAD_MESSAGEDELAYED_COUNT;

// Буфер длЯ произвольного использованиЯ в коллбеках из задачи MemoryThread_Task()
__no_init uint8_t aMemoryThreadTaskBuffer[1024];

// Хендлер задачи
static TaskHandle_t xMemoryThreadTaskHandle;

// Вернуть свободное место в очередЯх
bool MemoryThread_GetQueueSpaces( uint32_t *pQueueSpaceMain, uint32_t *pQueueSpaceDelayed )
{
	bool bResult = false;
	do
	{
		if( NULL == pMemoryThread_QueueHdl )
			break;
		if( NULL == pMemoryThread_QueueDelayedHdl )
			break;
		if( NULL != pQueueSpaceMain )
			*pQueueSpaceMain = ( uint32_t ) uxQueueSpacesAvailable( pMemoryThread_QueueHdl );
		if( NULL != pQueueSpaceDelayed )
			*pQueueSpaceDelayed = ( uint32_t ) uxQueueSpacesAvailable( pMemoryThread_QueueDelayedHdl );
		bResult = true;
	} while( 0 );
	return bResult;
}


// Добавить сообщение с коллбеком в очередь
// pMessage			- сообщение с заполненным полем pMessage->Header.xCallback
// ( Flags & MEMORYTHREAD_FLAGS_WAITFORCOMPLETE )	- необходимо дождатьсЯ завершениЯ выполнениЯ коллбека, прежде чем выходить.
// ( Flags & MEMORYTHREAD_FLAGS_WAITFORFS )			- при нарушении работы файловой системы, отложить выполнение коллбека до восстановлениЯ файловой системы
//		(эти флаги взаимоисключающие, т.к. подразумеваетсЯ, что файлуха может никогда не поднЯтьсЯ, а останавливать задачу нельзЯ)
bool MemoryThread_AppendMessage( MemoryThreadMessage_t *pMessage, uint32_t Flags )
{
	assert_param( NULL != pMemoryThread_QueueHdl );
	assert_param( NULL != pMessage );
	assert_param( NULL != pMessage->Header.xCallback );
	assert_param( !( ( Flags & MEMORYTHREAD_FLAGS_WAITFORCOMPLETE ) && ( Flags & MEMORYTHREAD_FLAGS_WAITFORFS ) ) );

	// Возможна ситуациЯ, когда в одном коллбеке MemoryThread_Task(), производитсЯ вызов MemoryThread_AppendMessage() с другим коллбеком.
	// Например, коллбек записи сообщениЯ в черный Ящик дублирует это сообщение в лог.
	// При этом, если при вызове MemoryThread_AppendMessage() будет выставлен флаг MEMORYTHREAD_FLAGS_WAITFORCOMPLETE,
	// то задача очевидно зависнет (исполЯемый коллбек ожидает выполнениЯ следующего коллбека, который будет вызван только после завершениЯ исполнЯемого).
	// В основной программе необходимо исключить такую ситуацию при вызовах MemoryThread_AppendMessage(),
	// а здесь проконтролировать.
	if( Flags & MEMORYTHREAD_FLAGS_WAITFORCOMPLETE )
		assert_param( xMemoryThreadTaskHandle != xTaskGetCurrentTaskHandle( ) );

	// Контроль заполнениЯ очередей
	uint32_t QueueMainEmptySpace, QueueDelayedEmptySpace;
	assert_param( MemoryThread_GetQueueSpaces( &QueueMainEmptySpace, &QueueDelayedEmptySpace ) );
	if( QueueMainEmptySpace < MemoryThread_MinQueueEmptySpace )
		MemoryThread_MinQueueEmptySpace = QueueMainEmptySpace;
	if( QueueDelayedEmptySpace < MemoryThread_MinQueueDelayedEmptySpace )
		MemoryThread_MinQueueDelayedEmptySpace = QueueDelayedEmptySpace;

	pMessage->Header.fDelayForFS = ( MEMORYTHREAD_FLAGS_WAITFORFS & Flags ) ? 1 : 0;
	if( !( MEMORYTHREAD_FLAGS_WAITFORCOMPLETE & Flags ) )
	{	// Просто добавить коллбек в очередь, без необходимости ожиданиЯ его завершениЯ
		pMessage->Header.OpCompleteMask = 0;
		return ( pdPASS == xQueueSend( pMemoryThread_QueueHdl, pMessage, 0 ) );
	}
	else
	{	// Подыскать свободный бит в группе и использовать его для синхронизации с завершением обработки сообщениЯ
		assert_param( NULL != pMemoryThread_EventGroupHdl );
		taskENTER_CRITICAL( );
		EventBits_t FreeBitMask = ( 1 << POSITION_VAL( MemoryThread_EventGroupFreeBits ) );
		assert_param( FreeBitMask < MEMORYTHREAD_EVENTFREEBITS_bm );
		assert_param( 0 == ( FreeBitMask & xEventGroupGetBits( pMemoryThread_EventGroupHdl ) ) );
		MemoryThread_EventGroupFreeBits &= ~FreeBitMask;
		pMessage->Header.OpCompleteMask = FreeBitMask;
		taskEXIT_CRITICAL( );
		// Добавить коллбек в очередь
		if( pdPASS != xQueueSend( pMemoryThread_QueueHdl, pMessage, 0 ) )
			return false;
		// Ожидать завершениЯ работы коллбека
		assert_param( FreeBitMask & xEventGroupWaitBits( pMemoryThread_EventGroupHdl, FreeBitMask, true, true, portMAX_DELAY ) );
		taskENTER_CRITICAL( );
		MemoryThread_EventGroupFreeBits |= FreeBitMask;
		taskEXIT_CRITICAL( );
		return true;		
	}
}

// Проверка, что файловая система готова к работе.
// ИспользуетсЯ вместо оригинальной функции, если у этого проекта файлуха в принципе не используетсЯ.
__weak bool FileSystem_CheckFSReadyBool( void )
{
	return false;
}

/***************************************************************************
  Задача MemoryThread_Task() - песочница длЯ медленно выполнЯемых команд -
главным образом, длЯ работы с SD-картой.
  Задача главным образом взаимодействует с очередью *pMemoryThread_QueueHdl.
Если в очереди поЯвлЯетсЯ сообщение, задача извлекает его и вызывает
содержащийсЯ в сообщении коллбек.
  Если принЯтое сообщение требует длЯ себЯ файловую систему, а ее нет -
это сообщение перемещаетсЯ в специальную очередь *pMemoryThread_QueueDelayedHdl,
где сообщение будет находитьсЯ до запуска файловой системы. В то же времЯ,
новые поступающие сообщениЯ (без требований FS) продолжат исполнЯтьсЯ.
  Если при приеме очередного сообщениЯ поЯвлЯетсЯ файловаЯ система, а очередь
*pMemoryThread_QueueDelayedHdl не пуста - сначала исполнЯютсЯ все коллбеки
из этой очереди, и только потом сообщениЯ из основной очереди.
  Если при приеме очередного сообщениЯ поЯвлЯетсЯ файловаЯ система, очередь
*pMemoryThread_QueueDelayedHdl не пуста и новое сообщение требует FS - это
сообщение перемещаетсЯ в хвост *pMemoryThread_QueueDelayedHdl и затем исполнЯютсЯ
все коллбеки из этой очереди.
***************************************************************************/
static void MemoryThread_Task( void *pParameters )
{
	( void ) pParameters;
	assert_param( NULL != pMemoryThread_QueueHdl );
	assert_param( NULL != pMemoryThread_QueueDelayedHdl );
	static MemoryThreadMessage_t Message;
	while( 1 )
	{
		// Ждать поЯвлениЯ очередного коллбека в очереди, но не извлекать его
		QueueHandle_t pQueueHdl = pMemoryThread_QueueHdl;
		assert_param( pdTRUE == xQueuePeek( pQueueHdl, &Message, portMAX_DELAY ) );
		assert_param( NULL != Message.Header.xCallback );
		// Проверить готовность файловой системы
		if( FileSystem_CheckFSReadyBool( ) )
		{	// ФайловаЯ система в порЯдке
			if( uxQueueMessagesWaiting( pMemoryThread_QueueDelayedHdl ) )
			{	// Если в очереди отложенных сообщений что-нибудь есть - сначала выполнить сообщение оттуда
				pQueueHdl = pMemoryThread_QueueDelayedHdl;
			}
		}
		else
		{	// ФайловаЯ система не в порЯдке
			if( Message.Header.fDelayForFS )
			{	// Коллбек требует обЯзательного выполнениЯ после инициализации файловой системы
				assert_param( 0 == Message.Header.OpCompleteMask );
				// Извлечь коллбек из основной очереди и переложить в очередь отложенных сообщений до восстановлениЯ файлухи
				assert_param( pdTRUE == xQueueReceive( pMemoryThread_QueueHdl, &Message, 0 ) );
				assert_param( pdPASS == xQueueSend( pMemoryThread_QueueDelayedHdl, &Message, 0 ) );
				// Коллбек отложен. Перейти к ожиданию следующего
				continue;
			}
		}
		// Извлечь сообщение из очереди и выполнить коллбек
		assert_param( pdTRUE == xQueueReceive( pQueueHdl, &Message, 0 ) );
		Message.Header.xCallback( &Message );
		if( Message.Header.OpCompleteMask )
		{
			assert_param( NULL != pMemoryThread_EventGroupHdl );
			( void ) xEventGroupSetBits( pMemoryThread_EventGroupHdl, Message.Header.OpCompleteMask );
		}
	}
}

#if 0
static void MemoryThread_Task( void *pParameters )
{
	( void ) pParameters;
	assert_param( NULL != pMemoryThread_QueueHdl );
	assert_param( NULL != pMemoryThread_QueueDelayedHdl );
	while( 1 )
	{
		static MemoryThreadMessage_t Message;
		// Ждать поЯвлениЯ очередного коллбека в очереди
		assert_param( pdTRUE == xQueuePeek( pMemoryThread_QueueHdl, &Message, portMAX_DELAY ) );	// пока не извлекать коллбек из очереди
		assert_param( NULL != Message.Header.xCallback );
		bool bFS_Ready = FileSystem_CheckFSReadyBool( );
		if( !bFS_Ready && Message.Header.fDelayForFS )
		{	// ФайловаЯ система не готова, а коллбек требует обЯзательного выполнениЯ после инициализации файлухи
			assert_param( 0 == Message.Header.OpCompleteMask );
			// Извлечь коллбек из очереди и переложить в другую очередь до восстановлениЯ файлухи
			assert_param( pdTRUE == xQueueReceive( pMemoryThread_QueueHdl, &Message, 0 ) );
			assert_param( pdPASS == xQueueSend( pMemoryThread_QueueDelayedHdl, &Message, 0 ) );
			// Коллбек отложен. Перейти к ожиданию следующего
			continue;
		}

/*		// Перед выполнением принЯтого коллбека, сначала проверить очередь отложенных коллбеков		
		if( bFS_Ready && uxQueueMessagesWaiting( pMemoryThread_QueueDelayedHdl ) )
		{	// ФайловаЯ система готова, и в отложенной очереди есть сообщениЯ, ожидающие файлуху
			while( pdTRUE == xQueueReceive( pMemoryThread_QueueDelayedHdl, &Message, 0 ) )
				Message.Header.xCallback( &Message );			// выполнить все коллбеки из отложенной очереди
			// !! если в процессе выполнениЯ этих сообщений файлуха снова упадет, выполнены они уже не будут.
			// !! при необходимости, можно контролировать выполнение по тому же флагу,
			// !! и при ошибке снова складывать в отложенную очередь.
		}
*/

		// Извлечь из основной очереди и выполнить принЯтый коллбек
		assert_param( pdTRUE == xQueueReceive( pMemoryThread_QueueHdl, &Message, 0 ) );
		Message.Header.xCallback( &Message );
		if( Message.Header.OpCompleteMask )
		{
			assert_param( NULL != pMemoryThread_EventGroupHdl );
			( void ) xEventGroupSetBits( pMemoryThread_EventGroupHdl, Message.Header.OpCompleteMask );
		}

		// Если в результате основного коллбека поЯвилась файлуха, выполнить очередь отложенных коллбеков		
		bFS_Ready = FileSystem_CheckFSReadyBool( );
		if( bFS_Ready && uxQueueMessagesWaiting( pMemoryThread_QueueDelayedHdl ) )
		{	// ФайловаЯ система готова, и в отложенной очереди есть сообщениЯ, ожидающие файлуху
			while( pdTRUE == xQueueReceive( pMemoryThread_QueueDelayedHdl, &Message, 0 ) )
				Message.Header.xCallback( &Message );			// выполнить все коллбеки из отложенной очереди
			// !! если в процессе выполнениЯ этих сообщений файлуха снова упадет, выполнены они уже не будут.
			// !! при необходимости, можно контролировать выполнение по тому же флагу,
			// !! и при ошибке снова складывать в отложенную очередь.
		}

	}
}
#endif

bool MemoryThread_TaskInit( void )
{
	assert_param( MEMORYTHREAD_MESSAGE_SIZE == sizeof( MemoryThreadMessage_t ) );
	assert_param( taskSCHEDULER_NOT_STARTED == xTaskGetSchedulerState( ) );

	assert_param( NULL == pMemoryThread_QueueHdl );
	assert_param( NULL != ( pMemoryThread_QueueHdl = xQueueCreate( MEMORYTHREAD_MESSAGE_COUNT, sizeof( MemoryThreadMessage_t ) ) ) );

	assert_param( NULL == pMemoryThread_QueueDelayedHdl );
	assert_param( NULL != ( pMemoryThread_QueueDelayedHdl = xQueueCreate( MEMORYTHREAD_MESSAGEDELAYED_COUNT, sizeof( MemoryThreadMessage_t ) ) ) );

	assert_param( NULL == pMemoryThread_EventGroupHdl );
	assert_param( NULL != ( pMemoryThread_EventGroupHdl = xEventGroupCreate( ) ) );
	MemoryThread_EventGroupFreeBits = MEMORYTHREAD_EVENTFREEBITS_bm;

	assert_param( pdTRUE == xTaskCreate( MemoryThread_Task, TASK_MEMORY_THREAD_NAME, TASK_MEMORY_THREAD_STACK_SIZE, NULL, TASK_MEMORY_THREAD_PRIORITY, &xMemoryThreadTaskHandle ) );
	assert_param( NULL != xMemoryThreadTaskHandle );
	SysState_AppendTaskHandler( xMemoryThreadTaskHandle );

	MemoryThread_SprintfMutexGive( );
	MemoryThread_BufferMutexGive( );

	return true;
}

//volatile uint32_t MemoryThread_SprintfMutex_CallAddress = 0;
//volatile TickType_t MemoryThread_SprintfMutex_xTicksToWait = 0xFFFF;
//volatile const char *pMemoryThread_SprintfMutex_CallFunctionName = NULL;

// Функции длЯ разграничениЯ доступа к snprintf()
void MemoryThread_SprintfMutexGive( void )
{
	assert_param( NULL != EventGroup_System );
	( void ) xEventGroupSetBits( EventGroup_System, EVENTSYSTEM_SPRINTF_READY );
//	MemoryThread_SprintfMutex_CallAddress = 0;
//	MemoryThread_SprintfMutex_xTicksToWait = 0xFFFF;
//	pMemoryThread_SprintfMutex_CallFunctionName = NULL;
}

bool MemoryThread_SprintfMutexTake( TickType_t xTicksToWait )
{
	assert_param( NULL != EventGroup_System );
	const EventBits_t EventToWait = EVENTSYSTEM_SPRINTF_READY;
	return ( EventToWait & xEventGroupWaitBits( EventGroup_System, EventToWait, pdTRUE, pdTRUE, xTicksToWait ) );
}

/*bool MemoryThread_SprintfMutexTakeExt( TickType_t xTicksToWait, const char *pFunc )
{
	assert_param( NULL != EventGroup_System );
	const EventBits_t EventToWait = EVENTSYSTEM_SPRINTF_READY;
	bool Result = ( EventToWait & xEventGroupWaitBits( EventGroup_System, EventToWait, pdTRUE, pdTRUE, xTicksToWait ) );
	if( Result )
	{
		MemoryThread_SprintfMutex_CallAddress = __get_LR( );
		MemoryThread_SprintfMutex_xTicksToWait = xTicksToWait;
		pMemoryThread_SprintfMutex_CallFunctionName = pFunc;
	}
	return Result;
}*/

// Функции длЯ разграничениЯ доступа к буферу
// Сделано без использованиЯ операционки т.к. подразумеваетсЯ,
// что буфер используют только коллбеки из MemoryThread_Task(), и они не могут пересекатьсЯ
static bool bMemoryThread_BufferMutex = false;
void MemoryThread_BufferMutexGive( void )
{
	ATOMIC_WRITE( bMemoryThread_BufferMutex, true );
}

bool MemoryThread_BufferMutexTake( void )
{
	bool Result;
	ENTER_CRITICAL_SECTION( );
	Result = bMemoryThread_BufferMutex;
	bMemoryThread_BufferMutex = false;
	EXIT_CRITICAL_SECTION( );
	return Result;
}

