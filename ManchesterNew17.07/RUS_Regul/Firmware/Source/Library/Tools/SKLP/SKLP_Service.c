// SKLP_Service.h
// Протокол последовательной шины СКЛ, обслуживание принятых пакетов.
#include "ProjectConfig.h"		// конфиг платформы, конфиг задачи.
#include "stm32xxxx_hal.h"		// дрова периферии
#include "SKLP_MS_Transport.h"	// транспорт
#include "SKLP_Service.h"		// родной
//#include "SKLP_Memory.h"		// инициализация памяти
#include "RebootUtils.h"		// ResetInfo, BuildInfo
#include "eeprom_emul.h"				// EEPROM Emulation
#include "SKLP_BlackBox.h"
#include "Logger.h"
#include <stdio.h>				// sscanf( Date )
#include "Utils.h"				// ReverseByteOrder32() for LoochDeviceSerial.BCD

// Тип обработчика комады СКЛ - номер и вектор
typedef struct SKLP_CallbackAssign_struct
{
	SKLP_Command_t	Command;
	SKLP_Callback_t	xCallback;
} SKLP_CallbackAssign_t;

// Таблица обработчиков команд.
// "Стандартные" обработчики, реализованные в SKLP_ServiceDefault.c, сразу же инициализируются.
// Место под прочие обработчики резервируются, их необходимо добавлять вызовом SKLP_ServiceSetCallback()
SKLP_CallbackAssign_t aSKLP_Callbacks[] __PLACE_AT_RAM_CCM__ =
{
	{ SKLP_COMMAND_STOP_LOGGING,		NULL },
	{ SKLP_COMMAND_ID_GET,				SKLP_ProcessCommand_ID_Get },
	{ SKLP_COMMAND_ID_SET,				SKLP_ProcessCommand_ID_Set },
	{ SKLP_COMMAND_DATA_ACQ_GET_LONG,	NULL },
	{ SKLP_COMMAND_NVM_GET,				NULL },
	{ SKLP_COMMAND_NVM_SET,				NULL },
	{ SKLP_COMMAND_WORKTIME_GET,		SKLP_ProcessCommand_WorkTime_Get },
	{ SKLP_COMMAND_WORKTIME_INC,		SKLP_ProcessCommand_WorkTime_Inc },
	{ SKLP_COMMAND_WORKTIME_SET,		SKLP_ProcessCommand_WorkTime_Set },
	{ SKLP_COMMAND_DATA_ACQ_GET,		NULL },
	{ SKLP_COMMAND_MEMORY_ERASE,		NULL },
	{ SKLP_COMMAND_MEMORY_STATE_GET,	SKLP_ProcessCommand_MemoryGetState },
	{ SKLP_COMMAND_MEMORY_READ, 		NULL }, 	// Особая команда из-за другого формата ответа, должна обрабатывается по-особому в SKLP_ProcessPacket()
	{ SKLP_COMMAND_TIME_LOG_START_SET,	NULL },
	{ SKLP_COMMAND_TIME_LOG_START_GET,	NULL },
	{ SKLP_COMMAND_TIME_LOG_STOP_SET,	NULL },
	{ SKLP_COMMAND_TIME_LOG_STOP_GET,	NULL },
	{ SKLP_COMMAND_TIME_SYNC,			SKLP_ProcessCommand_TimeSinchronization },
	{ SKLP_COMMAND_TIME_SYNC2,			SKLP_ProcessCommand_TimeSinchronization },
	{ SKLP_COMMAND_MEMORY_INFO_GET,		NULL },
	{ SKLP_COMMAND_BAUD_SET,		 	SKLP_ProcessCommand_BaudSet },
	{ SKLP_COMMAND_GOTO_BOOTLOADER,		SKLP_ProcessCommand_GoTo_BootLoader },
	{ SKLP_COMMAND_DATA_ACQ_START,		NULL },
};

// Массив для обработчиков дополнительных команд
#ifndef	SKLP_CALLBACKS_AUX_COUNT
#define	SKLP_CALLBACKS_AUX_COUNT	3
#endif	//SKLP_CALLBACKS_AUX_COUNT
__no_init SKLP_CallbackAssign_t aSKLP_CallbacksAux[SKLP_CALLBACKS_AUX_COUNT];

