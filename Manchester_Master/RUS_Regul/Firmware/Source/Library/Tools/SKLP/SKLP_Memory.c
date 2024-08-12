// SKLP_Memory.c
// ������ � ������ ������ �� ��������� ���
#include "ProjectConfig.h"		// ������ ���������, ������ ������.
#include "stm32xxxx_hal.h"		// ����� ���������
#include "SKLP_Memory.h"		// ������
#include "SKLP_MS_Transport.h"
#include "SKLP_MS_TransportInterface.h"
#include "FileSystem.h"			// �������� �������
#include "MemoryThread.h"
#include "LogFile.h"
#include "Logger.h"
#include "MathUtils.h"			// GetGreatestPowerOf2()
#include <string.h>				// memcpy()
#include <stdio.h>				// memcpy()
#include "common_rcc.h"

/*/ ���������� ���� �������� ������.
// ������ ����������� � �������� SKLP_MEMSECTORSIZE (�� 512 ����)
// ����� ������ ������� ������ ���� ������ SKLP_MEMBLOCKSIZEMAX (32 �������)

// ���������� � �������� �� ���������
#define	CEIL( _VALUE_, _BASE_ )									( ( ( _VALUE_ ) <= 0 ) ? ( 0 ) : ( ( ( ( ( _VALUE_ ) - 1 ) / ( _BASE_ ) ) + 1 ) * ( _BASE_ ) ) )
// ��������� ������ ���������� ������� ������� � ������������� �� ���� �����
#define	REGION_SECTOR_START( _SECTOR_START_ )					CEIL( ( uint32_t ) ( _SECTOR_START_ ), ( uint32_t ) SKLP_MEMBLOCKSIZEMAX )
// ��������� ������ ���������� ������� ������� ��� ������������
#define	REGION_SECTOR_FINISH( _SECTOR_START_, _SIZE_BYTES_ )	( ( _SECTOR_START_ ) + CEIL( ( uint32_t ) ( _SIZE_BYTES_ ), ( uint32_t ) SKLP_MEMSECTORSIZE ) / ( uint32_t ) SKLP_MEMSECTORSIZE - 1 )
*/


// �������� ������ �� �������� ������
typedef struct SKLP_MemoryRegion_struct
{
	SKLP_MemoryRegionAlias_t	Alias;			// ���������� ������������ �������
	SKLP_MemoryRegionType_t		Type;			// ��� ������� � ������, �������������� � ������
	void const					*pHdl;			// ������� ������� ������, ����. ( LogFileHdl_t * ) ��� ������
	// ����, �������������� ��� ������ Update()
	uint32_t					Size;			// ������ �������� ������� � ������
	SKLP_SectorIndex_t			iSectorStart;	// ��������� ������, ���� ����������� ������.
	SKLP_SectorIndex_t			SectorsCount;	// ������ ������� � ��������.
	// ������ ������ ��������������� �������� � ��������� �������.
	// ������ ����� ���� ����������, � ����� ����������� �� �������� ���������������� ����������.
	// ��� ����������� � ����������� ������ ������ ������������� �� SKLP_MEMBLOCKSIZEMAX.
	// ����� ������������� �������������� ������ �� �������� ���������� ��������.
	// ���� ������� ���������� ��� ����������� MemInfo ��� ���������� ������� [0x41].
	// ����� � MemInfo ��������� Alias, ����� �� �������� ������ ����� ���������� ���������� �������.
} SKLP_MemoryRegion_t;

/*/ ��������� �������� �������� ��-���������, � ������
#define	FILE_MAXSIZE_VARIABLE		( ( uint32_t ) 0 )						// ������ ����������� �������, ������� �� ������� �����
#ifndef	FILE_MAXSIZE_BLACKBOX
#define	FILE_MAXSIZE_BLACKBOX		( ( uint32_t ) ( 16 * 1024 ) )
#endif	// FILE_MAXSIZE_BLACKBOX
#ifndef	FILE_MAXSIZE_TECHLOG
#define	FILE_MAXSIZE_TECHLOG		( ( uint32_t ) ( 256 * 1024 ) )
#endif	// FILE_MAXSIZE_TECHLOG
#ifndef	FILE_MAXSIZE_MAINDATA
//#define	FILE_MAXSIZE_MAINDATA		( ( uint32_t ) ( 128 * 1024 * 1024 ) )
#define	FILE_MAXSIZE_MAINDATA		FILE_MAXSIZE_VARIABLE
#endif	// FILE_MAXSIZE_MAINDATA
*/

// ���������� ������� ������� ��������. ����������� ��� ������� ���������.
#ifndef	SKLP_MEMEMORY_REGION_COUNT_MAX
#define	SKLP_MEMEMORY_REGION_COUNT_MAX		7		// ���������� �������� ��-���������
#endif	// SKLP_MEMEMORY_REGION_COUNT_MAX

#define	SKLP_MEMINFO_FORMAT_V0				0
#define	SKLP_MEMINFO_FORMAT_V1				1

#ifndef	SKLP_MEMINFO_FORMAT
#define	SKLP_MEMINFO_FORMAT					SKLP_MEMINFO_FORMAT_V1
#endif	// SKLP_MEMINFO_FORMAT

/*extern LogFileHdl_t LogFileHdl_BlackBox;
extern LogFileHdl_t LogFileHdl_TechLog;
static const SKLP_MemoryRegion_t aSKLP_MemoryRegionsDefault[ ] =
{
	{ SKLP_MEMEMORY_REGION_ALIAS_BLACKBOX,	SKLP_MemoryRegionType_File,	&LogFileHdl_BlackBox,	FILE_MAXSIZE_BLACKBOX / SKLP_MEMSECTORSIZE },
	{ SKLP_MEMEMORY_REGION_ALIAS_DATATECH,	SKLP_MemoryRegionType_File, &LogFileHdl_TechLog,	FILE_MAXSIZE_BLACKBOX / SKLP_MEMSECTORSIZE },
	{ SKLP_MEMEMORY_REGION_ALIAS_DATAMAIN,	SKLP_MemoryRegionType_File, &LogFileHdl_BlackBox,	FILE_MAXSIZE_BLACKBOX / SKLP_MEMSECTORSIZE },
};*/

