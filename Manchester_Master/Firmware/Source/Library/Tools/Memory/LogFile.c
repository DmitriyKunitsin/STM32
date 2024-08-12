// Logger.c
#include "ProjectConfig.h"		// конфиг платформы, конфиг задачи.
#include "stm32xxxx_hal.h"		// дрова периферии
#include "MemoryThread.h"
#include "LogFile.h"
#include "FreeRTOS.h"
#include "Task.h"
#include "Semphr.h"
#include "string.h"
#include "Logger.h"				// Logger_WriteRecord()
#include "SKLP_Service.h"
#include <stdio.h>				// snprintf()

FRESULT LogFile_ValidateHdl( LogFileHdl_t *pLogFileHdl )
{
	if( ( NULL == pLogFileHdl ) ||
		( NULL == pLogFileHdl->pFile ) ||
		( NULL == pLogFileHdl->pFileName ) )
		return FR_INVALID_PARAMETER;
	return FR_OK;
}

FRESULT LogFile_Open( LogFileHdl_t *pLogFileHdl )
{
	FRESULT FResult;
	do
	{
		if( FR_OK != ( FResult = LogFile_ValidateHdl( pLogFileHdl ) ) )
			break;
		if( FR_OK != ( FResult = FileSystem_CheckFSReady( ) ) )
			break;
		if( FR_OK != ( FResult = FileSystem_CheckFileOpen( pLogFileHdl->pFile ) ) )
		{
//			if( FR_OK != ( FRESULT ) ( pLogFileHdl->pFile->err ) )
//				if( FR_OK != ( FResult = f_close( pLogFileHdl->pFile ) ) )
//					break;
			if( FR_OK != ( FResult = f_open( pLogFileHdl->pFile, pLogFileHdl->pFileName, FA_READ | ( SKLP_MemoryFlags.SD0_ErrorMemWrite ? 0 : ( FA_WRITE | FA_OPEN_ALWAYS ) ) ) ) )
				break;
		}
	} while( 0 );
	return FResult;
}

FRESULT LogFile_WriteRecord( LogFileHdl_t *pLogFileHdl, char const *pRecord, uint32_t const RecordSize )
{
	return LogFile_WriteRecordExt( pLogFileHdl, pRecord, RecordSize, LOGFILE_WRITEFLAGS_FORCESYNC, NULL );
}

