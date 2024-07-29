// FileSystem.c
// Вспомогательные функции файловой системы
#include "ProjectConfig.h"		// конфиг платформы, конфиг задачи.
#include "stm32xxxx_hal.h"		// дрова периферии
#include "FileSystem.h"			// Родной
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


#define	FILESYSTEM_REINIT_TIMEOUT_ms	500		// таймаут между попытками смонтировать файловую систему при отказе

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
// Отработанный вариант - реализациЯ инициализации и перезагрузки файловой системы через задачу FileSystem_Task()
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
// Задача по обслуживанию файловой системы
// - только SD-card
// - инициализация+тест
// - форматирование
static void FileSystem_Task( void *pParameters )
{
	static volatile FRESULT FResult;
	EventBits_t EventBitsResult;

	// Перейти к инициализации файловой системы
	assert_param( NULL != EventGroup_System );
	( void ) xEventGroupSetBits( EventGroup_System, EVENTSYSTEM_FS_INIT );

	DWT_AppendTimestampTag( "FS Start" );
	while( 1 )
	{	// Ожидать запросов на операцию форматированиЯ или инициализации
		EventBitsResult = xEventGroupWaitBits( EventGroup_System, EVENTSYSTEM_FS_INIT | EVENTSYSTEM_FS_FORMAT, pdFALSE, pdFALSE, portMAX_DELAY );
		// Сбросить флаг готовности файловой системы. Флаг инициализации пока остаетсЯ.
		xEventGroupClearBits( EventGroup_System, EVENTSYSTEM_FS_READY );
		// Проверить, какой запрос был адресован к задаче
		if( EventBitsResult & EVENTSYSTEM_FS_INIT )
		{	// Пришел запрос на инициализацию или переинициализацию файловой системы
			// Сбросить флаг форматированиЯ
			xEventGroupClearBits( EventGroup_System, EVENTSYSTEM_FS_FORMAT );
			// Перелинковать драйвер
			// !!! Ранее сначало было размонтирование, но прежде необходимо линковать драйвер,
			// !!! т.к. f_mount(), даже длЯ размонтированиЯ, требует инициалимзированный SDPath
			FATFS_UnLinkDriver( SDPath );
			assert_param( 0 == FATFS_LinkDriver( &SD_Driver, SDPath ) );
			// Размонтировать SD-карту, если ранее была смонтирована
			FResult = f_mount( NULL, ( TCHAR const * ) SDPath, 1 );	
			assert_param( FR_OK == FResult );

			// Произвести монтирование SD-карты.
			// Приводит к вызову BSP_SD_Init(), который инициализирует периферию, сбрасывает питание и шлет инициализирующие посылки в SD-карту
//			LedTimerChangePeriod( 0.1 );
			FResult = f_mount( &SDFatFs, ( TCHAR const * ) SDPath, 1 ); 	// Регистрировать и монтировать
//			LedTimerChangePeriod( 1.0 );
			static uint16_t FS_InitCount = 0;
			FS_InitCount++;

			switch( FResult )
			{
			case FR_OK:				// ФайловаЯ система поднЯлась
				// Проверить работоспособность - произвести серию файловых операций
				FResult = FileSystem_TestFS( "TestFS.txt" );
				if( ( FR_OK == FResult ) || ( FR_WRITE_PROTECTED == FResult ) )
				{	// Тест прошел успешно
					portENTER_CRITICAL( );
					( void ) xEventGroupClearBits( EventGroup_System, EVENTSYSTEM_FS_INIT );	// инициализациЯ файловой системы завершена
					( void ) xEventGroupSetBits( EventGroup_System, EVENTSYSTEM_FS_READY );		// файловаЯ система готова к работе
					SKLP_MemoryFlags_t Flags = SKLP_MemoryFlags;
					Flags.SD0_ErrorMemInit	= 0;
					Flags.SD0_ErrorMemWrite	= ( FR_OK == FResult ) ? 0 : 1;
					Flags.SD0_ErrorMemRead	= 0;
					SKLP_MemoryFlags = Flags;
					bFS_ReadOnly = ( FR_OK == FResult ) ? 0 : 1;
					portEXIT_CRITICAL( );
					// Отправить сообщение в лог.
					// !! Важно! После помещениЯ сообщениЯ в основную очередь задачи MemoryTread_Task(), задача будет
					// !! разбужена и приступит к обработке сообщениЯ. Если при этом очередь отложенных сообщений будет не пуста,
					// !! сначала будут обработаны сообщениЯ оттуда.
					// !! Таким образом, отправка сообщениЯ в очередь после завершениЯ инициализации файлухи необходима
					// !! длЯ немедленной обработки накопившихсЯ сообщений, ожидающих поЯвлениЯ файлухи.
//					assert_param( Logger_WriteRecord( "[FatFS] Mount on uSD complete.", LOGGER_FLAGS_APPENDTIMESTAMP | LOGGER_FLAGS_STATICTEXT | LOGGER_FLAGS_WAITEFORFS ) );
					char aMsg[60];
					snprintf( aMsg, sizeof( aMsg ), "[FatFS] Mount on uSD complete (attempt #%d).", FS_InitCount );
					assert_param( Logger_WriteRecord( aMsg, LOGGER_FLAGS_APPENDTIMESTAMP | LOGGER_FLAGS_WAITEFORFS ) );
					// Перейти к ожиданию запросов на переиницализацию или форматирование
					DWT_AppendTimestampTag( "FS Ready" );
					continue;
				}
				// Тест не пройден. Поступить также, как и при неготовности SD или файловой системы

			case FR_DISK_ERR:		// Ошибка работы с SD-картой (низкоуровневаЯ?)
			case FR_NOT_READY:		// Ошибка работы с SD-картой (низкий уровень работает, но результат кривой с точки зрениЯ FatFS?)
				// Возникла ошибка. Причина пока не устанавливаетсЯ, обработка одинаковаЯ
				vTaskDelay( pdMS_TO_TICKS( FILESYSTEM_REINIT_TIMEOUT_ms ) );					// выдержать "разумную" паузу перед переинициализацией
				assert_param( EVENTSYSTEM_FS_INIT & xEventGroupGetBits( EventGroup_System ) );	// контролировать, что флаг инициализации никто не сбросил
				continue;																		// начать инициализацию сначала

			case FR_NO_FILESYSTEM: 	// SD-карта исправна (считан Boot-сектор), но FAT32 не обнаружена
				// Произвести форматирование
				portENTER_CRITICAL( );
				( void ) xEventGroupClearBits( EventGroup_System, EVENTSYSTEM_FS_INIT );		// считать файловую систему предварительно инициализированной
				( void ) xEventGroupSetBits( EventGroup_System, EVENTSYSTEM_FS_FORMAT );		// оставить запрос на форматирование
				SKLP_MemoryFlags.SD0_ErrorMemInit = 1;
				portEXIT_CRITICAL( );
				// Перейти к обработке запроса на форматирование
				continue;
//				break;

			default:				// НеобрабатываемаЯ ошибка. Если будет возникать - разобратьсЯ и найти способ решить
				assert_param( 0 );
			}
		}

		if( EventBitsResult & EVENTSYSTEM_FS_FORMAT )
		{	// Пришел запрос на форматирование SD-карты
			if( SKLP_FlagsModule.DataSaving )
			{	// Пока модуль находитсЯ в автономном режиме - ни в коем случае не форматировать. Лучше еше раз попробовать инициализировать файлуху
				vTaskDelay( pdMS_TO_TICKS( 100 ) );
				FileSystem_Reinit( "[FatFS] Skip Query to SD formatting." );
				continue;
				// !!!! Вообще-то требование следующее: форматирование допускаетсЯ только при переходе в автономный режим.
			}
			// !!! На времЯ разбирательств с SD с сектором 1024 КБ, полностью перекрыть форматирование!
			vTaskDelay( pdMS_TO_TICKS( 100 ) );
			FileSystem_Reinit( "[FatFS] Skip Query to SD formatting." );
			continue;

			// Вызвать коллбек перед началом операции
			FileSystem_FormatBeforeCallback( );
//			LedTimerChangePeriod( 0.1 );
			// Размонтировать
			FResult = f_mount( NULL, ( TCHAR const * ) SDPath, 1 );
			assert_param( FR_OK == FResult );
			// Перелинковать драйвер
			FATFS_UnLinkDriver( SDPath );
			assert_param( 0 == FATFS_LinkDriver( &SD_Driver, SDPath ) );
			// Снова монтировать, на этот раз отложенно - длЯ подготовки к форматированию
			FResult = f_mount(  &SDFatFs, ( TCHAR const * ) SDPath, 0 );
			// Произвести форматирование
			FResult = f_mkfs( ( TCHAR const * ) SDPath, 0, SD_RECORD_SIZE );
			assert_param( Logger_WriteRecord( "[FatFS] Formatting uSD complete.", LOGGER_FLAGS_APPENDTIMESTAMP | LOGGER_FLAGS_STATICTEXT | LOGGER_FLAGS_WAITEFORFS ) );
//			LedTimerChangePeriod( 1.0 );
			// Результат не проверЯть. Перейти к инициализации
			FileSystem_Reinit( "???" );
			continue;
		}
		// Контроль ошибки при организации бесконечного цикла
		break;
	}
	assert_param( 0 );
}