static SKLP_MemoryRegion_t aSKLP_MemoryRegions[ SKLP_MEMEMORY_REGION_COUNT_MAX ] = { 0 };
static bool bSKLP_MemoryRegionsUpdated = false;
static uint8_t SKLP_MemInfoFormat = 0;

// ���������� ������ �������
// ����� ������������ �������� ������� ��� �������������
bool SKLP_MemoryRegionAppend( SKLP_MemoryRegionAlias_t Alias, SKLP_MemoryRegionType_t Type, void const *pHdl )
{
	assert_param( 0 != Alias.ID );
	assert_param( ( Type > SKLP_MemoryRegionType_EMPTY ) && ( Type < SKLP_MemoryRegionType_TOTAL ) );
	assert_param( NULL != pHdl );
	bool bRegionUpdated = false;
	for( int i = 0; i < SIZEOFARRAY( aSKLP_MemoryRegions ); i++ )
	{
		SKLP_MemoryRegion_t *pRegion = &aSKLP_MemoryRegions[i];
		if( !bRegionUpdated && ( ( Alias.ID == pRegion->Alias.ID ) || ( SKLP_MemoryRegionType_EMPTY == pRegion->Type ) ) )
		{	// ������ ������ ��� ���� �� ������
			pRegion->Alias			= Alias;
			pRegion->Type			= Type;
			pRegion->pHdl			= pHdl;
			// ��� ���� ������ ���� ������������������� � ��������� Update()
			pRegion->Size			= 0;
			pRegion->iSectorStart	= 0;
			pRegion->SectorsCount	= 0;
			bSKLP_MemoryRegionsUpdated = false;		// ���� �������� ��������� ��������, ������ ���������� ������� Update() ��� ��������������
			bRegionUpdated = true;
			continue;
		}
		if( bRegionUpdated && ( Alias.ID == pRegion->Alias.ID ) )
		{	// ������ ������ � ������ ���������������, �� ��� ����� - ��������
			*pRegion = ( SKLP_MemoryRegion_t ) { 0 };
			continue;
		}
	}
	return bRegionUpdated;
}

// �������� ���������� �� �������� (������������ ������� �� ������� ����������� ������)
static bool SKLP_MemoryRegionsUpdate( void )
{
	bSKLP_MemoryRegionsUpdated = false;
	SKLP_MemInfoFormat = SKLP_MEMINFO_FORMAT_V0;
	SKLP_SectorIndex_t iSectorStart = SKLP_MEMBLOCKSIZEMAX;		// �� �������� ������������� � �������� �������. 0 - ������� ���������/������ �������
	for( int i = 0; i < SIZEOFARRAY( aSKLP_MemoryRegions ); i++ )
	{
		SKLP_MemoryRegion_t *pRegion = &aSKLP_MemoryRegions[i];
		if( ( 0 == pRegion->Alias.ID ) || ( NULL == pRegion->pHdl ) ||
			( pRegion->Type <= SKLP_MemoryRegionType_EMPTY ) || ( pRegion->Type >= SKLP_MemoryRegionType_TOTAL ) )
		{	// � ���������� �������� ���-�� �� ���. �������� ��� ������
			*pRegion = ( SKLP_MemoryRegion_t ) { 0 };
			continue;
		}
		SKLP_SectorIndex_t SectorsCount = 0;
		uint32_t RegionSize = 0;
		switch( pRegion->Type )
		{
			case SKLP_MemoryRegionType_File:
			{
				LogFileHdl_t *pFileHdl = ( LogFileHdl_t * ) pRegion->pHdl;
				assert_param( FR_OK == LogFile_ValidateHdl( pFileHdl ) );
				if( FR_OK != FileSystem_CheckFSReady( ) )
					break;
				// �������� ������ �����
				uint32_t FileSize;
				if( FR_OK == FileSystem_CheckFileOpen( pFileHdl->pFile ) )
					// ���� ������, ��� ������ ��������
					FileSize = pFileHdl->pFile->fsize;
				else
				{	// ���� ������, �������� ��� ������, �� ��������� ��������
					FILINFO FileInfo;
					if( FR_OK != LogFile_GetFileInfo( pFileHdl->pFileName, &FileInfo ) )
						break;
					FileSize = FileInfo.fsize;
					// LogFile_GetFileInfo() ���������� ������� � MemoryThread_Task(), ������� ���������� � SD ����� FatFS, �������� ����� ��������������!
					// ����������������, �������� ������ ����� FileSystem_GetFileInfo() �������, ��� ����� f_open().
				}
				FileSize		+= sizeof( SKLP_SIGNATURE_SAVING_STOP ) - 1;
				RegionSize		= FileSize;
				SectorsCount	= ( FileSize + SKLP_MEMSECTORSIZE ) / SKLP_MEMSECTORSIZE;
				break;
			}
			case SKLP_MemoryRegionType_DiskImage:
			{
				Diskio_drvTypeDef *pDiskIODriver = ( Diskio_drvTypeDef * ) pRegion->pHdl;
				// ��������� ������ ����� � ��������
				uint32_t DiskSizeSectors;
				DRESULT DiskResult = DiskIODriver_GetSectorsCount( pDiskIODriver, &DiskSizeSectors );
				if( RES_OK != DiskResult )
					break;
				SectorsCount = DiskSizeSectors;
				break;
			}
			default:
				assert_param( 0 );
		}
		assert_param( 0 == ( iSectorStart % SKLP_MEMBLOCKSIZEMAX ) );
		pRegion->Size			= RegionSize;
		pRegion->iSectorStart	= iSectorStart;
		pRegion->SectorsCount	= SectorsCount;
		iSectorStart += ( ( SectorsCount + SKLP_MEMBLOCKSIZEMAX ) / SKLP_MEMBLOCKSIZEMAX ) * SKLP_MEMBLOCKSIZEMAX;
		
		// ���� ��������� ������, �������� �� ������� "�����������" -
		// ������������ ����� ��� ��������� MemInfo, ����������� ������������ ���� ��������.
		if( ( SKLP_MEMEMORY_REGION_ALIAS_BLACKBOX.ID != pRegion->Alias.ID ) &&
			( SKLP_MEMEMORY_REGION_ALIAS_DATATECH.ID != pRegion->Alias.ID ) &&
			( SKLP_MEMEMORY_REGION_ALIAS_DATAMAIN.ID != pRegion->Alias.ID ) &&
			( SKLP_MEMEMORY_REGION_ALIAS_DATAASYNC.ID != pRegion->Alias.ID ) )
			SKLP_MemInfoFormat = SKLP_MEMINFO_FORMAT_V1;
	}
	bSKLP_MemoryRegionsUpdated = true;
	return bSKLP_MemoryRegionsUpdated;
}