// pRecord			- Адрес буфера под запись. ДопускаетсЯ NULL, при этом файл будет открыт и перемотан, а запись производитсЯ не будет.
// RecordSize		- Размер буфера под запись. ДопускаетсЯ 0, при этом файл будет открыт и перемотан, а запись производитсЯ не будет.
// Flags			- Дополнительные опции:
// - LOGFILE_WRITEFLAGS_FORCESYNC	необходимо синхронизировать файл после записи в него
// - LOGFILE_WRITEFLAGS_FORCEALIGN	перед записью необходимо выровнЯть позицию по размеру записи
// pFilePosition	- ОпциональнаЯ сохранЯемаЯ позициЯ файла, длЯ ускоренной перемотки больших файлов
FRESULT LogFile_WriteRecordExt( LogFileHdl_t *pLogFileHdl, void const *pRecord, uint32_t const RecordSize, uint32_t Flags, FIL_Position_t *pFilePosition )
{
	assert_param( FR_OK == LogFile_ValidateHdl( pLogFileHdl ) );
	assert_param( 0 == ( Flags & ~LOGFILE_WRITEFLAGS_ALL ) );
	FRESULT FResult;
	do
	{
		// Проверить готовность файловой системы. Открыть файл на запись, если еще не открыт.
		if( FR_OK != ( FResult = LogFile_Open( pLogFileHdl ) ) )
			break;
		FIL *pFile = pLogFileHdl->pFile;
		// Проверить, что разрешена запись в файл
		if( 0 == ( pFile->flag & FA_WRITE ) )
		{
			FResult = FR_WRITE_PROTECTED;
			break;
		}
		// Перемотать в конец, если не перемотан
		if( NULL == pFilePosition )
		{
			if( FR_OK != ( FResult = FileSystem_SeekToEOF( pFile ) ) )
				break;
		}
		else
		{
			FileSystem_FilePositionRestore( pFile, pFilePosition );
			if( FR_OK != ( FResult = FileSystem_SeekToEOF_Incremental( pFile, pFilePosition, FS_SEEK_SIZE_MAX ) ) )
				break;
		}
		// Если запись пустаЯ, на этом закончить
		if( ( NULL == pRecord ) || ( 0 == RecordSize ) )
			break;
		// При необходимости, произвести выравнивание
		if( LOGFILE_WRITEFLAGS_FORCEALIGN & Flags )
		{
			uint32_t Misalign = pFile->fsize % RecordSize;
			if( 0 != Misalign )
			{	// Выравнивание нарушено! ВыровнЯть.
				if( FR_OK != ( FResult = FileSystem_SeekFromEOF( pFile, RecordSize - Misalign ) ) )
					break;
			}
			assert_param( 0 == ( pFile->fptr % RecordSize ) );
		}
		// Произвести запись
		uint32_t BytesWritten;
		if( FR_OK != ( FResult = f_write( pFile, pRecord, RecordSize, &BytesWritten ) ) )
			break;
		if( BytesWritten != RecordSize )
		{
			FResult = FR_INT_ERR;
			break;
		}
		// При необходимости, произвести синхронизацию
		if( LOGFILE_WRITEFLAGS_FORCESYNC & Flags )
		{
			if( FR_OK != ( FResult = f_sync( pFile ) ) )
				break;
		}
		// При необходимости, сохранить текущую позицию файла
		FileSystem_FilePositionSave( pFile, pFilePosition );
	} while( 0 );

	// Проверить результат операции записи
	if( FR_OK != FResult )
	{	// Что-то пошло не так
		pLogFileHdl->ErrorCount++;
		if( ( FR_DISK_ERR == FResult ) || ( FR_NOT_READY == FResult ) )
		{	// При ошибках файловой системы, произвести переинициализацию файловой системы
			ATOMIC_WRITE( SKLP_MemoryFlags.SD0_ErrorMemWrite, 1 );
			if ( NULL == pRecord )
				pRecord = "???";
			FileSystem_Reinit( ( char * ) pRecord );
		}
	}
	return FResult; 
}

FRESULT LogFile_Clear( LogFileHdl_t *pLogFileHdl )
{
	assert_param( NULL != pLogFileHdl );
	FRESULT FResult;
	TickType_t TimestampStart = xTaskGetTickCount( );
	uint32_t FileSizeSaved = 0;
	do
	{
		if( FR_OK != ( FResult = LogFile_ValidateHdl( pLogFileHdl ) ) )
			break;
		if( FR_OK != ( FResult = FileSystem_CheckFSReady( ) ) )
			break;
		if( FR_OK == FileSystem_CheckFileOpen( pLogFileHdl->pFile ) )
		{
			FileSizeSaved = pLogFileHdl->pFile->fsize;
			if( FR_OK != ( FResult = f_close( pLogFileHdl->pFile ) ) )
				break;
		}
		else
		{
			FILINFO FileInfo;
			if( FR_OK != ( FResult = FileSystem_GetFileInfo( pLogFileHdl->pFileName, &FileInfo ) ) )
				break;
			FileSizeSaved = FileInfo.fsize;
		}
		if( SKLP_MemoryFlags.SD0_ErrorMemWrite  )
		{
			FResult = FR_WRITE_PROTECTED;
			break;
		}
		if( FR_NO_FILE == ( FResult = f_unlink( pLogFileHdl->pFileName ) ) )
			break;
	} while( 0 );

	// Сформировать сообщение по результату удалениЯ файла
	char aLoggerRecord[80];
	assert_param( MemoryThread_SprintfMutexTake( 5 ) );
	if( FR_OK == FResult )
	{	// Удаление файла произведено успешно
		float TimeExpended_s = ( xTaskGetTickCount( ) - TimestampStart ) / ( float ) configTICK_RATE_HZ;
		snprintf( aLoggerRecord, sizeof( aLoggerRecord ), "Erasing file %s (%lu bytes) completed for %lu ms.",
														pLogFileHdl->pFileName, FileSizeSaved, ( uint32_t ) ( TimeExpended_s * 1000 ) );
	}
	else if( FR_NO_FILE == FResult )
	{	// Файл отсутствует (не считаетсЯ за ошибку)
		snprintf( aLoggerRecord, sizeof( aLoggerRecord ), "Erasing file %s skipped because of file not exist.", pLogFileHdl->pFileName );
		FResult = FR_OK;
	}
	else
	{	// При удалении возникла ошибка
		pLogFileHdl->ErrorCount++;
		snprintf( aLoggerRecord, sizeof( aLoggerRecord ), "Error while erasing file %s!", pLogFileHdl->pFileName );
	}
	MemoryThread_SprintfMutexGive( );
	assert_param( Logger_WriteRecord( aLoggerRecord, LOGGER_FLAGS_APPENDTIMESTAMP ) );

	if( ( FR_DISK_ERR == FResult ) || ( FR_NOT_READY == FResult ) )
	{	// При возникновении ошибок доступа к FS или SD, произвести переинициализацию файловой системы
		ATOMIC_WRITE( SKLP_MemoryFlags.SD0_ErrorMemWrite, 1 );
		FileSystem_Reinit( pLogFileHdl->pFileName );
	}
	
	return FResult;
}