// Запуск переинициализации файловой системы
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
// Новый вариант - реализациЯ инициализации и перезагрузки файловой системы коллбеки задачи MemoryThread_Task()
// #####################################################
// #####################################################
#include "MemoryThread.h"

static TimerHandle_t FS_TimerHdl = NULL;

static void FileSystem_ReinitCB( MemoryThreadMessage_t *pMessage );
static void FileSystem_ReinitInternal( bool bDelayed );

// Добавить коллбек в MemoryThread_Task() на переинициализацию файловой системы
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
		// Инициализировать таймер переинициализации FS
		assert_param( NULL == FS_TimerHdl );
		FS_TimerHdl = xTimerCreate( "FS", pdMS_TO_TICKS( FILESYSTEM_REINIT_TIMEOUT_ms ), pdFALSE, NULL, FileSystem_TimerCB );
		assert_param( NULL != FS_TimerHdl );
		// Таймер не запускать, будет запущен только после неудачной инициализации

		// Запустить процесс немедленной инициализации.
		// Флаги состоЯниЯ автомата инициализации будут выставлены там же.
		FileSystem_ReinitInternal( false );

		Result = true;
	} while( 0 );
	
	return Result;
}

// Запуск переинициализации FS, только если до сих пор все было в порЯдке
void FileSystem_Reinit( char * pReason )
{
	( void ) pReason;
	if( FR_OK == FileSystem_CheckFSReady( ) )
		FileSystem_ReinitInternal( false );
}

