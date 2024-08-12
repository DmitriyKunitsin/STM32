// SKLP_ServiceDefault.h
// Протокол последовательной шины СКЛ, обработчики принятых пакетов по-умолчанию.
#include "ProjectConfig.h"		// конфиг платформы, конфиг задачи.
#include "stm32xxxx_hal.h"		// дрова периферии
#include "SKLP_MS_Transport.h"	// транспорт
#include "SKLP_Service.h"		// родной
//#include "eeprom_emul.h"				// EEPROM Emulation
#include "Logger.h"
#include "TaskConfig.h"			// EventGroup_System
#include "Utils.h"				// ReverseByteOrder32() for LoochDeviceSerial.BCD
#include <stdio.h>				// snprintf()

// [0x37, 0x3F]	Синхронизация времени комплекса
__weak void SKLP_ProcessCommand_TimeSinchronization_CB( SKLP_Time_t SKLP_TimeNew )		{ ATOMIC_WRITE( SKLP_Time, SKLP_TimeNew ); }
__weak SKLP_CommandResult_t SKLP_ProcessCommand_TimeSinchronization( uint8_t *pQuery, uint8_t **ppAnswer )
{
	SKLP_CommandResult_t ReturnValue = SKLP_COMMAND_RESULT_ERROR;
	if( ( sizeof( SKLP_TimeSinchronization1_Query_t ) == SKLP_SIZE_DATA_QUERY( pQuery ) ) ||
		( sizeof( SKLP_TimeSinchronization2_Query_t ) == SKLP_SIZE_DATA_QUERY( pQuery ) ) )
	{
		*ppAnswer = pQuery;
		SKLP_Time_t *pTime = &( ( SKLP_TimeSinchronization1_Query_t * ) ( *ppAnswer + SKLP_OFFSET_DATA_QUERY ) )->Time;
		if( SKLP_TimeValidate( pTime ) )
		{
			static uint8_t bSyncFromCommand = false;
			SKLP_ProcessCommand_TimeSinchronization_CB( *pTime );
			if( !bSyncFromCommand )
			{
				assert_param( Logger_WriteRecord( "[SKLP] Time sync from Command!", LOGGER_FLAGS_STATICTEXT | LOGGER_FLAGS_APPENDTIMESTAMP ) );
#ifdef	EVENTSYSTEM_RTC_SYNC
				( void ) xEventGroupSetBits( EventGroup_System, EVENTSYSTEM_RTC_SYNC );
#endif
				bSyncFromCommand = true;
			}
			ReturnValue = SKLP_COMMAND_RESULT_NO_REPLY; 	// На эту команду не принято отвечать
		}
	}
	return ReturnValue;
}

#ifdef	SKLP_DEVICE_TYPE_DEFAULT
#define	USE_SKLP_COOMAND_ID_GETSET_V2
#endif

// [0x01]	Вернуть идентификатор прибора
__weak SKLP_CommandResult_t SKLP_ProcessCommand_ID_Get( uint8_t *pQuery, uint8_t **ppAnswer )
{
	SKLP_CommandResult_t ReturnValue = SKLP_COMMAND_RESULT_ERROR;
	if( SKLP_ADDRESS_BROADCAST != pQuery[SKLP_OFFSET_ADDRESS] )
	{	// Проверить размер поля данных
		if( 0 == SKLP_SIZE_DATA_QUERY( pQuery ) )
		{	// Вернуть идентификатор
			*ppAnswer = pQuery;
#ifdef	USE_SKLP_COOMAND_ID_GETSET_V2
			SKLP_GetLoochID_v2_Answer_t *pAnswerBody = ( SKLP_GetLoochID_v2_Answer_t * ) ( *ppAnswer + SKLP_OFFSET_DATA_ANSWER );
#else
			SKLP_GetLoochID_Answer_t *pAnswerBody = ( SKLP_GetLoochID_Answer_t * ) ( *ppAnswer + SKLP_OFFSET_DATA_ANSWER );
#endif
			ATOMIC_WRITE( pAnswerBody->FlagsModule, SKLP_FlagsModule );
			pAnswerBody->Serial			= LoochDeviceSerial;
#ifdef	USE_SKLP_COOMAND_ID_GETSET_V2
			pAnswerBody->Type			= SKLP_DEVICE_TYPE_DEFAULT;
#endif
			pAnswerBody->SoftVersion	= LoochSoftVersion;
			ReturnValue = sizeof( *pAnswerBody );
		}
	}
	return ReturnValue;
}