typedef struct LogFileMessage_FileRead_struct
{
	MemoryThreadMessageHeader_t	Header;
	LogFileHdl_t 				*pLogFileHdl;
	uint32_t					FilePos;
	uint8_t	const				*pBuffer;
	uint32_t					BytesToRead;
	uint32_t					*pBytesReaded;
	FRESULT						*pFResult;
} LogFileMessage_FileRead_t;

static void LogFile_ReadCB( MemoryThreadMessage_t *pMessage )
{
	assert_param( NULL != pMessage );
	assert_param( LogFile_ReadCB == pMessage->Header.xCallback );
	LogFileMessage_FileRead_t *pFileReadMessage = ( LogFileMessage_FileRead_t * ) pMessage;
	FRESULT FResult = FR_INVALID_PARAMETER;
	LogFileHdl_t *pLogFileHdl = pFileReadMessage->pLogFileHdl;
	// Массив длЯ хранениЯ заголовка структуры FIL с целью восстановлениЯ контекста файла при сбое
	static uint8_t aFIL_HeaderBufferSaved[ sizeof( *pLogFileHdl->pFile ) - sizeof( pLogFileHdl->pFile->buf ) ];
	do
	{
		if( FR_OK != ( FResult = LogFile_Open( pLogFileHdl ) ) )
			break;
		if( pLogFileHdl->pFile->fptr == pLogFileHdl->pFile->fsize )
			if( FR_OK != ( FResult = f_sync( pLogFileHdl->pFile ) ) )		// файл перемотан в конец, скорее всего туда была произведена запись
				break;														// на всЯкий случай, синхронизировать файл, если он не был синхронизирован после последней записи
		if( pLogFileHdl->pFile->fptr != pFileReadMessage->FilePos )
			if( FR_OK != ( FResult = f_lseek( pLogFileHdl->pFile, pFileReadMessage->FilePos ) ) )
				break;
		// Сохранить контекст файла перед потенциально аварийной операцией
		memcpy( aFIL_HeaderBufferSaved, pLogFileHdl->pFile, sizeof( aFIL_HeaderBufferSaved ) );
		static volatile int Dummy = 0;
		if( 32768 == pFileReadMessage->FilePos )
			Dummy = 1;
		FResult = f_read( pLogFileHdl->pFile, ( void * ) pFileReadMessage->pBuffer, pFileReadMessage->BytesToRead, pFileReadMessage->pBytesReaded );
	} while( 0 );
	if( FR_OK != FResult )
	{
		pLogFileHdl->ErrorCount++;
// Убрал перемонтирование при невозможности считать блок
/*		if( ( FR_DISK_ERR == FResult ) || ( FR_NOT_READY == FResult ) )
		{
			ATOMIC_WRITE( SKLP_MemoryFlags.SD0_ErrorMemRead, 1 );
			FileSystem_Reinit( );
		}
*/

		// !! ВыЯвлены проблемы, когда часть секторов на SD битые. Попытка считываниЯ таких секторов вываливаетсЯ по таймауту.
		// !! При этом сохранЯетсЯ возможность чтениЯ других секторов в блоке, т.е. есть возможность считать хотЯ бы часть информации.
		// !! При использовании f_read() ошибка чтениЯ SD приводит к FR_DISK_ERR, файл блокируетсЯ, текущаЯ позициЯ и текущий кластер рассогласованы.
		// !! Если запрошенный блок был достаточно большой, либо попал на границу кластеров, то f_read() разбивает чтение
		// !! на несколько чтений. До возникновениЯ первой ошибки, позициЯ в файле будет нормально передвигатьсЯ, а количество
		// !! считанных байт увеличиватьсЯ.
		if( FR_DISK_ERR == FResult )
		{	// Восстановить ранее сохраненный контекст, т.к. при FR_DISK_ERR файл считаетсЯ битым и с ним нельзЯ штатно продолжать работу
			memcpy( pLogFileHdl->pFile, aFIL_HeaderBufferSaved, sizeof( aFIL_HeaderBufferSaved ) );
			assert_param( FR_OK == pLogFileHdl->pFile->err );	// подтвердить, что контекст возвращен в рабочее состоЯние
			// Рассчитать количество байт, которые осталось попытатьсЯ считать из запрошенного блока
			assert_param( *pFileReadMessage->pBytesReaded <= pFileReadMessage->BytesToRead );
			uint32_t BytesRemain = pFileReadMessage->BytesToRead - *pFileReadMessage->pBytesReaded;
			// Прочитать блок посекторно
			while( BytesRemain > 0 )
			{	
				uint32_t BytesToRead = ( ( BytesRemain > FS_SECTOR_SIZE ) ? FS_SECTOR_SIZE : BytesRemain );
				uint32_t BytesReaded;
				// ФункциЯ оптимизирована длЯ посекторного чтениЯ.
				// При необходимости вычитываниЯ произвольных областей, код необходимо переработать
				uint32_t FilePos = pLogFileHdl->pFile->fptr;
				assert_param( 0 == ( FilePos % FS_SECTOR_SIZE ) );
				// Проконтролировать, что чтение будет производитьсЯ с требуемой позиции
				uint32_t FilePosRequired = pFileReadMessage->FilePos + pFileReadMessage->BytesToRead - BytesRemain;
				if( FilePos != FilePosRequired )
				{
					if( FR_OK != ( FResult = f_lseek( pLogFileHdl->pFile, FilePosRequired ) ) )
						break;
					FilePos = pLogFileHdl->pFile->fptr;
					if( FilePos != FilePosRequired )
					{
						FResult = FR_INT_ERR;
						break;
					}
				}
				// Сохранить контекст файла перед потенциально аварийной операцией
				memcpy( aFIL_HeaderBufferSaved, pLogFileHdl->pFile, sizeof( aFIL_HeaderBufferSaved ) );
				FResult = f_read(	pLogFileHdl->pFile,
									( void * ) ( pFileReadMessage->pBuffer + ( pFileReadMessage->BytesToRead - BytesRemain ) ),
									BytesToRead, &BytesReaded );
				switch( FResult )
				{
				case FR_OK:			// Сектор считан штатно
					assert_param( BytesReaded == BytesToRead );
					*pFileReadMessage->pBytesReaded += BytesReaded;
					assert_param( BytesReaded <= BytesRemain );
					BytesRemain -= BytesReaded;
					break;
					
				case FR_DISK_ERR:	// Не удалось считать сектор из-за проблемы с SD. Перейти к чтению следующего сектора
					// Восстановить ранее сохраненный контекст, т.к. при FR_DISK_ERR файл считаетсЯ битым и с ним нельзЯ продолжать работу
					memcpy( pLogFileHdl->pFile, aFIL_HeaderBufferSaved, sizeof( aFIL_HeaderBufferSaved ) );
					assert_param( FR_OK == pLogFileHdl->pFile->err );
					// Прокрутить файл на следующий сектор
					FResult = f_lseek( pLogFileHdl->pFile, FilePos + BytesToRead );
					if( pLogFileHdl->pFile->fptr != ( FilePos + BytesToRead ) )
						FResult = FR_DISK_ERR;
					if( FR_OK != FResult )
						break;		// Не удалось прокрутить файл. Нарушение в коде или в FAT
					// Прокрутка файла удалась. Не увеличивать количество считанных байт, но уменьшить количество оставшихсЯ байт.
					assert_param( BytesToRead <= BytesRemain );
					BytesRemain -= BytesToRead;
					break;
					
				default:			// Произошла еще какаЯ-то ошибка. Завершить выполнение, вернуть эту ошибку.
					BytesRemain = 0;
					break;
				}
			}
		}
	}
	*pFileReadMessage->pFResult = FResult;
}

