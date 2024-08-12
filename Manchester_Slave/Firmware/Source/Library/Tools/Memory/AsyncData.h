// Async.h
// ќрганизаци€ записи асинхронных данных в файл
#ifndef	ASYNCDATA_H
#define	ASYNCDATA_H

#include <stdbool.h>
#include <stdint.h>
#include "MemoryThread.h"
#include "SKLP_Time.h"			// SKLP_Time_t

// “ипы тегов
typedef enum AsyncDataTag_enum
{
	AsyncDataTag_TextPrivate	= 0x01,		// текстовая строка для для отладочной информации
	AsyncDataTag_TextPublic		= 0x02,		// текстовая строка для RD5
	AsyncDataTag_MPI_Assembling	= 0x11,		// информация о структуре комплекса
	AsyncDataTag_MPI_Module		= 0x12,		// информация о подключенном модуле
} AsyncDataTag_t;

// «аголовок каждой записи в файл асинхронных данных
#pragma pack( 1 )
typedef struct AsyncDataHeader_struct
{
	uint16_t		Start;		// 0x0A0D
	uint16_t		Size;		// размер всего последующего блока, включая CRC
	SKLP_Time_t		Time;		// время формирования блока
	AsyncDataTag_t	Tag;		// тип блока
	// uint8_t		aData[];	// блок данных призвольного размера
	// uint16_t		CRC16;		// контрольная сумма, начиная с поля Start
} AsyncDataHeader_t;
#pragma pack( )

// —ообщение для задачи MemoryThread_Task(), через которое передается блок для записи в файл
#pragma pack( 1 )
typedef struct AsyncDataMessage_struct
{
	MemoryThreadMessageHeader_t	Header;				// стандартный заголовок сообщения для MemoryThread_Task()
	AsyncDataHeader_t			AsyncDataHeader;	// заголовок записываемого блока
	uint8_t	*pData;									// указатель на записываемые данные
	// «аполнение оставшегося пространства MemoryThreadMessage_t
	// через этот буфер можно передавать данные для записи в файл, если они небольшого размера
	uint8_t	aData[ sizeof( MemoryThreadMessage_t ) - ( sizeof( Header ) + sizeof( AsyncDataHeader ) + sizeof( pData ) ) ];
} AsyncDataMessage_t;
#pragma pack( )

// –ежимы формировани€ сообщени€
#define	ASYNCDATA_FLAGS_STATIC				( 1 << 0 )	// при сброшенном флаге строка копируетс€ в буфер сообщени€, размер буфера ограничен. ѕри установленном флаге в сообщение копируетс€ только адрес буфера, но при этом буфер не должен модифицироватьс€ до завершени€ операции записи.
#define	ASYNCDATA_FLAGS_WAITEFORCOMPLETE	( 1 << 2 )	// ожидать завершени€ записи сообщени€ на носитель - только если файловая система готова к работе
#define	ASYNCDATA_FLAGS_WAITEFORFS			( 1 << 3 )	// при неработоспособности файловой системы, отложить эту запись до восстановления файловой системы

// —формировать сообщение и перредать его в очередь MemoryThread_Task()
bool AsyncData_WriteRecord( AsyncDataTag_t Tag, void *pData, uint16_t DataSize, uint32_t Flags );							// —формировать сообщение на основе тэга и передаваемых данных
bool AsyncData_WriteRecordExt( AsyncDataTag_t Tag, AsyncDataMessage_t *pMessage, uint16_t DataSize, uint32_t Flags );	// «аполнить заголовок в подготовленном сообщении

#endif	// ASYNCDATA_H

