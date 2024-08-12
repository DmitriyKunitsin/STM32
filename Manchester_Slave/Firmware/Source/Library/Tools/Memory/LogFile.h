// LogFile.h
// ќрганизаци€ логгировани€
#ifndef	LOGFILE_H
#define	LOGFILE_H

#include "FileSystem.h"
#include "MemoryThread.h"

//#define LOGFILE_FILESYSTEM_TIMEOUT		portMAX_DELAY
//#define LOGFILE_READ_TIMEOUT			( 3000 * configTICK_RATE_HZ )
#define	LOGFILE_WRITEFLAGS_FORCESYNC	( 1 << 0 )	// необходимо синхронизировать файл после записи в него (доп. нагрузка на FS, но гарантия обновления размера файла)
#define	LOGFILE_WRITEFLAGS_FORCEALIGN	( 1 << 1 )	// необходимо выравнивать позицию файла по размеру записи
#define	LOGFILE_WRITEFLAGS_ALL		( LOGFILE_WRITEFLAGS_FORCESYNC | LOGFILE_WRITEFLAGS_FORCEALIGN )

typedef uint32_t LogFileTag_t;

typedef struct LogFileHdl_struct
{
	FIL			*pFile;
	char /*const*/	*pFileName;
//	int			*pFileErrorCount;
	int			ErrorCount;
} LogFileHdl_t;

typedef struct LogFileMessage_struct
{
	MemoryThreadMessageHeader_t	Header;
	LogFileHdl_t const			*pLogFileHdl;
} LogFileMessage_t;

//bool LogFile_ValidateHdl( LogFileHdl_t const *pLogFileHdl );
FRESULT LogFile_ValidateHdl( LogFileHdl_t *pLogFileHdl );
FRESULT LogFile_WriteRecord( LogFileHdl_t *pLogFileHdl, char const *pRecord, uint32_t const RecordSize );
FRESULT LogFile_WriteRecordExt( LogFileHdl_t *pLogFileHdl, void const *pRecord, uint32_t const RecordSize, uint32_t Flags, FIL_Position_t *pFilePosition );
//void LogFile_ClearCB( MemoryThreadMessage_t *pMessage );
FRESULT LogFile_Clear( LogFileHdl_t *pLogFileHdl );
FRESULT LogFile_Read( LogFileHdl_t *pLogFileHdl, uint32_t FilePos, void const *pBuffer, uint32_t const BytesToRead, uint32_t *pBytesReaded );
FRESULT LogFile_GetFileInfo( char *pFileName, FILINFO *pFileInfo );
FRESULT LogFile_Open( LogFileHdl_t *pLogFileHdl );

// #########################################
// #########################################
// !!! “ест!
// !!! ƒобавил работу с диском напрямую, в обход FatFS,
// !!! но через драйвер этого диска, реализованый в ff_gen_drv.c.
// !!! ѕеренести бы в отдельный модуль.
#include "ff_gen_drv.h"			// Diskio_drvTypeDef
DRESULT DiskIODriver_GetSectorsCount( Diskio_drvTypeDef *pDiskIODriver, uint32_t *pDiskSectorsCount );
DRESULT DiskIODriver_ReadSectors( Diskio_drvTypeDef *pDiskIODriver, uint8_t *pBuffer, uint32_t SectorStart, uint8_t SectorsCount );


#endif	// LOGFILE_H

