// NVM.c
#include "NVM.h"		// ������
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


// �������� ������������ ����
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

// ������� ��� �� RAM � ��������� �����
NVM_Result_t NVM_TagGetFromRAM( NVM_Tag_t const *pTag, void *pData )
{
	assert_param( NVM_TagValidate( pTag ) );
	assert_param( NULL != pData );

	memcpy( pData, pTag->pLocation, pTag->Size );
	return NVM_Result_Ok;
}

// ������� ��� �� NVM � ��������� �����
NVM_Result_t NVM_TagLoadReal( NVM_Tag_t const *pTag, void *pData )
{
//static char aTagMsg[] = "Tag #0";
//DWT_AppendTimestampTag( aTagMsg );
//aTagMsg[5]++;

	assert_param( NVM_TagValidate( pTag ) );
	assert_param( NULL != pData );

	NVM_Result_t Result = NVM_Result_FailNVM;
	// ������� ��� �� NVM
	if( EEPROM_READ_OK == EE_ReadVariable( pTag->ID, pData, pTag->Size ) )
		// ��������� ������������ ��������� ������
		if( NULL != pTag->xValidationCB )
			Result = pTag->xValidationCB( pTag, pData );
		else
			Result = NVM_Result_Ok;
	return Result;
}

// ��������� ��� �� NVM ��� ���������
NVM_Result_t NVM_TagLoad( NVM_Tag_t const *pTag )
{
	assert_param( NVM_TagValidate( pTag ) );

	// ������� ��� �� NVM
	if( NVM_Result_Ok != NVM_TagLoadReal( pTag, pTag->pLocation ) )
		// ������� �� NVM �� �������, ������� �������� ��-���������
		if( NULL != pTag->pDefault )
			memcpy( pTag->pLocation, pTag->pDefault, pTag->Size );
		else
			memset( pTag->pLocation, 0, pTag->Size );
	

	// ������� ������� �� ���������� ������
	NVM_Result_t Result = NVM_Result_Ok;
	if( NULL != pTag->xResetCB )
		Result = pTag->xResetCB( pTag, NULL );

	return Result;
}

// �������� ���������� RAM � NVM
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

// ��������� �� RAM � NVM
NVM_Result_t NVM_TagSave( NVM_Tag_t const *pTag, void *pWrittenData )
{
	assert_param( NVM_TagValidate( pTag ) );

	NVM_Result_t Result;
	do
	{
		Result = NVM_Result_FailNVM;
		// ��������� ����������� ���� ����� ������� � EEPROM
		if( NVM_State.EEPROM_TempLock1 )
			break;
		if( !EEPROM_WriteCheck( pTag->ID, pTag->pLocation, pTag->Size, pWrittenData ) )
			break;
		Result = NVM_Result_Ok;
	} while( 0 );
	return Result;
}