// [0x02]	Установить идентификатор прибора
__weak SKLP_CommandResult_t SKLP_ProcessCommand_ID_Set( uint8_t *pQuery, uint8_t **ppAnswer )
{
	SKLP_CommandResult_t ReturnValue = SKLP_COMMAND_RESULT_ERROR;
	if( SKLP_ADDRESS_BROADCAST != pQuery[SKLP_OFFSET_ADDRESS] )
	{	// Проверить размер поля данных
		SKLP_SetLoochID_Query_t *pQueryBody = ( SKLP_SetLoochID_Query_t * ) ( pQuery + SKLP_OFFSET_DATA_QUERY );
		if( sizeof( *pQueryBody ) == SKLP_SIZE_DATA_QUERY( pQuery ) )
		{	// Размер совпал
			// Оставить запись о смене серийника в логе
			char aLogMessage[64];
			assert_param( MemoryThread_SprintfMutexTake( 1 ) );
			snprintf( aLogMessage, sizeof( aLogMessage ), "Changing Serial Number %08lx to %08lx",
				ReverseByteOrder32( LoochDeviceSerial.BCD ), ReverseByteOrder32( pQueryBody->Serial.BCD ) );
			MemoryThread_SprintfMutexGive( );
			assert_param( Logger_WriteRecord( aLogMessage, LOGGER_FLAGS_APPENDTIMESTAMP /*| LOGGER_FLAGS_WAITEFORCOMPLETE */) );

			// Записать новый серийник, не проверяя валидность.
			EE_WriteVariable( EE_TAG( EEPROM_ID_Serial, pQueryBody->Serial ) );
			// Считать обратно
			EE_ReadVariable( EE_TAG( EEPROM_ID_Serial, LoochDeviceSerial ) );
			// Версию прошивки - игнорировать
			// Не котролировать выполнение операции записи - это должен выполнять софт верхнего уровня

			// Приготовить ответ
			*ppAnswer = pQuery;
#ifdef	USE_SKLP_COOMAND_ID_GETSET_V2
			SKLP_SetLoochID_v2_Answer_t *pAnswerBody = ( SKLP_SetLoochID_v2_Answer_t * ) ( *ppAnswer + SKLP_OFFSET_DATA_ANSWER );
#else
			SKLP_SetLoochID_Answer_t *pAnswerBody = ( SKLP_SetLoochID_Answer_t * ) ( *ppAnswer + SKLP_OFFSET_DATA_ANSWER );
#endif
			ATOMIC_WRITE( pAnswerBody->FlagsModule, SKLP_FlagsModule );
			pAnswerBody->Serial			= LoochDeviceSerial;
#ifdef	USE_SKLP_COOMAND_ID_GETSET_V2
			pAnswerBody->Type			= SKLP_DEVICE_TYPE_DEFAULT;
#endif
			pAnswerBody->SoftVersion	= LoochSoftVersion;
			ReturnValue = sizeof( *pAnswerBody );
		}
	}
	return ReturnValue;
}

// [0x07] Вернуть отработанное время
__weak SKLP_CommandResult_t SKLP_ProcessCommand_WorkTime_Get( uint8_t *pQuery, uint8_t **ppAnswer )
{
	SKLP_CommandResult_t ReturnValue = SKLP_COMMAND_RESULT_ERROR;
	if( SKLP_ADDRESS_BROADCAST != pQuery[SKLP_OFFSET_ADDRESS] )
	{	// Проверить размер поля данных
		if( 0 == SKLP_SIZE_DATA_QUERY( pQuery ) )
		{	// Вернуть отработанное время
			*ppAnswer = pQuery;
			SKLP_WorkTimeAnswer_t *pAnswerBody = ( SKLP_WorkTimeAnswer_t * ) ( *ppAnswer + SKLP_OFFSET_DATA_ANSWER );
			ATOMIC_WRITE( pAnswerBody->FlagsModule, SKLP_FlagsModule );
			pAnswerBody->Signature			= SKLP_SIGNATURE_COMMAND_WORKTIME;
			pAnswerBody->WorkTimeCounter	= SKLP_WorkTimeCounter;		// не атомарно, т.к. модификация может быть только в этом же потоке
			ReturnValue = sizeof( *pAnswerBody );
		}
	}
	return ReturnValue;
}

