// SKLP_Memory.c
// Доступ к памяти модуля по протоколу СКЛ
#include "ProjectConfig.h"		// конфиг платформы, конфиг задачи.
#include "stm32xxxx_hal.h"		// дрова периферии
#include "SKLP_Memory.h"		// Родной
#include "SKLP_MS_Transport.h"
#include "SKLP_MS_TransportInterface.h"
#include "FileSystem.h"			// Файловая система
#include "MemoryThread.h"
#include "LogFile.h"
#include "Logger.h"
#include "MathUtils.h"			// GetGreatestPowerOf2()
#include <string.h>				// memcpy()
#include <stdio.h>				// memcpy()
#include "common_rcc.h"

/*/ Объявление всех регионов памяти.
// Адреса указываются в секторах SKLP_MEMSECTORSIZE (по 512 байт)
// Адрес начала региона должен быть кратен SKLP_MEMBLOCKSIZEMAX (32 сектора)

// Округление к большему по основанию
#define	CEIL( _VALUE_, _BASE_ )									( ( ( _VALUE_ ) <= 0 ) ? ( 0 ) : ( ( ( ( ( _VALUE_ ) - 1 ) / ( _BASE_ ) ) + 1 ) * ( _BASE_ ) ) )
// Генерация адреса начального сектора региона с выравниванием по базе вверх
#define	REGION_SECTOR_START( _SECTOR_START_ )					CEIL( ( uint32_t ) ( _SECTOR_START_ ), ( uint32_t ) SKLP_MEMBLOCKSIZEMAX )
// Генерация адреса последнего сектора региона без выравнивания
#define	REGION_SECTOR_FINISH( _SECTOR_START_, _SIZE_BYTES_ )	( ( _SECTOR_START_ ) + CEIL( ( uint32_t ) ( _SIZE_BYTES_ ), ( uint32_t ) SKLP_MEMSECTORSIZE ) / ( uint32_t ) SKLP_MEMSECTORSIZE - 1 )
*/


// Описание одного из регионов памяти
typedef struct SKLP_MemoryRegion_struct
{
	SKLP_MemoryRegionAlias_t	Alias;			// Уникальное наименование региона
	SKLP_MemoryRegionType_t		Type;			// Тип доступа к памяти, смаппированной в регион
	void const					*pHdl;			// Хендлер области памЯти, напр. ( LogFileHdl_t * ) длЯ файлов
	// ПолЯ, рассчитываемые при вызове Update()
	uint32_t					Size;			// Размер исходной области в байтах
	SKLP_SectorIndex_t			iSectorStart;	// Стартовый сектор, куда смаппирован регион.
	SKLP_SectorIndex_t			SectorsCount;	// Размер региона в секторах.
	// Регион обычно характеризуетсЯ размером и начальным адресом.
	// Размер может быть фиксирован, а может извлекаетсЯ их хендлера соответствующими средствами.
	// При отображении в виртуальную памЯть размер выравниваетсЯ на SKLP_MEMBLOCKSIZEMAX.
	// Адрес автоматически рассчитываетсЯ исходЯ из размеров предыдущих регионов.
	// Весь процесс происходит при составлении MemInfo при выполнении команды [0x41].
	// Также в MemInfo выводитсЯ Alias, чтобы ПО верхнего уровнЯ могло определить назначение региона.
} SKLP_MemoryRegion_t;

