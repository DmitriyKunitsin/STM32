// AsyncData.c
// ����������� ������ ����������� ������ � ����
#include "ProjectConfig.h"		// ������ ���������, ������ ������.
#include "stm32xxxx_hal.h"		// ����� ���������
#include "LogFile.h"
#include "AsyncData.h"			// ������
#include "Utils.h"				// CalcCRC16SKLP()
#include "SKLP_Service.h"		// SKLP_Time
#include <string.h>				// memcpy()

// ���� ��� ���������� ����������� ������
static FIL FileDataAsync_FIL;
LogFileHdl_t LogFileHdl_AsyncData = { &FileDataAsync_FIL, "DataAsyn.dat", 0 };

#define	ASYNCDATA_RECORDSIZE_TRIM	( sizeof( uint16_t ) + sizeof( uint16_t ) )		// ������� ����� ����� Size � �������� �������� ������ � ���� - ���� ��������� � �������
#define	ASYNCDATA_RECORDSIZE_CRC	( sizeof( uint16_t ) )							// ������ ���� CRC16

// ������� �� ����������������� ������ MemoryThread_Task(), ������� ��������������� ��������� ������ � ���� ����� ��������� � �������� �������
static void AsyncData_WriteRecordCB( MemoryThreadMessage_t *pMessage )
{
	assert_param( NULL != pMessage );
	assert_param( AsyncData_WriteRecordCB == pMessage->Header.xCallback );
	AsyncDataMessage_t *pAsyncDataMessage = ( AsyncDataMessage_t * ) pMessage;

	// �������� ���������� �� ������������� aMemoryThreadTaskBuffer.
	// ��������������, ��� ���� ����� ���������� ������ �������� �� MemoryThread_Task(), � ��� ���������� ������ ���������������
	assert_param( MemoryThread_BufferMutexTake( ) );

	uint16_t BufferSize = pAsyncDataMessage->AsyncDataHeader.Size + ASYNCDATA_RECORDSIZE_TRIM;					// ��������� ������ ������ ��� ������
	assert_param( BufferSize <= sizeof( aMemoryThreadTaskBuffer ) );
	uint16_t DataSize = BufferSize - sizeof( pAsyncDataMessage->AsyncDataHeader ) - ASYNCDATA_RECORDSIZE_CRC;	// ������ ���� ������ � �����

	// �������� ��������� � �����
	uint16_t BufferPos = 0;
	*( AsyncDataHeader_t * ) ( BufferPos + aMemoryThreadTaskBuffer ) = pAsyncDataMessage->AsyncDataHeader;
	BufferPos += sizeof( pAsyncDataMessage->AsyncDataHeader );

	// �������� ������ � �����
	uint8_t *pData = pAsyncDataMessage->pData;
	if( NULL == pData )
		pData = pAsyncDataMessage->aData;		// ������ ��� ������ ��������� ��������������� � ���������
	memcpy( BufferPos + aMemoryThreadTaskBuffer, pData, DataSize );
	BufferPos += DataSize;

	// �������� CRC � �����
	*( uint16_t * ) ( BufferPos + aMemoryThreadTaskBuffer ) = CalcCRC16SKLP( aMemoryThreadTaskBuffer, BufferPos );
	BufferPos += ASYNCDATA_RECORDSIZE_CRC;

	assert_param( BufferPos == BufferSize );
	// ��������� ������ ������ � ����
	LogFile_WriteRecord( &LogFileHdl_AsyncData, ( char const * ) aMemoryThreadTaskBuffer, BufferPos );
	// ���������� ����� ��� ������������ �������������
	MemoryThread_BufferMutexGive( );
}

// ������������ ��������� � �������� ��� � ������� MemoryThread_Task()

// ��������� ��������� � �������������� ���������
// pAsyncDataMessage	- ��������� �� ���������, � ������� ��������� ������ ���� ������ (pData �/��� aData)
bool AsyncData_WriteRecordExt( AsyncDataTag_t Tag, AsyncDataMessage_t *pMessage, uint16_t DataSize, uint32_t Flags )
{
	assert_param( NULL != pMessage );
	uint8_t *pData = pMessage->pData;
	if( NULL == pData )
		pData = pMessage->aData;
	assert_param( ( ( 0 == DataSize ) && ( NULL == pData ) ) || ( ( 0 != DataSize ) && ( NULL != pData ) ) );
	
	bool Result = false;
	do
	{
		uint16_t RecordSize = sizeof( pMessage->AsyncDataHeader ) + DataSize + ASYNCDATA_RECORDSIZE_CRC;
		if( RecordSize > sizeof( aMemoryThreadTaskBuffer ) )
			break;
		
		pMessage->AsyncDataHeader.Start = 0x0A0D;
		pMessage->AsyncDataHeader.Size = RecordSize - ASYNCDATA_RECORDSIZE_TRIM;
		ATOMIC_WRITE( pMessage->AsyncDataHeader.Time, SKLP_Time );
		pMessage->AsyncDataHeader.Tag = Tag;
		pMessage->Header.xCallback = AsyncData_WriteRecordCB;

		uint32_t AppendMessageFlags = 0;
		if( Flags & ASYNCDATA_FLAGS_WAITEFORCOMPLETE )
			AppendMessageFlags |= MEMORYTHREAD_FLAGS_WAITFORCOMPLETE;
		if( Flags & ASYNCDATA_FLAGS_WAITEFORFS )
			AppendMessageFlags |= MEMORYTHREAD_FLAGS_WAITFORFS;
		Result = MemoryThread_AppendMessage( ( MemoryThreadMessage_t * ) pMessage, AppendMessageFlags );
	} while( 0 );
	return Result;
}

// ����������� � ������� ���������, ������� ����� ���������� � MemoryThread_Taks().
// � ���������� ���������� ������ ����� ��������� � ���� "DataAsyn.dat" � ������������� ���������������.
// Tag			- ��� ������
// pData		- ����� ������, ���������� ����������.
//				���� ����� 0 � ��� ��������� ������, �� ������ ����������� ������������� ����� strlen()
// DataSize		- ������ ������
// Flags		- ASYNCDATA_FLAGS_STATIC == 0
//				������ ����� �������� � ���������, � ����� ������ AsyncData_WriteRecord()
//				������� ������������ ������ ����� ����� ������������ ��������.
//				���������� ������ ������ ��������� ��������� ��������� (������� ����� ����)
//				- ASYNCDATA_FLAGS_STATIC == 1
//				����� ��������� ����� ������� ������ ����� ������.
//				���������� �������������, ��� �� ���������� ���������� ����� ��� ������ �� ����� ��������������.
//				- ASYNCDATA_FLAGS_WAITEFORCOMPLETE	
//				��������� ���������� ���������� ������ � ����
bool AsyncData_WriteRecord( AsyncDataTag_t Tag, void *pData, uint16_t DataSize, uint32_t Flags )
{
	AsyncDataMessage_t Message;

	// ��������� ���� ������ (pData � aData), ��� ��������� ������� AsyncData_WriteRecordExt()
	if( ( 0 == DataSize ) && ( NULL != pData ) &&
		( ( AsyncDataTag_TextPrivate == Tag ) || ( AsyncDataTag_TextPublic == Tag ) ) )
		DataSize = strlen( pData );

	if( Flags & ASYNCDATA_FLAGS_STATIC )
		Message.pData = pData;
	else
	{
		Message.pData = NULL;
		memcpy( Message.aData, pData, DataSize );
	}

	return AsyncData_WriteRecordExt( Tag, &Message, DataSize, Flags );
}


