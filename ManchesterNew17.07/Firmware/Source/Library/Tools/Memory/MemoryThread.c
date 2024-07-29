// MemoryThread.c
#include "ProjectConfig.h"		// ������ ���������, ������ ������.
#include "stm32xxxx_hal.h"		// ����� ���������
#include "MemoryThread.h"
#include "TaskConfig.h"
#include "Queue.h"
//#include "FileSystem.h"			// FileSystem_CheckFSReady()

static QueueHandle_t pMemoryThread_QueueHdl = NULL;
static QueueHandle_t pMemoryThread_QueueDelayedHdl = NULL;
static EventGroupHandle_t pMemoryThread_EventGroupHdl = NULL;

// ����� ��������� ����� ��� ������������� � �������� ������� ��� �������� ���������� ��������� ���������
//static EventBits_t MemoryThread_EventGroupFreeBits = 0x00FFFFFF;	// 1 ������������� ���������� (���������������) ����
#define MEMORYTHREAD_EVENTFREEBITS_bm		( ( uint16_t ) 0xFFFF )
static uint16_t MemoryThread_EventGroupFreeBits = 0;	// 1 ������������� ���������� (���������������) ����

// �������� ������������ ������������� ������ ���������
uint32_t MemoryThread_MinQueueEmptySpace		= MEMORYTHREAD_MESSAGE_COUNT;
uint32_t MemoryThread_MinQueueDelayedEmptySpace	= MEMORYTHREAD_MESSAGEDELAYED_COUNT;

// ����� ��� ������������� ������������� � ��������� �� ������ MemoryThread_Task()
__no_init uint8_t aMemoryThreadTaskBuffer[1024];

// ������� ������
static TaskHandle_t xMemoryThreadTaskHandle;

// ������� ��������� ����� � ��������
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


// �������� ��������� � ��������� � �������
// pMessage			- ��������� � ����������� ����� pMessage->Header.xCallback
// ( Flags & MEMORYTHREAD_FLAGS_WAITFORCOMPLETE )	- ���������� ��������� ���������� ���������� ��������, ������ ��� ��������.
// ( Flags & MEMORYTHREAD_FLAGS_WAITFORFS )			- ��� ��������� ������ �������� �������, �������� ���������� �������� �� �������������� �������� �������
//		(��� ����� �����������������, �.�. ���������������, ��� ������� ����� ������� �� ���������, � ������������� ������ ������)
bool MemoryThread_AppendMessage( MemoryThreadMessage_t *pMessage, uint32_t Flags )
{
	assert_param( NULL != pMemoryThread_QueueHdl );
	assert_param( NULL != pMessage );
	assert_param( NULL != pMessage->Header.xCallback );
	assert_param( !( ( Flags & MEMORYTHREAD_FLAGS_WAITFORCOMPLETE ) && ( Flags & MEMORYTHREAD_FLAGS_WAITFORFS ) ) );

	// �������� ��������, ����� � ����� �������� MemoryThread_Task(), ������������ ����� MemoryThread_AppendMessage() � ������ ���������.
	// ��������, ������� ������ ��������� � ������ ���� ��������� ��� ��������� � ���.
	// ��� ����, ���� ��� ������ MemoryThread_AppendMessage() ����� ��������� ���� MEMORYTHREAD_FLAGS_WAITFORCOMPLETE,
	// �� ������ �������� �������� (���������� ������� ������� ���������� ���������� ��������, ������� ����� ������ ������ ����� ���������� ������������).
	// � �������� ��������� ���������� ��������� ����� �������� ��� ������� MemoryThread_AppendMessage(),
	// � ����� �����������������.
	if( Flags & MEMORYTHREAD_FLAGS_WAITFORCOMPLETE )
		assert_param( xMemoryThreadTaskHandle != xTaskGetCurrentTaskHandle( ) );

	// �������� ���������� ��������
	uint32_t QueueMainEmptySpace, QueueDelayedEmptySpace;
	assert_param( MemoryThread_GetQueueSpaces( &QueueMainEmptySpace, &QueueDelayedEmptySpace ) );
	if( QueueMainEmptySpace < MemoryThread_MinQueueEmptySpace )
		MemoryThread_MinQueueEmptySpace = QueueMainEmptySpace;
	if( QueueDelayedEmptySpace < MemoryThread_MinQueueDelayedEmptySpace )
		MemoryThread_MinQueueDelayedEmptySpace = QueueDelayedEmptySpace;

	pMessage->Header.fDelayForFS = ( MEMORYTHREAD_FLAGS_WAITFORFS & Flags ) ? 1 : 0;
	if( !( MEMORYTHREAD_FLAGS_WAITFORCOMPLETE & Flags ) )
	{	// ������ �������� ������� � �������, ��� ������������� �������� ��� ����������
		pMessage->Header.OpCompleteMask = 0;
		return ( pdPASS == xQueueSend( pMemoryThread_QueueHdl, pMessage, 0 ) );
	}
	else
	{	// ��������� ��������� ��� � ������ � ������������ ��� ��� ������������� � ����������� ��������� ���������
		assert_param( NULL != pMemoryThread_EventGroupHdl );
		taskENTER_CRITICAL( );
		EventBits_t FreeBitMask = ( 1 << POSITION_VAL( MemoryThread_EventGroupFreeBits ) );
		assert_param( FreeBitMask < MEMORYTHREAD_EVENTFREEBITS_bm );
		assert_param( 0 == ( FreeBitMask & xEventGroupGetBits( pMemoryThread_EventGroupHdl ) ) );
		MemoryThread_EventGroupFreeBits &= ~FreeBitMask;
		pMessage->Header.OpCompleteMask = FreeBitMask;
		taskEXIT_CRITICAL( );
		// �������� ������� � �������
		if( pdPASS != xQueueSend( pMemoryThread_QueueHdl, pMessage, 0 ) )
			return false;
		// ������� ���������� ������ ��������
		assert_param( FreeBitMask & xEventGroupWaitBits( pMemoryThread_EventGroupHdl, FreeBitMask, true, true, portMAX_DELAY ) );
		taskENTER_CRITICAL( );
		MemoryThread_EventGroupFreeBits |= FreeBitMask;
		taskEXIT_CRITICAL( );
		return true;		
	}
}

