// FileSystem.h
// Вспомогательные функции файловой системы
#ifndef	FILESYSTEM_H
#define	FILESYSTEM_H
#include "FF.h"					// FAT FS
#include "FreeRTOS.h"			// TickType_t
#include <stdbool.h>

// РЯд функций оптимизирован под стандартный размер сектора
#define	FS_SECTOR_SIZE				512

// Рекомендованный максимальный размер перемотки файла
#ifndef	FS_SEEK_SIZE_MAX
#define FS_SEEK_SIZE_MAX		( 64 * 1024 * 1024 )	// 0.5 с?
#endif

// Структура длЯ сохранениЯ текущей позиции файла
typedef struct FIL_Position_struct
{
	// ПолЯ, сохранЯемые из стуктуры FIL
	DWORD	fptr;			/* File read/write pointer (Zeroed on file open) */
	DWORD	fsize;			/* File size */
	DWORD	sclust;			/* File start cluster (0:no cluster chain, always 0 when fsize is 0) */
	DWORD	clust;			/* Current cluster of fpter (not valid when fprt is 0) */
	DWORD	dsect;			/* Sector number appearing in buf[] (0:invalid) */
	// КонтрольнаЯ сумма
	DWORD	CheckSumm;	// [24]
} FIL_Position_t;

// Опциональные коллбеки перед и после форматирования
void FileSystem_FormatAfterCallback( void );
void FileSystem_FormatBeforeCallback( void );


// Запуск переинициализации файловой системы
//void FileSystem_Reinit( FRESULT Reason, char *pFileneame );
//void FileSystem_Reinit( void );
//#define	FileSystem_Reinit( __REASON__ )	FileSystem_ReinitExt( ( __REASON__ ),  __FILE__, __LINE__, __FUNCTION__ )
//void FileSystem_ReinitExt(  char * pReason, const char * pSourceFile, uint32_t Line, const char *pFunc );
void FileSystem_Reinit( char * pReason );

// Найти в директории файл с самой новой датой
FRESULT FileSystem_FindLastFile( const TCHAR *pFileDir, FILINFO *pResultFileInfo );

// Получить информацию о файле
FRESULT FileSystem_GetFileInfo( const TCHAR *pFileName, FILINFO *pResultFileInfo );

// Найти по базовому имени файл данных с максимальным номером, и вернуть его номер
FRESULT FileSystem_FindLastEnumeratedFile( const TCHAR *pFileDir, const TCHAR *pBaseName, const TCHAR *pExtension, int32_t *pResultNumber, FILINFO *pResultFileInfo );

// Создать новое имя нумерованного файла на основе базового имени, расширения и номера
FRESULT FileSystem_CreateEnumeratedFileName( const TCHAR *pBaseName, const TCHAR *pExtension, int32_t Number, TCHAR *pResultName );

// Удалить все нумерованные файлы в указанной директории
FRESULT FileSystem_DeleteEnumeratedFiles( const TCHAR *pFileDir, const TCHAR *pBaseName, const TCHAR *pExtension, int32_t *pDeletedFilesCount );

// Проверка работоспособности файловой системы
FRESULT FileSystem_TestFS( char *pFileName );

// Проверка, что указанный файл уже открыт
FRESULT FileSystem_CheckFileOpen( FIL *pFile );

// Перемотать файл в конец
FRESULT FileSystem_SeekToEOF( FIL *pFile );

// Перемотать файл в конец, пошагово, с сохранением промежуточных позиций
FRESULT FileSystem_SeekToEOF_Incremental( FIL *pFile, FIL_Position_t *pFilePosition, uint32_t SeekSizeMax );

// Перемотать файл относительно конца
FRESULT FileSystem_SeekFromEOF( FIL *pFile, int32_t OffsetFromEOF );

// Проверка, что файловая система готова к работе
FRESULT FileSystem_CheckFSReady( void );
//bool FileSystem_CheckFSReadyBool( void )	{ return ( FR_OK == FileSystem_CheckFSReady( ) ); } 
bool FileSystem_CheckFSReadyBool( void );

// Дождаться готовности файловой системы
FRESULT FileSystem_WaitFSReady( TickType_t TicksToWait );

// Вернуть свободное место
FRESULT FileSystem_GetFreeSpace( uint64_t *pFreeSpace );

// Скопировать из одного файла в другой.
FRESULT FileSystem_FileCopy( FIL *pFileSource, FIL *pFileDestination, uint32_t BytesToCopy );

// Отрезать начало файла и сместить оставшуюсЯ часть на начало.
FRESULT FileSystem_FileTrancateHead( FIL *pFile, char *pFileName, uint32_t ResultSize );

// Сохранение текущей позиции открытого файла
FRESULT FileSystem_FilePositionSave( FIL *pFile, FIL_Position_t *pFilePosition );

// Восстановление текущей позиции открытого файла
FRESULT FileSystem_FilePositionRestore( FIL *pFile, FIL_Position_t *pFilePosition );

#endif	// FILESYSTEM_H