// Запуск переинициализации FS
static void FileSystem_ReinitInternal( bool bDelayed )
{
	assert_param( NULL != EventGroup_System );
	portENTER_CRITICAL( );
	( void ) xEventGroupClearBits( EventGroup_System, EVENTSYSTEM_FS_READY | EVENTSYSTEM_FS_FORMAT );
	( void ) xEventGroupSetBits( EventGroup_System, EVENTSYSTEM_FS_INIT );
	SKLP_MemoryFlags.SD0_ErrorMemInit = 1;
	if( !bDelayed )
	{	// Добавить коллбек в MemoryThread_Task() на переинициализацию файловой системы
		assert_param( MemoryThread_AppendCallback( FileSystem_ReinitCB, 0 ) );
	}
	else
	{	// Запустить таймер, который добавит коллбек на переинициализацию через таймаут
		assert_param( NULL != FS_TimerHdl );
		assert_param( pdPASS == xTimerStart( FS_TimerHdl, 0 ) );
	}
	portEXIT_CRITICAL( );
}

// Коллбек из MemoryThread_Task() длЯ переинициализации файловой системы
// - только SD-card
// - инициализация+тест
// - без форматированиЯ
static void FileSystem_ReinitCB( MemoryThreadMessage_t *pMessage )
{
	assert_param( NULL != pMessage );
	assert_param( FileSystem_ReinitCB == pMessage->Header.xCallback );

	static volatile FRESULT FResult;
	assert_param( NULL != EventGroup_System );
		
	// Проверить, что выставлен флаг переинициализации файловой системы
	assert_param( EVENTSYSTEM_FS_INIT == ( ( EVENTSYSTEM_FS_READY | EVENTSYSTEM_FS_INIT ) & xEventGroupGetBits( EventGroup_System ) ) );

	// Перелинковать драйвер SD-карты
	FATFS_UnLinkDriver( SDPath );
	assert_param( 0 == FATFS_LinkDriver( &SD_Driver, SDPath ) );

	// Размонтировать SD-карту, если ранее была смонтирована.
	// BSP_SD_DeInit() при этом не вызываетсЯ,
	// т.к. в FatFS не предусомотрено обращений к драйверу устройства при размонтировании.
	FResult = f_mount( NULL, ( TCHAR const * ) SDPath, 1 );	
	assert_param( FR_OK == FResult );

	// Произвести монтирование SD-карты.
	// Приводит к вызову BSP_SD_Init(), который инициализирует периферию, сбрасывает питание и шлет инициализирующие посылки в SD-карту
	FResult = f_mount( &SDFatFs, ( TCHAR const * ) SDPath, 1 );
	static uint16_t FS_MountCount = 0;
	FS_MountCount++;

	// Проверить результат сонтированиЯ
	switch( FResult )
	{
	case FR_OK:				// ФайловаЯ система поднЯлась
		// Проверить работоспособность - произвести серию файловых операций
		FResult = FileSystem_TestFS( "TestFS.txt" );
		if( ( FR_OK == FResult ) || ( FR_WRITE_PROTECTED == FResult ) )
		{	// Тест прошел успешно
			portENTER_CRITICAL( );
			( void ) xEventGroupClearBits( EventGroup_System, EVENTSYSTEM_FS_INIT );	// инициализациЯ файловой системы завершена
			( void ) xEventGroupSetBits( EventGroup_System, EVENTSYSTEM_FS_READY );		// файловаЯ система готова к работе
			SKLP_MemoryFlags_t Flags = SKLP_MemoryFlags;
			Flags.SD0_ErrorMemInit	= 0;
			Flags.SD0_ErrorMemRead	= 0;
			Flags.SD0_ErrorMemWrite	= ( FR_OK == FResult ) ? 0 : 1;
			SKLP_MemoryFlags = Flags;
			portEXIT_CRITICAL( );
			// Отправить сообщение в лог.
			// !! Важно! После помещениЯ сообщениЯ в основную очередь задачи MemoryTread_Task(), задача будет
			// !! разбужена и приступит к обработке сообщениЯ. Если при этом очередь отложенных сообщений будет не пуста,
			// !! сначала будут обработаны сообщениЯ оттуда.
			// !! Таким образом, отправка сообщениЯ в очередь после завершениЯ инициализации файлухи необходима
			// !! длЯ немедленной обработки накопившихсЯ сообщений, ожидающих поЯвлениЯ файлухи.
//			char aMsg[60];
//			snprintf( aMsg, sizeof( aMsg ), "[FatFS] Mount on uSD complete (attempt #%d).", FS_MountCount );
			char aMsg[120];
			extern char aSD_CID_ASCII[];
			snprintf( aMsg, sizeof( aMsg ), "[FatFS] Mount on uSD complete (attempt #%d), **SD.CID %s**", FS_MountCount, aSD_CID_ASCII );
			assert_param( Logger_WriteRecord( aMsg, LOGGER_FLAGS_APPENDTIMESTAMP | LOGGER_FLAGS_WAITEFORFS ) );
			DWT_AppendTimestampTag( "FS Ready" );
//			HAL_SD_GetCardCID();

			// Завершить переиницализацию успешно
			break;
		}
		// Тест не пройден. Поступить также, как и при неготовности SD или файловой системы

	case FR_DISK_ERR:		// Ошибка работы с SD-картой (низкоуровневаЯ?)
	case FR_NOT_READY:		// Ошибка работы с SD-картой (низкий уровень работает, но с т.з. FatFS что-то не то)
	case FR_NO_FILESYSTEM:	// SD-карта исправна (считан Boot-сектор), но FAT32 не обнаружена
		{	// Возникла ошибка. Причина пока не устанавливаетсЯ, обработка одинаковаЯ
			// Запустить процесс задержанной инициализации.
			// Флаги состоЯниЯ автомата инициализации будут выставлены там же.
			FileSystem_ReinitInternal( true );
		}
		break;

	default:				// НеобрабатываемаЯ ошибка. Если будет возникать - разобратьсЯ и найти способ решить
		assert_param( 0 );
	}
}


