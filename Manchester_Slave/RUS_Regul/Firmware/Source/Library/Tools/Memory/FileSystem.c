// FileSystem.c
// ��������������� ������� �������� �������
#include "ProjectConfig.h"		// ������ ���������, ������ ������.
#include "stm32xxxx_hal.h"		// ����� ���������
#include "FileSystem.h"			// ������
#include "TaskConfig.h"
#include "FF.h"					// FAT FS
#include "FF_gen_drv.h"			// FAT FS Mod
#include <string.h>				// strcmp()
#include <ctype.h>				// toupper()
#include <stdio.h>				// snprintf()
#include "common_sd.h"			// SD_RECORD_SIZE
#include "MathUtils.h"			// GetGreatestPowerOf2()
#include "Logger.h"
#include "SKLP_Service.h"		// SKLP_MemoryFlags
#include "SKLP_Memory.h"		// SKLP_Memory_TmpBufferGet()


#define	FILESYSTEM_REINIT_TIMEOUT_ms	500		// ������� ����� ��������� ������������ �������� ������� ��� ������

extern void LedTimerChangePeriod( float Sec );
__weak void FileSystem_FormatAfterCallback( void )	{ assert_param( 0 ); }
__weak void FileSystem_FormatBeforeCallback( void )	{	}

static char SDPath[4]; 				// SD card logical drive path, init while FATFS_LinkDriver()
static __no_init FATFS SDFatFs;		// File system object for SD card logical drive, cleared while f_mount()
static __no_init FIL FileTmp;		// cleared while f_open()

//#define	FS_USE_TASK_FILESYSTEM
#define	FS_USE_TASK_MEMORYTHREAD

#if		defined( FS_USE_TASK_FILESYSTEM )
// #####################################################
// #####################################################
// ������������ ������� - ���������� ������������� � ������������ �������� ������� ����� ������ FileSystem_Task()
// #####################################################
// #####################################################
static void FileSystem_Task( void *pParameters );

bool FileSystem_TaskInit( void )
{
	assert_param( taskSCHEDULER_NOT_STARTED == xTaskGetSchedulerState( ) );
	bool Result = false;

	do
	{
		TaskHandle_t xTaskHandle;
		if( pdTRUE != xTaskCreate( FileSystem_Task, TASK_FILESYSTEM_NAME, TASK_FILESYSTEM_STACK_SIZE, NULL, TASK_FILESYSTEM_PRIORITY, &xTaskHandle ) )
			break;
		if( NULL == xTaskHandle )
			break;
		SysState_AppendTaskHandler( xTaskHandle );
		SKLP_MemoryFlags.SD0_ErrorMemInit = 1;
		Result = true;
	} while( 0 );
	
	return Result;
}

