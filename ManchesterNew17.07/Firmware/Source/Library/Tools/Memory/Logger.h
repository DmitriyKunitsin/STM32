// Logger.h
// ќрганизаци€ логгировани€
#ifndef	LOGGER_H
#define	LOGGER_H
#include <stdbool.h>
#include <stdint.h>
//#include "SKLP_Service.h"
#include "SKLP_Time.h"
#include "MemoryThread.h"

// –ежимы формировани€ сообщени€
#define	LOGGER_FLAGS_STATICTEXT			( 1 << 0 )	// при сброшенном флаге строка копируетс€ в буфер сообщени€, размер буфера ограничен. ѕри установленном флаге в сообщение копируетс€ только адрес буфера, но при этом буфер не должен модифицироватьс€ до завершени€ операции записи.
#define	LOGGER_FLAGS_APPENDTIMESTAMP	( 1 << 1 )	// добавить в сообщение текущий тик FreeRTOS и системное врем€
#define	LOGGER_FLAGS_WAITEFORCOMPLETE	( 1 << 2 )	// ожидать завершени€ записи сообщени€ на носитель - только если файловая система готова к работе
#define	LOGGER_FLAGS_WAITEFORFS			( 1 << 3 )	// при неработоспособности файловой системы, отложить эту запись до восстановления файловой системы

#pragma pack( 1 )
typedef struct LoggerMessage_struct
{
	MemoryThreadMessageHeader_t	Header;
	TickType_t					TimeStampTicks;
	SKLP_Time_t					TimeStampSKLP;
	char const					*pTextExt;
	char						aTextInt[ sizeof( MemoryThreadMessage_t ) - ( sizeof( Header ) + sizeof( TimeStampTicks ) + sizeof( TimeStampSKLP ) + sizeof( pTextExt ) ) ];
} LoggerMessage_t;
#pragma pack( )
// ѕри установленном LOGGER_FLAGS_STATICTEXT, данные под запись берутся из буфера, на который указывает pTextExt (обычно это Flash-память).
// Ѕуфер может быть произвольным, размер данных под запись ограничен '\0' в конце.
// Ќеобходимо гарантировать, что буфер не будет изменен до завершения записи.
// ѕри сброшенном LOGGER_FLAGS_STATICTEXT, устанавливается ( pTextExt := NULL),
// и данные копируются непосредственно в тело сообщения в поле aTextInt[], его размер зависит от MEMORYTHREAD_MESSAGE_SIZE.

bool Logger_WriteRecord( char const *pText, uint32_t Flags );
bool Logger_WriteRecordExt( LoggerMessage_t *pMessage, uint32_t Flags );
//bool Logger_Clear( void );

#endif	// LOGGER_H

