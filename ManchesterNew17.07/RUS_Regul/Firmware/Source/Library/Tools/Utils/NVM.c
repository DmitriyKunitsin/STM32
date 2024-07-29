// NVM.c
#include "NVM.h"		// Родной
#if defined(STM32L4)
#include "eeprom_emul.h"
#include "NVM.h"
#endif // #if defined(STM32L4)
#if defined(STM32F4)
#include "eeprom.h"
#endif // #if defined(STM32F4)
#include <string.h>		// memcpy()
#include "platform_common.h"
#include "FreeRTOS.h"
#include "task.h"		// taskENTER_CRITICAL()

volatile NVM_State_t NVM_State = { 0 };

#if defined (STM32L4)
static bool EEPROM_writeBufToEeprom( uint16_t EEPROM_Address, uint8_t *pBuffer, uint16_t BufferSize );
static bool EEPROM_isEqualEepromBuf( uint16_t EEPROM_Address, uint8_t *pBuffer, uint16_t BufferSize, uint8_t *pReadBuffer, uint16_t ReadBufferSize );
#endif // #if defined (STM32L4)


// Проверка добустимости тега
static bool NVM_TagValidate( NVM_Tag_t const *pTag )
{
	bool Result = false;
	do
	{
		if( NULL == pTag )
			break;
		if( ( 0 == pTag->Size ) || ( pTag->Size > 256 ) )
			break;
		if( !IS_SRAM( pTag->pLocation ) || !IS_SRAM( ( uint8_t * ) pTag->pLocation + pTag->Size ) )
			break;
		if( ( NULL !=  pTag->pDefault ) &&
			( !IS_SRAM( pTag->pDefault ) || !IS_SRAM( ( uint8_t * ) pTag->pDefault + pTag->Size ) ) &&
			( !IS_FLASH( pTag->pDefault ) || !IS_FLASH( ( uint8_t * ) pTag->pDefault + pTag->Size ) ) )
			break;
		Result = true;
	} while( 0 );
	return Result;
}

// Считать тэг из RAM в указанный буфер
NVM_Result_t NVM_TagGetFromRAM( NVM_Tag_t const *pTag, void *pData )
{
	assert_param( NVM_TagValidate( pTag ) );
	assert_param( NULL != pData );

	memcpy( pData, pTag->pLocation, pTag->Size );
	return NVM_Result_Ok;
}

// Считать тэг из NVM в указанный буфер
NVM_Result_t NVM_TagLoadReal( NVM_Tag_t const *pTag, void *pData )
{
//static char aTagMsg[] = "Tag #0";
//DWT_AppendTimestampTag( aTagMsg );
//aTagMsg[5]++;

	assert_param( NVM_TagValidate( pTag ) );
	assert_param( NULL != pData );

	NVM_Result_t Result = NVM_Result_FailNVM;
	// Считать тег из NVM
	if( EEPROM_READ_OK == EE_ReadVariable( pTag->ID, pData, pTag->Size ) )
		// Проверить допустимость считанных данных
		if( NULL != pTag->xValidationCB )
			Result = pTag->xValidationCB( pTag, pData );
		else
			Result = NVM_Result_Ok;
	return Result;
}

// Загрузить тэг из NVM или константы
NVM_Result_t NVM_TagLoad( NVM_Tag_t const *pTag )
{
	assert_param( NVM_TagValidate( pTag ) );

	// Считать тег из NVM
	if( NVM_Result_Ok != NVM_TagLoadReal( pTag, pTag->pLocation ) )
		// Считать из NVM не удалось, принЯть значение по-умолчанию
		if( NULL != pTag->pDefault )
			memcpy( pTag->pLocation, pTag->pDefault, pTag->Size );
		else
			memset( pTag->pLocation, 0, pTag->Size );
	

	// Вызвать коллбек по обновлению данных
	NVM_Result_t Result = NVM_Result_Ok;
	if( NULL != pTag->xResetCB )
		Result = pTag->xResetCB( pTag, NULL );

	return Result;
}