DWORD bFS_ReadOnly = 0;
// ������ �� ������������ �������� �������
// - ������ SD-card
// - �������������+����
// - ��������������
static void FileSystem_Task( void *pParameters )
{
	static volatile FRESULT FResult;
	EventBits_t EventBitsResult;

	// ������� � ������������� �������� �������
	assert_param( NULL != EventGroup_System );
	( void ) xEventGroupSetBits( EventGroup_System, EVENTSYSTEM_FS_INIT );

	DWT_AppendTimestampTag( "FS Start" );
	while( 1 )
	{	// ������� �������� �� �������� �������������� ��� �������������
		EventBitsResult = xEventGroupWaitBits( EventGroup_System, EVENTSYSTEM_FS_INIT | EVENTSYSTEM_FS_FORMAT, pdFALSE, pdFALSE, portMAX_DELAY );
		// �������� ���� ���������� �������� �������. ���� ������������� ���� ��������.
		xEventGroupClearBits( EventGroup_System, EVENTSYSTEM_FS_READY );
		// ���������, ����� ������ ��� ��������� � ������
		if( EventBitsResult & EVENTSYSTEM_FS_INIT )
		{	// ������ ������ �� ������������� ��� ����������������� �������� �������
			// �������� ���� ��������������
			xEventGroupClearBits( EventGroup_System, EVENTSYSTEM_FS_FORMAT );
			// ������������� �������
			// !!! ����� ������� ���� ���������������, �� ������ ���������� ��������� �������,
			// !!! �.�. f_mount(), ���� ��� ���������������, ������� ������������������� SDPath
			FATFS_UnLinkDriver( SDPath );
			assert_param( 0 == FATFS_LinkDriver( &SD_Driver, SDPath ) );
			// �������������� SD-�����, ���� ����� ���� ������������
			FResult = f_mount( NULL, ( TCHAR const * ) SDPath, 1 );	
			assert_param( FR_OK == FResult );

			// ���������� ������������ SD-�����.
			// �������� � ������ BSP_SD_Init(), ������� �������������� ���������, ���������� ������� � ���� ���������������� ������� � SD-�����
//			LedTimerChangePeriod( 0.1 );
			FResult = f_mount( &SDFatFs, ( TCHAR const * ) SDPath, 1 ); 	// �������������� � �����������
//			LedTimerChangePeriod( 1.0 );
			static uint16_t FS_InitCount = 0;
			FS_InitCount++;

			switch( FResult )
			{
			case FR_OK:				// �������� ������� ���������
				// ��������� ����������������� - ���������� ����� �������� ��������
				FResult = FileSystem_TestFS( "TestFS.txt" );
				if( ( FR_OK == FResult ) || ( FR_WRITE_PROTECTED == FResult ) )
				{	// ���� ������ �������
					portENTER_CRITICAL( );
					( void ) xEventGroupClearBits( EventGroup_System, EVENTSYSTEM_FS_INIT );	// ������������� �������� ������� ���������
					( void ) xEventGroupSetBits( EventGroup_System, EVENTSYSTEM_FS_READY );		// �������� ������� ������ � ������
					SKLP_MemoryFlags_t Flags = SKLP_MemoryFlags;
					Flags.SD0_ErrorMemInit	= 0;
					Flags.SD0_ErrorMemWrite	= ( FR_OK == FResult ) ? 0 : 1;
					Flags.SD0_ErrorMemRead	= 0;
					SKLP_MemoryFlags = Flags;
					bFS_ReadOnly = ( FR_OK == FResult ) ? 0 : 1;
					portEXIT_CRITICAL( );
					// ��������� ��������� � ���.
					// !! �����! ����� ��������� ��������� � �������� ������� ������ MemoryTread_Task(), ������ �����
					// !! ��������� � ��������� � ��������� ���������. ���� ��� ���� ������� ���������� ��������� ����� �� �����,
					// !! ������� ����� ���������� ��������� ������.
					// !! ����� �������, �������� ��������� � ������� ����� ���������� ������������� ������� ����������
					// !! ��� ����������� ��������� ������������ ���������, ��������� ��������� �������.
//					assert_param( Logger_WriteRecord( "[FatFS] Mount on uSD complete.", LOGGER_FLAGS_APPENDTIMESTAMP | LOGGER_FLAGS_STATICTEXT | LOGGER_FLAGS_WAITEFORFS ) );
					char aMsg[60];
					snprintf( aMsg, sizeof( aMsg ), "[FatFS] Mount on uSD complete (attempt #%d).", FS_InitCount );
					assert_param( Logger_WriteRecord( aMsg, LOGGER_FLAGS_APPENDTIMESTAMP | LOGGER_FLAGS_WAITEFORFS ) );
					// ������� � �������� �������� �� ���������������� ��� ��������������
					DWT_AppendTimestampTag( "FS Ready" );
					continue;
				}
				// ���� �� �������. ��������� �����, ��� � ��� ������������ SD ��� �������� �������

			case FR_DISK_ERR:		// ������ ������ � SD-������ (��������������?)
			case FR_NOT_READY:		// ������ ������ � SD-������ (������ ������� ��������, �� ��������� ������ � ����� ������ FatFS?)
				// �������� ������. ������� ���� �� ���������������, ��������� ����������
				vTaskDelay( pdMS_TO_TICKS( FILESYSTEM_REINIT_TIMEOUT_ms ) );					// ��������� "��������" ����� ����� ������������������
				assert_param( EVENTSYSTEM_FS_INIT & xEventGroupGetBits( EventGroup_System ) );	// ��������������, ��� ���� ������������� ����� �� �������
				continue;																		// ������ ������������� �������

			case FR_NO_FILESYSTEM: 	// SD-����� �������� (������ Boot-������), �� FAT32 �� ����������
				// ���������� ��������������
				portENTER_CRITICAL( );
				( void ) xEventGroupClearBits( EventGroup_System, EVENTSYSTEM_FS_INIT );		// ������� �������� ������� �������������� ������������������
				( void ) xEventGroupSetBits( EventGroup_System, EVENTSYSTEM_FS_FORMAT );		// �������� ������ �� ��������������
				SKLP_MemoryFlags.SD0_ErrorMemInit = 1;
				portEXIT_CRITICAL( );
				// ������� � ��������� ������� �� ��������������
				continue;
//				break;

			default:				// ���������������� ������. ���� ����� ��������� - ����������� � ����� ������ ������
				assert_param( 0 );
			}
		}

		if( EventBitsResult & EVENTSYSTEM_FS_FORMAT )
		{	// ������ ������ �� �������������� SD-�����
			if( SKLP_FlagsModule.DataSaving )
			{	// ���� ������ ��������� � ���������� ������ - �� � ���� ������ �� �������������. ����� ��� ��� ����������� ���������������� �������
				vTaskDelay( pdMS_TO_TICKS( 100 ) );
				FileSystem_Reinit( "[FatFS] Skip Query to SD formatting." );
				continue;
				// !!!! ������-�� ���������� ���������: �������������� ����������� ������ ��� �������� � ���������� �����.
			}
			// !!! �� ����� �������������� � SD � �������� 1024 ��, ��������� ��������� ��������������!
			vTaskDelay( pdMS_TO_TICKS( 100 ) );
			FileSystem_Reinit( "[FatFS] Skip Query to SD formatting." );
			continue;

			// ������� ������� ����� ������� ��������
			FileSystem_FormatBeforeCallback( );
//			LedTimerChangePeriod( 0.1 );
			// ��������������
			FResult = f_mount( NULL, ( TCHAR const * ) SDPath, 1 );
			assert_param( FR_OK == FResult );
			// ������������� �������
			FATFS_UnLinkDriver( SDPath );
			assert_param( 0 == FATFS_LinkDriver( &SD_Driver, SDPath ) );
			// ����� �����������, �� ���� ��� ��������� - ��� ���������� � ��������������
			FResult = f_mount(  &SDFatFs, ( TCHAR const * ) SDPath, 0 );
			// ���������� ��������������
			FResult = f_mkfs( ( TCHAR const * ) SDPath, 0, SD_RECORD_SIZE );
			assert_param( Logger_WriteRecord( "[FatFS] Formatting uSD complete.", LOGGER_FLAGS_APPENDTIMESTAMP | LOGGER_FLAGS_STATICTEXT | LOGGER_FLAGS_WAITEFORFS ) );
//			LedTimerChangePeriod( 1.0 );
			// ��������� �� ���������. ������� � �������������
			FileSystem_Reinit( "???" );
			continue;
		}
		// �������� ������ ��� ����������� ������������ �����
		break;
	}
	assert_param( 0 );
}

// ������ ����������������� �������� �������
void FileSystem_ReinitExt( char *pReason, const char *pSourceFile, uint32_t Line, const char *pFunc )
{
	assert_param( NULL != EventGroup_System );
//	static char aMessage[170];
//	snprintf( aMessage, sizeof( aMessage ), "[FatFS] Remount because of %s in %s at %u", pReason, pSourceFile, Line );
//	assert_param( Logger_WriteRecord( aMessage, LOGGER_FLAGS_APPENDTIMESTAMP /*| LOGGER_FLAGS_WAITEFORFS */) );
	portENTER_CRITICAL( );
	( void ) xEventGroupClearBits( EventGroup_System, EVENTSYSTEM_FS_READY | EVENTSYSTEM_FS_FORMAT );
	( void ) xEventGroupSetBits( EventGroup_System, EVENTSYSTEM_FS_INIT );
	SKLP_MemoryFlags.SD0_ErrorMemInit = 1;
	portEXIT_CRITICAL( );
}

#elif	defined( FS_USE_TASK_MEMORYTHREAD )
// #####################################################
// #####################################################
// ����� ������� - ���������� ������������� � ������������ �������� ������� �������� ������ MemoryThread_Task()
// #####################################################
// #####################################################
#include "MemoryThread.h"

static TimerHandle_t FS_TimerHdl = NULL;

static void FileSystem_ReinitCB( MemoryThreadMessage_t *pMessage );
static void FileSystem_ReinitInternal( bool bDelayed );

// �������� ������� � MemoryThread_Task() �� ����������������� �������� �������
static void FileSystem_TimerCB( TimerHandle_t xTimer )
{
	assert_param( FS_TimerHdl == xTimer );
//	assert_param( MemoryThread_AppendCallback( FileSystem_ReinitCB, 0 ) );
	FileSystem_ReinitInternal( false );
}

bool FileSystem_TaskInit( void )
{
	bool Result = false;
	do
	{
		// ���������������� ������ ����������������� FS
		assert_param( NULL == FS_TimerHdl );
		FS_TimerHdl = xTimerCreate( "FS", pdMS_TO_TICKS( FILESYSTEM_REINIT_TIMEOUT_ms ), pdFALSE, NULL, FileSystem_TimerCB );
		assert_param( NULL != FS_TimerHdl );
		// ������ �� ���������, ����� ������� ������ ����� ��������� �������������

		// ��������� ������� ����������� �������������.
		// ����� ��������� �������� ������������� ����� ���������� ��� ��.
		FileSystem_ReinitInternal( false );

		Result = true;
	} while( 0 );
	
	return Result;
}

