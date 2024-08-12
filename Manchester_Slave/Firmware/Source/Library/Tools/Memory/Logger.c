// Logger.c
#include "ProjectConfig.h"		// ������ ���������, ������ ������.
#include "stm32xxxx_hal.h"		// ����� ���������
#include "LogFile.h"
#include "Logger.h"
#include "SKLP_Service.h"		// SKLP_Time
#include "Task.h"
#include "Semphr.h"
#include <stdio.h>
#include <string.h>

#define LOGGERMESSAGE_DIRECTMARK	( ( TickType_t ) -1 )		// ����, ��������� ���������� ���������� ����� ������� ��� ������ ���������
#ifndef	LOGGER_FILE_MAXSIZE
#define LOGGER_FILE_MAXSIZE			( FS_SECTOR_SIZE * 1024 )	// ����������� ������� ����� �������, ����� ������ � ���� MemInfo
#endif

static FIL LogFile;
LogFileHdl_t LogFileHdl_TechLog = { &LogFile, "Log.txt", 0 };

static void Logger_WriteRecordCB( MemoryThreadMessage_t *pMessage )
{
	assert_param( NULL != pMessage );
	assert_param( Logger_WriteRecordCB == pMessage->Header.xCallback );
	LoggerMessage_t *pLoggerMessage = ( LoggerMessage_t * ) pMessage;

	char const *pRecord = ( ( NULL == pLoggerMessage->pTextExt ) ? pLoggerMessage->aTextInt : pLoggerMessage->pTextExt );
	int RecordSize;

	assert_param( MemoryThread_BufferMutexTake( ) );		// �������� ���������� �� ������������� aMemoryThreadTaskBuffer
	if( LOGGERMESSAGE_DIRECTMARK == pLoggerMessage->TimeStampTicks )
		RecordSize = strlen( pRecord );
	else
	{	// ������ ������:
		// "[0000AB86] [2015.06.29 12:07:45.02] Message Text\r\n"
		assert_param( MemoryThread_SprintfMutexTake( 5 ) );		// �������� ���������� �� ������������� snprintf()
		RecordSize = snprintf(			// snprintf() ���������� ���������� ���������� ��������
			( char * ) aMemoryThreadTaskBuffer, sizeof( aMemoryThreadTaskBuffer ),
			"[%08X] [%4hu.%02hhu.%02hhu %02hhu:%02hhu:%02hhu.%02hhu] %s\r\n",
			pLoggerMessage->TimeStampTicks,
			pLoggerMessage->TimeStampSKLP.YearFrom2000 + ( uint16_t ) 2000,
			pLoggerMessage->TimeStampSKLP.Month,
			pLoggerMessage->TimeStampSKLP.Day,
			pLoggerMessage->TimeStampSKLP.Hour,
			pLoggerMessage->TimeStampSKLP.Minute,
			pLoggerMessage->TimeStampSKLP.Second,
			pLoggerMessage->TimeStampSKLP.SecondHundr,
			pRecord );
		MemoryThread_SprintfMutexGive( );
		pRecord = ( char * ) aMemoryThreadTaskBuffer;
	}
	do
	{
		if( 0 == RecordSize )
			break;
		LogFileHdl_t * const pLogFile = &LogFileHdl_TechLog;
		FRESULT FResult;
		// ������� ����, ���� ��� �� ������ - ��� ����� ����������� ������ ������ � ����
		if( FR_OK != ( FResult = LogFile_WriteRecord( pLogFile, NULL, 0 ) ) )
			break;
		// ��������� ������ ����� ����
		uint32_t FileSizeOriginal = pLogFile->pFile->fsize;
		if( FileSizeOriginal > ( uint32_t ) ( LOGGER_FILE_MAXSIZE * 0.85f ) )	// 0.75f
		{	// ������ �������� ���������� �����. ������� ������ � �������� �����
			uint32_t ResultSize = ( uint32_t ) ( LOGGER_FILE_MAXSIZE * 0.5f );
			// ���������� ������������ �� 32K ���������� ����������� ������!!!
			ResultSize = ResultSize - ( ( 32 * 1024 ) - ( FileSizeOriginal - ResultSize ) % ( 32 * 1024 ) );
			// ������ ����� ������ ��������
			TickType_t StartTimestamp = xTaskGetTickCount( );
			// ���������� ������� �����
			if( FR_OK != ( FResult = FileSystem_FileTrancateHead( pLogFile->pFile, pLogFile->pFileName, ResultSize ) ) )
				break;
			// �������� ����������� �����
			uint32_t TimeElapsed_ms = ( ( xTaskGetTickCount( ) - StartTimestamp ) * 1000 ) / configTICK_RATE_HZ;
			// ����������� ����� � ����������� ��������
			assert_param( MemoryThread_SprintfMutexTake( 5 ) );
			LoggerMessage_t Message;
			Message.pTextExt = NULL;
			snprintf( ( char * ) Message.aTextInt, sizeof( Message.aTextInt ),
						"----- Log file truncated from %u to %u for %u ms -----",
						FileSizeOriginal, pLogFile->pFile->fsize, TimeElapsed_ms );
			MemoryThread_SprintfMutexGive( );
			assert_param( Logger_WriteRecordExt( &Message, LOGGER_FLAGS_APPENDTIMESTAMP | LOGGER_FLAGS_WAITEFORFS ) );
		}
		// ���������� ������ � ���
		if( FR_OK != ( FResult = LogFile_WriteRecord( pLogFile, pRecord, RecordSize ) ) )
			break;
		( void ) FResult;
	} while( 0 );
	MemoryThread_BufferMutexGive( );
}

