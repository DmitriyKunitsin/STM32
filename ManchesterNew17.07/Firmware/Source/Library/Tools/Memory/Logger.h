// Logger.h
// ����������� ������������
#ifndef	LOGGER_H
#define	LOGGER_H
#include <stdbool.h>
#include <stdint.h>
//#include "SKLP_Service.h"
#include "SKLP_Time.h"
#include "MemoryThread.h"

// ������ ������������ ���������
#define	LOGGER_FLAGS_STATICTEXT			( 1 << 0 )	// ��� ���������� ����� ������ ���������� � ����� ���������, ������ ������ ���������. ��� ������������� ����� � ��������� ���������� ������ ����� ������, �� ��� ���� ����� �� ������ ���������������� �� ���������� �������� ������.
#define	LOGGER_FLAGS_APPENDTIMESTAMP	( 1 << 1 )	// �������� � ��������� ������� ��� FreeRTOS � ��������� �����
#define	LOGGER_FLAGS_WAITEFORCOMPLETE	( 1 << 2 )	// ������� ���������� ������ ��������� �� �������� - ������ ���� �������� ������� ������ � ������
#define	LOGGER_FLAGS_WAITEFORFS			( 1 << 3 )	// ��� ������������������� �������� �������, �������� ��� ������ �� �������������� �������� �������

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
// ��� ������������� LOGGER_FLAGS_STATICTEXT, ������ ��� ������ ������� �� ������, �� ������� ��������� pTextExt (������ ��� Flash-������).
// ����� ����� ���� ������������, ������ ������ ��� ������ ��������� '\0' � �����.
// ���������� �������������, ��� ����� �� ����� ������� �� ���������� ������.
// ��� ���������� LOGGER_FLAGS_STATICTEXT, ��������������� ( pTextExt := NULL),
// � ������ ���������� ��������������� � ���� ��������� � ���� aTextInt[], ��� ������ ������� �� MEMORYTHREAD_MESSAGE_SIZE.

bool Logger_WriteRecord( char const *pText, uint32_t Flags );
bool Logger_WriteRecordExt( LoggerMessage_t *pMessage, uint32_t Flags );
//bool Logger_Clear( void );

#endif	// LOGGER_H