// ������ ����������������� FS, ������ ���� �� ��� ��� ��� ���� � �������
void FileSystem_Reinit( char * pReason )
{
	( void ) pReason;
	if( FR_OK == FileSystem_CheckFSReady( ) )
		FileSystem_ReinitInternal( false );
}

// ������ ����������������� FS
static void FileSystem_ReinitInternal( bool bDelayed )
{
	assert_param( NULL != EventGroup_System );
	portENTER_CRITICAL( );
	( void ) xEventGroupClearBits( EventGroup_System, EVENTSYSTEM_FS_READY | EVENTSYSTEM_FS_FORMAT );
	( void ) xEventGroupSetBits( EventGroup_System, EVENTSYSTEM_FS_INIT );
	SKLP_MemoryFlags.SD0_ErrorMemInit = 1;
	if( !bDelayed )
	{	// �������� ������� � MemoryThread_Task() �� ����������������� �������� �������
		assert_param( MemoryThread_AppendCallback( FileSystem_ReinitCB, 0 ) );
	}
	else
	{	// ��������� ������, ������� ������� ������� �� ����������������� ����� �������
		assert_param( NULL != FS_TimerHdl );
		assert_param( pdPASS == xTimerStart( FS_TimerHdl, 0 ) );
	}
	portEXIT_CRITICAL( );
}

// ������� �� MemoryThread_Task() ��� ����������������� �������� �������
// - ������ SD-card
// - �������������+����
// - ��� ��������������
static void FileSystem_ReinitCB( MemoryThreadMessage_t *pMessage )
{
	assert_param( NULL != pMessage );
	assert_param( FileSystem_ReinitCB == pMessage->Header.xCallback );

	static volatile FRESULT FResult;
	assert_param( NULL != EventGroup_System );
		
	// ���������, ��� ��������� ���� ����������������� �������� �������
	assert_param( EVENTSYSTEM_FS_INIT == ( ( EVENTSYSTEM_FS_READY | EVENTSYSTEM_FS_INIT ) & xEventGroupGetBits( EventGroup_System ) ) );

	// ������������� ������� SD-�����
	FATFS_UnLinkDriver( SDPath );
	assert_param( 0 == FATFS_LinkDriver( &SD_Driver, SDPath ) );

	// �������������� SD-�����, ���� ����� ���� ������������.
	// BSP_SD_DeInit() ��� ���� �� ����������,
	// �.�. � FatFS �� �������������� ��������� � �������� ���������� ��� ���������������.
	FResult = f_mount( NULL, ( TCHAR const * ) SDPath, 1 );	
	assert_param( FR_OK == FResult );

	// ���������� ������������ SD-�����.
	// �������� � ������ BSP_SD_Init(), ������� �������������� ���������, ���������� ������� � ���� ���������������� ������� � SD-�����
	FResult = f_mount( &SDFatFs, ( TCHAR const * ) SDPath, 1 );
	static uint16_t FS_MountCount = 0;
	FS_MountCount++;

	// ��������� ��������� ������������
	switch( FResult )
	{
	case FR_OK:				// �������� ������� ���������
		// ��������� ����������������� - ���������� ����� �������� ��������
		FResult = FileSystem_TestFS( "TestFS.txt" );
		if( ( FR_OK == FResult ) || ( FR_WRITE_PROTECTED == FResult ) )
		{	// ���� ������ �������
			portENTER_CRITICAL( );
			( void ) xEventGroupClearBits( EventGroup_System, EVENTSYSTEM_FS_INIT );	// ������������� �������� ������� ���������
			( void ) xEventGroupSetBits( EventGroup_System, EVENTSYSTEM_FS_READY );		// �������� ������� ������ � ������
			SKLP_MemoryFlags_t Flags = SKLP_MemoryFlags;
			Flags.SD0_ErrorMemInit	= 0;
			Flags.SD0_ErrorMemRead	= 0;
			Flags.SD0_ErrorMemWrite	= ( FR_OK == FResult ) ? 0 : 1;
			SKLP_MemoryFlags = Flags;
			portEXIT_CRITICAL( );
			// ��������� ��������� � ���.
			// !! �����! ����� ��������� ��������� � �������� ������� ������ MemoryTread_Task(), ������ �����
			// !! ��������� � ��������� � ��������� ���������. ���� ��� ���� ������� ���������� ��������� ����� �� �����,
			// !! ������� ����� ���������� ��������� ������.
			// !! ����� �������, �������� ��������� � ������� ����� ���������� ������������� ������� ����������
			// !! ��� ����������� ��������� ������������ ���������, ��������� ��������� �������.
//			char aMsg[60];
//			snprintf( aMsg, sizeof( aMsg ), "[FatFS] Mount on uSD complete (attempt #%d).", FS_MountCount );
			char aMsg[120];
			extern char aSD_CID_ASCII[];
			snprintf( aMsg, sizeof( aMsg ), "[FatFS] Mount on uSD complete (attempt #%d), **SD.CID %s**", FS_MountCount, aSD_CID_ASCII );
			assert_param( Logger_WriteRecord( aMsg, LOGGER_FLAGS_APPENDTIMESTAMP | LOGGER_FLAGS_WAITEFORFS ) );
			DWT_AppendTimestampTag( "FS Ready" );
//			HAL_SD_GetCardCID();

			// ��������� ���������������� �������
			break;
		}
		// ���� �� �������. ��������� �����, ��� � ��� ������������ SD ��� �������� �������

	case FR_DISK_ERR:		// ������ ������ � SD-������ (��������������?)
	case FR_NOT_READY:		// ������ ������ � SD-������ (������ ������� ��������, �� � �.�. FatFS ���-�� �� ��)
	case FR_NO_FILESYSTEM:	// SD-����� �������� (������ Boot-������), �� FAT32 �� ����������
		{	// �������� ������. ������� ���� �� ���������������, ��������� ����������
			// ��������� ������� ����������� �������������.
			// ����� ��������� �������� ������������� ����� ���������� ��� ��.
			FileSystem_ReinitInternal( true );
		}
		break;

	default:				// ���������������� ������. ���� ����� ��������� - ����������� � ����� ������ ������
		assert_param( 0 );
	}
}


// ������ ����������������� �������� �������
void FileSystem_ReinitExt( char *pReason, const char *pSourceFile, uint32_t Line, const char *pFunc )
{
	FileSystem_ReinitInternal( false );
}

#endif	// FS_USE_TASK_XXX
// #####################################################
// #####################################################