// Сравнить содержимое RAM и NVM
NVM_Result_t NVM_TagCompareRealAndRAM( NVM_Tag_t const *pTag )
{
	assert_param( NVM_TagValidate( pTag ) );
	NVM_Result_t Result = NVM_Result_FailNVM;
	#if defined (STM32L4)
	bool isEqual = false;
	uint8_t adata[EEPROM_BLOCK_MAXVARIABLES] = {0};
	isEqual = EEPROM_isEqualEepromBuf( pTag->ID, pTag->pLocation, pTag->Size, adata, EEPROM_BLOCK_MAXVARIABLES );
	if ( isEqual )
	{
		Result = NVM_Result_Ok;
	}
	#endif // #if defined (STM32L4)
	#if defined (STM32F4)
	if( EEPROM_COMPARE_OK == EE_CompareVariable( pTag->ID, pTag->pLocation, pTag->Size ) )
		Result = NVM_Result_Ok;
	#endif // #if defined (STM32F4)
	
	return Result;
}

// Сохранить из RAM в NVM
NVM_Result_t NVM_TagSave( NVM_Tag_t const *pTag, void *pWrittenData )
{
	assert_param( NVM_TagValidate( pTag ) );

	NVM_Result_t Result;
	do
	{
		Result = NVM_Result_FailNVM;
		// Проверить температуру чипа перед записью в EEPROM
		if( NVM_State.EEPROM_TempLock1 )
			break;
		if( !EEPROM_WriteCheck( pTag->ID, pTag->pLocation, pTag->Size, pWrittenData ) )
			break;
		Result = NVM_Result_Ok;
	} while( 0 );
	return Result;
}

/*/ ПринЯть новое значение тега и сохранить в NVM
// При повышенной температуре - отказ доступка к NVM
NVM_Result_t NVM_TagSaveNew( NVM_Tag_t const *pTag, void *pNewData, void *pWrittenData )
{
	assert_param( NVM_TagValidate( pTag ) );
	assert_param( NULL != pNewData );

	NVM_Result_t Result;
	do
	{
		// Проверить допустимость новых данных
		Result = NVM_Result_FailData;
		if( NULL != pTag->xValidationCB )
			if( NVM_Result_Ok != pTag->xValidationCB( pTag, pNewData ) )
				break;
		
		// Записать в NVM
		Result = NVM_Result_FailNVM;
		// Проверить температуру чипа перед записью в EEPROM
		if( NVM_State.EEPROM_TempLock1 )
			break;
		if( !EEPROM_WriteCheck( pTag->ID, pNewData, pTag->Size, pWrittenData ) )
			break;

		// Скопировать записанный блок в ОЗУ
		if( NULL == pWrittenData )
			pWrittenData = pNewData;
		if( NULL != pTag->xResetCB )
			Result = pTag->xResetCB( pTag, pWrittenData );
		else
		{
			taskENTER_CRITICAL( );
			memcpy( pTag->pLocation, pWrittenData, pTag->Size );
			taskEXIT_CRITICAL( );
			Result = NVM_Result_Ok;
		}
	} while( 0 );
	return Result;
}
*/
// ПринЯть новое значение тега и сохранить в NVM
// При повышенной температуре - отказ доступка к NVM, но в RAM значение изменЯетсЯ на новое
NVM_Result_t NVM_TagSaveNew( NVM_Tag_t const *pTag, void *pNewData, void *pWrittenData )
{
	assert_param( NVM_TagValidate( pTag ) );
	assert_param( NULL != pNewData );

	NVM_Result_t Result;
	do
	{
		// Проверить допустимость новых данных
		Result = NVM_Result_FailData;
		if( NULL != pTag->xValidationCB )
			if( NVM_Result_Ok != pTag->xValidationCB( pTag, pNewData ) )
				break;
		
		// Записать в NVM
		Result = NVM_Result_FailNVM;
		// Проверить температуру чипа перед записью в EEPROM
		if( !NVM_State.EEPROM_TempLock1 )
		{
			if( EEPROM_WriteCheck( pTag->ID, pNewData, pTag->Size, pWrittenData ) )
				Result = NVM_Result_Ok;
		}
		if( NVM_Result_Ok != Result )
		{	// Записать не удалось, но RAM надо все равно обновить
			if( NULL != pWrittenData )
				memcpy( pWrittenData, pNewData, pTag->Size );
//			Result = NVM_Result_Ok;		// Вернуть "Ок", даже если записи в NVM не произошло? иначе CalibrLooch неадекватно работает при превышении температуры модулЯ
		}

		// Скопировать записанный блок в ОЗУ
		if( NULL == pWrittenData )
			pWrittenData = pNewData;
		if( NULL != pTag->xResetCB )
//			Result = pTag->xResetCB( pTag, pWrittenData );
			pTag->xResetCB( pTag, pWrittenData );
		else
		{
			taskENTER_CRITICAL( );
			memcpy( pTag->pLocation, pWrittenData, pTag->Size );
			taskEXIT_CRITICAL( );
//			Result = NVM_Result_Ok;
		}
	} while( 0 );
	return Result;
}