// ������������� �������� ��-���������
void SKLP_MemoryRegionsInitDefault( void )
{
	// 0 ������ ����
	extern LogFileHdl_t LogFileHdl_BlackBox;
	assert_param( SKLP_MemoryRegionAppend( SKLP_MEMEMORY_REGION_ALIAS_BLACKBOX, SKLP_MemoryRegionType_File,	&LogFileHdl_BlackBox ) );

	// 1 ��������������� ���
	extern LogFileHdl_t LogFileHdl_TechLog;
	assert_param( SKLP_MemoryRegionAppend( SKLP_MEMEMORY_REGION_ALIAS_DATATECH, SKLP_MemoryRegionType_File,	&LogFileHdl_TechLog ) );

	// 2 �������� ������
	extern LogFileHdl_t LogFileHdl_MainData;
	assert_param( SKLP_MemoryRegionAppend( SKLP_MEMEMORY_REGION_ALIAS_DATAMAIN, SKLP_MemoryRegionType_File,	&LogFileHdl_MainData ) );

	// 3 ������ ������
	// �� ����������������, �������� ������

	// !!! ���� - ����� SD-�����, ����������� �� ������� ����!
//	assert_param( SKLP_MemoryRegionAppend( SKLP_MEMEMORY_REGION_ALIAS_SD_IMAGE, SKLP_MemoryRegionType_DiskImage,	&SD_Driver ) );
	// !!! ���������� ���� ������� �� ��� _�����_ ������� ������������ ������ � ��������������
	// !!! ������� � ������ ������ �� RD5 (��-��������, ����� ���), ������� ���� ���������.
}

__weak void SKLP_MemoryRegionsInit( void )
{
	SKLP_MemoryRegionsInitDefault( );
}

// ��������� �� ����� ������ ������, ������ ���� ���������� � �������
SKLP_ModuleMode_t *pSKLP_ModuleMode = NULL;

SKLP_CommandResult_t SKLP_ProcessCommand_MemoryInfoGet( uint8_t *pQuery, uint8_t **ppAnswer );
SKLP_CommandResult_t SKLP_ProcessCommand_MemoryRead( uint8_t *pQuery, uint8_t **ppAnswer );
SKLP_CommandResult_t SKLP_ProcessCommand_MemoryErase( uint8_t *pQuery, uint8_t **ppAnswer );

// �� ������� ���������, ������ ������ ������������ ������ ������������,
// � ������� ����������� 4 ����������� �������:
// - �������� ������
// - ��������������� ������
// - ������ ����
// - �������������� ������
// ��������������, ��� ��� ������� ����������� � ������ ������ ����������� ��������,
// ������ � �������� ����������� ������� (���������������) � ������� �� ������������.
// ��� ���������� ������ � ������ ������ ���������� ������� [0x41],
// ����� ������� ������������ ��������� ������ ������ - �� ����� ������� ���������� ������ �� ��������.
// ����� ������ ���������� ������� [0x22] ��� ����������� ������������ ��� �������,
// ��� ���� ��������� ��������� ������ ������������ ������� � ���������� ��������.
//
// ������������� � ���������������� ������, ������ ���������� ������ SD-����� ���������� ������������,
// �.�. �� SD ����������� �������� �������.
// ����� ����, ��������� ������ (����. ������ ����) ������� ����������� �� �� SD, � � uC.Flash, �.�. ������ �� ������ ��������.
// �������������� ��������� ������������ ������ � ���� �������� �������� ������ � ������ ���������
// � ������ ������������ ����������� ������, ��������������� ��� ���������� �� ��������� ���.
//
// �� ������� [0x41] ���������� �������������� ��� ���������� ����������� �����/�������,
// � ������������ ���������� � ���, ��� � �������� ������ ������ - ��������� ������ � ���������� ��������.
// ��������� ������ ������� ������� ������ ���� ���������� �� ����� ����������.
// ���������� �������� �������� ������ ���� ����������� �� ������ ����������� ������� ������������� �����,
// ���� ����� �������� �������������.
//
// �� ������� [0x22] ���������� ����������, � ����� ������ ������� ���������� ���������,
// � ����� ����������� ������ �� ��������� �������, ���������������� ���������, ������������� ��� �������.
// ��� ���������� ���������� �� �����, ������������ ��������� ���� ����� ���������� ����������,
// �.�. ��� ��������� ��������� �������� ����� ��������� � ����������� ����, ��� ����� ������ ������������ �����.
// �������������� ��������� ���� ��������, � ��� ��������� ��������� �� ��������� ���������, ��� ����������� �������
// ���������� ��������� � ����� �� �����. ��� ��������� � ����� �������, ���������� ������� ������� ���� � ������� �����.
//
// ������ ������, ����������� ��� ���������� ������� [0x22]:
// - ��������� � �������������� �������
// - ����� �� ������� �������
// - ����� �� ������� �����
// �������������� ������ �������: ������������ ���������� ����� � ������� ������������ �������.
// ��� �������, ������� ���������� ���������� � ����� ������ ���� ��������� ������.
//
// ������ �������� � ���� ������� - ���������� � ������������.
// � ���������� ������ ���������� ���������� ���������� ������ � ����� �������� � ����������� ������,
// �������� ������� � ������ ����. ��� ����� ��������� ������� �� ������, �� ��� ���� ����������� �� ����������.
// � ������������ ������ ������ � ����� �� ������������.
// ��� �������� � ���������� �����, ���������� ��������� ������� ����������� ����, ����� �������� ��������� ����� ��� �������.