// ��������, ��� ��������� ���� ��� ������
FRESULT FileSystem_CheckFileOpen( FIL *pFile )
{
	if( NULL == pFile )
		return FR_INT_ERR;				// ������ ��������
	if( NULL == pFile->fs )
		return FR_NO_FILE;				// ���� ������
	if( ( 0 == pFile->fs->fs_type ) || ( pFile->fs->id != pFile->id ) )
	{
//		pFile->fs = NULL;				// �������� ������� ���� �������������� � �� ����� ���� ���� ��� ������ - ������� �������
//		pFile->err = 0;
//		return FR_NO_FILE;				// ������ ���� ������
		return FR_INVALID_OBJECT;
	}
	return ( FRESULT ) pFile->err;
//	return FR_OK;
}

// ���������� ���� � �����
FRESULT FileSystem_SeekToEOF( FIL *pFile )
{
	assert_param( NULL != pFile );
	if( pFile->fptr == pFile->fsize )
		return FR_OK;
	return f_lseek( pFile, pFile->fsize );
}

// ���������� ���� � �����, ��������, � ����������� ������������� �������
// pFile			- ����������� ��������� �����.
// pFilePosition	- ��������� �� ���������, ���� ��������� ������������� � ��������� �������.
// SeekSizeMax		- ������������ ������ ����� ���������.
// 		!! ����, ��� ����������� ������ �������� �������:
//		- SeekSizeMax ������ ���� ������ ������� ��������!
//		- ���� ���������� ��������� ��������� ������ SeekSizeMax, ��������� ������������ �� �������, ������� SeekSizeMax!
FRESULT FileSystem_SeekToEOF_Incremental( FIL *pFile, FIL_Position_t *pFilePosition, uint32_t SeekSizeMax )
{
	assert_param( ( NULL != pFile ) && ( NULL != pFilePosition ) );
	SeekSizeMax = GetGreatestPowerOf2( SeekSizeMax );
	assert_param( SeekSizeMax > ( 4 * 1024 * 1024 ) );
	FRESULT FResult = FR_OK;
	while( pFile->fptr != pFile->fsize )
	{
		uint32_t FileOffset = pFile->fsize;
		if( ( FileOffset - pFile->fptr ) > SeekSizeMax )
			FileOffset = ( ( pFile->fptr + SeekSizeMax ) / SeekSizeMax ) * SeekSizeMax;		// ��� ������� ���������� ���������� ������������ ����� ���������
		if( FR_OK != ( FResult = f_lseek( pFile, FileOffset ) ) )
			break;
		// ����� ������ ��������� ��������� ������� �����, ����� ��� ��������� ������ �������� ������� ���������� ��������� � ����������� �������
		if( FR_OK != ( FResult = FileSystem_FilePositionSave( pFile, pFilePosition ) ) )
			break;
	}
	return FResult;
}

// ���������� ���� ������������ �����
FRESULT FileSystem_SeekFromEOF( FIL *pFile, int32_t OffsetFromEOF )
{
	assert_param( NULL != pFile );
	if( OffsetFromEOF <= 0 )
		assert_param( pFile->fsize >= -OffsetFromEOF );						// ��������, ��� ��������� ����� �� �������� ����� ������ �����
	return f_lseek( pFile, pFile->fsize + OffsetFromEOF );
}

// ��������, ��� �������� ������� ������ � ������
FRESULT FileSystem_CheckFSReady( void )
{
	if( EVENTSYSTEM_FS_READY & xEventGroupGetBits( EventGroup_System ) )
		return FR_OK;
	else
		return FR_NOT_READY;
}

bool FileSystem_CheckFSReadyBool( void )
{
	return ( FR_OK == FileSystem_CheckFSReady( ) );
} 


// ��������� ���������� �������� �������
FRESULT FileSystem_WaitFSReady( TickType_t TicksToWait )
{
	assert_param( NULL != EventGroup_System );
	const EventBits_t BitsToWait = EVENTSYSTEM_FS_READY;
	EventBits_t BitsResult = xEventGroupWaitBits( EventGroup_System, BitsToWait, pdFALSE, pdTRUE, TicksToWait );
	return ( ( BitsToWait == ( BitsResult & BitsToWait ) ) ? FR_OK : FR_NOT_READY );
}

// �������� ����������������� �������� �������
FRESULT FileSystem_TestFS( char *pFileName )
{
/*	// Test INGK SD
	// ******************************************
	const uint32_t BlockSize = 64 * FS_SECTOR_SIZE;
	const uint32_t BadSector = 4381440;
	const uint32_t GoodSector = 0;
	extern const uint8_t * const * pTmpBuffer;
	uint8_t *pBuffer = ( uint8_t * ) pTmpBuffer;
	static volatile DWORD RESULT;
	RESULT = SD_read( pBuffer, GoodSector, BlockSize / FS_SECTOR_SIZE );
	RESULT = SD_read( pBuffer, BadSector, BlockSize / FS_SECTOR_SIZE );
//	RESULT = SD_read( pBuffer, BadSector, BlockSize / FS_SECTOR_SIZE );
	RESULT = SD_read( pBuffer, GoodSector, BlockSize / FS_SECTOR_SIZE );
	// ******************************************
*/
	FRESULT res;		// FatFs function common result code
	assert_param( NULL != pFileName );
	do
	{
		char aTestWrite[] = "Test File System!";
		char aTestRead[ sizeof( aTestWrite ) ];
		UINT BytesCount;

		// ������� ����� ���� � ������� ��� ������
		res = f_open( &FileTmp, pFileName, FA_CREATE_ALWAYS | FA_WRITE );
		if( FR_OK != res )
		{
			if( FR_DISK_ERR == res )
				res = FR_WRITE_PROTECTED;
			break;
		}
	
		// �������� ���������
		assert_param( VALIDATE_ALIGN( aTestWrite, sizeof( uint32_t ) ) );
		res = f_write( &FileTmp, aTestWrite, sizeof( aTestWrite ), &BytesCount );
		if( ( sizeof( aTestWrite ) != BytesCount ) || ( FR_OK != res ) )
		{
			if( FR_DISK_ERR == res )
				res = FR_WRITE_PROTECTED;
			break;
		}
	
		// ������� ����
		res = f_close( &FileTmp );
		if( FR_OK != res )
			break;
	
		// ������� ��� ������������ ����
		res = f_open( &FileTmp, pFileName, FA_READ | FA_OPEN_EXISTING );
		if( FR_OK != res )
			break;
	
		// ������� ���������
		assert_param( VALIDATE_ALIGN( aTestRead, sizeof( uint32_t ) ) );
		res = f_read( &FileTmp, aTestRead, sizeof( aTestRead ), &BytesCount );
		if( ( sizeof( aTestRead ) != BytesCount ) || ( FR_OK != res ) )
			break;
	
		// ������� ����
		res = f_close( &FileTmp );
	
		// �������� ���������
		if( 0 != strcmp( aTestWrite, aTestRead ) )
			break;
	
		// ������� ����
		res = f_unlink( pFileName );
		if( FR_OK != res )
			break;
	}while( 0 );
	if( NULL != FileTmp.fs )
		f_close( &FileTmp );

	return res;
}