/*/ Установка размеров регионов по-умолчанию, в байтах
#define	FILE_MAXSIZE_VARIABLE		( ( uint32_t ) 0 )						// регион переменного размера, зависит от размера файла
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

// Объявление пустого массива регионов. ЗаполнЯетсЯ при запуске программы.
#ifndef	SKLP_MEMEMORY_REGION_COUNT_MAX
#define	SKLP_MEMEMORY_REGION_COUNT_MAX		7		// Количество регионов по-умолчанию
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

// Добавление нового региона
// Можно организовать удаление региона при необходимости
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
		{	// Найден пустой или этот же регион
			pRegion->Alias			= Alias;
			pRegion->Type			= Type;
			pRegion->pHdl			= pHdl;
			// Эти полЯ должны быть проинициализированы в процедуре Update()
			pRegion->Size			= 0;
			pRegion->iSectorStart	= 0;
			pRegion->SectorsCount	= 0;
			bSKLP_MemoryRegionsUpdated = false;		// была изменена структура регионов, теперь необходимо вызвать Update() длЯ переадрессации
			bRegionUpdated = true;
			continue;
		}
		if( bRegionUpdated && ( Alias.ID == pRegion->Alias.ID ) )
		{	// Найден регион с нужным идентификатором, но это дубль - очистить
			*pRegion = ( SKLP_MemoryRegion_t ) { 0 };
			continue;
		}
	}
	return bRegionUpdated;
}

// Обновить информацию по регионам (распределить регионы по адресам виртуальной памЯти)
static bool SKLP_MemoryRegionsUpdate( void )
{
	bSKLP_MemoryRegionsUpdated = false;
	SKLP_MemInfoFormat = SKLP_MEMINFO_FORMAT_V0;
	SKLP_SectorIndex_t iSectorStart = SKLP_MEMBLOCKSIZEMAX;		// не начинать распределение с нулевого сектора. 0 - признак отсутствЯ/ошибки региона
	for( int i = 0; i < SIZEOFARRAY( aSKLP_MemoryRegions ); i++ )
	{
		SKLP_MemoryRegion_t *pRegion = &aSKLP_MemoryRegions[i];
		if( ( 0 == pRegion->Alias.ID ) || ( NULL == pRegion->pHdl ) ||
			( pRegion->Type <= SKLP_MemoryRegionType_EMPTY ) || ( pRegion->Type >= SKLP_MemoryRegionType_TOTAL ) )
		{	// С заЯвленным регионом что-то не так. Обнулить его совсем
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
				// Получить размер файла
				uint32_t FileSize;
				if( FR_OK == FileSystem_CheckFileOpen( pFileHdl->pFile ) )
					// Файл открыт, его размер известен
					FileSize = pFileHdl->pFile->fsize;
				else
				{	// Файл закрыт, получить его размер, не производЯ открытиЯ
					FILINFO FileInfo;
					if( FR_OK != LogFile_GetFileInfo( pFileHdl->pFileName, &FileInfo ) )
						break;
					FileSize = FileInfo.fsize;
					// LogFile_GetFileInfo() отправлЯет коллбек в MemoryThread_Task(), который обращаетсЯ к SD через FatFS, операциЯ может подтормаживать!
					// Предположительно, получить размер через FileSystem_GetFileInfo() быстрее, чем через f_open().
				}
				FileSize		+= sizeof( SKLP_SIGNATURE_SAVING_STOP ) - 1;
				RegionSize		= FileSize;
				SectorsCount	= ( FileSize + SKLP_MEMSECTORSIZE ) / SKLP_MEMSECTORSIZE;
				break;
			}
			case SKLP_MemoryRegionType_DiskImage:
			{
				Diskio_drvTypeDef *pDiskIODriver = ( Diskio_drvTypeDef * ) pRegion->pHdl;
				// Прочитать размер диска в секторах
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
		
		// Если обнаружен регион, отличный от некогда "стандартных" -
		// использовать новый тип структуры MemInfo, допускающий произвольный набо регионов.
		if( ( SKLP_MEMEMORY_REGION_ALIAS_BLACKBOX.ID != pRegion->Alias.ID ) &&
			( SKLP_MEMEMORY_REGION_ALIAS_DATATECH.ID != pRegion->Alias.ID ) &&
			( SKLP_MEMEMORY_REGION_ALIAS_DATAMAIN.ID != pRegion->Alias.ID ) &&
			( SKLP_MEMEMORY_REGION_ALIAS_DATAASYNC.ID != pRegion->Alias.ID ) )
			SKLP_MemInfoFormat = SKLP_MEMINFO_FORMAT_V1;
	}
	bSKLP_MemoryRegionsUpdated = true;
	return bSKLP_MemoryRegionsUpdated;
}

// Инициализатор регионов по-умолчанию
void SKLP_MemoryRegionsInitDefault( void )
{
	// 0 Черный ящик
	extern LogFileHdl_t LogFileHdl_BlackBox;
	assert_param( SKLP_MemoryRegionAppend( SKLP_MEMEMORY_REGION_ALIAS_BLACKBOX, SKLP_MemoryRegionType_File,	&LogFileHdl_BlackBox ) );

	// 1 Технологический лог
	extern LogFileHdl_t LogFileHdl_TechLog;
	assert_param( SKLP_MemoryRegionAppend( SKLP_MEMEMORY_REGION_ALIAS_DATATECH, SKLP_MemoryRegionType_File,	&LogFileHdl_TechLog ) );

	// 2 Основные данные
	extern LogFileHdl_t LogFileHdl_MainData;
	assert_param( SKLP_MemoryRegionAppend( SKLP_MEMEMORY_REGION_ALIAS_DATAMAIN, SKLP_MemoryRegionType_File,	&LogFileHdl_MainData ) );

	// 3 Сжатые данные
	// Не инициализируетсЯ, остаетсЯ пустым

	// !!! Тест - образ SD-карты, проверЯетсЯ на проекте ИНГК!
//	assert_param( SKLP_MemoryRegionAppend( SKLP_MEMEMORY_REGION_ALIAS_SD_IMAGE, SKLP_MemoryRegionType_DiskImage,	&SD_Driver ) );
	// !!! Добавление этой области на МПИ _перед_ файлами интегральных данных и акселерометров
	// !!! привело к отказу чтениЯ из RD5 (по-видимому, косЯк там), поэтому пока выпиливаю.
}

__weak void SKLP_MemoryRegionsInit( void )
{
	SKLP_MemoryRegionsInitDefault( );
}

// Указатель на режим работы модулЯ, должен быть реализован в проекте
SKLP_ModuleMode_t *pSKLP_ModuleMode = NULL;

SKLP_CommandResult_t SKLP_ProcessCommand_MemoryInfoGet( uint8_t *pQuery, uint8_t **ppAnswer );
SKLP_CommandResult_t SKLP_ProcessCommand_MemoryRead( uint8_t *pQuery, uint8_t **ppAnswer );
SKLP_CommandResult_t SKLP_ProcessCommand_MemoryErase( uint8_t *pQuery, uint8_t **ppAnswer );

// По текщему протоколу, память модуля представляет единое пространство,
// в котором расположено 4 стандартных региона:
// - основные данные
// - технологические данные
// - черный ящик
// - архивированные данные
// Предполагается, что все регионы расположены в рамках одного физического носителя,
// данные в регионах заполняются линейно (последовательно) и регионы не пересекаются.
// Для считывании данных с модуля клиент использует команду [0x41],
// через которую возвращается структура памяти модуля - по каким адресам расположен каждый из регионов.
// Затем клиент использует команду [0x22] для вычитывания интересующей его области,
// для чего указывает начальный сектор вычитываемой области и количество секторов.
//
// Применительно к разрабатываемому пакету, прямое считывание памяти SD-карты посекторно бессмысленно,
// т.к. на SD установлена файловая система.
// Кроме того, некоторые данные (напр. черный ящик) логично располагать не на SD, а в uC.Flash, т.е. вообще на другом носителе.
// Предполагается выполнить маппирование файлов и иных структур хранения данных с разных носителей
// в единое пространство виртуальной памяти, предоставляемое для считывания по протоколу СКЛ.
//
// По команде [0x41] необходимо просканировать все подлежащие отображению файлы/области,
// и предоставить информацию о них, как о регионах единой памяти - начальный сектор и количество секторов.
// Начальный сектор каждого региона должен быть фиксирован на этапе компиляции.
// Количество секторов регионов должно либо вычисляться на основе актуального размера отображаемого файла,
// либо также являться фиксированным.
//
// По команде [0x22] необходимо определить, к какой именно области происходит обращение,
// и далее скопировать данные из выбранной области, воспользовавшись драйвером, обслуживающим эту область.
// При считывании информации из файла, нежелательно закрывать файл после очередного считывания,
// т.к. при очередном обращении придется снова открывать и проматывать файл, что может занять значительное время.
// Предполагается оставлять файл открытым, а при очередном обращении на ситывание проверять, что считываемая область
// попрежнему относится к этому же файлу. При обращении к новой области, необходимо закрыть текущий файл и открыть новый.
//
// Всякие ошибки, возникающие при выполнении команды [0x22]:
// - обращение к несуществующей области
// - выход за пределы области
// - выход за пределы файла
// обрабатываются единым образом: возвращается нормальный ответ с пакетом запрошенного размера.
// Все области, которые невозможно отобразить в пакет должны быть заполнены нулями.
//
// Модуль работает в двух режимах - автономном и неавтономном.
// В автономном режиме происходит сохранение получаемых данных в файлы основных и технических данных,
// фиксация событий в черный ящик. Эти файлы постоянно открыты на запись, но при этом допускается их считывание.
// В неавтономном режиме запись в файлы не производится.
// При переходе в автономный режим, необходимо закрывать текущий считываемый файл, чтобы основная программа могла его удалить.

// ИнициализациЯ доступа к памЯти модулЯ
bool SKLP_Memory_Init( void )
{
	assert_param( taskSCHEDULER_NOT_STARTED == xTaskGetSchedulerState( ) );
	SKLP_MemoryRegionsInit( );

	// Установить начальное состояние флага наличиЯ данных в памЯти
	SKLP_MemoryFlags.SD0_DataPresent = 0;		// Записанных данных нет. Флаг должен быть установлен при инициализации основной задачи проекта, котораЯ открывает/проверЯет файл основных данных

	// Инициализировать стандартные коллбеки на запросы СКЛ по обращению к памЯти
	SKLP_ServiceSetCallback( SKLP_COMMAND_MEMORY_ERASE, 	SKLP_ProcessCommand_MemoryErase );
	SKLP_ServiceSetCallback( SKLP_COMMAND_MEMORY_READ, 		SKLP_ProcessCommand_MemoryRead );
	SKLP_ServiceSetCallback( SKLP_COMMAND_MEMORY_INFO_GET, 	SKLP_ProcessCommand_MemoryInfoGet );

	return true;
}

// [0x41] Обработчик команды протокола СКЛ чтения структуры памяти
SKLP_CommandResult_t SKLP_ProcessCommand_MemoryInfoGet( uint8_t *pQuery, uint8_t **ppAnswer )
{
	SKLP_CommandResult_t ReturnValue = SKLP_COMMAND_RESULT_ERROR;
	do
	{
		if( SKLP_ADDRESS_BROADCAST == pQuery[SKLP_OFFSET_ADDRESS] )
			break;		// команда не должна быть широковещательной
		if( 0 != SKLP_SIZE_DATA_QUERY( pQuery ) )
			break;		// команда не должна содержать аргументов

		*ppAnswer = pQuery;		// вернуть ответ через буфер запроса
		SKLP_MemInfoAnswer_t *pAnswerBody = ( SKLP_MemInfoAnswer_t * ) ( *ppAnswer + SKLP_OFFSET_DATA_ANSWER );
		if( sizeof( *pAnswerBody ) > SKLP_SIZE_DATA_MAX_ANSWER )
			break;		// размер ответа превышает буфер
		*pAnswerBody = ( SKLP_MemInfoAnswer_t ) { 0 };		// Заполнить весь ответ нулями. Далее часть полей будут заполнены действительными значениями.
		ReturnValue = sizeof( *pAnswerBody );				// Даже если при последующем заполнении возникнут ошибки, все равно вернуть структуру полностью.

		// Получить актуальную информацию по регионам
		if( !SKLP_MemoryRegionsUpdate( ) )
			break;		// Не удалось получить информацию. Вернуть пустой MemInfo.
		// Заполнить поле флагов по результату считывания
		ATOMIC_WRITE( pAnswerBody->MemoryFlags, SKLP_MemoryFlags );

		// Заполнить заголовок
		pAnswerBody->Info.Format		= SKLP_MEMINFO_FORMAT_V0;
		pAnswerBody->Info.MemSize		= 0;	// Можно подставить размер SD. Однако это виртуальнаЯ карта памЯти, к SD прЯмого отношениЯ нет. Пока никому не надо.
		pAnswerBody->Info.SectorSize	= SKLP_MEMSECTORSIZE;
		pAnswerBody->Info.DiskCount 	= 1;
		pAnswerBody->Info.Flags[0]		= pAnswerBody->MemoryFlags;
		// Определить рекомендованный размер буфера длЯ чтениЯ памЯти командами [0x22]
		uint32_t BufferSizeMax_Sectors = SKLP_Memory_TmpBufferGetSize( ) / SKLP_MEMSECTORSIZE;		// размер буфера в секторах
		pAnswerBody->Info.v1_Aux.AuxReadBufferSize = GetGreatestPowerOf2( BufferSizeMax_Sectors );	// подравнЯть размер буфера под степень двойки, с целью оптимизации доступа к файловой системе
		assert_param( 0 != pAnswerBody->Info.v1_Aux.AuxReadBufferSize );
		
		// Пройтись по всем заявленным регионам
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
			// Индекс последнего байта в последнем секторе (только для техлога и черного ящика)
			SKLP_ByteIndex_t	iLastByte		= pRegion->Size % SKLP_MEMSECTORSIZE;
			//	Определить, в какую область вернуть информацию о регионе
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
			{	// Обнаружено поле с новым идентификатором, и допускаетсЯ использовать MemInfo_v1
				// Определить место длЯ новой структуры данных
				SKLP_MemInfo_StructData_t *pAuxDataStruct = ( SKLP_MemInfo_StructData_t * ) ( &pAnswerBody->Info.v1_Aux + 1 );
				pAuxDataStruct += AuxDataStructCount;
				// Проверить, что расширеннаЯ структура влазит в буфер ответа
				if( ( sizeof( *pAnswerBody ) + sizeof( *pAuxDataStruct ) * ( AuxDataStructCount + 1 ) ) <= SKLP_SIZE_DATA_MAX_ANSWER )
				{	// Добавить новую структуру в ответ
					pAuxDataStruct->iSectorFirst	= iSectorFirst;
					pAuxDataStruct->iSectorLast		= iSectorLast;
					pAuxDataStruct->v1_Alias		= pRegion->Alias.ID;
					AuxDataStructCount++;
				}
			}
		}
		if( AuxDataStructCount > 0 )
		{	// Были добавлены новые данные в MemInfo, теперь это MemInfo_v1
			assert_param( SKLP_MEMINFO_FORMAT_V1 == SKLP_MemInfoFormat );
			pAnswerBody->Info.Format = SKLP_MEMINFO_FORMAT_V1;
			// Вычислить размер добавленных данных и добавить его к размеру результирующего ответа
			int AuxDataSize = sizeof( SKLP_MemInfo_StructData_t ) * ( AuxDataStructCount );
			ReturnValue += AuxDataSize;
			assert_param( ReturnValue <= SKLP_SIZE_DATA_MAX_ANSWER );
			pAnswerBody->Info.v1_Aux.AuxDataStructCount = AuxDataStructCount;
			// Заполнить "CRC8"
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

// [0x22] Обработчик команды протокола СКЛ чтения памяти
// !! Особый формат ответа: если обычный коллбек возвращает размер тела ответного пакета (оно на 3 меньше полного размера),
// !! то SKLP_ProcessCommand_MemoryRead() возвращает полный размер.
// !! Таким образом, при вызове SKLP_ProcessCommand_MemoryRead() из SKLP_ProcessPacket(), требуетсЯ дополнительнаЯ обработка!
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
			break;		// структура регионов памЯти должна быть предварительно инициализирована при чтении MemInfo
		if( SKLP_ADDRESS_BROADCAST == pQuery[SKLP_OFFSET_ADDRESS] )
			break;		// команда не должна быть широковещательной
		if( sizeof( *pQueryBody ) != SKLP_SIZE_DATA_QUERY( pQuery ) )
			break;		// не соответствует размер поля данных в запросе
		if( 0 != pQueryBody->DiskNum )
			break;		// неправильный номер запрашиваемого устройства памяти
		if( 0 == pQueryBody->SectorsCount )
			break;		// неправильное количество запрашиваемых секторов
		uint32_t DataSize = ( ( uint32_t ) ( pQueryBody->SectorsCount ) ) * SKLP_MEMSECTORSIZE;			// Размер запрашиваемых данных
		uint32_t AnswerSize = sizeof( *pAnswerFrame ) - sizeof( pAnswerFrame->aBuffer ) + DataSize;		// Реальный размер ответного пакета зависит от запрашиваемого объема данных
		if( sizeof( *pAnswerFrame ) < AnswerSize )
			break;		// запрошенные данные не влезают в размер допустимого ответа!

		// Получить доступ к временному буферу для чтения и передачи данных
		pAnswerFrame = ( SKLP_MemReadAnswerFrame_t * ) SKLP_Memory_TmpBufferGet( AnswerSize );
		if( NULL == pAnswerFrame )
			break;		// Не удалось получить доступ к буферу достаточного размера, команда не может быть выполнена
		// Добавить коллбек на освобождение буфера после завершениЯ передачи пакета в последовательный интерфейс.
		SKLP_SetTxCompleteCB( pInterface, SKLP_ProcessCommand_MemoryRead_ReleaseBuffer );
		// При возникновении вышеобозначенных ошибок, ответ на команду не будет возвращен

		// Определить, к какому региону виртуальной памяти производится обращение
		for( int i = 0; i < SIZEOFARRAY( aSKLP_MemoryRegions ); i++ )
		{
			SKLP_MemoryRegion_t *pRegion = &aSKLP_MemoryRegions[i];
			if( SKLP_MemoryRegionType_EMPTY == pRegion->Type )
				continue;
			// Проверить, к этому ли региону происходит обращение
			if( ( pQueryBody->iSectorStart < pRegion->iSectorStart ) ||	
				( pQueryBody->iSectorStart >= ( pRegion->iSectorStart + pRegion->SectorsCount ) ) )
				continue;
			
			// Обнаружен соответствующий регион.
			// Даже если при попытке вычитывания памяти возникнут ошибки,
			// завершить цикл по оставшимся регионам и вернуть корректный ответ.
			i = SIZEOFARRAY( aSKLP_MemoryRegions );		// Условие для завершения цикла по регионам

			*ppAnswer = ( uint8_t * ) pAnswerFrame;
			ReturnValue = AnswerSize;	// в отличии от коллбека на обычную команду, здесь возвращаетсЯ полный размер формируемого пакета

			// Заполнить поля ответного пакета.
			pAnswerFrame->SectorsCount	= pQueryBody->SectorsCount;
			pAnswerFrame->MemoryFlags	= ( SKLP_MemoryFlags_t ) { 0 };		// Сбросить все флаги памяти. Будут выставлены по результату попытки чтения данных.
			memset( pAnswerFrame->aBuffer, 0, DataSize );					// Принято заполнять весь буфер нулями, а потом дописывать реальные данные, если будут
			
			// Произвести считывание данных в соответствии с драйвером соответствующего региона
			switch( pRegion->Type )
			{
			case SKLP_MemoryRegionType_File:	// В регион смаппирован файл с uSD
				{
					LogFileHdl_t *pFileHdl = ( LogFileHdl_t * ) pRegion->pHdl;
					assert_param( FR_OK == LogFile_ValidateHdl( pFileHdl ) );
					static FIL_Position_t FilePositionSaved = { 0 };
					// Определить требуемую позицию в файле, с которой производить чтение
					uint32_t FilePos = ( pQueryBody->iSectorStart - pRegion->iSectorStart ) * SKLP_MEMSECTORSIZE;
					if( FilePos == FilePositionSaved.fptr )
					{	// Определено, что требуемаЯ позициЯ совпадает с позицией при предыдущем запросе.
						// Видимо, это запрос на повторное чтение блока.
						// LogFile_Read() корректно перемотает на требуемую позицию при помощи f_lseek(),
						// но длЯ больших файлов это может занЯть значительное времЯ.
						// Можно попробовать мгновенно восстановить позицию файла при предыдущем запросе.
						FileSystem_FilePositionRestore( pFileHdl->pFile, &FilePositionSaved );
						// Если обращение производитсЯ к другому файлу, то позициЯ не будет восстановлена.
						// Ответ не проверЯть - если позицию восстановить не удалось, отработает f_lseek() в LogFile_Read()
					}
					// Сохранить текущую позицию файла перед операцией чтениЯ
					FileSystem_FilePositionSave( pFileHdl->pFile, &FilePositionSaved );
					// Произвести чтение
					uint32_t BytesReaded = 0;
					FRESULT FResult = LogFile_Read( pFileHdl, FilePos, pAnswerFrame->aBuffer, DataSize, &BytesReaded );
					bool bEOF = ( pFileHdl->pFile->fptr == pFileHdl->pFile->fsize );
					if( ( FR_OK == FResult ) && bEOF )
					{	// Достигнут конец файла. Добавление "STOP" после конца файла - тест!
						uint32_t BytesToAppend = sizeof( SKLP_SIGNATURE_SAVING_STOP ) - 1;
						if( BytesToAppend > ( DataSize - BytesReaded ) )
							BytesToAppend = DataSize - BytesReaded;
						memcpy( ( char * ) ( pAnswerFrame->aBuffer ) + BytesReaded, SKLP_SIGNATURE_SAVING_STOP, BytesToAppend );
						// !!! как обработать попадание STOP на границу блока?
						break;
					}

					// !! Важно !! LogFile_Read() организован так, что в случае нарушениЯ чтениЯ файла одним блоком,
					// !! этот блок будет вычитан посекторно.
					// !! При этом, если часть секторов не будут прочитаны, то результат все равно будет FR_OK,
					// !! но количество считанных байт будет меньше запрошенного.
					// !! В то же времЯ, файл все равно будет перемотан на конец запрошенного блока.
					// !! Это поведение существенно отличаетсЯ от принЯтого поведениЯ функций типа fread().
					if( ( FR_OK != FResult ) || ( ( DataSize > BytesReaded ) && !bEOF ) )
					{	// Оставить запись в лог
						static char aMsgBuffer[150];
						char * const pBuff = aMsgBuffer;
						uint32_t const BuffSize = sizeof( aMsgBuffer );
						// !! нельзЯ использовать aMemoryThreadTaskBuffer[], так как он привЯзан к задаче MemoryThread_Task()
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

			case SKLP_MemoryRegionType_DiskImage:	// В регион смаппирован образ диска
				{
					Diskio_drvTypeDef *pDiskioDriver = ( Diskio_drvTypeDef * ) pRegion->pHdl;
					// Произвести чтение запрошенных секторов
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

	// Проверить результат чтения памяти
	if( ReturnValue >= SKLP_COMMAND_RESULT_RETURN )
	{	// Был подготовлен буфер для передачи по RS-485 (возможно, заполенный нулями)
		// Заполнить поле флагов по результату считывания
		ATOMIC_WRITE( pAnswerFrame->MemoryFlags, SKLP_MemoryFlags );
		// Доступ к буферу будет освобожден при повторном вызове этого обработчика с нулевыми аргументами - после завершения передачи пакета
	}

	return ReturnValue;
}

// [0x20] Коллбек из команды стираниЯ памЯти SKLP_ProcessCommand_MemoryErase()
// ДлЯ проектов ИНГК и АКП - возможность заменить коллбек, чтобы переводить модули в автономный режим при стирании памЯти
__weak void SKLP_ProcessCommand_MemoryEraseCB( MemoryThreadMessage_t *pMessage )
{
}

// [0x20] Коллбек из команды стираниЯ памЯти SKLP_ProcessCommand_MemoryErase()
// Произвести удаление основных файлов, лог не удалЯть
static void SKLP_Memory_EraseCB( MemoryThreadMessage_t *pMessage )
{
	assert_param( SKLP_Memory_EraseCB == pMessage->Header.xCallback );

	// Стереть все файлы, кроме техлога
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
		ATOMIC_WRITE( SKLP_MemoryFlags.SD0_DataPresent, 0 );	// если стирание прошло успешно, сбросить флаг наличиЯ данных
}

// [0x20] Команда стираниЯ памЯти. Только в неавтономном режиме.
// ДлЯ модулей ИНГК, АКП - автоматический переход в автономный режим, независимо от результата стираниЯ памЯти.
// Может приходить по три раза подряд! (интервал ~400 мс?)
SKLP_CommandResult_t SKLP_ProcessCommand_MemoryErase( uint8_t *pQuery, uint8_t **ppAnswer )
{
	SKLP_CommandResult_t Result = SKLP_COMMAND_RESULT_ERROR;
	// Проверить размер поля данных
	if( 0 == SKLP_SIZE_DATA_QUERY( pQuery ) )
	{
		taskENTER_CRITICAL( );
		// Проверить, какой сейчас режим:
		assert_param( NULL != pSKLP_ModuleMode );
		switch( *pSKLP_ModuleMode )
		{
		case SKLP_ModuleMode_Auto:			// Автономный режим
		case SKLP_ModuleMode_AutoDrilling:
		case SKLP_ModuleMode_AutoLogging:
			// Пропустить команду
			break;

		case SKLP_ModuleMode_NotAuto:		// Неавтономный режим
			// Выставить флаг наличия данных, чтобы мастер не решил, что стирание памЯти еще не завершено
			ATOMIC_WRITE( SKLP_MemoryFlags.SD0_DataPresent, 1 );
			// Модули ИНГК и АКП должны быть переведены в автономный режим. МПИ переходит в автономный режим по отдельной команде
			assert_param( MemoryThread_AppendCallback( SKLP_ProcessCommand_MemoryEraseCB, 0 ) );
			// Поместить в очередь коллбек на стирание памяти
			assert_param( MemoryThread_AppendCallback( SKLP_Memory_EraseCB, 0 ) );		// когда/если данные будут стерты, флаг SKLP_MemoryFlags.SD0_DataPresent также будет сброшен
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

// Инициализировать доступ к лог-файлу (открыть на запись, перемотать), и отправить отчет в лог
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
		// Произвести пустую запись в файл - это приведет к открытию и перемотке
		if( FR_OK != ( FResult = LogFile_WriteRecordExt( pLogFileExt->pLogFileHdl, NULL, 0, 0, pLogFileExt->pPositionSaved ) ) )
		{	pSubString = "Can't open or create or seek to EOF file!";		break; }
		if( 0 == ( pLogFileExt->pLogFileHdl->pFile->flag & FA_WRITE ) )
		{	pSubString = "File is Write Protected!";	FResult = FR_WRITE_PROTECTED;	break;	}
		pSubString = "Ok.";
		// Считать дату последней модификации файла
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
	
	// Сформировать отчет об открытии файла
	uint32_t MessageSize = 0;
	assert_param( MemoryThread_SprintfMutexTake( 5 ) );
	MessageSize += snprintf( pBuff + MessageSize, BuffSize - MessageSize, "Accessing to %s... %s",
								pLogFileExt->pLogFileHdl->pFileName, pSubString );
	if( ( FR_OK == FResult ) || ( FR_WRITE_PROTECTED == FResult ) )
	{	// Файл успешно открыт
		uint32_t FileSize = pLogFileExt->pLogFileHdl->pFile->fsize;
		pSubString = ".";
		if( 0 == FileSize )
			pSubString = " - empty.";
		else
		{
			ATOMIC_WRITE( SKLP_MemoryFlags.SD0_DataPresent, 1 );		// файл ненулевого размера, выставить флаг наличиЯ данных на SD
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

// Оценить свободное место на SD-карте
/*void SKLP_Memory_ReportFreeSpace( void )
{
	assert_param( MemoryThread_BufferMutexTake( ) );
	assert_param( MemoryThread_SprintfMutexTake( 5 ) );
	// Оценить свободное место на SD-карте
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
	// Оценить свободное место на SD-карте
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


// Функции, которые необходимо реализовать в проектно-зависимой части (если они нужны)
// !!! Убраны __weak-реализации функций, чтобы вынудить реализацию функций в проекте.
// !!! Был прецендент, когда SKLP_Memory_TmpBufferGet() был реализован, а SKLP_Memory_TmpBufferGetSize() не был реализован,
// !!! вследствии чего был трудноуловимый вылет на проекте ИНГК.
/*
__weak void *SKLP_Memory_TmpBufferGet( uint32_t BufferMinSize )	{ return NULL; }	// Получение доступа на запись к временному буферу
__weak uint32_t SKLP_Memory_TmpBufferGetSize( void )				{ return 0; }		// Узнать размер временного буфера
__weak void SKLP_Memory_TmpBufferRelease( bool FromISR )			{ }					// Освобождение доступа к буферу
*/