// Чтение файла
// ОперациЯ чтениЯ передаетсЯ в низкоприоритетную задачу MemoryThread_Task().
// ВызывающаЯ задача (в т.ч. приоритетнаЯ) ожидает завершениЯ операции чтениЯ.
FRESULT LogFile_Read( LogFileHdl_t *pLogFileHdl, uint32_t FilePos, void const *pBuffer, uint32_t const BytesToRead, uint32_t *pBytesReaded )
{
	FRESULT FResult = FR_INVALID_PARAMETER;
	do
	{
		if( FR_OK != LogFile_ValidateHdl( pLogFileHdl ) )
			break;
		if( NULL == pBuffer )
			break;
		
		LogFileMessage_FileRead_t Message;
		Message.Header.xCallback	= LogFile_ReadCB;
		Message.pLogFileHdl			= pLogFileHdl;
		Message.FilePos				= FilePos;
		Message.pBuffer				= pBuffer;
		Message.BytesToRead			= BytesToRead;
		Message.pBytesReaded		= pBytesReaded;
		Message.pFResult			= &FResult;			// получить результат операции чтениЯ в результат выполнениЯ функции

		assert_param( sizeof( Message ) <= sizeof( MemoryThreadMessage_t ) );
		assert_param( MemoryThread_AppendMessage( ( MemoryThreadMessage_t * ) &Message, MEMORYTHREAD_FLAGS_WAITFORCOMPLETE ) );	// дождатьсЯ завершениЯ операции чтениЯ
	} while( 0 );

	return FResult;
}