/*
FRESULT FileSystem_TestFS2( char *pFileName )
{
	FRESULT res;		// FatFs function common result code
	assert_param( NULL != pFileName );
	do
	{
		static FIL TestFile;
#error "������� �� ����������� FIL FileTmp."
		// ������� ��� ������������ ����
		res = f_open( &TestFile, pFileName, FA_WRITE | FA_OPEN_EXISTING );
		if( FR_OK != res )
			break;

		extern uint8_t *pTmpBuffer;
		const uint16_t BuffSize = 32 * 1024;
		uint8_t *pBuffer = pTmpBuffer;
		assert_param( NULL != pBuffer );
	
		// ������� ���� ����
		UINT BytesCount;
		assert_param( VALIDATE_ALIGN( pBuffer, sizeof( uint32_t ) ) );
		int ReadCount = TestFile.fsize / BuffSize;
		while( ReadCount-- )
		{
			if( 0 == ( ReadCount % 5 ) )
			{
				res = f_lseek( &TestFile, TestFile.fptr - BuffSize );
				if( FR_OK != res )
					break;
			}
//			res = f_read( &TestFile, pBuffer, BuffSize, &BytesCount );
			res = f_write( &TestFile, pBuffer, BuffSize, &BytesCount );
			if( ( BuffSize != BytesCount ) || ( FR_OK != res ) )
				break;
		}
	
		// ������� ����
		f_close( &TestFile );

		if( -1 == ReadCount )
			res = FR_OK;
		else
			res = FR_INT_ERR;
		
	}while( 0 );

	return res;
}
*/

// ����� � ���������� ���� � ����� ����� �����
// pFileDir			����������, � ������� ������ ����
// pResultFileInfo	������� ���� ���������� � �����
FRESULT FileSystem_FindLastFile( const TCHAR *pFileDir, FILINFO *pResultFileInfo )
{
	FRESULT FResult;
	DIR Dir;
	FILINFO FileInfo;
	int FileCount = 0;

	do
	{
		FResult = FR_INVALID_PARAMETER;
		// ��������� ���������
		if( ( NULL == pFileDir ) || ( NULL == pResultFileInfo ) )
			break;

		// ������� ���������� ��� ���������
		FResult = f_opendir( &Dir, pFileDir );
		if( FR_OK != FResult )
			break;

		// ������������� � ������ ������ � ������ ����������
		FResult = f_readdir ( &Dir, NULL );
		if( FR_OK != FResult )
			break;
		
		// ��������� ����� �� ���� ������ � ����������
		WORD LastFileTime = 0;
		WORD LastFileDate = 0;
		do
		{
			// ������� ���������� �� ��������� �����
			FResult = f_readdir ( &Dir, &FileInfo );
			if( FR_OK != FResult )
				break;
			if( '\0' == FileInfo.fname[0] )
				break;		// ��������� ����� ����������

			FileCount++;
			// ������� ���� ����������� ���������� ����� � ����� ����� ������
			if( ( FileInfo.fdate > LastFileDate ) ||
				( ( FileInfo.fdate == LastFileDate ) && ( FileInfo.ftime > LastFileTime ) ) )
			{
				LastFileTime = FileInfo.ftime;
				LastFileDate = FileInfo.fdate;
				*pResultFileInfo = FileInfo;
			}
		} while( 1 );

		// ������� ����������
		FResult = f_closedir( &Dir );
		if( FR_OK != FResult )
			break;

		// ������� ���������� � �����
		if( FileCount > 0 )
			FResult = FR_OK;
		else
			FResult = FR_NO_FILE;
	} while( 0 );

	return FResult;}

// �������� ���������� � �����
FRESULT FileSystem_GetFileInfo( const TCHAR *pFileName, FILINFO *pResultFileInfo )
{
	FRESULT FResult;
	DIR Dir;
	FILINFO FileInfo;
	do
	{
		FResult = FR_INVALID_PARAMETER;
		// ��������� ���������
		if( ( NULL == pFileName ) || ( NULL == pResultFileInfo ) )
			break;

		// ������� ���������� ��� ���������
		// !!��������!!
		FResult = f_opendir( &Dir, "\\" );
		if( FR_OK != FResult )
			break;

		// ������������� � ������ ������ � ������ ����������
		FResult = f_readdir ( &Dir, NULL );
		if( FR_OK != FResult )
			break;
		
		// ��������� ����� �� ���� ������ � ����������
		do
		{
			// ������� ���������� �� ��������� �����
			FResult = f_readdir ( &Dir, &FileInfo );
			if( FR_OK != FResult )
				break;
			if( '\0' == FileInfo.fname[0] )
			{	// ��������� ����� ����������, � ���� �� ������
				FResult = FR_NO_FILE;
				break;
			}

			// ��������� ���������� ����� ����� � �������
			int i = 0;
			int FileNameLenght = strlen( pFileName );
			for( ; i < FileNameLenght; i++ )
				if( toupper( FileInfo.fname[i] ) != toupper( pFileName[i] ) )
					break;
			if( i != FileNameLenght )
				continue;		// ������������ ����� ����� � �������

			// ������ ���� � ����������� ������
			*pResultFileInfo = FileInfo;
			assert_param( FR_OK == FResult );
			break;
		} while( 1 );
	} while( 0 );

	return FResult;
}