// Запуск переинициализации файловой системы
void FileSystem_ReinitExt( char *pReason, const char *pSourceFile, uint32_t Line, const char *pFunc )
{
	FileSystem_ReinitInternal( false );
}

#endif	// FS_USE_TASK_XXX
// #####################################################
// #####################################################

// Проверка, что указанный файл уже открыт
FRESULT FileSystem_CheckFileOpen( FIL *pFile )
{
	if( NULL == pFile )
		return FR_INT_ERR;				// кривой аргумент
	if( NULL == pFile->fs )
		return FR_NO_FILE;				// файл закрыт
	if( ( 0 == pFile->fs->fs_type ) || ( pFile->fs->id != pFile->id ) )
	{
//		pFile->fs = NULL;				// файловаЯ система была размонтирована в то времЯ пока файл был открыт - навести порЯдок
//		pFile->err = 0;
//		return FR_NO_FILE;				// теперь файл закрыт
		return FR_INVALID_OBJECT;
	}
	return ( FRESULT ) pFile->err;
//	return FR_OK;
}

// Перемотать файл в конец
FRESULT FileSystem_SeekToEOF( FIL *pFile )
{
	assert_param( NULL != pFile );
	if( pFile->fptr == pFile->fsize )
		return FR_OK;
	return f_lseek( pFile, pFile->fsize );
}

// Перемотать файл в конец, пошагово, с сохранением промежуточных позиций
// pFile			- указательна структуру файла.
// pFilePosition	- указатель на структуру, куда сохранЯть промежуточную и последнюю позиции.
// SeekSizeMax		- максимальный размер одной перемотки.
// 		!! Тест, длЯ оптимизации работы файловой системы:
//		- SeekSizeMax должен быть кратен размеру кластера!
//		- если оставшаЯсЯ дистанциЯ перемотки больше SeekSizeMax, перемотка производитсЯ до позиции, кратной SeekSizeMax!
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
			FileOffset = ( ( pFile->fptr + SeekSizeMax ) / SeekSizeMax ) * SeekSizeMax;		// при длинных перемотках обеспечить выранвивание точки перемотки
		if( FR_OK != ( FResult = f_lseek( pFile, FileOffset ) ) )
			break;
		// После каждой перемотки сохранить позицию файла, чтобы при нарушении работы файловой системы продолжить перемотку с сохраненной позиции
		if( FR_OK != ( FResult = FileSystem_FilePositionSave( pFile, pFilePosition ) ) )
			break;
	}
	return FResult;
}

// Перемотать файл относительно конца
FRESULT FileSystem_SeekFromEOF( FIL *pFile, int32_t OffsetFromEOF )
{
	assert_param( NULL != pFile );
	if( OffsetFromEOF <= 0 )
		assert_param( pFile->fsize >= -OffsetFromEOF );						// проверка, что перемотка назад не перейдет через начало файла
	return f_lseek( pFile, pFile->fsize + OffsetFromEOF );
}

// Проверка, что файловая система готова к работе
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