#if defined (STM32L4)
// Разграничение доступа к EEPROM_Emul из разных процессов
void EE_MutexInit( void );
static void EE_MutexTake( void );
static void EE_MutexGive( void );
#endif

#include "SKLP_Service.h"	// TemperatureBoard
// Произвести запись блока в EEPROM с проверками.
// Если новый блок совпадает с содержимым EEPROM, то перезапись не производитсЯ
// После записи блок снова считываетсЯ и сверЯетсЯ с требуемым.
// EEPROM_Address	- идентификатор блока в EEPROM
// pBuffer			- блок под запись
// BufferSize		- размер блока
// pBufferResult	- если не NULL, вернуть сюда результат считываниЯ
bool EEPROM_WriteCheck( uint16_t EEPROM_Address, void *pBuffer, uint16_t BufferSize, void *pBufferResult )
{
	bool Result = true;
	bool bFirstPass = true;	// первый проход - сравнение, запись. второй проход - только сравнение

#if defined (STM32L4)
	EE_MutexTake( );
	uint8_t adata[EEPROM_BLOCK_MAXVARIABLES] = {0};
	bool isEqual = true;
	bool isWriteOk = false;
	
	// Проверить содержимое EEPROM на совпадение с буфером
	isEqual = EEPROM_isEqualEepromBuf( EEPROM_Address, ( (uint8_t*)pBuffer ), BufferSize, adata, EEPROM_BLOCK_MAXVARIABLES );
	if ( !isEqual )
	{
		// Содержимое EEPROM не совпало с блоком, произвести запись в EEPROM
		// Проверить температуру чипа перед записью в EEPROM
		if( NVM_State.EEPROM_TempLock2 )
		{
			EE_MutexGive( );
			return false;
		}
		isWriteOk = EEPROM_writeBufToEeprom( EEPROM_Address, ( (uint8_t*)pBuffer ), BufferSize );
		if ( !isWriteOk )
		{
			EE_MutexGive( );
			// в случае неуспешной записи выйти из функции с ошибкой
			return false;
		}
		// После успешной записи снова проверить содержимое EEPROM на совпадение с буфером
		Result &= EEPROM_isEqualEepromBuf( EEPROM_Address, ( (uint8_t*)pBuffer ), BufferSize, adata, EEPROM_BLOCK_MAXVARIABLES );
	}
	// Совпадение, запись не требуетсЯ. 
	// Успешно выйти из функции, при необходимости скопировав прочитанные данные в указанный буфер
	if( NULL != pBufferResult )
	{
		for ( uint16_t i = 0; i < BufferSize; i++ )
		{
			((uint8_t*)pBufferResult)[i] = adata[i];
		}
	}
	EE_MutexGive( );
#endif // #if defined (STM32L4)

#if defined (STM32F4)
	do
	{
		// Проверить содержимое EEPROM на совпадение с буфером
		if( EEPROM_COMPARE_OK == EE_CompareVariable( EEPROM_Address, pBuffer, BufferSize ) )
		{	// Совпадение. При необходимости, считать данные в указанный буфер
			if( NULL == pBufferResult )
				Result = true;
			else
				Result = ( EEPROM_READ_OK == EE_ReadVariable( EEPROM_Address, pBufferResult, BufferSize ) );
			break;
		}
		if( !bFirstPass )
			break;

		// Проверить температуру чипа перед записью в EEPROM
		if( NVM_State.EEPROM_TempLock2 )
			break;
		
		// Произвести запись
		if( EEPROM_WRITE_OK != EE_WriteVariable( EEPROM_Address, pBuffer, BufferSize ) )
			break;
		bFirstPass = false;
	} while( 1 );
#endif // #if defined (STM32F4)

	return Result;

}