// ����� �� �������� ����� ���� ������ � ������������ �������, � ������� ��� �����
// pFileDir			����������, � ������� ������ ����
// pBaseName		������� ����� ����� �����, �� 1 �� 7 ��������, ��������������� '\0'
// pExtension		���������� ������
// pResultNumber	���� ������� ����� ������ ���������� ���������� �����
// pResultFileInfo	���� �� NULL, ������� ���� ���������� � �����
FRESULT FileSystem_FindLastEnumeratedFile( const TCHAR *pFileDir, const TCHAR *pBaseName, const TCHAR *pExtension, int32_t *pResultNumber, FILINFO *pResultFileInfo )
{
	FRESULT FResult;
	DIR Dir;
	FILINFO FileInfo;

	do
	{
		FResult = FR_INVALID_PARAMETER;
		// ��������� ���������
		if( ( NULL == pFileDir ) || ( NULL == pBaseName ) || ( NULL == pResultNumber ) )
			break;

		// ��������� ����� ������� ����� �����
		uint32_t BaseNameLenght = strlen( pBaseName );
		if( ( BaseNameLenght < 1 ) || ( BaseNameLenght > 7 ) )
			break;	
	
		// ��������� ����� ����������
		if( ( NULL != pExtension ) && ( strlen( pExtension ) > 3 ) )
			break;	

		// ������� ���������� ��� ���������
		FResult = f_opendir( &Dir, pFileDir );
		if( FR_OK != FResult )
			break;

		// ������������� � ������ ������ � ������ ����������
		FResult = f_readdir ( &Dir, NULL );
		if( FR_OK != FResult )
			break;
		
		// ��������� ����� �� ���� ������ � ����������
		int MaxFileNumber = -1;
		do
		{
			// ������� ���������� �� ��������� �����
			FResult = f_readdir ( &Dir, &FileInfo );
			if( FR_OK != FResult )
				break;
			if( '\0' == FileInfo.fname[0] )
				break;		// ��������� ����� ����������

			// ��������� ���������� ����� ����� � �������
			int i = 0;
			for( ; i < BaseNameLenght; i++ )
				if( toupper( FileInfo.fname[i] ) != toupper( pBaseName[i] ) )
					break;
			if( i != BaseNameLenght )
				continue;		// ������������ ����� ����� � �������

			// ������ ����� ����� ��������� � �������. ������� �����.
			int32_t FileNumber = -1;
			do
			{
				// ������� ��������� ������ ����� �����
				char c = FileInfo.fname[i++];

				if( '\0' == c )
				{	// ��������� ����� ����� �����, ���������� ���
					if( NULL != pExtension )
						FileNumber = -1;		// ������ ���� ���� ����������, �� ��� ���
					break;
				}
				
				if( '.' == c  )
				{	// ��������� ����� ����� �����, ���������� ����
					if( NULL != pExtension )
					{	// �������� ����������
						int ExtensionLenght = strlen( &FileInfo.fname[i] );
						for( int j = 0; j < ExtensionLenght + 1; i++, j++ )
							if( toupper( FileInfo.fname[i] ) != toupper( pExtension[j] ) )
							{
								FileNumber = -1;		// ���������� �� �������
								break;
							}
						
					}
					else
						FileNumber = -1;				// ���������� �� �������
					break;
				}

				if( !isdigit( c ) )
				{
					FileNumber = -1;
					break;		// ������ ����� ���-�� ������
				}

				// ���������� �����, ������� � ����� �����
				if( -1 == FileNumber )
					FileNumber = 0;
				FileNumber = FileNumber * 10 + c - '0';
			} while( i < 9 );

			// ������� ���������� ����� � ������������ ��������� �� ������� ������
			if( FileNumber > MaxFileNumber )
			{
				MaxFileNumber = FileNumber;
				if( NULL != pResultFileInfo )
					*pResultFileInfo = FileInfo;
			}
		} while( 1 );

		// ������� ����������
		FResult = f_closedir( &Dir );
		if( FR_OK != FResult )
			break;

		// ������� ��������� ������������ �����
		if( MaxFileNumber > -1 )
		{
			*pResultNumber = MaxFileNumber;
			
			FResult = FR_OK;
		}
		else
			FResult = FR_NO_FILE;
	} while( 0 );

	return FResult;
}

// ������� ����� ��� ������������� ����� �� ������ �������� �����, ���������� � ������
FRESULT FileSystem_CreateEnumeratedFileName( const TCHAR *pBaseName, const TCHAR *pExtension, int32_t Number, TCHAR *pResultName )
{
	FRESULT Result = FR_INT_ERR;
	do
	{
		Result = FR_INVALID_PARAMETER;
		if( ( NULL == pBaseName ) || ( NULL == pResultName ) || ( Number < 0 ) )
			break;

		if( ( NULL != pExtension ) && strlen( pExtension ) > 3 )
			break;

		int BaseNameLenght = strlen( pBaseName );
		if( BaseNameLenght > 7 )
			break;
		strcpy( pResultName, pBaseName );

		int i = 7;
		while( i >= BaseNameLenght )
		{
			pResultName[i--] = '0' + Number % 10;
			Number /= 10;
		}
		if( 0 != Number )
			break;

		if( NULL == pExtension )
			pResultName[8] = '\0';
		else
		{
			pResultName[8] = '.';
			strcpy( pResultName + 9, pExtension );
		}
		Result = FR_OK;
	}
	while( 0 );

	return Result;
}

// ������� ��� ������������ ����� � ��������� ����������
// pFileDir				����������, � ������� ������ �����
// pBaseName			������� ����� ����� �����, �� 1 �� 7 ��������, ��������������� '\0'
// pExtension			���������� ������
// pDeletedFilesCount	���������� ��������� ������
FRESULT FileSystem_DeleteEnumeratedFiles( const TCHAR *pFileDir, const TCHAR *pBaseName, const TCHAR *pExtension, int32_t *pDeletedFilesCount )
{
	FRESULT FResult;
	DIR Dir;
	FILINFO FileInfo;

	do
	{
		FResult = FR_INVALID_PARAMETER;
		if( NULL != pDeletedFilesCount )
			*pDeletedFilesCount = 0;
		
		// ��������� ���������
		if( ( NULL == pFileDir ) || ( NULL == pBaseName ) )
			break;

		// ��������� ����� ������� ����� �����
		uint32_t BaseNameLenght = strlen( pBaseName );
		if( ( BaseNameLenght < 1 ) || ( BaseNameLenght > 7 ) )
			break;	
	
		// ��������� ����� ����������
		if( ( NULL != pExtension ) && ( strlen( pExtension ) > 3 ) )
			break;	

		// ������� ���������� ��� ���������
		FResult = f_opendir( &Dir, pFileDir );
		if( FR_OK != FResult )
			break;

		// ������������� � ������ ���� ������ � ������ ����������
		FResult = f_readdir ( &Dir, NULL );
		if( FR_OK != FResult )
			break;
		
		// ��������� ����� �� ���� ������ � ����������
		do
		{
			// ������� ���������� �� ��������� �����
			FResult = f_readdir ( &Dir, &FileInfo );
			if( FR_OK != FResult )
				break;
			if( '\0' == FileInfo.fname[0] )
				break;		// ��������� ����� ����������

			// ��������� ���������� ����� ����� � �������
			int i = 0;
			for( ; i < BaseNameLenght; i++ )
				if( toupper( FileInfo.fname[i] ) != toupper( pBaseName[i] ) )
					break;
			if( i != BaseNameLenght )
				continue;		// ������������ ����� ����� � �������

			// ������ ����� ����� ��������� � �������. ������� �����.
			int32_t FileNumber = -1;
			do
			{
				// ������� ��������� ������ ����� �����
				char c = FileInfo.fname[i++];

				if( '\0' == c )
				{	// ��������� ����� ����� �����, ���������� ���
					if( NULL != pExtension )
						FileNumber = -1;		// ������ ���� ���� ����������, �� ��� ���
					break;
				}
				
				if( '.' == c  )
				{	// ��������� ����� ����� �����, ���������� ����
					if( NULL != pExtension )
					{	// �������� ����������
						int ExtensionLenght = strlen( &FileInfo.fname[i] );
						for( int j = 0; j < ExtensionLenght + 1; i++, j++ )
							if( toupper( FileInfo.fname[i] ) != toupper( pExtension[j] ) )
							{
								FileNumber = -1;		// ���������� �� �������
								break;
							}
						
					}
					else
						FileNumber = -1;				// ���������� �� �������
					break;
				}

				if( !isdigit( c ) )
				{
					FileNumber = -1;
					break;		// ������ ����� ���-�� ������
				}

				// ���������� �����, ������� � ����� �����
				if( -1 == FileNumber )
					FileNumber = 0;
				FileNumber = FileNumber * 10 + c - '0';
			} while( i < 9 );

			if( FileNumber > -1 )
			{	// ������ ���� � ���������� ������. �������.
				FResult = f_unlink( FileInfo.fname );
				if( FR_OK != FResult )
					break;
				// ���� ������� ������
				if( NULL != pDeletedFilesCount )
					( *pDeletedFilesCount )++;
			}
		} while( 1 );

		// ������� ����������
		f_closedir( &Dir );
	} while( 0 );

	return FResult;
}

