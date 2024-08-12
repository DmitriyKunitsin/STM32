// MemoryThread.h
// Ќизкоприоритетный поток дл€ обработки доступа к файловой системе,
// а также для других разовых низкоприоритетных задач
#ifndef	MEMORY_THREAD_H
#define	MEMORY_THREAD_H
#include <stdbool.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "Semphr.h"
#include "Event_groups.h"

// –абота с потоком осуществляется через посылку сообщений в очередь потока.
// ѕоток извлекает сообщение из очереди и вызывает встроенный в сообщение коллбек.
// “акже в сообщении могут быть вставлены данные, которые будут переданы коллбеку.

#define	MEMORYTHREAD_MESSAGE_SIZE			192			// ѕолный размер сообщения
#define	MEMORYTHREAD_BUFFER_SIZE			1024		// –азмер буфера aMemoryThreadTaskBuffer[], который можно произвольно использовать в коллбеках

#ifndef	MEMORYTHREAD_MESSAGE_COUNT
#define	MEMORYTHREAD_MESSAGE_COUNT			20			// –азмер буфера под сообщени€
#endif	// MEMORYTHREAD_MESSAGE_COUNT

#ifndef	MEMORYTHREAD_MESSAGEDELAYED_COUNT
#define	MEMORYTHREAD_MESSAGEDELAYED_COUNT	15			// –азмер буфера под отложенные сообщени€
#endif	// MEMORYTHREAD_MESSAGEDELAYED_COUNT

struct MemoryThreadMessage_struct;
typedef struct MemoryThreadMessage_struct MemoryThreadMessage_t;
typedef void ( *MemoryThreadCallback_t ) ( MemoryThreadMessage_t * );

// «аголовок передаваемого в MemoryThread_Task() сообщения
#pragma pack( 1 )
typedef struct MemoryThreadMessageHeader_struct
{
	MemoryThreadCallback_t	xCallback;				//  оллбек, вызываемый дл€ обработки этого сообщени€
	uint16_t				OpCompleteMask;			// ћаска битов в pMemoryThread_EventGroupHdl, которые необходимо выставить после завершения выполнения коллбека
	struct
	{
		uint16_t			fDelayForFS	:1;			// Ќеобходимо дождаться инициализации файловой системы, прежде чем выполнять этот коллбек
		uint16_t			fReserved	:15;
	};
}
MemoryThreadMessageHeader_t;
#pragma pack( )

// —ообщение MemoryThread_Task(), состоит из общего заголовка с коллбеком, и произвольных данных.
#pragma pack( 1 )
struct MemoryThreadMessage_struct
{
	MemoryThreadMessageHeader_t	Header;
	uint8_t	aData[ MEMORYTHREAD_MESSAGE_SIZE - sizeof( Header ) ];	// ѕередаваемые данные
};
#pragma pack( )

// ƒополнительный параметр при вызове MemoryThread_AppendMessage()
#define	MEMORYTHREAD_FLAGS_WAITFORCOMPLETE		( 1 << 0 )		// необходимо ожидать завершения выполнения коллбека
#define	MEMORYTHREAD_FLAGS_WAITFORFS			( 1 << 1 )		// при нарушении работы файловой системы, отложить выполнение коллбека до восстановления файловой системы

bool MemoryThread_TaskInit( void );																// »нициализация
//bool MemoryThread_AppendMessage( MemoryThreadMessage_t *pMessage, bool WaitForComplete );		// ќтправка сообщения в очередь
bool MemoryThread_AppendMessage( MemoryThreadMessage_t *pMessage, uint32_t Flags );			// ќтправка сообщения в очередь
inline bool MemoryThread_AppendCallback( MemoryThreadCallback_t xCallback, uint32_t Flags )	// ќтправка упрощенного сообщения (только коллбек, без данных)
{	MemoryThreadMessageHeader_t Header = { 0 };
	Header.xCallback = xCallback;
	return MemoryThread_AppendMessage( ( MemoryThreadMessage_t * ) &Header, Flags );
}

bool MemoryThread_GetQueueSpaces( uint32_t *pQueueSpaceMain, uint32_t *pQueueSpaceDelayed );

// ‘ункции для разграничения доступа к snprintf()
void MemoryThread_SprintfMutexGive( void );
bool MemoryThread_SprintfMutexTake( TickType_t xTicksToWait );
//bool MemoryThread_SprintfMutexTakeExt( TickType_t xTicksToWait, const char *pFunc );
//#define	MemoryThread_SprintfMutexTake( xTicksToWait )	MemoryThread_SprintfMutexTakeExt( ( xTicksToWait ), __FUNCTION__ )

// Ѕуфер для произвольного использования в коллбеках из задачи MemoryThread_Task()
extern uint8_t aMemoryThreadTaskBuffer[MEMORYTHREAD_BUFFER_SIZE];
// ‘ункции для контроля доступа к буферу
void MemoryThread_BufferMutexGive( void );
bool MemoryThread_BufferMutexTake( void );

#endif	// MEMORY_THREAD_H

