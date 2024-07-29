// SKLP_Memory.h
// ������ � �������� ������ ������ �� ��������� ���
#ifndef	SKLP_MEMORY_H
#define	SKLP_MEMORY_H

#include "SKLP_Service.h"
#include "LogFile.h"

// ������ ������ - ������� ����������� ������ ������, � ������� ������������ ��������
// ���������� ������. ����� ����������� ������ ����� ���� ������� �� ������� [0x41],
// ���������� ������ - �� ������� [0x22].

// ���� ����������� ������ �������
typedef enum SKLP_MemoryRegionType_enum
{
	SKLP_MemoryRegionType_EMPTY = 0,		// ������ �������, �� �� ����������
	SKLP_MemoryRegionType_File,				// ���� � ������� �������� ������� (������ �� uSD)
	SKLP_MemoryRegionType_FileSet, 			// ����� ������ � ������� �������� �������
	SKLP_MemoryRegionType_EEPROM,			// �������� EEPROM �� ���� Flash
	SKLP_MemoryRegionType_Flash,			// ������ ������ �� Flash
	SKLP_MemoryRegionType_DiskImage,		// ������ ������ � ������ �����
	SKLP_MemoryRegionType_TOTAL,			// ���������� ��������� �����
} SKLP_MemoryRegionType_t;

// ������������ �������, ������ ��� ������������� ��� �����������
typedef union SKLP_MemoryRegionAlias_union
{
	char		aTxt[4];					// ��������� ������������� ��� ��������� �������������
	uint32_t	ID;							// �������� ������������� ��� ��������� � ������
} SKLP_MemoryRegionAlias_t;
 
 // �������������� (������) �������� ��-���������
 // � MemInfo.v0 ������������� ������ ��������, �� � �������� MAIN � ZIP
 // ���� ������������� ���� Mark, ������� ����� ����������� ������������ ��� �������� �������.
 // [N] - ���������� ����� ������� � MemInfo.v0
 // [*] - ����� ������� �� �������������� � MemInfo.v0 ��-�� ����������� ������� MemInfo
 // [+] - ������ ����������� � MemInfo.v0.
#define	SKLP_MEMEMORY_REGION_ALIAS_BLACKBOX		( ( SKLP_MemoryRegionAlias_t ) { "BBOX" } )		// [0*]	������ ����
#define	SKLP_MEMEMORY_REGION_ALIAS_TEXTLOG		( ( SKLP_MemoryRegionAlias_t ) { "LOG" } )		// [1*]	��������� ���, ������ TECH �� ���� ��������
#define	SKLP_MEMEMORY_REGION_ALIAS_DATATECH		( ( SKLP_MemoryRegionAlias_t ) { "TECH" } )		// [1*]	��������������� ������ (��� �  ���?)
#define	SKLP_MEMEMORY_REGION_ALIAS_DATAMAIN		( ( SKLP_MemoryRegionAlias_t ) { "MAIN" } )		// [2]	�������� ������
#define	SKLP_MEMEMORY_REGION_ALIAS_DATAZIP		( ( SKLP_MemoryRegionAlias_t ) { "ZIP" } )		// [3]	������������ ������ (��������� �������� ������� ������?)
#define	SKLP_MEMEMORY_REGION_ALIAS_DATAASYNC	( ( SKLP_MemoryRegionAlias_t ) { "ASYN" } )		// [3]	����������� ������, ���, ������ ZIP
#define	SKLP_MEMEMORY_REGION_ALIAS_DATAINTEGRAL	( ( SKLP_MemoryRegionAlias_t ) { "INTG" } )		// [4+]	������������ (������������) ������ �������, ���
#define	SKLP_MEMEMORY_REGION_ALIAS_ACCELOSC		( ( SKLP_MemoryRegionAlias_t ) { "ACCL" } )		// [5+]	������������� ��������������, ���
#define	SKLP_MEMEMORY_REGION_ALIAS_SD_IMAGE		( ( SKLP_MemoryRegionAlias_t ) { "SDIM" } )		// [??+]	����� SD-�����

typedef struct LogFileExt_struct
{
	LogFileHdl_t	*pLogFileHdl;
	char			*pDescription;
	uint32_t		FileMaxSize;
	FIL_Position_t	*pPositionSaved;
} LogFileExt_t;

bool SKLP_Memory_Init( void );
// ���������� ������ �������
bool SKLP_MemoryRegionAppend( SKLP_MemoryRegionAlias_t Alias, SKLP_MemoryRegionType_t Type, void const *pHdl );
// ������������� �������� ������ ��-���������
void SKLP_MemoryRegionsInitDefault( void );
// ������������ (�������-���������) ������������� �������� ������
extern void SKLP_MemoryRegionsInit( void );

// ���������������� ������ � ���-����� (������� �� ������, ����������), � ��������� ����� � ���
void SKLP_Memory_OpenAndReport( LogFileExt_t *pLogFileExt );
void SKLP_Memory_ReportFreeSpace( void );

// �������� ���������� � �������, ����������� � ����������� �������

extern SKLP_ModuleMode_t *pSKLP_ModuleMode;										// ��������� �� ����� ������ ������, ������ ���� ���������� � �������
extern void *SKLP_Memory_TmpBufferGet( uint32_t BufferMinSize );	// ��������� ������� �� ������ � ���������� ������
extern uint32_t SKLP_Memory_TmpBufferGetSize( void );			// ������ ������ ���������� ������
extern void SKLP_Memory_TmpBufferRelease( bool FromISR );		// ������������ ������� � ������

#endif	// SKLP_MEMORY_H