// ������� ��������� �����
FRESULT FileSystem_GetFreeSpace( uint64_t *pFreeSpace )
{
	DWORD FreeClusters;
	FATFS *pFATFS;
	FRESULT FResult = f_getfree( SDPath, &FreeClusters, &pFATFS );
	assert_param( NULL != pFreeSpace );
	*pFreeSpace = ( ( ( uint64_t ) FreeClusters ) * FS_SECTOR_SIZE ) * pFATFS->csize;
	return FResult;
}


#define	BUFFER_SIZE				( FS_SECTOR_SIZE * 10 )
#define	TEST_SECTOR_ADDRESS		( ( ( uint64_t ) 10 ) * 1024 * 1024 * 1024 )
#include <stdlib.h>
#include "common_sd.h"
void FileSystem_TestSD( void )
{
	static uint8_t aBufferWrite[BUFFER_SIZE], aBufferRead[BUFFER_SIZE];
	static uint8_t aResults[1024] = { 0 };
	static int CurrentCycle = 0;
	int	Result;
	static struct
	{
		uint32_t WriteSuccess;
		uint32_t WriteFail;
		uint32_t ReadSuccess;
		uint32_t ReadFail;
		uint32_t CompareSuccess;
		uint32_t CompareFail;
	} Stat = { 0 };

	// ����������� ����� ��� ������
	for( int i = 0; i < BUFFER_SIZE; i++ )
		aBufferWrite[i] = ( uint8_t ) rand( );

	do
	{
		// �������� ����� � SD
		Result = BSP_SD_WriteBlocks_DMA( ( uint32_t * ) aBufferWrite, TEST_SECTOR_ADDRESS / FS_SECTOR_SIZE, BUFFER_SIZE / FS_SECTOR_SIZE );
		if( MSD_OK == Result )
			Stat.WriteSuccess++;
		else
		{
			Stat.WriteFail++;
			break;
		}
		
		// ������� ������ �� SD
		Result = BSP_SD_ReadBlocks_DMA( ( uint32_t * ) aBufferRead, TEST_SECTOR_ADDRESS / FS_SECTOR_SIZE, BUFFER_SIZE / FS_SECTOR_SIZE );
		if( MSD_OK == Result )
			Stat.ReadSuccess++;
		else
		{
			Stat.ReadFail++;
			break;
		}
		
		// ��������� ����������� ������
		Result = memcmp( aBufferRead, aBufferWrite, BUFFER_SIZE );
		if( 0 == Result )
			Stat.CompareSuccess++;
		else
		{
			aResults[CurrentCycle]++;
			Stat.CompareFail++;
			if( ( Stat.CompareFail > 100 ) || aResults[CurrentCycle] > 4 )
				aResults[CurrentCycle] = 0xFF;
			break;
		}

		CurrentCycle = ( CurrentCycle + 1 ) % SIZEOFARRAY( aResults );
		
	} while( 0 );
	
}

// ����������� �� ������ ����� � ������.
// ����������� ������������ �� � ������, � � ������� ������� � � ������� �������.
FRESULT FileSystem_FileCopy( FIL *pFileSource, FIL *pFileDestination, uint32_t BytesToCopy )
{
	FRESULT FResult;
//	uint32_t BufferSize = SKLP_Memory_TmpBufferGetSize( );
//	BufferSize = ( BufferSize / 512 ) * 512;
/*	// ��������� ������ ������, ������� ������� ��������
	uint32_t BufferSizeMax = SKLP_Memory_TmpBufferGetSize( );
	uint32_t BufferSize = 512;
	while( BufferSize <= BufferSizeMax )
		BufferSize *= 2;
	BufferSize /= 2;
*/
	// ��������� ������ ������, ������� ������� �������� (�������� � ������� ������ ����� ������� �������)
	uint32_t BufferSize = GetGreatestPowerOf2( SKLP_Memory_TmpBufferGetSize( ) );
	
	assert_param( 0 != BufferSize );
	void *pBuffer = NULL;
	do
	{
		if( FR_OK != ( FResult = FileSystem_CheckFSReady( ) ) )
			break;
		FResult = FR_INVALID_PARAMETER;
		if( ( NULL == pFileSource ) || ( NULL == pFileDestination ) || ( 0 == BytesToCopy ) )
			break;
		FResult = FR_NO_FILE;
		if( ( NULL == pFileSource->fs ) || ( NULL == pFileDestination->fs ) )
			break;
		FResult = FR_INT_ERR;
		if( NULL == ( pBuffer = SKLP_Memory_TmpBufferGet( BufferSize ) ) )
			break;
		// ��������� ������������ ������
		if( !VALIDATE_ALIGN( pBuffer, sizeof( uint32_t ) ) )
			break;
		// ��������� ������������ ������
		if( BytesToCopy > FS_SECTOR_SIZE )
		{
			if( !VALIDATE_ALIGN( pFileSource->fptr, sizeof( uint32_t ) ) )
				break;
			if( !VALIDATE_ALIGN( pFileDestination->fptr, sizeof( uint32_t ) ) )
				break;
			// !!! ���������� � ����� � ���, ��� ������ ������� ������ ����� FatFS �������� � ���������
			// !!! ��������� � SD �� ��������� ��������, ��� ����� �������� � ������ SD � �������������
			// !!! ������� ������. ��� STM32l4xx �� ������� �������� ����� SDMMC.DMA � ������������ ������,
			// !!! �.�. ������ � ������������� ������� ����� ���� ��������!
			// !!! ��� ������ ����� ��������, ������ ������������ ����� ������������� ����������� ����� FatFS � ������ �� ����������
		}

		while( BytesToCopy )
		{
			uint32_t BytesReaded, BytesWrited;
			uint32_t BytesToRead = ( ( BytesToCopy <= BufferSize ) ? BytesToCopy : BufferSize );
			if( FR_OK != ( FResult = f_read( pFileSource, pBuffer, BytesToRead, &BytesReaded ) ) )
				break;
			if( FR_OK != ( FResult = f_write( pFileDestination, pBuffer, BytesReaded, &BytesWrited ) ) )
				break;
			if( BytesReaded != BytesWrited )
			{
				FResult = FR_INT_ERR;
				break;
			}
			assert_param( BytesWrited <= BytesToCopy );
			BytesToCopy -= BytesWrited;	
		}
		if( FR_OK != FResult )
			break;
		// �������� ����� ���� �� ������� ������� - ���� �� ������� ��� ������ ���������� �������
		if( FR_OK != ( FResult = f_truncate( pFileDestination ) ) )
			break;
		if( 0 != BytesToCopy )
			FResult = FR_INT_ERR;
	} while( 0 );
	if( NULL != pBuffer )
		SKLP_Memory_TmpBufferRelease( false );
	return FResult;
}