// Дождаться готовности файловой системы
FRESULT FileSystem_WaitFSReady( TickType_t TicksToWait )
{
	assert_param( NULL != EventGroup_System );
	const EventBits_t BitsToWait = EVENTSYSTEM_FS_READY;
	EventBits_t BitsResult = xEventGroupWaitBits( EventGroup_System, BitsToWait, pdFALSE, pdTRUE, TicksToWait );
	return ( ( BitsToWait == ( BitsResult & BitsToWait ) ) ? FR_OK : FR_NOT_READY );
}

// Проверка работоспособности файловой системы
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

		// Создать новый файл и открыть для записи
		res = f_open( &FileTmp, pFileName, FA_CREATE_ALWAYS | FA_WRITE );
		if( FR_OK != res )
		{
			if( FR_DISK_ERR == res )
				res = FR_WRITE_PROTECTED;
			break;
		}
	
		// Записать сигнатуру
		assert_param( VALIDATE_ALIGN( aTestWrite, sizeof( uint32_t ) ) );
		res = f_write( &FileTmp, aTestWrite, sizeof( aTestWrite ), &BytesCount );
		if( ( sizeof( aTestWrite ) != BytesCount ) || ( FR_OK != res ) )
		{
			if( FR_DISK_ERR == res )
				res = FR_WRITE_PROTECTED;
			break;
		}
	
		// Закрыть файл
		res = f_close( &FileTmp );
		if( FR_OK != res )
			break;
	
		// Открыть уже существующий файл
		res = f_open( &FileTmp, pFileName, FA_READ | FA_OPEN_EXISTING );
		if( FR_OK != res )
			break;
	
		// Считать сигнатуру
		assert_param( VALIDATE_ALIGN( aTestRead, sizeof( uint32_t ) ) );
		res = f_read( &FileTmp, aTestRead, sizeof( aTestRead ), &BytesCount );
		if( ( sizeof( aTestRead ) != BytesCount ) || ( FR_OK != res ) )
			break;
	
		// Закрыть файл
		res = f_close( &FileTmp );
	
		// Сравнить сигнатуры
		if( 0 != strcmp( aTestWrite, aTestRead ) )
			break;
	
		// Удалить файл
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
#error "Перейти на расшаренный FIL FileTmp."
		// Открыть уже существующий файл
		res = f_open( &TestFile, pFileName, FA_WRITE | FA_OPEN_EXISTING );
		if( FR_OK != res )
			break;

		extern uint8_t *pTmpBuffer;
		const uint16_t BuffSize = 32 * 1024;
		uint8_t *pBuffer = pTmpBuffer;
		assert_param( NULL != pBuffer );
	
		// Считать весь файл
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
	
		// Закрыть файл
		f_close( &TestFile );

		if( -1 == ReadCount )
			res = FR_OK;
		else
			res = FR_INT_ERR;
		
	}while( 0 );

	return res;
}
*/

// Найти в директории файл с самой новой датой
// pFileDir			директория, в которой искать файл
// pResultFileInfo	вернуть сюда информацию о файле
FRESULT FileSystem_FindLastFile( const TCHAR *pFileDir, FILINFO *pResultFileInfo )
{
	FRESULT FResult;
	DIR Dir;
	FILINFO FileInfo;
	int FileCount = 0;

	do
	{
		FResult = FR_INVALID_PARAMETER;
		// Проверить аргументы
		if( ( NULL == pFileDir ) || ( NULL == pResultFileInfo ) )
			break;

		// Открыть директорию для просмотра
		FResult = f_opendir( &Dir, pFileDir );
		if( FR_OK != FResult )
			break;

		// Приготовиться к чтению файлов с начала директории
		FResult = f_readdir ( &Dir, NULL );
		if( FR_OK != FResult )
			break;
		
		// Запустить поиск по всем файлам в директории
		WORD LastFileTime = 0;
		WORD LastFileDate = 0;
		do
		{
			// Считать информацию об очередном файле
			FResult = f_readdir ( &Dir, &FileInfo );
			if( FR_OK != FResult )
				break;
			if( '\0' == FileInfo.fname[0] )
				break;		// достигнут конец директории

			FileCount++;
			// Сверить дату модификации очередного файла с самым новым файлом
			if( ( FileInfo.fdate > LastFileDate ) ||
				( ( FileInfo.fdate == LastFileDate ) && ( FileInfo.ftime > LastFileTime ) ) )
			{
				LastFileTime = FileInfo.ftime;
				LastFileDate = FileInfo.fdate;
				*pResultFileInfo = FileInfo;
			}
		} while( 1 );

		// Закрыть директорию
		FResult = f_closedir( &Dir );
		if( FR_OK != FResult )
			break;

		// Вернуть информацию о файле
		if( FileCount > 0 )
			FResult = FR_OK;
		else
			FResult = FR_NO_FILE;
	} while( 0 );

	return FResult;}