#if defined (STM32L4)

/**
  * @brief  Произвести запись буфера в EEPROM.
  * @note  	
  *
  *
  * @param  EEPROM_Address	- идентификатор блока в EEPROM
  * @param  pBuffer			- блок под запись
  * @param  BufferSize		- размер блока
  * @retval bool isWriteOk:	0 - не успешно, 1 - успешнаЯ запись.
  */

/**
  * @brief  Проверить содержимое EEPROM на совпадение с буфером.
  * @note  	
  *
  *
  * @param  EEPROM_Address	- идентификатор блока в EEPROM
  * @param  pBuffer			- блок под запись
  * @param  BufferSize		- размер блока
  * @param  pReadBuffer		- буфер с результатом чтениЯ из EEPROM 
  * @param  ReadBufferSize	- размер буфера с результатом чтениЯ из EEPROM
  * @retval bool isEqual:	0 - не совпали, 1 - совпали.
  */
#if defined (STM32L4)
static bool EEPROM_isEqualEepromBuf( uint16_t EEPROM_Address, uint8_t *pBuffer, uint16_t BufferSize, uint8_t *pReadBuffer, uint16_t ReadBufferSize )
{
	assert_param( pBuffer != NULL );
	assert_param( pReadBuffer != NULL );
	assert_param( ReadBufferSize >= BufferSize );	// буфер на результат чтениЯ должен быть равен или больше размера блока
	
	EE_DATA_TYPE data = 0U;
	EE_Status status = EE_NO_DATA;
	bool isEqual = true;
	// Проверить содержимое EEPROM на совпадение с буфером
	for ( uint16_t i = 0; i < BufferSize; i++ )
	{
		uint16_t address = EEPROM_Address * EEPROM_BLOCK_MAXVARIABLES + i;
		data = 0U;
		// прочитать переменную в data
		//status = ReadVariable( address, &data );
		status = EE_ReadVariable32bits( address, &data );
		if( EE_OK != status )
		{
			// если чтение неуспешно, то выйти из функции с ошибкой
			return false;
		}
		// сравнить прочитанный байт из EEPROM с байтом из блока
		if ( (uint8_t)data == pBuffer[i] )
		{
			// если равны, то положить байт во временный массив
			pReadBuffer[i] = (uint8_t)data;
		}
		else
		{
			// если не равны, то изменить переменную isEqual
			isEqual &= false;
		}
	}
	return isEqual;
}
#endif // #if defined (STM32L4)

static bool EEPROM_writeBufToEeprom( uint16_t EEPROM_Address, uint8_t *pBuffer, uint16_t BufferSize )
{
	assert_param( pBuffer != NULL );
	
	EE_DATA_TYPE data = 0U;
	EE_Status status = EE_NO_DATA;
	// произвести запись в EEPROM
	for ( uint16_t i = 0; i < BufferSize; i++ )
	{
		uint16_t address = EEPROM_Address * EEPROM_BLOCK_MAXVARIABLES + i;
		data = 0U;
		data = pBuffer[i];
		//status = WriteVariable( address, data );
		status = EE_WriteVariable32bits( address, data );
		if( EE_STATUSMASK_CLEANUP & status )
			status = EE_CleanUp( );	// EE_CleanUp_IT заменен на EE_CleanUp 19.12.2020 после теста модулЯ питаниЯ
		if( EE_STATUSMASK_ERROR & status )
			return false;
	}
	return true;
}