typedef struct LogFileMessage_FileGetInfo_struct
{
	MemoryThreadMessageHeader_t	Header;
	char 						*pFileName;
	FILINFO						*pFileInfo;
	FRESULT						*pFResult;
} LogFileMessage_FileGetInfo_t;

static void LogFile_GetFileInfoCB( MemoryThreadMessage_t *pMessage )
{
	assert_param( NULL != pMessage );
	assert_param( LogFile_GetFileInfoCB == pMessage->Header.xCallback );
	LogFileMessage_FileGetInfo_t *pFileMessage = ( LogFileMessage_FileGetInfo_t * ) pMessage;
	FRESULT FResult;
	do
	{
		FResult = FR_INVALID_PARAMETER;
		if( ( NULL == pFileMessage->pFileInfo ) || ( NULL == pFileMessage->pFResult ) || ( NULL == pFileMessage->pFileName ) )
			break;
		if( FR_OK != ( FResult = FileSystem_CheckFSReady( ) ) )
			break;
		if( FR_OK != ( FResult = FileSystem_GetFileInfo( pFileMessage->pFileName, pFileMessage->pFileInfo ) ) )
			break;
	} while( 0 );
	if( FR_OK != FResult )
	{
		if( ( FR_DISK_ERR == FResult ) || ( FR_NOT_READY == FResult ) )
		{
			ATOMIC_WRITE( SKLP_MemoryFlags.SD0_ErrorMemRead, 1 );
			FileSystem_Reinit( pFileMessage->pFileName );
		}
	}
	*pFileMessage->pFResult = FResult;
}