// Получить информацию о файле
FRESULT FileSystem_GetFileInfo( const TCHAR *pFileName, FILINFO *pResultFileInfo )
{
	FRESULT FResult;
	DIR Dir;
	FILINFO FileInfo;
	do
	{
		FResult = FR_INVALID_PARAMETER;
		// Проверить аргументы
		if( ( NULL == pFileName ) || ( NULL == pResultFileInfo ) )
			break;

		// Открыть директорию для просмотра
		// !!корневаЯ!!
		FResult = f_opendir( &Dir, "\\" );
		if( FR_OK != FResult )
			break;

		// Приготовиться к чтению файлов с начала директории
		FResult = f_readdir ( &Dir, NULL );
		if( FR_OK != FResult )
			break;
		
		// Запустить поиск по всем файлам в директории
		do
		{
			// Считать информацию об очередном файле
			FResult = f_readdir ( &Dir, &FileInfo );
			if( FR_OK != FResult )
				break;
			if( '\0' == FileInfo.fname[0] )
			{	// Достигнут конец директории, а файл не найден
				FResult = FR_NO_FILE;
				break;
			}

			// Проверить совпадение имени файла с искомым
			int i = 0;
			int FileNameLenght = strlen( pFileName );
			for( ; i < FileNameLenght; i++ )
				if( toupper( FileInfo.fname[i] ) != toupper( pFileName[i] ) )
					break;
			if( i != FileNameLenght )
				continue;		// несовпадение имени файла с искомым

			// Найден файл с совпадающим именем
			*pResultFileInfo = FileInfo;
			assert_param( FR_OK == FResult );
			break;
		} while( 1 );
	} while( 0 );

	return FResult;
}

// Найти по базовому имени файл данных с максимальным номером, и вернуть его номер
// pFileDir			директория, в которой искать файл
// pBaseName		базовая часть имени файла, от 1 до 7 символов, заканчивающихся '\0'
// pExtension		расширение файлов
// pResultNumber	сюда вернуть номер самого последнего найденного файла
// pResultFileInfo	если не NULL, вернуть сюда информацию о файле
FRESULT FileSystem_FindLastEnumeratedFile( const TCHAR *pFileDir, const TCHAR *pBaseName, const TCHAR *pExtension, int32_t *pResultNumber, FILINFO *pResultFileInfo )
{
	FRESULT FResult;
	DIR Dir;
	FILINFO FileInfo;

	do
	{
		FResult = FR_INVALID_PARAMETER;
		// Проверить аргументы
		if( ( NULL == pFileDir ) || ( NULL == pBaseName ) || ( NULL == pResultNumber ) )
			break;

		// Проверить длину базовой части имени
		uint32_t BaseNameLenght = strlen( pBaseName );
		if( ( BaseNameLenght < 1 ) || ( BaseNameLenght > 7 ) )
			break;	
	
		// Проверить длину расширения
		if( ( NULL != pExtension ) && ( strlen( pExtension ) > 3 ) )
			break;	

		// Открыть директорию для просмотра
		FResult = f_opendir( &Dir, pFileDir );
		if( FR_OK != FResult )
			break;

		// Приготовиться к чтению файлов с начала директории
		FResult = f_readdir ( &Dir, NULL );
		if( FR_OK != FResult )
			break;
		
		// Запустить поиск по всем файлам в директории
		int MaxFileNumber = -1;
		do
		{
			// Считать информацию об очередном файле
			FResult = f_readdir ( &Dir, &FileInfo );
			if( FR_OK != FResult )
				break;
			if( '\0' == FileInfo.fname[0] )
				break;		// достигнут конец директории

			// Проверить совпадение имени файла с искомым
			int i = 0;
			for( ; i < BaseNameLenght; i++ )
				if( toupper( FileInfo.fname[i] ) != toupper( pBaseName[i] ) )
					break;
			if( i != BaseNameLenght )
				continue;		// несовпадение имени файла с искомым

			// Начало имени файла совпадает с искомым. Считать номер.
			int32_t FileNumber = -1;
			do
			{
				// Считать очередной символ имени файла
				char c = FileInfo.fname[i++];

				if( '\0' == c )
				{	// достигнут конец имени файла, расширения нет
					if( NULL != pExtension )
						FileNumber = -1;		// должно было быть расширение, но его нет
					break;
				}
				
				if( '.' == c  )
				{	// достигнут конец имени файла, расширение есть
					if( NULL != pExtension )
					{	// Сравнить расширения
						int ExtensionLenght = strlen( &FileInfo.fname[i] );
						for( int j = 0; j < ExtensionLenght + 1; i++, j++ )
							if( toupper( FileInfo.fname[i] ) != toupper( pExtension[j] ) )
							{
								FileNumber = -1;		// расширение не совпало
								break;
							}
						
					}
					else
						FileNumber = -1;				// расширение не совпало
					break;
				}

				if( !isdigit( c ) )
				{
					FileNumber = -1;
					break;		// вместо цифры что-то другое
				}

				// Нормальная цифра, считать в номер файла
				if( -1 == FileNumber )
					FileNumber = 0;
				FileNumber = FileNumber * 10 + c - '0';
			} while( i < 9 );

			// Сверить полученный номер с максимальным найденным на текущий момент
			if( FileNumber > MaxFileNumber )
			{
				MaxFileNumber = FileNumber;
				if( NULL != pResultFileInfo )
					*pResultFileInfo = FileInfo;
			}
		} while( 1 );

		// Закрыть директорию
		FResult = f_closedir( &Dir );
		if( FR_OK != FResult )
			break;

		// Вернуть найденный максимальный номер
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

// Создать новое имя нумерованного файла на основе базового имени, расширения и номера
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

// Удалить все нумерованные файлы в указанной директории
// pFileDir				директория, в которой искать файлы
// pBaseName			базовая часть имени файла, от 1 до 7 символов, заканчивающихся '\0'
// pExtension			расширение файлов
// pDeletedFilesCount	количество удаленных файлов
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
		
		// Проверить аргументы
		if( ( NULL == pFileDir ) || ( NULL == pBaseName ) )
			break;

		// Проверить длину базовой части имени
		uint32_t BaseNameLenght = strlen( pBaseName );
		if( ( BaseNameLenght < 1 ) || ( BaseNameLenght > 7 ) )
			break;	
	
		// Проверить длину расширения
		if( ( NULL != pExtension ) && ( strlen( pExtension ) > 3 ) )
			break;	

		// Открыть директорию для просмотра
		FResult = f_opendir( &Dir, pFileDir );
		if( FR_OK != FResult )
			break;

		// Приготовиться к чтению имен файлов с начала директории
		FResult = f_readdir ( &Dir, NULL );
		if( FR_OK != FResult )
			break;
		
		// Запустить поиск по всем файлам в директории
		do
		{
			// Считать информацию об очередном файле
			FResult = f_readdir ( &Dir, &FileInfo );
			if( FR_OK != FResult )
				break;
			if( '\0' == FileInfo.fname[0] )
				break;		// достигнут конец директории

			// Проверить совпадение имени файла с искомым
			int i = 0;
			for( ; i < BaseNameLenght; i++ )
				if( toupper( FileInfo.fname[i] ) != toupper( pBaseName[i] ) )
					break;
			if( i != BaseNameLenght )
				continue;		// несовпадение имени файла с искомым

			// Начало имени файла совпадает с искомым. Считать номер.
			int32_t FileNumber = -1;
			do
			{
				// Считать очередной символ имени файла
				char c = FileInfo.fname[i++];

				if( '\0' == c )
				{	// достигнут конец имени файла, расширения нет
					if( NULL != pExtension )
						FileNumber = -1;		// должно было быть расширение, но его нет
					break;
				}
				
				if( '.' == c  )
				{	// достигнут конец имени файла, расширение есть
					if( NULL != pExtension )
					{	// Сравнить расширения
						int ExtensionLenght = strlen( &FileInfo.fname[i] );
						for( int j = 0; j < ExtensionLenght + 1; i++, j++ )
							if( toupper( FileInfo.fname[i] ) != toupper( pExtension[j] ) )
							{
								FileNumber = -1;		// расширение не совпало
								break;
							}
						
					}
					else
						FileNumber = -1;				// расширение не совпало
					break;
				}

				if( !isdigit( c ) )
				{
					FileNumber = -1;
					break;		// вместо цифры что-то другое
				}

				// Нормальная цифра, считать в номер файла
				if( -1 == FileNumber )
					FileNumber = 0;
				FileNumber = FileNumber * 10 + c - '0';
			} while( i < 9 );

			if( FileNumber > -1 )
			{	// Найден файл с подходящим именем. Удалить.
				FResult = f_unlink( FileInfo.fname );
				if( FR_OK != FResult )
					break;
				// Файл успешно удален
				if( NULL != pDeletedFilesCount )
					( *pDeletedFilesCount )++;
			}
		} while( 1 );

		// Закрыть директорию
		f_closedir( &Dir );
	} while( 0 );

	return FResult;
}

