// AsyncData.c
// Организация записи асинхронных данных в файл
#include "ProjectConfig.h"		// конфиг платформы, конфиг задачи.
#include "stm32xxxx_hal.h"		// дрова периферии
#include "LogFile.h"
#include "AsyncData.h"			// родной
#include "Utils.h"				// CalcCRC16SKLP()
#include "SKLP_Service.h"		// SKLP_Time
#include <string.h>				// memcpy()

// Файл длЯ сохранениЯ асинхронных данных
static FIL FileDataAsync_FIL;
LogFileHdl_t LogFileHdl_AsyncData = { &FileDataAsync_FIL, "DataAsyn.dat", 0 };

#define	ASYNCDATA_RECORDSIZE_TRIM	( sizeof( uint16_t ) + sizeof( uint16_t ) )		// Разница между полем Size и реальным размером записи в файл - полЯ сигнатуры и размера
#define	ASYNCDATA_RECORDSIZE_CRC	( sizeof( uint16_t ) )							// Размер полЯ CRC16

// Коллбек из низкоприоритетной задачи MemoryThread_Task(), который непосредственно выполнЯет запись в файл через обращение к файловой системе
static void AsyncData_WriteRecordCB( MemoryThreadMessage_t *pMessage )
{
	assert_param( NULL != pMessage );
	assert_param( AsyncData_WriteRecordCB == pMessage->Header.xCallback );
	AsyncDataMessage_t *pAsyncDataMessage = ( AsyncDataMessage_t * ) pMessage;

	// Получить разрешение на использование aMemoryThreadTaskBuffer.
	// ПредполагаетсЯ, что этот буфер используют только коллбеки из MemoryThread_Task(), а они вызываютсЯ строго последовательно
	assert_param( MemoryThread_BufferMutexTake( ) );

	uint16_t BufferSize = pAsyncDataMessage->AsyncDataHeader.Size + ASYNCDATA_RECORDSIZE_TRIM;					// требуемый размер буфера под запись
	assert_param( BufferSize <= sizeof( aMemoryThreadTaskBuffer ) );
	uint16_t DataSize = BufferSize - sizeof( pAsyncDataMessage->AsyncDataHeader ) - ASYNCDATA_RECORDSIZE_CRC;	// размер полЯ данных в блоке

	// Записать заголовок в буфер
	uint16_t BufferPos = 0;
	*( AsyncDataHeader_t * ) ( BufferPos + aMemoryThreadTaskBuffer ) = pAsyncDataMessage->AsyncDataHeader;
	BufferPos += sizeof( pAsyncDataMessage->AsyncDataHeader );

	// Записать данные в буфер
	uint8_t *pData = pAsyncDataMessage->pData;
	if( NULL == pData )
		pData = pAsyncDataMessage->aData;		// данные под запись находЯтсЯ непосредственно в сообщении
	memcpy( BufferPos + aMemoryThreadTaskBuffer, pData, DataSize );
	BufferPos += DataSize;

	// Записать CRC в буфер
	*( uint16_t * ) ( BufferPos + aMemoryThreadTaskBuffer ) = CalcCRC16SKLP( aMemoryThreadTaskBuffer, BufferPos );
	BufferPos += ASYNCDATA_RECORDSIZE_CRC;

	assert_param( BufferPos == BufferSize );
	// Выполнить запись буфера в файл
	LogFile_WriteRecord( &LogFileHdl_AsyncData, ( char const * ) aMemoryThreadTaskBuffer, BufferPos );
	// Освободить буфер длЯ последующего использованиЯ
	MemoryThread_BufferMutexGive( );
}

// Сформировать сообщение и передать его в очередь MemoryThread_Task()

// Заполнить заголовок в подготовленном сообщении
// pAsyncDataMessage	- указатель на сообщение, в котором заполнено только поле данных (pData и/или aData)
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

// Подготовить и послать сообщение, которое будет обработано в MemoryThread_Taks().
// В результате переданные данные будут сохранены в файл "DataAsyn.dat" с установленным форматированием.
// Tag			- тип записи
// pData		- адрес данных, подлежащих сохранению.
//				Если равен 0 и тег текстовых данных, то размер вычислЯетсЯ автоматически через strlen()
// DataSize		- размер данных
// Flags		- ASYNCDATA_FLAGS_STATIC == 0
//				данные будут встроены в сообщение, и после вызова AsyncData_WriteRecord()
//				область передаваемых данных можно будет использовать повторно.
//				Допустимый размер данных ограничен размерами сообщениЯ (порЯдка сотни байт)
//				- ASYNCDATA_FLAGS_STATIC == 1
//				через сообщение будет передан только адрес данных.
//				Необходимо гарантировать, что до завершениЯ сохранениЯ файла эти данные не будут модифицированы.
//				- ASYNCDATA_FLAGS_WAITEFORCOMPLETE	
//				ДождатьсЯ завершениЯ сохранениЯ данных в файл
bool AsyncData_WriteRecord( AsyncDataTag_t Tag, void *pData, uint16_t DataSize, uint32_t Flags )
{
	AsyncDataMessage_t Message;

	// Заполнить полЯ данных (pData и aData), все остальное сделает AsyncData_WriteRecordExt()
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