FRESULT LogFile_GetFileInfo( char *pFileName, FILINFO *pFileInfo )
{
	FRESULT FResult;
	do
	{
		FResult = FR_INVALID_PARAMETER;
		if( ( NULL == pFileName ) || ( NULL == pFileInfo ) )
			break;
		
		LogFileMessage_FileGetInfo_t Message;
		Message.Header.xCallback	= LogFile_GetFileInfoCB;
		Message.pFileName			= pFileName;
		Message.pFileInfo			= pFileInfo;
		Message.pFResult			= &FResult;

		assert_param( sizeof( Message ) <= sizeof( MemoryThreadMessage_t ) );
		assert_param( MemoryThread_AppendMessage( ( MemoryThreadMessage_t * ) &Message, MEMORYTHREAD_FLAGS_WAITFORCOMPLETE ) );	// дождатьсЯ завершениЯ операции
	} while( 0 );

	return FResult;
}


// #########################################
// #########################################
// !!! Тест!
// !!! Добавил работу с диском напрЯмую, в обход FatFS,
// !!! но через драйвер этого диска, реализованый в ff_gen_drv.c.
// !!! Перенести бы в отдельный модуль.
//
// Функции DiskIODriver_GetSectorsCount() DiskIODriver_ReadSectors() допускаетсЯ вызывать из любой задачи,
// при этом будет произведена транслЯциЯ вызова в MemoryThread_Task() через MemoryThread_AppendMessage(),
// и все обращениЯ к диску будут производитьсЯ уже из MemoryThread_Task().
// Таким образом, все обращениЯ к диску (через этот драйвер, или через файловую систему)
// производЯтсЯ через одну задачу и не пересекаютсЯ.
//
// !!! ПодразумеваетсЯ, что драйвер диска (напр., SD_Driver) уже подлинкован,
// !!! SD-карта смонтирована, т.е. SD-карта включена и инициализирована
// !!! (при этом FatFS при может быть и не обнаружена).

// Проверить инициализацию драйвера диска
static DRESULT DiskIODriver_Validate( Diskio_drvTypeDef *pDiskIODriver )
{
	DRESULT Result = RES_PARERR;
	do
	{
		if( NULL == pDiskIODriver )
			break;
		if( NULL == pDiskIODriver->disk_initialize )
			break;
		if( NULL == pDiskIODriver->disk_status )
			break;
		if( NULL == pDiskIODriver->disk_read )
			break;
#if	( _USE_WRITE == 1 )
		if( NULL == pDiskIODriver->disk_write )
			break;
#endif
#if	( _USE_IOCTL == 1 )
		if( NULL == pDiskIODriver->disk_ioctl )
			break;
#endif
		Result = RES_OK;
	} while( 0 );
	return Result;
}

// DiskIODriver_GetSectorsCount()
// Считать размер диска через служебный вызов disk_ioctl()
typedef struct DiskIODriverMessage_GetInfo_struct
{
	MemoryThreadMessageHeader_t	Header;
	Diskio_drvTypeDef			*pDiskIODriver;
	uint32_t 					*pDiskSectorsCount;
	DRESULT						*pDResult;
} DiskIODriverMessage_GetInfo_t;

static void DiskIODriver_GetSectorsCountCB( MemoryThreadMessage_t *pMessage )
{
	assert_param( NULL != pMessage );
	assert_param( DiskIODriver_GetSectorsCountCB == pMessage->Header.xCallback );
	DiskIODriverMessage_GetInfo_t *pDiskMessage = ( DiskIODriverMessage_GetInfo_t * ) pMessage;
	do
	{
		if( NULL == pDiskMessage->pDResult )
			break;
		*pDiskMessage->pDResult = RES_PARERR;
		if( NULL == pDiskMessage->pDiskSectorsCount )
			break;
		*pDiskMessage->pDResult = DiskIODriver_Validate( pDiskMessage->pDiskIODriver );
		if( RES_OK != *pDiskMessage->pDResult )
			break;
		
		// Получить готовность диска
		DSTATUS DiskStatus = pDiskMessage->pDiskIODriver->disk_status( );
		*pDiskMessage->pDResult = RES_ERROR;
		if( ( STA_NOINIT | STA_NODISK ) & DiskStatus )
			break;
		// Получить размер диска
		*pDiskMessage->pDResult = pDiskMessage->pDiskIODriver->disk_ioctl( GET_SECTOR_COUNT, pDiskMessage->pDiskSectorsCount );
	} while( 0 );
}