// �������� ������ ����� � �������� ���������� ����� �� ������.
// ������������ ��� ������� ����� ������������, ����� �� �������� ��������� ��������.
// pFile		- ����� ��������� ��������� �����, ���������� �������
// pFileName	- ��� ��������� �����
// ResultSize	- ��������� ������ ����������� �����
// � ���������� pFile ����� �������, ������ � ��������� � �����
FRESULT FileSystem_FileTrancateHead( FIL *pFile, char *pFileName, uint32_t ResultSize )
{
	FRESULT FResult;
	do
	{
		if( ( NULL == pFileName ) || ( NULL == pFile ) || ( NULL == pFile->fs ) )
		{	FResult = FR_INVALID_PARAMETER;		break; }		// ������������ �������� ��� ���� ������
		if( pFile->fsize < ResultSize )
		{	FResult = FR_OK;					break; }		// ���� � ��� �������� �������, ��� ���������
		if( FR_OK != ( FResult = FileSystem_CheckFSReady( ) ) )
			break;
		if( FR_OK == FileSystem_CheckFileOpen( &FileTmp ) )
			if( FR_OK != ( FResult = f_close( &FileTmp ) ) )	// ��� �������� ������������ ��������� ����, ������� ������ ������ ���� �������� ����� ���������� ���������� ��������
				break;											// ���� �� ��� ������, ��� ��������� ������ ��� ������� ��� ��������

		char aFileTmpName[14] = "tmp.bin";
		if( FR_OK != ( FResult = f_open( &FileTmp, aFileTmpName, FA_WRITE | FA_OPEN_ALWAYS ) ) )
			break;												// �� ������� ������� ��������� ����
		if( FR_OK != ( FResult = f_lseek( pFile, pFile->fsize - ResultSize ) ) )
			break;												// �� ������� ���������� �������� ����
		if( FR_OK != ( FResult = FileSystem_FileCopy( pFile, &FileTmp, ResultSize ) ) )
			break;												// �� ������� ���������� �����������
		if( FR_OK != ( FResult = f_close( pFile ) ) )
			break;
		if( FR_OK != ( FResult = f_close( &FileTmp ) ) )
			break;
		if( FR_OK != ( FResult = f_unlink( pFileName ) ) )
			break;
		if( FR_OK != ( FResult = f_rename( aFileTmpName, pFileName ) ) )
			break;
		if( FR_OK != ( FResult = f_open( pFile, pFileName, FA_WRITE | FA_OPEN_ALWAYS | FA_READ ) ) )
			break;
		if( FR_OK != ( FResult = f_lseek( pFile, pFile->fsize ) ) )
			break;
	} while( 0 );
	if( FR_OK == FileSystem_CheckFileOpen( &FileTmp ) )
		f_close( &FileTmp );
	if( ( FR_DISK_ERR == FResult ) || ( FR_NOT_READY == FResult ) )
	{
		ATOMIC_WRITE( SKLP_MemoryFlags.SD0_ErrorMemWrite, 1 );
		FileSystem_Reinit( pFileName );
	}
	return FResult;
}

// ���������� ������� ������ ������� ��� fatfs (��-��������� ��������� � diskio.c � ���������� 0)
DWORD get_fattime( void )
{
	SKLP_Time_t SKLP_Time_Safe;
	ATOMIC_WRITE( SKLP_Time_Safe, SKLP_Time );
	DosDateTime_t DosDateTime = Time_SKLP2DOS( &SKLP_Time_Safe.Time6 );
	return *( DWORD * ) &DosDateTime;
}

// ���������� ������� ������� ��������� �����
FRESULT FileSystem_FilePositionSave( FIL *pFile, FIL_Position_t *pFilePosition )
{
	FRESULT FResult = FR_INVALID_PARAMETER;
	do
	{
		if( ( NULL == pFile ) || ( NULL == pFilePosition ) )
			break;
		if( FR_OK != ( FResult = FileSystem_CheckFileOpen( pFile ) ) )
			break;
		FIL_Position_t FilePositionTmp;
		FilePositionTmp.fptr		= pFile->fptr;
		FilePositionTmp.fsize		= pFile->fsize;
		FilePositionTmp.sclust		= pFile->sclust;
		FilePositionTmp.clust		= pFile->clust;
		FilePositionTmp.dsect		= pFile->dsect;
		FilePositionTmp.CheckSumm	= pFile->fptr + pFile->fsize + pFile->sclust + pFile->clust + pFile->dsect;
		ATOMIC_WRITE( *pFilePosition, FilePositionTmp );
		FResult = FR_OK;
	} while( 0 );
	return FResult;
}

// �������������� ������� ������� ��������� �����
FRESULT FileSystem_FilePositionRestore( FIL *pFile, FIL_Position_t *pFilePosition )
{
	FRESULT FResult = FR_INVALID_PARAMETER;
	do
	{
		if( ( NULL == pFile ) || ( NULL == pFilePosition ) )
			break;
		if( FR_OK != ( FResult = FileSystem_CheckFileOpen( pFile ) ) )
			break;
		FResult = FR_INVALID_OBJECT;
//		if( 0 != ( pFilePosition->fptr ) % FS_SECTOR_SIZE )
//			break;
		uint32_t CheckSumm = pFilePosition->fptr + pFilePosition->fsize + pFilePosition->sclust + pFilePosition->clust + pFilePosition->dsect;
		if( CheckSumm != pFilePosition->CheckSumm )
			break;		// ��������� ����������� ����������� ������
		if( pFile->sclust != pFilePosition->sclust )
			break;		// ������������ ���������� �������� � �������� ����� � ��� ����������� �������
		if( pFile->fsize < pFilePosition->fsize )
			break;		// ������� ������ ����� ������, ��� ��� ��� ���������� �������
		pFile->fptr 	= pFilePosition->fptr;
		pFile->clust	= pFilePosition->clust;
		pFile->dsect	= pFilePosition->dsect;
		FResult = FR_OK;
	} while( 0 );
	return FResult;
}