// [0x08] Инкрементировать отработанное время
__weak SKLP_CommandResult_t SKLP_ProcessCommand_WorkTime_Inc( uint8_t *pQuery, uint8_t **ppAnswer )
{
	SKLP_CommandResult_t Result = SKLP_COMMAND_RESULT_ERROR;
	// Проверить размер поля данных
	if( 0 == SKLP_SIZE_DATA_QUERY( pQuery ) )
	{	// Инкрементировать отработанное время
		ATOMIC_WRITE( SKLP_WorkTimeCounter, SKLP_WorkTimeCounter + 1 );
		Result = SKLP_COMMAND_RESULT_NO_REPLY;
	}
	return Result;
}

// [0x09] Установить отработанное время
__weak SKLP_CommandResult_t SKLP_ProcessCommand_WorkTime_Set( uint8_t *pQuery, uint8_t **ppAnswer )
{
	SKLP_CommandResult_t ReturnValue = SKLP_COMMAND_RESULT_ERROR;
	if( SKLP_ADDRESS_BROADCAST != pQuery[SKLP_OFFSET_ADDRESS] )
	{	// Проверить размер поля данных
		SKLP_WorkTimeSetQuery_t *pQueryBody = ( SKLP_WorkTimeSetQuery_t * ) ( pQuery + SKLP_OFFSET_DATA_QUERY );
		if( sizeof( *pQueryBody ) == SKLP_SIZE_DATA_QUERY( pQuery ) )
		{
			// Проверить формат поля данных
			if( SKLP_SIGNATURE_COMMAND_WORKTIME == pQueryBody->Signature )
			{	// Установить отработанное время и вернуть его
				*ppAnswer = pQuery;
				SKLP_WorkTimeAnswer_t *pAnswerBody = ( SKLP_WorkTimeAnswer_t * ) ( *ppAnswer + SKLP_OFFSET_DATA_ANSWER );
				pAnswerBody->Signature		= SKLP_SIGNATURE_COMMAND_WORKTIME;
				taskENTER_CRITICAL( );
					SKLP_WorkTimeCounter			= pQueryBody->Counter;
					pAnswerBody->WorkTimeCounter	= SKLP_WorkTimeCounter;
					pAnswerBody->FlagsModule		= SKLP_FlagsModule;
				taskEXIT_CRITICAL( );
				ReturnValue = sizeof( *pAnswerBody );
			}
		}
	}
	return ReturnValue;
}

// [0x21] Вернуть состояние памяти
__weak SKLP_CommandResult_t SKLP_ProcessCommand_MemoryGetState( uint8_t *pQuery, uint8_t **ppAnswer )
{
	SKLP_CommandResult_t ReturnValue = SKLP_COMMAND_RESULT_ERROR;
	if( SKLP_ADDRESS_BROADCAST != pQuery[SKLP_OFFSET_ADDRESS] )
	{	// Проверить размер поля данных
		if( 0 == SKLP_SIZE_DATA_QUERY( pQuery ) )
		{	// Вернуть состояние памяти
			*ppAnswer = pQuery;
			SKLP_MemoryGetStateAnswer_t *pAnswerBody = ( SKLP_MemoryGetStateAnswer_t * ) ( *ppAnswer + SKLP_OFFSET_DATA_ANSWER );
			taskENTER_CRITICAL( );
				pAnswerBody->FlagsModule	= SKLP_FlagsModule;
				pAnswerBody->MemoryFlags	= SKLP_MemoryFlags;
			taskEXIT_CRITICAL( );
			ReturnValue = sizeof( *pAnswerBody );
		}
	}
	return ReturnValue;
}