/*/ ������� ����� �������� ���� � ��������� � NVM
// ��� ���������� ����������� - ����� �������� � NVM
NVM_Result_t NVM_TagSaveNew( NVM_Tag_t const *pTag, void *pNewData, void *pWrittenData )
{
	assert_param( NVM_TagValidate( pTag ) );
	assert_param( NULL != pNewData );

	NVM_Result_t Result;
	do
	{
		// ��������� ������������ ����� ������
		Result = NVM_Result_FailData;
		if( NULL != pTag->xValidationCB )
			if( NVM_Result_Ok != pTag->xValidationCB( pTag, pNewData ) )
				break;
		
		// �������� � NVM
		Result = NVM_Result_FailNVM;
		// ��������� ����������� ���� ����� ������� � EEPROM
		if( NVM_State.EEPROM_TempLock1 )
			break;
		if( !EEPROM_WriteCheck( pTag->ID, pNewData, pTag->Size, pWrittenData ) )
			break;

		// ����������� ���������� ���� � ���
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
// ������� ����� �������� ���� � ��������� � NVM
// ��� ���������� ����������� - ����� �������� � NVM, �� � RAM �������� ���������� �� �����
NVM_Result_t NVM_TagSaveNew( NVM_Tag_t const *pTag, void *pNewData, void *pWrittenData )
{
	assert_param( NVM_TagValidate( pTag ) );
	assert_param( NULL != pNewData );

	NVM_Result_t Result;
	do
	{
		// ��������� ������������ ����� ������
		Result = NVM_Result_FailData;
		if( NULL != pTag->xValidationCB )
			if( NVM_Result_Ok != pTag->xValidationCB( pTag, pNewData ) )
				break;
		
		// �������� � NVM
		Result = NVM_Result_FailNVM;
		// ��������� ����������� ���� ����� ������� � EEPROM
		if( !NVM_State.EEPROM_TempLock1 )
		{
			if( EEPROM_WriteCheck( pTag->ID, pNewData, pTag->Size, pWrittenData ) )
				Result = NVM_Result_Ok;
		}
		if( NVM_Result_Ok != Result )
		{	// �������� �� �������, �� RAM ���� ��� ����� ��������
			if( NULL != pWrittenData )
				memcpy( pWrittenData, pNewData, pTag->Size );
//			Result = NVM_Result_Ok;		// ������� "��", ���� ���� ������ � NVM �� ���������? ����� CalibrLooch ����������� �������� ��� ���������� ����������� ������
		}

		// ����������� ���������� ���� � ���
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
// ������������� ������� � EEPROM_Emul �� ������ ���������
void EE_MutexInit( void );
static void EE_MutexTake( void );
static void EE_MutexGive( void );
#endif

#include "SKLP_Service.h"	// TemperatureBoard
// ���������� ������ ����� � EEPROM � ����������.
// ���� ����� ���� ��������� � ���������� EEPROM, �� ���������� �� ������������
// ����� ������ ���� ����� ����������� � ��������� � ���������.
// EEPROM_Address	- ������������� ����� � EEPROM
// pBuffer			- ���� ��� ������
// BufferSize		- ������ �����
// pBufferResult	- ���� �� NULL, ������� ���� ��������� ����������
bool EEPROM_WriteCheck( uint16_t EEPROM_Address, void *pBuffer, uint16_t BufferSize, void *pBufferResult )
{
	bool Result = true;
	bool bFirstPass = true;	// ������ ������ - ���������, ������. ������ ������ - ������ ���������

#if defined (STM32L4)
	EE_MutexTake( );
	uint8_t adata[EEPROM_BLOCK_MAXVARIABLES] = {0};
	bool isEqual = true;
	bool isWriteOk = false;
	
	// ��������� ���������� EEPROM �� ���������� � �������
	isEqual = EEPROM_isEqualEepromBuf( EEPROM_Address, ( (uint8_t*)pBuffer ), BufferSize, adata, EEPROM_BLOCK_MAXVARIABLES );
	if ( !isEqual )
	{
		// ���������� EEPROM �� ������� � ������, ���������� ������ � EEPROM
		// ��������� ����������� ���� ����� ������� � EEPROM
		if( NVM_State.EEPROM_TempLock2 )
		{
			EE_MutexGive( );
			return false;
		}
		isWriteOk = EEPROM_writeBufToEeprom( EEPROM_Address, ( (uint8_t*)pBuffer ), BufferSize );
		if ( !isWriteOk )
		{
			EE_MutexGive( );
			// � ������ ���������� ������ ����� �� ������� � �������
			return false;
		}
		// ����� �������� ������ ����� ��������� ���������� EEPROM �� ���������� � �������
		Result &= EEPROM_isEqualEepromBuf( EEPROM_Address, ( (uint8_t*)pBuffer ), BufferSize, adata, EEPROM_BLOCK_MAXVARIABLES );
	}
	// ����������, ������ �� ���������. 
	// ������� ����� �� �������, ��� ������������� ���������� ����������� ������ � ��������� �����
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
		// ��������� ���������� EEPROM �� ���������� � �������
		if( EEPROM_COMPARE_OK == EE_CompareVariable( EEPROM_Address, pBuffer, BufferSize ) )
		{	// ����������. ��� �������������, ������� ������ � ��������� �����
			if( NULL == pBufferResult )
				Result = true;
			else
				Result = ( EEPROM_READ_OK == EE_ReadVariable( EEPROM_Address, pBufferResult, BufferSize ) );
			break;
		}
		if( !bFirstPass )
			break;

		// ��������� ����������� ���� ����� ������� � EEPROM
		if( NVM_State.EEPROM_TempLock2 )
			break;
		
		// ���������� ������
		if( EEPROM_WRITE_OK != EE_WriteVariable( EEPROM_Address, pBuffer, BufferSize ) )
			break;
		bFirstPass = false;
	} while( 1 );
#endif // #if defined (STM32F4)

	return Result;

}

#if defined (STM32L4)

/**
  * @brief  ���������� ������ ������ � EEPROM.
  * @note  	
  *
  *
  * @param  EEPROM_Address	- ������������� ����� � EEPROM
  * @param  pBuffer			- ���� ��� ������
  * @param  BufferSize		- ������ �����
  * @retval bool isWriteOk:	0 - �� �������, 1 - �������� ������.
  */

