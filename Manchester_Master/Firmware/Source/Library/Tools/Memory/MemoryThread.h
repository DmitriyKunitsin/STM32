// MemoryThread.h
// ����������������� ����� ��� ��������� ������� � �������� �������,
// � ����� ��� ������ ������� ����������������� �����
#ifndef	MEMORY_THREAD_H
#define	MEMORY_THREAD_H
#include <stdbool.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "Semphr.h"
#include "Event_groups.h"

// ������ � ������� �������������� ����� ������� ��������� � ������� ������.
// ����� ��������� ��������� �� ������� � �������� ���������� � ��������� �������.
// ����� � ��������� ����� ���� ��������� ������, ������� ����� �������� ��������.

#define	MEMORYTHREAD_MESSAGE_SIZE			192			// ������ ������ ���������
#define	MEMORYTHREAD_BUFFER_SIZE			1024		// ������ ������ aMemoryThreadTaskBuffer[], ������� ����� ����������� ������������ � ���������

#ifndef	MEMORYTHREAD_MESSAGE_COUNT
#define	MEMORYTHREAD_MESSAGE_COUNT			20			// ������ ������ ��� ���������
#endif	// MEMORYTHREAD_MESSAGE_COUNT

#ifndef	MEMORYTHREAD_MESSAGEDELAYED_COUNT
#define	MEMORYTHREAD_MESSAGEDELAYED_COUNT	15			// ������ ������ ��� ���������� ���������
#endif	// MEMORYTHREAD_MESSAGEDELAYED_COUNT

struct MemoryThreadMessage_struct;
typedef struct MemoryThreadMessage_struct MemoryThreadMessage_t;
typedef void ( *MemoryThreadCallback_t ) ( MemoryThreadMessage_t * );

// ��������� ������������� � MemoryThread_Task() ���������
#pragma pack( 1 )
typedef struct MemoryThreadMessageHeader_struct
{
	MemoryThreadCallback_t	xCallback;				// �������, ���������� ��� ��������� ����� ���������
	uint16_t				OpCompleteMask;			// ����� ����� � pMemoryThread_EventGroupHdl, ������� ���������� ��������� ����� ���������� ���������� ��������
	struct
	{
		uint16_t			fDelayForFS	:1;			// ���������� ��������� ������������� �������� �������, ������ ��� ��������� ���� �������
		uint16_t			fReserved	:15;
	};
}
MemoryThreadMessageHeader_t;
#pragma pack( )

// ��������� MemoryThread_Task(), ������� �� ������ ��������� � ���������, � ������������ ������.
#pragma pack( 1 )
struct MemoryThreadMessage_struct
{
	MemoryThreadMessageHeader_t	Header;
	uint8_t	aData[ MEMORYTHREAD_MESSAGE_SIZE - sizeof( Header ) ];	// ������������ ������
};
#pragma pack( )

// �������������� �������� ��� ������ MemoryThread_AppendMessage()
#define	MEMORYTHREAD_FLAGS_WAITFORCOMPLETE		( 1 << 0 )		// ���������� ������� ���������� ���������� ��������
#define	MEMORYTHREAD_FLAGS_WAITFORFS			( 1 << 1 )		// ��� ��������� ������ �������� �������, �������� ���������� �������� �� �������������� �������� �������

bool MemoryThread_TaskInit( void );																// �������������
//bool MemoryThread_AppendMessage( MemoryThreadMessage_t *pMessage, bool WaitForComplete );		// �������� ��������� � �������
bool MemoryThread_AppendMessage( MemoryThreadMessage_t *pMessage, uint32_t Flags );			// �������� ��������� � �������
inline bool MemoryThread_AppendCallback( MemoryThreadCallback_t xCallback, uint32_t Flags )	// �������� ����������� ��������� (������ �������, ��� ������)
{	MemoryThreadMessageHeader_t Header = { 0 };
	Header.xCallback = xCallback;
	return MemoryThread_AppendMessage( ( MemoryThreadMessage_t * ) &Header, Flags );
}

bool MemoryThread_GetQueueSpaces( uint32_t *pQueueSpaceMain, uint32_t *pQueueSpaceDelayed );

// ������� ��� ������������� ������� � snprintf()
void MemoryThread_SprintfMutexGive( void );
bool MemoryThread_SprintfMutexTake( TickType_t xTicksToWait );
//bool MemoryThread_SprintfMutexTakeExt( TickType_t xTicksToWait, const char *pFunc );
//#define	MemoryThread_SprintfMutexTake( xTicksToWait )	MemoryThread_SprintfMutexTakeExt( ( xTicksToWait ), __FUNCTION__ )

// ����� ��� ������������� ������������� � ��������� �� ������ MemoryThread_Task()
extern uint8_t aMemoryThreadTaskBuffer[MEMORYTHREAD_BUFFER_SIZE];
// ������� ��� �������� ������� � ������
void MemoryThread_BufferMutexGive( void );
bool MemoryThread_BufferMutexTake( void );

#endif	// MEMORY_THREAD_H