// Статические данные, обнуляются при запуске
volatile SKLP_FlagsModule_t		SKLP_FlagsModule;
//volatile SKLP_MemoryFlags_t		SKLP_MemoryFlags = { 1, 0, 0, 0, 0, 0, 0, 0 };
volatile SKLP_MemoryFlags_t		SKLP_MemoryFlags = { 0, 1, 0, 0, 0, 0, 0, 0 };
volatile SKLP_Time_t			SKLP_Time;
volatile SKLP_WorkTimeCounter_t	SKLP_WorkTimeCounter;
volatile SKLP_Temp_t			TemperatureBoard = SKLP_TEMP_UNKNOWN;
//ModuleMode_t					ModuleMode;

// Идентификаторы модуля
__no_init LoochDeviceSerial_t	LoochDeviceSerial;		// Серийник
__no_init LoochSoftVersion_t	LoochSoftVersion;		// Дата и версия микропрограммы

// Инициализация
bool SKLP_ServiceInit( void )
{
/*	// Считать режим работы модулЯ из EEPROM
	ModuleMode_t ModuleModeSaved;
	if( EEPROM_READ_OK != EE_ReadVariable( EE_TAG( EEPROM_ModuleMode, ModuleModeSaved ) ) )
		ModuleModeSaved = ModuleMode_Unknown;
	ModuleMode = ModuleModeSaved;
	// Если режим кривой, он должен быть заменен на дефолтный в основной задаче модулЯ
*/
	// Считать серийник из EEPROM
	// Формат серийника: YYCCCNNN 12'345'678 храниться памяти как 0x78563412. Такое вот тяжелое наследие :(
	//if( EEPROM_READ_OK != EE_ReadVariable(  EE_TAG( EEPROM_ID_Serial, LoochDeviceSerial ) ) )
		//LoochDeviceSerial = SKLP_DEVICE_SERIAL_DEFAULT;			// считать не удалось, использовать дефолтный
	#warning "ZAKOMENTIL IF, NE DAET COMPILYATSYA SUKA! LICHNIY POHODU"
	// Считать дату сборки проекта из автоматического файла BuildInfo.c
	do
	{
		LoochSoftVersion = ( LoochSoftVersion_t ) { 0 };
		int Day, Month, Year;
		int ScanfResult = sscanf( aBuildInfo_Date, "%2d.%2d.%4d", &Day, &Month, &Year );
		if( 3 != ScanfResult )						break;
		if( ( Day < 1 ) || ( Day > 31 ) )			break;
		if( ( Month < 1 ) || ( Month > 12 ) )		break;
		if( ( Year < 2015 ) || ( Year >= 2100 ) )	break;
#ifdef	SKLP_USE_PROPER_VERSION_DATE_FORMAT
		assert_param( UInt8_to_BCD8( SKLP_SOFT_VERSION, &LoochSoftVersion.Version ) );
		assert_param( UInt8_to_BCD8( Day,				&LoochSoftVersion.Day ) );
		assert_param( UInt8_to_BCD8( Month, 			&LoochSoftVersion.Month ) );
		assert_param( UInt8_to_BCD8( Year - 2000,		&LoochSoftVersion.YearFrom2000 ) );
#else
		LoochSoftVersion.Version		= SKLP_SOFT_VERSION;
		LoochSoftVersion.Day			= Day;
		LoochSoftVersion.Month			= Month;
		LoochSoftVersion.YearFrom2000	= Year - 2000;
#endif
	} while( 0 );

	// Заполнить нулЯми дополнительный массив обработчиков команд
	for( int i = 0; i < SIZEOFARRAY( aSKLP_CallbacksAux ); i++ )
		 aSKLP_CallbacksAux[i] = ( SKLP_CallbackAssign_t ) { SKLP_COMMAND_FREE, NULL };

	// Оставить запись в логе
	if( 0 == ResetInfo.ResetCounter )
	{
		char aDeviceInfo[256];

#ifdef	SKLP_USE_PROPER_VERSION_DATE_FORMAT
		snprintf( aDeviceInfo, sizeof( aDeviceInfo ), "[SKLP] Device Type: %s; S/N: %08lx; Soft.Ver.: 20%02hhX.%02hhX.%02hhX (v.%hhX); Mode: %s",
#else
		snprintf( aDeviceInfo, sizeof( aDeviceInfo ), "[SKLP] Device Type: %s; S/N: %08lx; Soft.Ver.: 20%02hhu.%02hhu.%02hhu (v.%hhu); Mode: %s",
#endif
			SKLP_DEVICE_TYPE, ReverseByteOrder32( LoochDeviceSerial.BCD ),
			LoochSoftVersion.YearFrom2000, LoochSoftVersion.Month, LoochSoftVersion.Day, LoochSoftVersion.Version,
			( SKLP_FlagsModule.DataSaving ? "Autonomous" : "Not Autonomous" ) );

		assert_param( Logger_WriteRecord( aDeviceInfo, LOGGER_FLAGS_APPENDTIMESTAMP | LOGGER_FLAGS_WAITEFORFS ) );
	}
	
	return true;
}

// Ассоциировать команду и обработчик
// CommandNumber	номер команды. если найдена команда с этим номером - заменяется. иначе присваивается на свободное место. assert, если свободноо место закончилось.
// xNewCallback		адрес обработчика. если NULL, команда освобождается
// возвращает адрес предыдущего обработчика
SKLP_Callback_t SKLP_ServiceSetCallback( uint8_t CommandNumber, SKLP_Callback_t xNewCallback )
{
	assert_param( SKLP_COMMAND_FREE != CommandNumber );
	SKLP_Callback_t ReturnValue = NULL;
	SKLP_Command_t Command = ( SKLP_Command_t ) CommandNumber;
	int i;
	SKLP_CallbackAssign_t *pOldCallback = NULL, *pFirstFreeCallback = NULL;
	
	taskENTER_CRITICAL( );
	// Искать коллбек в основном массиве
	for( i = 0; i < SIZEOFARRAY( aSKLP_Callbacks ); i++ )
		if( Command == aSKLP_Callbacks[i].Command )
		{
			pOldCallback = &aSKLP_Callbacks[i];
			break;
		}
	if( NULL == pOldCallback )
	{	// Искать коллбек в дополнительном массиве
		for( i = 0; i < SIZEOFARRAY( aSKLP_CallbacksAux ); i++ )
			if( Command == aSKLP_CallbacksAux[i].Command )
			{
				pOldCallback = &aSKLP_CallbacksAux[i];
				break;
			}
			else if( ( NULL == pFirstFreeCallback ) && ( SKLP_COMMAND_FREE == aSKLP_CallbacksAux[i].Command ) )
				pFirstFreeCallback = &aSKLP_CallbacksAux[i];
	}
	if( NULL == pOldCallback )
	{	// Старая команда не найдена, использовать свободную ассоциацию
		assert_param( NULL != pFirstFreeCallback );		// Проверить, что найден свободный коллбек в дополнительном массиве
		pOldCallback = pFirstFreeCallback;
	}
#ifndef	SKLP_SERVICE_ALLOW_CHANGE_CALLBACK
	assert_param( NULL == pOldCallback->xCallback );	// При установленном SKLP_SERVICE_ALLOW_CHANGE_CALLBACK не допускается замена уже задействованных коллбеков
#endif	// SKLP_SERVICE_ALLOW_CHANGE_CALLBACK

	if( NULL == xNewCallback )
		pOldCallback->Command = SKLP_COMMAND_FREE;
	else
		pOldCallback->Command = Command;
	ReturnValue = pOldCallback->xCallback;
	pOldCallback->xCallback = xNewCallback;
	taskEXIT_CRITICAL( );
	
	return ReturnValue;
}

// Определение команды и вызов соответствующего обработчика.
// pQuery	- адрес буфера с запросом длЯ обработки, начинаЯ от [START] и заканчиваЯ [CRC]. Вообще, буфер имеет размер [255+3].
// ppAnswer	- адрес буфера, в который формируетсЯ ответ на запрос.
//			Как правило, *ppAnswer == pQuery, т.е. под ответ используетсЯ буфер с запросом.
//			Однако буфер запроса имеет ограниченый размер (258 байт?), и удлиненные ответы на [0x03] и [0x22] туда не влазЯт.
//			В этом случае коллбек должен использовать под ответ специализированный буфер, адрес которго возвращаетсЯ через *ppAnswer.
// return	- размер ответа в байтах:
//			  - если больше нулЯ, то в *ppAnswer находитсЯ сформированный ответ указанного размера;
//			  - если 0, значит команда обработана штатно, но отвечать на нее не требуетсЯ;
//			  - если меньше нулЯ, то при обработке возникла ошибка, и это есть код ошибки.
// !!! Добавлено пост-фактум !!!
// Через *ppAnswer в SKLP_ProcessCommand_Common() и далее в xCallback()
// передаетсЯ ( SKLP_Interface_t * ) - адрес интерфейса, от которого прилетел запрос.
// Коллбек, при необходимости, сможет менЯть поведение в зависимости от интерфейса запроса - это актуально длЯ SKLP_ProcessCommand_BaudSet(),
// который должен знать, в каком интерфейсе необходимо поменЯть скорость.
SKLP_CommandResult_t SKLP_ProcessCommand_Common( uint8_t *pQuery, uint8_t **ppAnswer )
{
	assert_param( NULL != pQuery );
	assert_param( NULL != ppAnswer );

	SKLP_Command_t Command = ( SKLP_Command_t ) pQuery[ SKLP_OFFSET_COMMAND ];
	SKLP_CommandResult_t Result = SKLP_COMMAND_RESULT_ERROR_NOCB;
	SKLP_Callback_t xCallback = NULL;
	uint8_t i;
	// Искать обработчик для принятой команды в основном списке
	for( i = 0; i < SIZEOFARRAY( aSKLP_Callbacks ); i++ )
		if( ( Command == aSKLP_Callbacks[i].Command ) && ( NULL != aSKLP_Callbacks[i].xCallback ) )
		{
			xCallback = aSKLP_Callbacks[i].xCallback;
			break;
		}
	if( NULL == xCallback )
	{	// Искать обработчик для принятой команды в дополнительном списке
		for( i = 0; i < SIZEOFARRAY( aSKLP_CallbacksAux ); i++ )
			if( ( Command == aSKLP_CallbacksAux[i].Command ) && ( NULL != aSKLP_CallbacksAux[i].xCallback ) )
			{
				xCallback = aSKLP_CallbacksAux[i].xCallback;
				break;
			}
	}
	if( NULL != xCallback )
	{	// Обработчик найден, произвести обработку команды
		Result = xCallback( pQuery, ppAnswer );
		if( SKLP_COMMAND_RESULT_ERROR == Result )
			Result = SKLP_COMMAND_RESULT_ERROR_CB;		// Если коллбек при ошибке вернул SKLP_COMMAND_RESULT_ERROR, изменить на SKLP_COMMAND_RESULT_ERROR_CB
	}

/*	if( Result <= SKLP_COMMAND_RESULT_ERROR )
	{	// В случае ошибки, в код ошибки добавить номер команды в запросе
		Result &= 0xFFFF00FF;
		Result |= ( ( uint32_t ) Command ) << 8;
	}
Перенесено в SKLP_ProcessPacket()
*/
	
	return Result;
}

// Перевод времени созданиЯ прошивки во времЯ СКЛ
SKLP_Time6_t Time_LoochSoftVersion2SKLP( LoochSoftVersion_t LoochSoftVersion )
{
	SKLP_Time6_t Result = { 0 };
#ifdef	SKLP_USE_PROPER_VERSION_DATE_FORMAT
	Result.Day			= 10 * ( LoochSoftVersion.Day & 0xF0 ) + ( LoochSoftVersion.Day & 0x0F );
	Result.Month		= 10 * ( LoochSoftVersion.Month & 0xF0 ) + ( LoochSoftVersion.Month & 0x0F );
	Result.YearFrom2000	= 10 * ( LoochSoftVersion.YearFrom2000 & 0xF0 ) + ( LoochSoftVersion.YearFrom2000 & 0x0F );
#else
	Result.Day			= LoochSoftVersion.Day;
	Result.Month		= LoochSoftVersion.Month;
	Result.YearFrom2000 = LoochSoftVersion.YearFrom2000;
#endif
	return Result;
}

// Идентификатор (серийник) изделия, формат BCD 0xNNCNCCYY
// YY	- год производства
// CCC	- децимальный номер изделиЯ
// NNN	- порЯдковый номер изделЯ в году
// Перевод серийника из BCD в Bin
LoochDeviceSerialBin_t SKLP_LoochDeviceSerial2Bin( LoochDeviceSerial_t SerialBCD )
{
	LoochDeviceSerialBin_t Result;
	Result.YearFrom2000	= 10 * ( ( SerialBCD.aBytes[0] & 0xF0 ) >> 4 ) + ( SerialBCD.aBytes[0] & 0x0F );
	Result.Decimal		= 100 * ( ( SerialBCD.aBytes[1] & 0xF0 ) >> 4 ) + 10 * ( SerialBCD.aBytes[1] & 0x0F ) + ( ( SerialBCD.aBytes[2] & 0xF0 ) >> 4 );
	Result.Number		= 100 * ( SerialBCD.aBytes[2] & 0x0F ) + 10 * ( ( SerialBCD.aBytes[3] & 0xF0 ) >> 4 ) + ( SerialBCD.aBytes[3] & 0x0F );
	return Result;
}


