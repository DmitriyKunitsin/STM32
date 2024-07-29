// FileSystem.h
// ��������������� ������� �������� �������
#ifndef	FILESYSTEM_H
#define	FILESYSTEM_H
#include "FF.h"					// FAT FS
#include "FreeRTOS.h"			// TickType_t
#include <stdbool.h>

// ��� ������� ������������� ��� ����������� ������ �������
#define	FS_SECTOR_SIZE				512

// ��������������� ������������ ������ ��������� �����
#ifndef	FS_SEEK_SIZE_MAX
#define FS_SEEK_SIZE_MAX		( 64 * 1024 * 1024 )	// 0.5 �?
#endif

// ��������� ��� ���������� ������� ������� �����
typedef struct FIL_Position_struct
{
	// ����, ����������� �� �������� FIL
	DWORD	fptr;			/* File read/write pointer (Zeroed on file open) */
	DWORD	fsize;			/* File size */
	DWORD	sclust;			/* File start cluster (0:no cluster chain, always 0 when fsize is 0) */
	DWORD	clust;			/* Current cluster of fpter (not valid when fprt is 0) */
	DWORD	dsect;			/* Sector number appearing in buf[] (0:invalid) */
	// ����������� �����
	DWORD	CheckSumm;	// [24]
} FIL_Position_t;

// ������������ �������� ����� � ����� ��������������
void FileSystem_FormatAfterCallback( void );
void FileSystem_FormatBeforeCallback( void );


// ������ ����������������� �������� �������
//void FileSystem_Reinit( FRESULT Reason, char *pFileneame );
//void FileSystem_Reinit( void );
//#define	FileSystem_Reinit( __REASON__ )	FileSystem_ReinitExt( ( __REASON__ ),  __FILE__, __LINE__, __FUNCTION__ )
//void FileSystem_ReinitExt(  char * pReason, const char * pSourceFile, uint32_t Line, const char *pFunc );
void FileSystem_Reinit( char * pReason );

// ����� � ���������� ���� � ����� ����� �����
FRESULT FileSystem_FindLastFile( const TCHAR *pFileDir, FILINFO *pResultFileInfo );

// �������� ���������� � �����
FRESULT FileSystem_GetFileInfo( const TCHAR *pFileName, FILINFO *pResultFileInfo );

// ����� �� �������� ����� ���� ������ � ������������ �������, � ������� ��� �����
FRESULT FileSystem_FindLastEnumeratedFile( const TCHAR *pFileDir, const TCHAR *pBaseName, const TCHAR *pExtension, int32_t *pResultNumber, FILINFO *pResultFileInfo );

// ������� ����� ��� ������������� ����� �� ������ �������� �����, ���������� � ������
FRESULT FileSystem_CreateEnumeratedFileName( const TCHAR *pBaseName, const TCHAR *pExtension, int32_t Number, TCHAR *pResultName );

// ������� ��� ������������ ����� � ��������� ����������
FRESULT FileSystem_DeleteEnumeratedFiles( const TCHAR *pFileDir, const TCHAR *pBaseName, const TCHAR *pExtension, int32_t *pDeletedFilesCount );

// �������� ����������������� �������� �������
FRESULT FileSystem_TestFS( char *pFileName );

// ��������, ��� ��������� ���� ��� ������
FRESULT FileSystem_CheckFileOpen( FIL *pFile );

// ���������� ���� � �����
FRESULT FileSystem_SeekToEOF( FIL *pFile );

// ���������� ���� � �����, ��������, � ����������� ������������� �������
FRESULT FileSystem_SeekToEOF_Incremental( FIL *pFile, FIL_Position_t *pFilePosition, uint32_t SeekSizeMax );

// ���������� ���� ������������ �����
FRESULT FileSystem_SeekFromEOF( FIL *pFile, int32_t OffsetFromEOF );

// ��������, ��� �������� ������� ������ � ������
FRESULT FileSystem_CheckFSReady( void );
//bool FileSystem_CheckFSReadyBool( void )	{ return ( FR_OK == FileSystem_CheckFSReady( ) ); } 
bool FileSystem_CheckFSReadyBool( void );

// ��������� ���������� �������� �������
FRESULT FileSystem_WaitFSReady( TickType_t TicksToWait );

// ������� ��������� �����
FRESULT FileSystem_GetFreeSpace( uint64_t *pFreeSpace );

// ����������� �� ������ ����� � ������.
FRESULT FileSystem_FileCopy( FIL *pFileSource, FIL *pFileDestination, uint32_t BytesToCopy );

// �������� ������ ����� � �������� ���������� ����� �� ������.
FRESULT FileSystem_FileTrancateHead( FIL *pFile, char *pFileName, uint32_t ResultSize );

// ���������� ������� ������� ��������� �����
FRESULT FileSystem_FilePositionSave( FIL *pFile, FIL_Position_t *pFilePosition );

// �������������� ������� ������� ��������� �����
FRESULT FileSystem_FilePositionRestore( FIL *pFile, FIL_Position_t *pFilePosition );

#endif	// FILESYSTEM_H


