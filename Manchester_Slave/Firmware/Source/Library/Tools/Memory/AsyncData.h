// Async.h
// ����������� ������ ����������� ������ � ����
#ifndef	ASYNCDATA_H
#define	ASYNCDATA_H

#include <stdbool.h>
#include <stdint.h>
#include "MemoryThread.h"
#include "SKLP_Time.h"			// SKLP_Time_t

// ���� �����
typedef enum AsyncDataTag_enum
{
	AsyncDataTag_TextPrivate	= 0x01,		// ��������� ������ ��� ��� ���������� ����������
	AsyncDataTag_TextPublic		= 0x02,		// ��������� ������ ��� RD5
	AsyncDataTag_MPI_Assembling	= 0x11,		// ���������� � ��������� ���������
	AsyncDataTag_MPI_Module		= 0x12,		// ���������� � ������������ ������
} AsyncDataTag_t;

// ��������� ������ ������ � ���� ����������� ������
#pragma pack( 1 )
typedef struct AsyncDataHeader_struct
{
	uint16_t		Start;		// 0x0A0D
	uint16_t		Size;		// ������ ����� ������������ �����, ������� CRC
	SKLP_Time_t		Time;		// ����� ������������ �����
	AsyncDataTag_t	Tag;		// ��� �����
	// uint8_t		aData[];	// ���� ������ ������������ �������
	// uint16_t		CRC16;		// ����������� �����, ������� � ���� Start
} AsyncDataHeader_t;
#pragma pack( )

// ��������� ��� ������ MemoryThread_Task(), ����� ������� ���������� ���� ��� ������ � ����
#pragma pack( 1 )
typedef struct AsyncDataMessage_struct
{
	MemoryThreadMessageHeader_t	Header;				// ����������� ��������� ��������� ��� MemoryThread_Task()
	AsyncDataHeader_t			AsyncDataHeader;	// ��������� ������������� �����
	uint8_t	*pData;									// ��������� �� ������������ ������
	// ���������� ����������� ������������ MemoryThreadMessage_t
	// ����� ���� ����� ����� ���������� ������ ��� ������ � ����, ���� ��� ���������� �������
	uint8_t	aData[ sizeof( MemoryThreadMessage_t ) - ( sizeof( Header ) + sizeof( AsyncDataHeader ) + sizeof( pData ) ) ];
} AsyncDataMessage_t;
#pragma pack( )

// ������ ������������ ���������
#define	ASYNCDATA_FLAGS_STATIC				( 1 << 0 )	// ��� ���������� ����� ������ ���������� � ����� ���������, ������ ������ ���������. ��� ������������� ����� � ��������� ���������� ������ ����� ������, �� ��� ���� ����� �� ������ ���������������� �� ���������� �������� ������.
#define	ASYNCDATA_FLAGS_WAITEFORCOMPLETE	( 1 << 2 )	// ������� ���������� ������ ��������� �� �������� - ������ ���� �������� ������� ������ � ������
#define	ASYNCDATA_FLAGS_WAITEFORFS			( 1 << 3 )	// ��� ������������������� �������� �������, �������� ��� ������ �� �������������� �������� �������

// ������������ ��������� � ��������� ��� � ������� MemoryThread_Task()
bool AsyncData_WriteRecord( AsyncDataTag_t Tag, void *pData, uint16_t DataSize, uint32_t Flags );							// ������������ ��������� �� ������ ���� � ������������ ������
bool AsyncData_WriteRecordExt( AsyncDataTag_t Tag, AsyncDataMessage_t *pMessage, uint16_t DataSize, uint32_t Flags );	// ��������� ��������� � �������������� ���������

#endif	// ASYNCDATA_H