// ������������� ������� � ������ ������
bool SKLP_Memory_Init( void )
{
	assert_param( taskSCHEDULER_NOT_STARTED == xTaskGetSchedulerState( ) );
	SKLP_MemoryRegionsInit( );

	// ���������� ��������� ��������� ����� ������� ������ � ������
	SKLP_MemoryFlags.SD0_DataPresent = 0;		// ���������� ������ ���. ���� ������ ���� ���������� ��� ������������� �������� ������ �������, ������� ���������/��������� ���� �������� ������

	// ���������������� ����������� �������� �� ������� ��� �� ��������� � ������
	SKLP_ServiceSetCallback( SKLP_COMMAND_MEMORY_ERASE, 	SKLP_ProcessCommand_MemoryErase );
	SKLP_ServiceSetCallback( SKLP_COMMAND_MEMORY_READ, 		SKLP_ProcessCommand_MemoryRead );
	SKLP_ServiceSetCallback( SKLP_COMMAND_MEMORY_INFO_GET, 	SKLP_ProcessCommand_MemoryInfoGet );

	return true;
}

// [0x41] ���������� ������� ��������� ��� ������ ��������� ������
SKLP_CommandResult_t SKLP_ProcessCommand_MemoryInfoGet( uint8_t *pQuery, uint8_t **ppAnswer )
{
	SKLP_CommandResult_t ReturnValue = SKLP_COMMAND_RESULT_ERROR;
	do
	{
		if( SKLP_ADDRESS_BROADCAST == pQuery[SKLP_OFFSET_ADDRESS] )
			break;		// ������� �� ������ ���� �����������������
		if( 0 != SKLP_SIZE_DATA_QUERY( pQuery ) )
			break;		// ������� �� ������ ��������� ����������

		*ppAnswer = pQuery;		// ������� ����� ����� ����� �������
		SKLP_MemInfoAnswer_t *pAnswerBody = ( SKLP_MemInfoAnswer_t * ) ( *ppAnswer + SKLP_OFFSET_DATA_ANSWER );
		if( sizeof( *pAnswerBody ) > SKLP_SIZE_DATA_MAX_ANSWER )
			break;		// ������ ������ ��������� �����
		*pAnswerBody = ( SKLP_MemInfoAnswer_t ) { 0 };		// ��������� ���� ����� ������. ����� ����� ����� ����� ��������� ��������������� ����������.
		ReturnValue = sizeof( *pAnswerBody );				// ���� ���� ��� ����������� ���������� ��������� ������, ��� ����� ������� ��������� ���������.

		// �������� ���������� ���������� �� ��������
		if( !SKLP_MemoryRegionsUpdate( ) )
			break;		// �� ������� �������� ����������. ������� ������ MemInfo.
		// ��������� ���� ������ �� ���������� ����������
		ATOMIC_WRITE( pAnswerBody->MemoryFlags, SKLP_MemoryFlags );

		// ��������� ���������
		pAnswerBody->Info.Format		= SKLP_MEMINFO_FORMAT_V0;
		pAnswerBody->Info.MemSize		= 0;	// ����� ���������� ������ SD. ������ ��� ����������� ����� ������, � SD ������� ��������� ���. ���� ������ �� ����.
		pAnswerBody->Info.SectorSize	= SKLP_MEMSECTORSIZE;
		pAnswerBody->Info.DiskCount 	= 1;
		pAnswerBody->Info.Flags[0]		= pAnswerBody->MemoryFlags;
		// ���������� ��������������� ������ ������ ��� ������ ������ ��������� [0x22]
		uint32_t BufferSizeMax_Sectors = SKLP_Memory_TmpBufferGetSize( ) / SKLP_MEMSECTORSIZE;		// ������ ������ � ��������
		pAnswerBody->Info.v1_Aux.AuxReadBufferSize = GetGreatestPowerOf2( BufferSizeMax_Sectors );	// ���������� ������ ������ ��� ������� ������, � ����� ����������� ������� � �������� �������
		assert_param( 0 != pAnswerBody->Info.v1_Aux.AuxReadBufferSize );
		
		// �������� �� ���� ���������� ��������
		uint8_t AuxDataStructCount = 0;
		for( int i = 0; i < SIZEOFARRAY( aSKLP_MemoryRegions ); i++ )
		{
			SKLP_MemoryRegion_t *pRegion = &aSKLP_MemoryRegions[i];
			if( SKLP_MemoryRegionType_EMPTY == pRegion->Type )
				continue;
			if( 0 == pRegion->SectorsCount)
				continue;
			SKLP_SectorIndex_t	iSectorFirst	= pRegion->iSectorStart;
			SKLP_SectorIndex_t	iSectorLast		= pRegion->iSectorStart + pRegion->SectorsCount - 1;
			// ������ ���������� ����� � ��������� ������� (������ ��� ������� � ������� �����)
			SKLP_ByteIndex_t	iLastByte		= pRegion->Size % SKLP_MEMSECTORSIZE;
			//	����������, � ����� ������� ������� ���������� � �������
			if( SKLP_MEMEMORY_REGION_ALIAS_BLACKBOX.ID == pRegion->Alias.ID )
			{
				pAnswerBody->Info.StructBlackBox.iSectorFirst	= iSectorFirst;
				pAnswerBody->Info.StructBlackBox.iSectorLast	= iSectorLast;
				pAnswerBody->Info.StructBlackBox.iLastByte		= iLastByte;
			}
			else if( SKLP_MEMEMORY_REGION_ALIAS_DATATECH.ID == pRegion->Alias.ID )
			{
				pAnswerBody->Info.StructTechData.iSectorFirst	= iSectorFirst;
				pAnswerBody->Info.StructTechData.iSectorLast	= iSectorLast;
				pAnswerBody->Info.StructTechData.iLastByte		= iLastByte;
			}
			else if( SKLP_MEMEMORY_REGION_ALIAS_DATAMAIN.ID == pRegion->Alias.ID )
			{
				pAnswerBody->Info.StructData.iSectorFirst		= iSectorFirst;
				pAnswerBody->Info.StructData.iSectorLast		= iSectorLast;
				pAnswerBody->Info.StructData.v1_Alias			= pRegion->Alias.ID;
			}
			else if( SKLP_MEMEMORY_REGION_ALIAS_DATAASYNC.ID == pRegion->Alias.ID )
			{
				pAnswerBody->Info.StructDataAsync.iSectorFirst	= iSectorFirst;
				pAnswerBody->Info.StructDataAsync.iSectorLast	= iSectorLast;
				pAnswerBody->Info.StructDataAsync.v1_Alias		= pRegion->Alias.ID;
			}
			else if( SKLP_MEMINFO_FORMAT_V1 == SKLP_MemInfoFormat )
			{	// ���������� ���� � ����� ���������������, � ����������� ������������ MemInfo_v1
				// ���������� ����� ��� ����� ��������� ������
				SKLP_MemInfo_StructData_t *pAuxDataStruct = ( SKLP_MemInfo_StructData_t * ) ( &pAnswerBody->Info.v1_Aux + 1 );
				pAuxDataStruct += AuxDataStructCount;
				// ���������, ��� ����������� ��������� ������ � ����� ������
				if( ( sizeof( *pAnswerBody ) + sizeof( *pAuxDataStruct ) * ( AuxDataStructCount + 1 ) ) <= SKLP_SIZE_DATA_MAX_ANSWER )
				{	// �������� ����� ��������� � �����
					pAuxDataStruct->iSectorFirst	= iSectorFirst;
					pAuxDataStruct->iSectorLast		= iSectorLast;
					pAuxDataStruct->v1_Alias		= pRegion->Alias.ID;
					AuxDataStructCount++;
				}
			}
		}
		if( AuxDataStructCount > 0 )
		{	// ���� ��������� ����� ������ � MemInfo, ������ ��� MemInfo_v1
			assert_param( SKLP_MEMINFO_FORMAT_V1 == SKLP_MemInfoFormat );
			pAnswerBody->Info.Format = SKLP_MEMINFO_FORMAT_V1;
			// ��������� ������ ����������� ������ � �������� ��� � ������� ��������������� ������
			int AuxDataSize = sizeof( SKLP_MemInfo_StructData_t ) * ( AuxDataStructCount );
			ReturnValue += AuxDataSize;
			assert_param( ReturnValue <= SKLP_SIZE_DATA_MAX_ANSWER );
			pAnswerBody->Info.v1_Aux.AuxDataStructCount = AuxDataStructCount;
			// ��������� "CRC8"
			*( ( ( uint8_t * ) &pAnswerBody->Info.CRC8 ) + AuxDataSize ) = 0;
		}
	} while( 0 );

	return ReturnValue;
}