/**
  * @brief  Произвести чтение EEPROM в буфер.
  * @note  	
  *
  *
  * @param  EEPROM_Address	- идентификатор блока в EEPROM
  * @param  pBuffer			- буфер под чтение
  * @param  BufferSize		- размер буфера под чтение
  * @retval bool isWriteOk:	0 - не успешно, 1 - успешнаЯ запись.
  */
static bool EEPROM_readEepromToBuf( uint16_t EEPROM_Address, uint8_t *pBuffer, uint16_t BufferSize )
{
	assert_param( pBuffer != NULL );
	
	EE_DATA_TYPE data = 0U;
	EE_Status status = EE_NO_DATA;
	// произвести чтение из EEPROM
	for ( uint16_t i = 0; i < BufferSize; i++ )
	{
		uint16_t address = EEPROM_Address * EEPROM_BLOCK_MAXVARIABLES + i;
		data = 0U;
		//status = ReadVariable( address, &data );
		status = EE_ReadVariable32bits( address, &data ); 
		if( EE_STATUSMASK_ERROR & status )
			return false;
		pBuffer[i] = data;
	}
	return true;
}
#endif // #if defined (STM32L4)


#if defined (STM32L4)
// Переопределить функцию чтениЯ тэга NVM 
uint16_t EE_ReadVariable( uint16_t VirtAddress, void *pData, uint16_t Size )
{
//static char aTagMsg[] = "Tag #0";
//DWT_AppendTimestampTag( aTagMsg );
//aTagMsg[5]++;

	EE_MutexTake( );
	bool isReadOk = false;
	uint16_t result = 0;
	isReadOk = EEPROM_readEepromToBuf( VirtAddress, pData, Size );
	if ( isReadOk )
	{
		result = 1;
	}
	EE_MutexGive( );
	return result;
}

// Переопределить функцию записи тэга NVM 
uint16_t EE_WriteVariable( uint16_t VirtAddress, void *pData, uint16_t Size )
{
	EE_MutexTake( );
	bool isWriteOk = false;
	uint16_t result = HAL_ERROR;
	isWriteOk = EEPROM_writeBufToEeprom( VirtAddress, pData, Size );
	if ( isWriteOk )
	{
		result = HAL_OK;
	}
	EE_MutexGive( );
	return result;
}

#ifdef	EEPROM_EMUL_L4_MUTEX
// Разграничение доступа к EEPROM_Emul из разных процессов
#include "FreeRTOS.h"
#include "Semphr.h"

static SemaphoreHandle_t xEE_MutexHdl = NULL;

void EE_MutexInit( void )
{
	assert_param( xTaskGetSchedulerState( ) == taskSCHEDULER_NOT_STARTED );
	assert_param( NULL == xEE_MutexHdl );
	xEE_MutexHdl = xSemaphoreCreateMutex();
	assert_param( NULL != xEE_MutexHdl );
}

static void EE_MutexTake( void )
{
	assert_param( NULL != xEE_MutexHdl );
	assert_param( pdFALSE != xSemaphoreTake( xEE_MutexHdl, pdMS_TO_TICKS( 10 ) ) );
}

static void EE_MutexGive( void )
{
	assert_param( NULL != xEE_MutexHdl );
	assert_param( pdFALSE != xSemaphoreGive( xEE_MutexHdl ) );
}

#else
void EE_MutexInit( void )			{}
static void EE_MutexTake( void )	{}
static void EE_MutexGive( void )	{}

#endif	// EEPROM_EMUL_L4_MUTEX

#endif // #if defined (STM32L4)