DRESULT DiskIODriver_GetSectorsCount( Diskio_drvTypeDef *pDiskIODriver, uint32_t *pDiskSectorsCount )
{
	DRESULT DResult;
	DiskIODriverMessage_GetInfo_t Message;
	Message.Header.xCallback	= DiskIODriver_GetSectorsCountCB;
	Message.pDiskIODriver		= pDiskIODriver;
	Message.pDiskSectorsCount	= pDiskSectorsCount;
	Message.pDResult			= &DResult;

	assert_param( sizeof( Message ) <= sizeof( MemoryThreadMessage_t ) );
	assert_param( MemoryThread_AppendMessage( ( MemoryThreadMessage_t * ) &Message, MEMORYTHREAD_FLAGS_WAITFORCOMPLETE ) );
	
	return DResult;
}

// DiskIODriver_ReadSectors()
// Считать блок секторов с диска через вызов disk_read()
typedef struct DiskIODriverMessage_ReadSectors_struct
{
	MemoryThreadMessageHeader_t	Header;
	Diskio_drvTypeDef			*pDiskIODriver;
	uint8_t						*pBuffer;
	uint32_t					SectorStart;
	uint8_t						SectorsCount;
	DRESULT						*pDResult;
} DiskIODriverMessage_ReadSectors_t;

static void DiskIODriver_ReadSectorsCB( MemoryThreadMessage_t *pMessage )
{
	assert_param( NULL != pMessage );
	assert_param( DiskIODriver_ReadSectorsCB == pMessage->Header.xCallback );
	DiskIODriverMessage_ReadSectors_t *pDiskMessage = ( DiskIODriverMessage_ReadSectors_t * ) pMessage;
	do
	{
		if( NULL == pDiskMessage->pDResult )
			break;
		*pDiskMessage->pDResult = RES_PARERR;
		if( NULL == pDiskMessage->pBuffer )
			break;
		*pDiskMessage->pDResult = DiskIODriver_Validate( pDiskMessage->pDiskIODriver );
		if( RES_OK != *pDiskMessage->pDResult )
			break;
		
		// Получить готовность диска
		DSTATUS DiskStatus = pDiskMessage->pDiskIODriver->disk_status( );
		*pDiskMessage->pDResult = RES_ERROR;
		if( ( STA_NOINIT | STA_NODISK ) & DiskStatus )
			break;
		// Прочитать область
		*pDiskMessage->pDResult = pDiskMessage->pDiskIODriver->disk_read( pDiskMessage->pBuffer, pDiskMessage->SectorStart, pDiskMessage->SectorsCount );
	} while( 0 );
}

DRESULT DiskIODriver_ReadSectors( Diskio_drvTypeDef *pDiskIODriver, uint8_t *pBuffer, uint32_t SectorStart, uint8_t SectorsCount )
{
	DRESULT DResult;
	DiskIODriverMessage_ReadSectors_t Message;
	Message.Header.xCallback	= DiskIODriver_ReadSectorsCB;
	Message.pDiskIODriver		= pDiskIODriver;
	Message.pBuffer				= pBuffer;
	Message.SectorStart			= SectorStart;
	Message.SectorsCount		= SectorsCount;
	Message.pDResult			= &DResult;

	assert_param( sizeof( Message ) <= sizeof( MemoryThreadMessage_t ) );
	assert_param( MemoryThread_AppendMessage( ( MemoryThreadMessage_t * ) &Message, MEMORYTHREAD_FLAGS_WAITFORCOMPLETE ) );
	
	return DResult;
}