void SKLP_ProcessCommand_MemoryRead_ReleaseBuffer( SKLP_Interface_t *pInterface )
{
	( void ) pInterface;
	SKLP_Memory_TmpBufferRelease( false );
}

// [0x22] ���������� ������� ��������� ��� ������ ������
// !! ������ ������ ������: ���� ������� ������� ���������� ������ ���� ��������� ������ (��� �� 3 ������ ������� �������),
// !! �� SKLP_ProcessCommand_MemoryRead() ���������� ������ ������.
// !! ����� �������, ��� ������ SKLP_ProcessCommand_MemoryRead() �� SKLP_ProcessPacket(), ��������� �������������� ���������!
SKLP_CommandResult_t SKLP_ProcessCommand_MemoryRead( uint8_t *pQuery, uint8_t **ppAnswer )
{
	assert_param( ( NULL != pQuery ) && ( NULL != ppAnswer ) );
	
	SKLP_CommandResult_t ReturnValue = SKLP_COMMAND_RESULT_ERROR;
	SKLP_Interface_t *pInterface = ( SKLP_Interface_t * ) *ppAnswer;
	SKLP_MemReadQuery_t *pQueryBody = ( SKLP_MemReadQuery_t * ) ( pQuery + SKLP_OFFSET_DATA_QUERY );
	SKLP_MemReadAnswerFrame_t *pAnswerFrame;
	do
	{
		if( !bSKLP_MemoryRegionsUpdated )
			break;		// ��������� �������� ������ ������ ���� �������������� ���������������� ��� ������ MemInfo
		if( SKLP_ADDRESS_BROADCAST == pQuery[SKLP_OFFSET_ADDRESS] )
			break;		// ������� �� ������ ���� �����������������
		if( sizeof( *pQueryBody ) != SKLP_SIZE_DATA_QUERY( pQuery ) )
			break;		// �� ������������� ������ ���� ������ � �������
		if( 0 != pQueryBody->DiskNum )
			break;		// ������������ ����� �������������� ���������� ������
		if( 0 == pQueryBody->SectorsCount )
			break;		// ������������ ���������� ������������� ��������
		uint32_t DataSize = ( ( uint32_t ) ( pQueryBody->SectorsCount ) ) * SKLP_MEMSECTORSIZE;			// ������ ������������� ������
		uint32_t AnswerSize = sizeof( *pAnswerFrame ) - sizeof( pAnswerFrame->aBuffer ) + DataSize;		// �������� ������ ��������� ������ ������� �� �������������� ������ ������
		if( sizeof( *pAnswerFrame ) < AnswerSize )
			break;		// ����������� ������ �� ������� � ������ ����������� ������!

		// �������� ������ � ���������� ������ ��� ������ � �������� ������
		pAnswerFrame = ( SKLP_MemReadAnswerFrame_t * ) SKLP_Memory_TmpBufferGet( AnswerSize );
		if( NULL == pAnswerFrame )
			break;		// �� ������� �������� ������ � ������ ������������ �������, ������� �� ����� ���� ���������
		// �������� ������� �� ������������ ������ ����� ���������� �������� ������ � ���������������� ���������.
		SKLP_SetTxCompleteCB( pInterface, SKLP_ProcessCommand_MemoryRead_ReleaseBuffer );
		// ��� ������������� ���������������� ������, ����� �� ������� �� ����� ���������

		// ����������, � ������ ������� ����������� ������ ������������ ���������
		for( int i = 0; i < SIZEOFARRAY( aSKLP_MemoryRegions ); i++ )
		{
			SKLP_MemoryRegion_t *pRegion = &aSKLP_MemoryRegions[i];
			if( SKLP_MemoryRegionType_EMPTY == pRegion->Type )
				continue;
			// ���������, � ����� �� ������� ���������� ���������
			if( ( pQueryBody->iSectorStart < pRegion->iSectorStart ) ||	
				( pQueryBody->iSectorStart >= ( pRegion->iSectorStart + pRegion->SectorsCount ) ) )
				continue;
			
			// ��������� ��������������� ������.
			// ���� ���� ��� ������� ����������� ������ ��������� ������,
			// ��������� ���� �� ���������� �������� � ������� ���������� �����.
			i = SIZEOFARRAY( aSKLP_MemoryRegions );		// ������� ��� ���������� ����� �� ��������

			*ppAnswer = ( uint8_t * ) pAnswerFrame;
			ReturnValue = AnswerSize;	// � ������� �� �������� �� ������� �������, ����� ������������ ������ ������ ������������ ������

			// ��������� ���� ��������� ������.
			pAnswerFrame->SectorsCount	= pQueryBody->SectorsCount;
			pAnswerFrame->MemoryFlags	= ( SKLP_MemoryFlags_t ) { 0 };		// �������� ��� ����� ������. ����� ���������� �� ���������� ������� ������ ������.
			memset( pAnswerFrame->aBuffer, 0, DataSize );					// ������� ��������� ���� ����� ������, � ����� ���������� �������� ������, ���� �����
			
			// ���������� ���������� ������ � ������������ � ��������� ���������������� �������
			switch( pRegion->Type )
			{
			case SKLP_MemoryRegionType_File:	// � ������ ����������� ���� � uSD
				{
					LogFileHdl_t *pFileHdl = ( LogFileHdl_t * ) pRegion->pHdl;
					assert_param( FR_OK == LogFile_ValidateHdl( pFileHdl ) );
					static FIL_Position_t FilePositionSaved = { 0 };
					// ���������� ��������� ������� � �����, � ������� ����������� ������
					uint32_t FilePos = ( pQueryBody->iSectorStart - pRegion->iSectorStart ) * SKLP_MEMSECTORSIZE;
					if( FilePos == FilePositionSaved.fptr )
					{	// ����������, ��� ��������� ������� ��������� � �������� ��� ���������� �������.
						// ������, ��� ������ �� ��������� ������ �����.
						// LogFile_Read() ��������� ���������� �� ��������� ������� ��� ������ f_lseek(),
						// �� ��� ������� ������ ��� ����� ������ ������������ �����.
						// ����� ����������� ��������� ������������ ������� ����� ��� ���������� �������.
						FileSystem_FilePositionRestore( pFileHdl->pFile, &FilePositionSaved );
						// ���� ��������� ������������ � ������� �����, �� ������� �� ����� �������������.
						// ����� �� ��������� - ���� ������� ������������ �� �������, ���������� f_lseek() � LogFile_Read()
					}
					// ��������� ������� ������� ����� ����� ��������� ������
					FileSystem_FilePositionSave( pFileHdl->pFile, &FilePositionSaved );
					// ���������� ������
					uint32_t BytesReaded = 0;
					FRESULT FResult = LogFile_Read( pFileHdl, FilePos, pAnswerFrame->aBuffer, DataSize, &BytesReaded );
					bool bEOF = ( pFileHdl->pFile->fptr == pFileHdl->pFile->fsize );
					if( ( FR_OK == FResult ) && bEOF )
					{	// ��������� ����� �����. ���������� "STOP" ����� ����� ����� - ����!
						uint32_t BytesToAppend = sizeof( SKLP_SIGNATURE_SAVING_STOP ) - 1;
						if( BytesToAppend > ( DataSize - BytesReaded ) )
							BytesToAppend = DataSize - BytesReaded;
						memcpy( ( char * ) ( pAnswerFrame->aBuffer ) + BytesReaded, SKLP_SIGNATURE_SAVING_STOP, BytesToAppend );
						// !!! ��� ���������� ��������� STOP �� ������� �����?
						break;
					}

					// !! ����� !! LogFile_Read() ����������� ���, ��� � ������ ��������� ������ ����� ����� ������,
					// !! ���� ���� ����� ������� ����������.
					// !! ��� ����, ���� ����� �������� �� ����� ���������, �� ��������� ��� ����� ����� FR_OK,
					// !! �� ���������� ��������� ���� ����� ������ ������������.
					// !! � �� �� �����, ���� ��� ����� ����� ��������� �� ����� ������������ �����.
					// !! ��� ��������� ����������� ���������� �� ��������� ��������� ������� ���� fread().
					if( ( FR_OK != FResult ) || ( ( DataSize > BytesReaded ) && !bEOF ) )
					{	// �������� ������ � ���
						static char aMsgBuffer[150];
						char * const pBuff = aMsgBuffer;
						uint32_t const BuffSize = sizeof( aMsgBuffer );
						// !! ������ ������������ aMemoryThreadTaskBuffer[], ��� ��� �� �������� � ������ MemoryThread_Task()
						uint32_t MessageSize = 0;
						assert_param( MemoryThread_SprintfMutexTake( 5 ) );
						MessageSize += snprintf( pBuff + MessageSize, BuffSize - MessageSize, "File error #%d while reading %s.", FResult, pFileHdl->pFileName );
						MessageSize += snprintf( pBuff + MessageSize, BuffSize - MessageSize, " Queried %u bytes from %u, readed %u bytes.", DataSize, FilePos, BytesReaded );
						static uint8_t ReadErrorCount = 0;
						const uint8_t ReadErrorCountMax = 100;
						if( ReadErrorCountMax == ++ReadErrorCount )
							MessageSize += snprintf( pBuff + MessageSize, BuffSize - MessageSize, " Stop logging this error." );
						MemoryThread_SprintfMutexGive( );
						if( ReadErrorCount <= ReadErrorCountMax )
							assert_param( Logger_WriteRecord( pBuff, LOGGER_FLAGS_APPENDTIMESTAMP /*| LOGGER_FLAGS_WAITEFORFS */) );
					}
				}
				break;

			case SKLP_MemoryRegionType_DiskImage:	// � ������ ����������� ����� �����
				{
					Diskio_drvTypeDef *pDiskioDriver = ( Diskio_drvTypeDef * ) pRegion->pHdl;
					// ���������� ������ ����������� ��������
					uint32_t DiskSectorStart = pQueryBody->iSectorStart - pRegion->iSectorStart;
					uint32_t DeskSectorsCount = pQueryBody->SectorsCount;
					DRESULT DiskResult = DiskIODriver_ReadSectors( pDiskioDriver, ( uint8_t * ) pAnswerFrame->aBuffer, DiskSectorStart, DeskSectorsCount );
					if( RES_OK != DiskResult )
						break;
				}
				break;
				
			default:
				break;
			}
		}
	} while( 0 );

	// ��������� ��������� ������ ������
	if( ReturnValue >= SKLP_COMMAND_RESULT_RETURN )
	{	// ��� ����������� ����� ��� �������� �� RS-485 (��������, ���������� ������)
		// ��������� ���� ������ �� ���������� ����������
		ATOMIC_WRITE( pAnswerFrame->MemoryFlags, SKLP_MemoryFlags );
		// ������ � ������ ����� ���������� ��� ��������� ������ ����� ����������� � �������� ����������� - ����� ���������� �������� ������
	}

	return ReturnValue;
}