/**
  * @brief  ��������� ���������� EEPROM �� ���������� � �������.
  * @note  	
  *
  *
  * @param  EEPROM_Address	- ������������� ����� � EEPROM
  * @param  pBuffer			- ���� ��� ������
  * @param  BufferSize		- ������ �����
  * @param  pReadBuffer		- ����� � ����������� ������ �� EEPROM 
  * @param  ReadBufferSize	- ������ ������ � ����������� ������ �� EEPROM
  * @retval bool isEqual:	0 - �� �������, 1 - �������.
  */
#if defined (STM32L4)
static bool EEPROM_isEqualEepromBuf( uint16_t EEPROM_Address, uint8_t *pBuffer, uint16_t BufferSize, uint8_t *pReadBuffer, uint16_t ReadBufferSize )
{
	assert_param( pBuffer != NULL );
	assert_param( pReadBuffer != NULL );
	assert_param( ReadBufferSize >= BufferSize );	// ����� �� ��������� ������ ������ ���� ����� ��� ������ ������� �����
	
	EE_DATA_TYPE data = 0U;
	EE_Status status = EE_NO_DATA;
	bool isEqual = true;
	// ��������� ���������� EEPROM �� ���������� � �������
	for ( uint16_t i = 0; i < BufferSize; i++ )
	{
		uint16_t address = EEPROM_Address * EEPROM_BLOCK_MAXVARIABLES + i;
		data = 0U;
		// ��������� ���������� � data
		//status = ReadVariable( address, &data );
		status = EE_ReadVariable32bits( address, &data );
		if( EE_OK != status )
		{
			// ���� ������ ���������, �� ����� �� ������� � �������
			return false;
		}
		// �������� ����������� ���� �� EEPROM � ������ �� �����
		if ( (uint8_t)data == pBuffer[i] )
		{
			// ���� �����, �� �������� ���� �� ��������� ������
			pReadBuffer[i] = (uint8_t)data;
		}
		else
		{
			// ���� �� �����, �� �������� ���������� isEqual
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
	// ���������� ������ � EEPROM
	for ( uint16_t i = 0; i < BufferSize; i++ )
	{
		uint16_t address = EEPROM_Address * EEPROM_BLOCK_MAXVARIABLES + i;
		data = 0U;
		data = pBuffer[i];
		//status = WriteVariable( address, data );
		status = EE_WriteVariable32bits( address, data );
		if( EE_STATUSMASK_CLEANUP & status )
			status = EE_CleanUp( );	// EE_CleanUp_IT ������� �� EE_CleanUp 19.12.2020 ����� ����� ������ �������
		if( EE_STATUSMASK_ERROR & status )
			return false;
	}
	return true;
}

/**
  * @brief  ���������� ������ EEPROM � �����.
  * @note  	
  *
  *
  * @param  EEPROM_Address	- ������������� ����� � EEPROM
  * @param  pBuffer			- ����� ��� ������
  * @param  BufferSize		- ������ ������ ��� ������
  * @retval bool isWriteOk:	0 - �� �������, 1 - �������� ������.
  */
static bool EEPROM_readEepromToBuf( uint16_t EEPROM_Address, uint8_t *pBuffer, uint16_t BufferSize )
{
	assert_param( pBuffer != NULL );
	
	EE_DATA_TYPE data = 0U;
	EE_Status status = EE_NO_DATA;
	// ���������� ������ �� EEPROM
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
// �������������� ������� ������ ���� NVM 
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

// �������������� ������� ������ ���� NVM 
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
// ������������� ������� � EEPROM_Emul �� ������ ���������
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