// Вернуть свободное место
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

	// Приготовить буфер для записи
	for( int i = 0; i < BUFFER_SIZE; i++ )
		aBufferWrite[i] = ( uint8_t ) rand( );

	do
	{
		// Записать буфер в SD
		Result = BSP_SD_WriteBlocks_DMA( ( uint32_t * ) aBufferWrite, TEST_SECTOR_ADDRESS / FS_SECTOR_SIZE, BUFFER_SIZE / FS_SECTOR_SIZE );
		if( MSD_OK == Result )
			Stat.WriteSuccess++;
		else
		{
			Stat.WriteFail++;
			break;
		}
		
		// Считать данные из SD
		Result = BSP_SD_ReadBlocks_DMA( ( uint32_t * ) aBufferRead, TEST_SECTOR_ADDRESS / FS_SECTOR_SIZE, BUFFER_SIZE / FS_SECTOR_SIZE );
		if( MSD_OK == Result )
			Stat.ReadSuccess++;
		else
		{
			Stat.ReadFail++;
			break;
		}
		
		// Проверить соответсвие данных
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

// Скопировать из одного файла в другой.
// Копирование производится не с начала, а с текущей позиции и в текущую позицию.
FRESULT FileSystem_FileCopy( FIL *pFileSource, FIL *pFileDestination, uint32_t BytesToCopy )
{
	FRESULT FResult;
//	uint32_t BufferSize = SKLP_Memory_TmpBufferGetSize( );
//	BufferSize = ( BufferSize / 512 ) * 512;
/*	// Подобрать размер буфера, кратный размеру кластера
	uint32_t BufferSizeMax = SKLP_Memory_TmpBufferGetSize( );
	uint32_t BufferSize = 512;
	while( BufferSize <= BufferSizeMax )
		BufferSize *= 2;
	BufferSize /= 2;
*/
	// Подобрать размер буфера, кратный размеру кластера (оставить в размере только самую старшую единицу)
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
		// Проверить выравнивание буфера
		if( !VALIDATE_ALIGN( pBuffer, sizeof( uint32_t ) ) )
			break;
		// Проверить выравнивание файлов
		if( BytesToCopy > FS_SECTOR_SIZE )
		{
			if( !VALIDATE_ALIGN( pFileSource->fptr, sizeof( uint32_t ) ) )
				break;
			if( !VALIDATE_ALIGN( pFileDestination->fptr, sizeof( uint32_t ) ) )
				break;
			// !!! Необходимо в свЯзи с тем, что чтение больших блоков через FatFS приводит к разбаению
			// !!! обращениЯ к SD на несколько операций, что может привести к чтению SD в невыравненную
			// !!! область памЯти. ДлЯ STM32l4xx не удалось работать через SDMMC.DMA в однобайтовом режиме,
			// !!! т.е. работа с невыравненной памЯтью может быть нарушена!
			// !!! При чтении малых областей, чтение производитсЯ через промежуточный выравненный буфер FatFS и отказа не происходит
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
		// Обрезать новый файл до текущей позиции - если он исходно был больше требуемого размера
		if( FR_OK != ( FResult = f_truncate( pFileDestination ) ) )
			break;
		if( 0 != BytesToCopy )
			FResult = FR_INT_ERR;
	} while( 0 );
	if( NULL != pBuffer )
		SKLP_Memory_TmpBufferRelease( false );
	return FResult;
}

// Отрезать начало файла и сместить оставшуюсЯ часть на начало.
// ИспользуетсЯ длЯ оберзки файла логгированиЯ, чтобы не превышал разуменых пределов.
// pFile		- адрес структуры открытого файла, подлежащий обрезке
// pFileName	- имЯ открытого файла
// ResultSize	- требуемый размер обрезанного файла
// В результате pFile будет обрезан, открыт и перемотан в конец
FRESULT FileSystem_FileTrancateHead( FIL *pFile, char *pFileName, uint32_t ResultSize )
{
	FRESULT FResult;
	do
	{
		if( ( NULL == pFileName ) || ( NULL == pFile ) || ( NULL == pFile->fs ) )
		{	FResult = FR_INVALID_PARAMETER;		break; }		// неправильный аргумент или файл закрыт
		if( pFile->fsize < ResultSize )
		{	FResult = FR_OK;					break; }		// файл и так меньшего размера, чем требуетсЯ
		if( FR_OK != ( FResult = FileSystem_CheckFSReady( ) ) )
			break;
		if( FR_OK == FileSystem_CheckFileOpen( &FileTmp ) )
			if( FR_OK != ( FResult = f_close( &FileTmp ) ) )	// длЯ операции используетсЯ временный файл, который всегда должен быть закрытым после завершениЯ предыдущих операций
				break;											// файл не был закрыт, или произошла ошибка при попытке его закрытиЯ

		char aFileTmpName[14] = "tmp.bin";
		if( FR_OK != ( FResult = f_open( &FileTmp, aFileTmpName, FA_WRITE | FA_OPEN_ALWAYS ) ) )
			break;												// не удалось открыть временный файл
		if( FR_OK != ( FResult = f_lseek( pFile, pFile->fsize - ResultSize ) ) )
			break;												// не удалось перемотать исходный файл
		if( FR_OK != ( FResult = FileSystem_FileCopy( pFile, &FileTmp, ResultSize ) ) )
			break;												// не удалось произвести копированиЯ
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

// Реализация функции взятия времени для fatfs (по-умолчанию находится в diskio.c и возвращает 0)
DWORD get_fattime( void )
{
	SKLP_Time_t SKLP_Time_Safe;
	ATOMIC_WRITE( SKLP_Time_Safe, SKLP_Time );
	DosDateTime_t DosDateTime = Time_SKLP2DOS( &SKLP_Time_Safe.Time6 );
	return *( DWORD * ) &DosDateTime;
}

// Сохранение текущей позиции открытого файла
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

// Восстановление текущей позиции открытого файла
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
			break;		// нарушение целостности сохраненных данных
		if( pFile->sclust != pFilePosition->sclust )
			break;		// несовпадение стартового кластера в открытом файле и его сохраненной позиции
		if( pFile->fsize < pFilePosition->fsize )
			break;		// текущий размер файла меньше, чем был при сохранении позиции
		pFile->fptr 	= pFilePosition->fptr;
		pFile->clust	= pFilePosition->clust;
		pFile->dsect	= pFilePosition->dsect;
		FResult = FR_OK;
	} while( 0 );
	return FResult;
}