// [0x20] ������� �� ������� �������� ������ SKLP_ProcessCommand_MemoryErase()
// ��� �������� ���� � ��� - ����������� �������� �������, ����� ���������� ������ � ���������� ����� ��� �������� ������
__weak void SKLP_ProcessCommand_MemoryEraseCB( MemoryThreadMessage_t *pMessage )
{
}

// [0x20] ������� �� ������� �������� ������ SKLP_ProcessCommand_MemoryErase()
// ���������� �������� �������� ������, ��� �� �������
static void SKLP_Memory_EraseCB( MemoryThreadMessage_t *pMessage )
{
	assert_param( SKLP_Memory_EraseCB == pMessage->Header.xCallback );

	// ������� ��� �����, ����� �������
	bool bClearResult = true;
	for( int i = 0; i < SIZEOFARRAY( aSKLP_MemoryRegions ); i++ )
	{
		SKLP_MemoryRegion_t *pRegion = &aSKLP_MemoryRegions[i];
		if(	( SKLP_MEMEMORY_REGION_ALIAS_DATATECH.ID != pRegion->Alias.ID ) &&
			( SKLP_MemoryRegionType_File == pRegion->Type ) &&
			( NULL != pRegion->pHdl ) )
			if( FR_OK != LogFile_Clear( ( LogFileHdl_t * ) pRegion->pHdl ) )
				bClearResult = false;
	}
	bSKLP_MemoryRegionsUpdated = false;

	if( bClearResult )
		ATOMIC_WRITE( SKLP_MemoryFlags.SD0_DataPresent, 0 );	// ���� �������� ������ �������, �������� ���� ������� ������
}