// ��������, ��� �������� ������� ������ � ������.
// ������������ ������ ������������ �������, ���� � ����� ������� ������� � �������� �� ������������.
__weak bool FileSystem_CheckFSReadyBool( void )
{
	return false;
}

/***************************************************************************
  ������ MemoryThread_Task() - ��������� ��� �������� ����������� ������ -
������� �������, ��� ������ � SD-������.
  ������ ������� ������� ��������������� � �������� *pMemoryThread_QueueHdl.
���� � ������� ���������� ���������, ������ ��������� ��� � ��������
������������ � ��������� �������.
  ���� �������� ��������� ������� ��� ���� �������� �������, � �� ��� -
��� ��������� ������������ � ����������� ������� *pMemoryThread_QueueDelayedHdl,
��� ��������� ����� ���������� �� ������� �������� �������. � �� �� �����,
����� ����������� ��������� (��� ���������� FS) ��������� �����������.
  ���� ��� ������ ���������� ��������� ���������� �������� �������, � �������
*pMemoryThread_QueueDelayedHdl �� ����� - ������� ����������� ��� ��������
�� ���� �������, � ������ ����� ��������� �� �������� �������.
  ���� ��� ������ ���������� ��������� ���������� �������� �������, �������
*pMemoryThread_QueueDelayedHdl �� ����� � ����� ��������� ������� FS - ���
��������� ������������ � ����� *pMemoryThread_QueueDelayedHdl � ����� �����������
��� �������� �� ���� �������.
***************************************************************************/
static void MemoryThread_Task( void *pParameters )
{
	( void ) pParameters;
	assert_param( NULL != pMemoryThread_QueueHdl );
	assert_param( NULL != pMemoryThread_QueueDelayedHdl );
	static MemoryThreadMessage_t Message;
	while( 1 )
	{
		// ����� ��������� ���������� �������� � �������, �� �� ��������� ���
		QueueHandle_t pQueueHdl = pMemoryThread_QueueHdl;
		assert_param( pdTRUE == xQueuePeek( pQueueHdl, &Message, portMAX_DELAY ) );
		assert_param( NULL != Message.Header.xCallback );
		// ��������� ���������� �������� �������
		if( FileSystem_CheckFSReadyBool( ) )
		{	// �������� ������� � �������
			if( uxQueueMessagesWaiting( pMemoryThread_QueueDelayedHdl ) )
			{	// ���� � ������� ���������� ��������� ���-������ ���� - ������� ��������� ��������� ������
				pQueueHdl = pMemoryThread_QueueDelayedHdl;
			}
		}
		else
		{	// �������� ������� �� � �������
			if( Message.Header.fDelayForFS )
			{	// ������� ������� ������������� ���������� ����� ������������� �������� �������
				assert_param( 0 == Message.Header.OpCompleteMask );
				// ������� ������� �� �������� ������� � ���������� � ������� ���������� ��������� �� �������������� �������
				assert_param( pdTRUE == xQueueReceive( pMemoryThread_QueueHdl, &Message, 0 ) );
				assert_param( pdPASS == xQueueSend( pMemoryThread_QueueDelayedHdl, &Message, 0 ) );
				// ������� �������. ������� � �������� ����������
				continue;
			}
		}
		// ������� ��������� �� ������� � ��������� �������
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
		// ����� ��������� ���������� �������� � �������
		assert_param( pdTRUE == xQueuePeek( pMemoryThread_QueueHdl, &Message, portMAX_DELAY ) );	// ���� �� ��������� ������� �� �������
		assert_param( NULL != Message.Header.xCallback );
		bool bFS_Ready = FileSystem_CheckFSReadyBool( );
		if( !bFS_Ready && Message.Header.fDelayForFS )
		{	// �������� ������� �� ������, � ������� ������� ������������� ���������� ����� ������������� �������
			assert_param( 0 == Message.Header.OpCompleteMask );
			// ������� ������� �� ������� � ���������� � ������ ������� �� �������������� �������
			assert_param( pdTRUE == xQueueReceive( pMemoryThread_QueueHdl, &Message, 0 ) );
			assert_param( pdPASS == xQueueSend( pMemoryThread_QueueDelayedHdl, &Message, 0 ) );
			// ������� �������. ������� � �������� ����������
			continue;
		}

/*		// ����� ����������� ��������� ��������, ������� ��������� ������� ���������� ���������		
		if( bFS_Ready && uxQueueMessagesWaiting( pMemoryThread_QueueDelayedHdl ) )
		{	// �������� ������� ������, � � ���������� ������� ���� ���������, ��������� �������
			while( pdTRUE == xQueueReceive( pMemoryThread_QueueDelayedHdl, &Message, 0 ) )
				Message.Header.xCallback( &Message );			// ��������� ��� �������� �� ���������� �������
			// !! ���� � �������� ���������� ���� ��������� ������� ����� ������, ��������� ��� ��� �� �����.
			// !! ��� �������������, ����� �������������� ���������� �� ���� �� �����,
			// !! � ��� ������ ����� ���������� � ���������� �������.
		}
*/

		// ������� �� �������� ������� � ��������� �������� �������
		assert_param( pdTRUE == xQueueReceive( pMemoryThread_QueueHdl, &Message, 0 ) );
		Message.Header.xCallback( &Message );
		if( Message.Header.OpCompleteMask )
		{
			assert_param( NULL != pMemoryThread_EventGroupHdl );
			( void ) xEventGroupSetBits( pMemoryThread_EventGroupHdl, Message.Header.OpCompleteMask );
		}

		// ���� � ���������� ��������� �������� ��������� �������, ��������� ������� ���������� ���������		
		bFS_Ready = FileSystem_CheckFSReadyBool( );
		if( bFS_Ready && uxQueueMessagesWaiting( pMemoryThread_QueueDelayedHdl ) )
		{	// �������� ������� ������, � � ���������� ������� ���� ���������, ��������� �������
			while( pdTRUE == xQueueReceive( pMemoryThread_QueueDelayedHdl, &Message, 0 ) )
				Message.Header.xCallback( &Message );			// ��������� ��� �������� �� ���������� �������
			// !! ���� � �������� ���������� ���� ��������� ������� ����� ������, ��������� ��� ��� �� �����.
			// !! ��� �������������, ����� �������������� ���������� �� ���� �� �����,
			// !! � ��� ������ ����� ���������� � ���������� �������.
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

// ������� ��� ������������� ������� � snprintf()
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

// ������� ��� ������������� ������� � ������
// ������� ��� ������������� ����������� �.�. ���������������,
// ��� ����� ���������� ������ �������� �� MemoryThread_Task(), � ��� �� ����� ������������
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