bool Logger_WriteRecordExt( LoggerMessage_t *pMessage, uint32_t Flags )
{
	assert_param( NULL != pMessage );

	if( Flags & LOGGER_FLAGS_APPENDTIMESTAMP )
	{
		taskENTER_CRITICAL( );
		pMessage->TimeStampSKLP = SKLP_Time;
		pMessage->TimeStampTicks = xTaskGetTickCount( );
		taskEXIT_CRITICAL( );
	}
	else
		pMessage->TimeStampTicks = LOGGERMESSAGE_DIRECTMARK;

	pMessage->Header.xCallback = Logger_WriteRecordCB;
	uint32_t AppendMessageFlags = 0;
	if( Flags & LOGGER_FLAGS_WAITEFORCOMPLETE )
		AppendMessageFlags |= MEMORYTHREAD_FLAGS_WAITFORCOMPLETE;
	if( Flags & LOGGER_FLAGS_WAITEFORFS )
		AppendMessageFlags |= MEMORYTHREAD_FLAGS_WAITFORFS;
	assert_param( sizeof( *pMessage ) <= sizeof( MemoryThreadMessage_t ) );
	return MemoryThread_AppendMessage( ( MemoryThreadMessage_t * ) pMessage, AppendMessageFlags );
}

bool Logger_WriteRecord( char const *pText, uint32_t Flags )
{
	assert_param( NULL != pText );
	LoggerMessage_t Message;

	if( Flags & LOGGER_FLAGS_STATICTEXT )
		Message.pTextExt = pText;
	else
	{
		Message.pTextExt = NULL;
		strncpy( Message.aTextInt, pText, sizeof( Message.aTextInt ) );
	}

	return Logger_WriteRecordExt( &Message, Flags );
}

/*/ ������� �� ������� �������� ������
// ��� ������ �� ���������, �� ������������ ���� ��������� ������, ������� ����� ����� ���������� ��� ��������� ������
bool Logger_Clear( void )
{
//	LogFileMessage_t LogFileMessage = { LogFile_ClearCB, &LogFileHdl_TechLog };
//	return MemoryThread_AppendMessage( ( MemoryThreadMessage_t * ) &LogFileMessage );
	MemoryThread_MarkDataPresent( &LogFileHdl_TechLog, false );
	return true;
}
*/