// [0x20] ������� �������� ������. ������ � ������������ ������.
// ��� ������� ����, ��� - �������������� ������� � ���������� �����, ���������� �� ���������� �������� ������.
// ����� ��������� �� ��� ���� ������! (�������� ~400 ��?)
SKLP_CommandResult_t SKLP_ProcessCommand_MemoryErase( uint8_t *pQuery, uint8_t **ppAnswer )
{
	SKLP_CommandResult_t Result = SKLP_COMMAND_RESULT_ERROR;
	// ��������� ������ ���� ������
	if( 0 == SKLP_SIZE_DATA_QUERY( pQuery ) )
	{
		taskENTER_CRITICAL( );
		// ���������, ����� ������ �����:
		assert_param( NULL != pSKLP_ModuleMode );
		switch( *pSKLP_ModuleMode )
		{
		case SKLP_ModuleMode_Auto:			// ���������� �����
		case SKLP_ModuleMode_AutoDrilling:
		case SKLP_ModuleMode_AutoLogging:
			// ���������� �������
			break;

		case SKLP_ModuleMode_NotAuto:		// ������������ �����
			// ��������� ���� ������� ������, ����� ������ �� �����, ��� �������� ������ ��� �� ���������
			ATOMIC_WRITE( SKLP_MemoryFlags.SD0_DataPresent, 1 );
			// ������ ���� � ��� ������ ���� ���������� � ���������� �����. ��� ��������� � ���������� ����� �� ��������� �������
			assert_param( MemoryThread_AppendCallback( SKLP_ProcessCommand_MemoryEraseCB, 0 ) );
			// ��������� � ������� ������� �� �������� ������
			assert_param( MemoryThread_AppendCallback( SKLP_Memory_EraseCB, 0 ) );		// �����/���� ������ ����� ������, ���� SKLP_MemoryFlags.SD0_DataPresent ����� ����� �������
			break;

		default:
			assert_param( 0 );
			break;
		}
		taskEXIT_CRITICAL( );

		Result = SKLP_COMMAND_RESULT_NO_REPLY;
	}
	return Result;
}