// [0x46] Установить скорость обмена.
// После более 1.5 с отсутствия "нормальных" команд на шине, необходимо автоматически установить скорость по-умолчанию 115200.
// Команда должна быть доступна только на "обычном" интерфейсе RS-485, т.к. на модеме SIG-60 скорость только 57600 и это предел.
// Через ppAnswer принимаетсЯ адрес интерфейса, в котором происходит переключение скорости.
// НепосредственнаЯ процедура переключениЯ скорости возлагаетсЯ на локальную организацию проекта SKLP_ProcessCommand_BaudSetLocal(),
// а по-умолчанию используетсЯ затычка __weak
__weak SKLP_CommandResult_t SKLP_ProcessCommand_BaudSetLocal( void *pTransportInterface, uint32_t NewBaudRate )
{
	return SKLP_COMMAND_RESULT_ERROR_NOCB;	// по-умолчанию, вернуть ошибку об отсутствии подходЯщего коллбека на интерфейсе
}

__weak SKLP_CommandResult_t SKLP_ProcessCommand_BaudSet( uint8_t *pQuery, uint8_t **ppAnswer )
{
	SKLP_CommandResult_t ReturnValue = SKLP_COMMAND_RESULT_ERROR;
	do
	{
		// Команда не должна быть широковещательной, т.к. предусмотрен ответ на нее
		if( SKLP_ADDRESS_BROADCAST == pQuery[SKLP_OFFSET_ADDRESS] )
			break;

		SKLP_BaudSetQuery_t *pQueryBody = ( SKLP_BaudSetQuery_t * ) ( pQuery + SKLP_OFFSET_DATA_QUERY );
		// Проверить размер поля данных
		if( sizeof( *pQueryBody ) != SKLP_SIZE_DATA_QUERY( pQuery ) )
			break;

		// Определить требуемую скорость UART
		uint32_t NewBaudRate;
		switch( pQueryBody->Speed )
		{
		case 0: 	NewBaudRate = SKLP_BAUD_DEFAULT;	break;
		case 1: 	NewBaudRate = 460800;				break;
		case 2: 	NewBaudRate = 1000000;				break;
		case 3: 	NewBaudRate = 2000000;				break;
		case 4: 	NewBaudRate = 3000000;				break;
		default:	NewBaudRate = 0;					break;
		}
		if( 0 == NewBaudRate )
			break;
		// РеальнаЯ допустимаЯ скорость на интерфейсе зависит от платформы, и эта проверка должна быть реальзована в локальной процедуре

		// Вызвать локальную процедуру длЯ подготовки к изменению скорости после завершениЯ отправки ответа
		ReturnValue = SKLP_ProcessCommand_BaudSetLocal( *ppAnswer, NewBaudRate );	// через ppAnswer передаетсЯ адрес интерфейса
		if( ReturnValue <= SKLP_COMMAND_RESULT_ERROR )
			break;		// локальнаЯ процедура не смогла установить скорость и вернула код ошибки

		// Подготовить ответ
		*ppAnswer = pQuery;
		SKLP_BaudSetAnswer_t *pAnswerBody = ( SKLP_BaudSetAnswer_t * ) ( *ppAnswer + SKLP_OFFSET_DATA_ANSWER );
		taskENTER_CRITICAL( );
		pAnswerBody->FlagsModule = SKLP_FlagsModule;
		pAnswerBody->MemoryFlags = SKLP_MemoryFlags;
		taskEXIT_CRITICAL( );
		ReturnValue = sizeof( *pAnswerBody );
	} while( 0 );
	return ReturnValue;
}

// [0xB1]	Перейти на BootLoader
// !! Тест!
// !! При получении этой команды произвести сброс модулЯ и переход на микропрограмму загрузчика.
// !! Загрузчик будет находитьсЯ в режиме идентификации в течении нескольких секунд,
// !! и если не пройдет процедуру идентификации, произведет переход на основную микропрограмму.
#include "RebootUtils.h"	// ResetToApplication()
__weak SKLP_CommandResult_t SKLP_ProcessCommand_GoTo_BootLoader( uint8_t *pQuery, uint8_t **ppAnswer )
{
	// Аргументов нет. Адрес может быть широковещательным. Отвечать не требуетсЯ.
#ifdef	FLASH_APPLICATION_BASE_BOOT
	// Произвести сброс и переход на загрузчик.
	ResetToApplication( FLASH_APPLICATION_BASE_BOOT );
	// !! ФункциЯ организована так, что сначала скидываетсЯ сообщение в лог и ожидаетсЯ завершение записи!
#else
	return SKLP_COMMAND_RESULT_ERROR_NOCB;
#endif	// FLASH_APPLICATION_BASE_BOOT
}