// ���������������� ������ � ���-����� (������� �� ������, ����������), � ��������� ����� � ���
void SKLP_Memory_OpenAndReport( LogFileExt_t *pLogFileExt )
{
	assert_param( NULL != pLogFileExt );

	FRESULT FResult;
	assert_param( MemoryThread_BufferMutexTake( ) );
	char * const pBuff = ( char * ) aMemoryThreadTaskBuffer;
	uint32_t const BuffSize = sizeof( aMemoryThreadTaskBuffer );
	char const *pSubString = NULL;
	char const *pTimeString = "???";
	DWT_Ticks_t TimestampStart = DWT_TimerGet( );
	do
	{
		// ���������� ������ ������ � ���� - ��� �������� � �������� � ���������
		if( FR_OK != ( FResult = LogFile_WriteRecordExt( pLogFileExt->pLogFileHdl, NULL, 0, 0, pLogFileExt->pPositionSaved ) ) )
		{	pSubString = "Can't open or create or seek to EOF file!";		break; }
		if( 0 == ( pLogFileExt->pLogFileHdl->pFile->flag & FA_WRITE ) )
		{	pSubString = "File is Write Protected!";	FResult = FR_WRITE_PROTECTED;	break;	}
		pSubString = "Ok.";
		// ������� ���� ��������� ����������� �����
		FILINFO FileInfo;
		if( FR_OK == FileSystem_GetFileInfo(  pLogFileExt->pLogFileHdl->pFileName, &FileInfo ) )
		{
			DosDateTime_t DosDateTime;
			DosDateTime.Time = FileInfo.ftime;
			DosDateTime.Date = FileInfo.fdate;
			SKLP_Time6_t SKLP_Time6 = Time_DOS2SKLP( DosDateTime );
			pTimeString = Time_SKLP2String( &SKLP_Time6 );
		}
	} while( 0 );
	float TimeToOpen_ms = DWT_TICKS2US( DWT_TimerGet( ) - TimestampStart ) / 1000.0f;
	
	// ������������ ����� �� �������� �����
	uint32_t MessageSize = 0;
	assert_param( MemoryThread_SprintfMutexTake( 5 ) );
	MessageSize += snprintf( pBuff + MessageSize, BuffSize - MessageSize, "Accessing to %s... %s",
								pLogFileExt->pLogFileHdl->pFileName, pSubString );
	if( ( FR_OK == FResult ) || ( FR_WRITE_PROTECTED == FResult ) )
	{	// ���� ������� ������
		uint32_t FileSize = pLogFileExt->pLogFileHdl->pFile->fsize;
		pSubString = ".";
		if( 0 == FileSize )
			pSubString = " - empty.";
		else
		{
			ATOMIC_WRITE( SKLP_MemoryFlags.SD0_DataPresent, 1 );		// ���� ���������� �������, ��������� ���� ������� ������ �� SD
			if( FileSize > pLogFileExt->FileMaxSize )
				pSubString = " - too big!";
		}
		MessageSize += snprintf( pBuff + MessageSize, BuffSize - MessageSize, " Size is %lu bytes%s", FileSize, pSubString );
		if( FileSize > 0 )
			MessageSize += snprintf( pBuff + MessageSize, BuffSize - MessageSize, " Last mod at %s.", pTimeString );
	}
	MessageSize += snprintf( pBuff + MessageSize, BuffSize - MessageSize, " %0.1f ms passed. (%s file)", TimeToOpen_ms, pLogFileExt->pDescription );
	MemoryThread_SprintfMutexGive( );
	assert_param( Logger_WriteRecord( pBuff, LOGGER_FLAGS_APPENDTIMESTAMP | LOGGER_FLAGS_WAITEFORFS ) );
		
	MemoryThread_BufferMutexGive( );
}

// ������� ��������� ����� �� SD-�����
/*void SKLP_Memory_ReportFreeSpace( void )
{
	assert_param( MemoryThread_BufferMutexTake( ) );
	assert_param( MemoryThread_SprintfMutexTake( 5 ) );
	// ������� ��������� ����� �� SD-�����
	char *pMessage = ( char * ) aMemoryThreadTaskBuffer;
	uint64_t FreeSpace;
	if( FR_OK == FileSystem_GetFreeSpace( &FreeSpace ) )
		snprintf( pMessage, sizeof( aMemoryThreadTaskBuffer ), "%llu bytes free space left on uSD.", FreeSpace );
	else
		pMessage = "Can't determinate free space on uSD!";
	MemoryThread_SprintfMutexGive( );
	assert_param( Logger_WriteRecord( pMessage, LOGGER_FLAGS_APPENDTIMESTAMP | LOGGER_FLAGS_WAITEFORFS ) );
	MemoryThread_BufferMutexGive( );
}
*/
void SKLP_Memory_ReportFreeSpace( void )
{
	// ������� ��������� ����� �� SD-�����
	uint64_t FreeSpace;
	if( FR_OK == FileSystem_GetFreeSpace( &FreeSpace ) )
	{
		assert_param( MemoryThread_BufferMutexTake( ) );
		assert_param( MemoryThread_SprintfMutexTake( 5 ) );
		snprintf( ( char * ) aMemoryThreadTaskBuffer, sizeof( aMemoryThreadTaskBuffer ), "%llu bytes free space left on uSD.", FreeSpace );
		MemoryThread_SprintfMutexGive( );
		assert_param( Logger_WriteRecord( ( char * ) aMemoryThreadTaskBuffer, LOGGER_FLAGS_APPENDTIMESTAMP | LOGGER_FLAGS_WAITEFORFS ) );
		MemoryThread_BufferMutexGive( );
	}
	else
		assert_param( Logger_WriteRecord( "Can't determinate free space on uSD!", LOGGER_FLAGS_STATICTEXT | LOGGER_FLAGS_APPENDTIMESTAMP | LOGGER_FLAGS_WAITEFORFS ) );
}


// �������, ������� ���������� ����������� � ��������-��������� ����� (���� ��� �����)
// !!! ������ __weak-���������� �������, ����� �������� ���������� ������� � �������.
// !!! ��� ����������, ����� SKLP_Memory_TmpBufferGet() ��� ����������, � SKLP_Memory_TmpBufferGetSize() �� ��� ����������,
// !!! ���������� ���� ��� �������������� ����� �� ������� ����.
/*
__weak void *SKLP_Memory_TmpBufferGet( uint32_t BufferMinSize )	{ return NULL; }	// ��������� ������� �� ������ � ���������� ������
__weak uint32_t SKLP_Memory_TmpBufferGetSize( void )				{ return 0; }		// ������ ������ ���������� ������
__weak void SKLP_Memory_TmpBufferRelease( bool FromISR )			{ }					// ������������ ������� � ������
*/

