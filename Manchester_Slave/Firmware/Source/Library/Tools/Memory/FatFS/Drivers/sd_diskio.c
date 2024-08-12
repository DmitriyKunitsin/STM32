/**
  ******************************************************************************
  * @file    sd_diskio.c
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    22-April-2014
  * @brief   SD Disk I/O driver
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
//#include "ff_gen_drv.h"
#include "..\ff_gen_drv.h"
#include "common_sd.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Block Size in Bytes */
#define	BLOCK_SIZE			512

/* Private variables ---------------------------------------------------------*/
/* Disk status */
static volatile DSTATUS Stat = STA_NOINIT;

/* Private function prototypes -----------------------------------------------*/
DSTATUS SD_initialize (void);
DSTATUS SD_status (void);
DRESULT SD_read (BYTE*, DWORD, BYTE);
#if _USE_WRITE == 1
  DRESULT SD_write (const BYTE*, DWORD, BYTE);
#endif /* _USE_WRITE == 1 */
#if _USE_IOCTL == 1
  DRESULT SD_ioctl (BYTE, void*);
#endif  /* _USE_IOCTL == 1 */
  
Diskio_drvTypeDef  SD_Driver =
{
  SD_initialize,
  SD_status,
  SD_read, 
#if  _USE_WRITE == 1
  SD_write,
#endif /* _USE_WRITE == 1 */
  
#if  _USE_IOCTL == 1
  SD_ioctl,
#endif /* _USE_IOCTL == 1 */
};



/*/#define	SD_FAILSTAT_REPEAT_COUNT_MAX	4
#define	SD_FAILSTAT_REPEAT_COUNT_MAX	1
//#define	SD_FAILSTAT_REPEAT_COUNT_MAX	0
#define	SD_FAILSTAT_COMP_SECTORS_COUNT		2
*/

#ifndef	SD_FAILSTAT_REPEAT_COUNT_MAX
#define	SD_FAILSTAT_REPEAT_COUNT_MAX	0		// ���������� ��������� ������� ��� ���������� �������� � ��������
#endif
#ifndef	SD_FAILSTAT_COMP_SECTORS_COUNT
#define	SD_FAILSTAT_COMP_SECTORS_COUNT	1		// ���������� �������� ��� ��������� � ������� ������������ �������� �� ���������� ��������
#endif

typedef struct SD_FailStat_struct
{
	SD_Fail_t	FailType;
	uint8_t		RepeatCount;
	uint32_t	TransactionCount;
	uint32_t	*pBuff;
	DWORD		Sector;
	BYTE		SectorCount;
	DWORD		TimeStatmp;
} SD_FailStat_t;

typedef enum SDOp_enum
{
	eSDOpNone = 0,
	eSDOpRead,
	eSDOpWrite,
	eSDOpGetStatus,
	eSDOpInit,
} SDOp_t;

typedef struct SD_Operation_struct
{
	SDOp_t		OpType;
	BYTE		SectorCount;
	DWORD		Sector;
	uint32_t	*pBuff;
} SD_Operation_t;

#ifndef	SD_FAILSTAT_OPCOUNT
#define	SD_FAILSTAT_OPCOUNT		1024
#endif	// SD_FAILSTAT_OPCOUNT

#ifndef	SD_FAILSTAT_FAILCOUNT
#define	SD_FAILSTAT_FAILCOUNT	32
#endif	// SD_FAILSTAT_FAILCOUNT

//#define	SD_TIMEOUT_TRANSFER_ms		100			// ���������� ������� �� �����/�������� ������ � SD �� DMA
// !!! � ��������� 100 �� ���� ��������������� �������� ��� ������ 34.5 �� � ����
//#define	SD_TIMEOUT_TRANSFER_ms		300			// ���������� ������� �� �����/�������� ������ � SD �� DMA
//#define	SD_TIMEOUT_READ_WRITE_ms	1000		// ���������� ������� �� ���������� �������� ������/������ SD
#define	SD_TIMEOUT_TRANSFER_TX_ms		1000		// ���������� ������� �� �������� ������ � SD �� DMA 		������������ �� 800 �� � ��� ��� ������ ��������������!!!
#define	SD_TIMEOUT_TRANSFER_RX_ms		300			// ���������� ������� �� ����� ������ �� SD �� DMA 			�� ���������. �� ���������, ��� ������� ��� ��, ��� � ��� ��������!
#define	SD_TIMEOUT_OP_COMPLETE_ms		400			// ���������� ������� �� ���������� �������� ������/������ SD	������������ �� 200 ��!!!
#define	SD_TIMEOUT_ALLOWED_ms			250			// "��������" ������� ��� ��������� � SD. ������� �������� ���������� �����������.

// ����������� ������ SD
SD_Operation_t aSD_Operations[SD_FAILSTAT_OPCOUNT] __PLACE_AT_RAM_CCM__;
SD_FailStat_t aSD_FailStat[SD_FAILSTAT_FAILCOUNT] __PLACE_AT_RAM_CCM__;	// ��� ������������ ������
static SD_Stat_t SD_Stat __PLACE_AT_RAM_CCM__;
// ����� ��� ��������� ���������� � ��������� ������
__no_init static uint8_t aCompareBuffer[ SD_FAILSTAT_COMP_SECTORS_COUNT * BLOCK_SIZE ];

// ���������� �� SD-�����
BSP_SD_CID_t SD_CID;
char aSD_CID_ASCII[70];


void SD_GetStat( SD_Stat_t *pStat )
{
	assert_param( NULL != pStat );
	ENTER_CRITICAL_SECTION( );
	*pStat = SD_Stat;
	SD_Stat.TimeoutMax_ms = 0;
	EXIT_CRITICAL_SECTION( );
}

static void SD_FailsAppend( SD_FailStat_t SD_FailStat )
{
	extern TickType_t xTaskGetTickCount( void );
	SD_FailStat.TimeStatmp = xTaskGetTickCount( );
	aSD_FailStat[ ( SD_Stat.FailCount++ ) % SIZEOFARRAY( aSD_FailStat ) ] = SD_FailStat;
	SD_Stat.aFails[SD_FailStat.FailType]++;
}

static void SD_OperationAppend( SD_Operation_t SD_Operation )
{
	aSD_Operations[ ( SD_Stat.OpCount++ ) % SIZEOFARRAY( aSD_Operations ) ] = SD_Operation;
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes a Drive
  * @param  None
  * @retval DSTATUS: Operation status
  */
DSTATUS SD_initialize(void)
{
  Stat = STA_NOINIT;
  static uint8_t bInitSuccess = 0;
  
  /* Configure the uSD device */
  aSD_CID_ASCII[0] = '\0';
  SD_CID = ( BSP_SD_CID_t ) { 0 };
  if(BSP_SD_Init() == MSD_OK)
  {
    Stat &= ~STA_NOINIT;
    bInitSuccess = 1;
	BSP_SD_GetCardCID( );
	char aOID[2] = {	( SD_CID.OEM_ApplicationID >> 8 ) & 0xFF,
						( SD_CID.OEM_ApplicationID >> 0 ) & 0xFF };
	char aPNM[5] = {	( SD_CID.ProductName2 >> 0 ) & 0xFF,
						( SD_CID.ProductName1 >> 24 ) & 0xFF,
						( SD_CID.ProductName1 >> 16 ) & 0xFF,
						( SD_CID.ProductName1 >> 8 ) & 0xFF,
						( SD_CID.ProductName1 >> 0 ) & 0xFF };
	snprintf( aSD_CID_ASCII, sizeof( aSD_CID_ASCII ),
		"MID=%02hhX OID=%.2s PNM=%.5s PRV=%hhu.%hhu PSN=%08X MDT=%hu.%hhu",
		( uint8_t ) SD_CID.ManufacturerID,
		aOID, aPNM,
		( uint8_t ) SD_CID.ProductVersionN, ( uint8_t ) SD_CID.ProductVersionM,
		( uint32_t ) SD_CID.ProductSerialNumber,
		( uint16_t ) ( SD_CID.ManufacturingDateYear + 2000 ), ( uint8_t ) SD_CID.ManufacturingDateMonth );
  }
  else
	{
		if( bInitSuccess )
		{
			SD_FailsAppend( ( SD_FailStat_t ) { eSD_Fail_Init, 0, 0, 0, 0, 0, 0 } );
		}
		bInitSuccess = 0;
	}
  SD_OperationAppend( ( SD_Operation_t ) { eSDOpInit, 0, 0, NULL } );
  return Stat;
}

/**
  * @brief  Gets Disk Status
  * @param  None
  * @retval DSTATUS: Operation status
  */
DSTATUS SD_status(void)
{
  Stat = STA_NOINIT;

//  if(BSP_SD_GetCardState() == HAL_OK)
  if(BSP_SD_GetCardState() == SD_TRANSFER_OK)
  {
    Stat &= ~STA_NOINIT;
  }
  else
  {
	  SD_FailsAppend( ( SD_FailStat_t ) { eSD_Fail_GetStatus, 0, 0, 0, 0, 0, 0 } );
  }
  SD_OperationAppend( ( SD_Operation_t ) { eSDOpGetStatus, 0, 0, NULL } );
  return Stat;
}


// ������ � ������ �������� � ����������� ��� �������
// pSD_FailStat		���������� � ��������, �� ����������� �� �������
// RepeatCountMax	���������� ���������� ������������
#include "ProjectConfig.h"		// ������ ���������, ������ ������.
#include "common_gpio.h"		// GPIO_Common_Write()
#include "common_rcc.h"			// DWT_Timer
#include "TaskConfig.h"
#include "Logger.h"
typedef uint8_t ( *pBSP_SD_ReadWrite_t ) ( uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks );
DRESULT SD_ReadWrite( pBSP_SD_ReadWrite_t pBSP_SD_ReadWrite, SD_FailStat_t *pSD_FailStat, uint8_t RepeatCountMax )
{
	DRESULT Result = RES_OK;

	GPIO_Common_Write( iGPIO_TestPinSD, GPIO_PIN_SET );
	do
	{
		if( NULL == pSD_FailStat )
		{
			Result = RES_PARERR;
			break;
		}

		if( RepeatCountMax > SD_FAILSTAT_REPEAT_COUNT_MAX )
		{
			Result = RES_PARERR;
			break;
		}

		SD_Operation_t Op = { eSDOpRead, pSD_FailStat->SectorCount, pSD_FailStat->Sector, pSD_FailStat->pBuff };
		EventBits_t EventMask;
		TickType_t TimeoutTransferDMA_ticks;
		if( BSP_SD_ReadBlocks_DMA == pBSP_SD_ReadWrite )
		{
			Op.OpType = eSDOpRead;
			EventMask = EVENTSYSTEM_FS_DMA_READ_COMPLETE;
			TimeoutTransferDMA_ticks = pdMS_TO_TICKS( SD_TIMEOUT_TRANSFER_RX_ms );
		}
		else if( BSP_SD_WriteBlocks_DMA == pBSP_SD_ReadWrite )
		{
			Op.OpType = eSDOpWrite;
			EventMask = EVENTSYSTEM_FS_DMA_WRITE_COMPLETE;
			TimeoutTransferDMA_ticks = pdMS_TO_TICKS( SD_TIMEOUT_TRANSFER_TX_ms );
		}
		else
			assert_param( 0 );
//		aSD_Operations[ ( SD_Stat.OpCount++ ) % SIZEOFARRAY( aSD_Operations ) ] = Op;
		SD_OperationAppend( Op );
		
		int i;
		for( i = 0; i <= RepeatCountMax; i++ )
		{
			static volatile int OpPhase = 0;
			uint32_t aTimestamps[4] = { 0 };
			aTimestamps[0] = DWT_TimerGet( );

			bool bResult = false;
			do
			{	
				OpPhase++;
				xEventGroupClearBits( EventGroup_System, EventMask );	// �� ������ ������ �������� �����, ������� ����� ���������� � ���������� �������� ������/������
				// �������� � SD ������� �� ������/������ �����
				HAL_StatusTypeDef ReadWriteResult;

/*				// ������������ ������ � �������� ��� ������ ����� ������ ����
				bool FakeFail = false;
				if( pSD_FailStat->Sector > 4200000 )
				{	// ����������� ����� ��������� ��������
					uint32_t Start = pSD_FailStat->Sector;
					uint32_t Finish = pSD_FailStat->Sector + pSD_FailStat->SectorCount - 1;
					const uint32_t Criteria = 1000;
					if(	( Start == ( ( Start / Criteria ) * Criteria ) ) ||
						( Finish == ( ( Finish / Criteria ) * Criteria ) ) ||
						( ( ( ( Start / Criteria + 1 ) * Criteria ) > Start ) && ( ( ( Start / Criteria + 1 ) * Criteria ) < Finish ) ) )
						FakeFail = true;
				}
				if( FakeFail )
					ReadWriteResult = HAL_OK;
				else
*/
					ReadWriteResult = ( HAL_StatusTypeDef ) pBSP_SD_ReadWrite( pSD_FailStat->pBuff, pSD_FailStat->Sector, pSD_FailStat->SectorCount );
				aTimestamps[1] = DWT_TimerGet( );
				if( HAL_OK != ReadWriteResult )
					break;

				// ������� ���������� ������/�������� ������ ����� DMA
				OpPhase++;
//				bool bWaitSuccess = ( EventMask & xEventGroupWaitBits( EventGroup_System, ( EventMask | EVENTSYSTEM_FS_DMA_ERROR ), pdTRUE, pdFALSE, TimeoutTransferDMA_ticks ) );
				bool bWaitSuccess = ( EventMask & xEventGroupWaitBits( EventGroup_System, EventMask, pdTRUE, pdTRUE, TimeoutTransferDMA_ticks ) );
				aTimestamps[2] = DWT_TimerGet( );
				if( !bWaitSuccess )
					break;

				// ��������� ���������� ��������, ��������� ��������� SD � ������� ����������� ��������
				OpPhase++;
				TickType_t WaitStartTick = xTaskGetTickCount( );
				while( 1 )
				{
					extern SD_HandleTypeDef uSdHandle;
					if( ( HAL_SD_STATE_READY == HAL_SD_GetState( &uSdHandle ) ) &&
						( HAL_SD_CARD_TRANSFER == HAL_SD_GetCardState( &uSdHandle ) ) )
					{	// �������� ��������� �������
						bResult = true;
						OpPhase = 0;
						break;
					}

					if( ( xTaskGetTickCount( ) - WaitStartTick ) > pdMS_TO_TICKS( SD_TIMEOUT_OP_COMPLETE_ms ) )
						break;	// ����� ���������� �� �������� �����
					// �������� ����� ����� ��������� ��������� ������ SDIO
					vTaskDelay( 1 );
				}
			} while( 0 );
			aTimestamps[3] = DWT_TimerGet( );
			if( !bResult )
			{	// �������� �� �������
				BSP_SD_Abort( );	// �������� - ���������� ��� ��������� �������� �� DMA, � ������ ������� �.�.
				BSP_SD_Abort( );	// ������ ���������� ������-�� �� ������� :-/
			}

			float Timeout_ms = DWT_TICKS2US( aTimestamps[3] - aTimestamps[0] ) / 1000.0f;
			SD_Stat.TimeoutAvg_ms = SD_Stat.TimeoutAvg_ms * 0.8f + Timeout_ms * 0.2f;
			if( Timeout_ms > SD_Stat.TimeoutMax_ms )
				SD_Stat.TimeoutMax_ms = Timeout_ms;

			if( Timeout_ms > SD_TIMEOUT_ALLOWED_ms )
			{	// ������� �������� �������� �����. �������� ������ � ���.
				float Timeout1_ms = DWT_TICKS2US( aTimestamps[1] - aTimestamps[0] ) / 1000.0f;
				float Timeout2_ms = DWT_TICKS2US( aTimestamps[2] - aTimestamps[1] ) / 1000.0f;
				float Timeout3_ms = DWT_TICKS2US( aTimestamps[3] - aTimestamps[2] ) / 1000.0f;
/*				assert_param( MemoryThread_BufferMutexTake( ) );
				char * const pBuff = ( char * ) aMemoryThreadTaskBuffer;
				uint32_t const BuffSize = sizeof( aMemoryThreadTaskBuffer );
*/
				static char aBuffer[100];		// ������ ������������ aMemoryThreadTaskBuffer[], �.�. SD_ReadWrite() ���������� ����� �������� �������� ��� ������, �������������� ��� ��� � aMemoryThreadTaskBuffer[]
				char * const pBuff = aBuffer;
				uint32_t const BuffSize = sizeof( aBuffer );
				uint32_t MessageSize = 0;
				assert_param( MemoryThread_SprintfMutexTake( 5 ) );
				MessageSize += snprintf( pBuff + MessageSize, BuffSize - MessageSize, "SD Timeout: %1.1f + %1.1f + %1.1f = %1.1f.", Timeout1_ms, Timeout2_ms, Timeout3_ms, Timeout_ms );
				char *pMsg;
				switch( Op.OpType )
				{
				case eSDOpRead:		pMsg = " Read %d sectors from %d.";	break;
				case eSDOpWrite:	pMsg = " Write %d sectors to %d.";	break;
				default:	assert_param( 0 );
				}
				MessageSize += snprintf( pBuff + MessageSize, BuffSize - MessageSize, pMsg, Op.SectorCount, Op.Sector );
				MemoryThread_SprintfMutexGive( );

				// ��������� ������� ������������ ���������� ����� � �������, ������������� ������������ ������� � ������
				const uint32_t MinEmptySpaceAllowed = 6;
				uint32_t MessageQueueEmptySpace;
				assert_param( MemoryThread_GetQueueSpaces( &MessageQueueEmptySpace, NULL ) );
				if( MessageQueueEmptySpace >= MinEmptySpaceAllowed )
				{	// ����� ����������
					if( MessageQueueEmptySpace == MinEmptySpaceAllowed )
					{	// ����� ���������, ��������� �������� ������ ����� ���������
						assert_param( MemoryThread_SprintfMutexTake( 5 ) );
						MessageSize += snprintf( pBuff + MessageSize, BuffSize - MessageSize, " (Overflow!)" );
						MemoryThread_SprintfMutexGive( );
					}
					assert_param( Logger_WriteRecord( pBuff, LOGGER_FLAGS_APPENDTIMESTAMP ) );
//					assert_param( Logger_WriteRecord( pBuff, LOGGER_FLAGS_APPENDTIMESTAMP | LOGGER_FLAGS_WAITEFORFS ) );
				}
				else
					;	// ����� �� ����������, ��� ������ ����� ���������
//				MemoryThread_BufferMutexGive( );
			}

			if( bResult )
				break;
		}
		pSD_FailStat->RepeatCount = i;
		if( i <= RepeatCountMax )
			Result = RES_OK;
		else
			Result = RES_ERROR;
	} while( 0 );
	GPIO_Common_Write( iGPIO_TestPinSD, GPIO_PIN_RESET );

	return Result;
}

/**
  * @brief  Reads Sector(s)
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT SD_read(BYTE *buff, DWORD sector, BYTE count)
{
	DRESULT Result;
	do
	{	// ��������� ���������
		if( ( NULL == buff ) || ( 0 == count ) || ( count > 128 ) )
		{
			Result = RES_PARERR;
			break;
		}
		// ��������� ������ �� ��������
		SD_FailStat_t SD_FailStat = { eSD_Fail_Read, 0, SD_Stat.ReadsCount, ( uint32_t * ) buff, sector, count, 0 };
		// ������� ����������� �������
		Result = SD_ReadWrite( BSP_SD_ReadBlocks_DMA, &SD_FailStat, SD_FAILSTAT_REPEAT_COUNT_MAX );
		if( 0 != SD_FailStat.RepeatCount )
			SD_FailsAppend( SD_FailStat );					// �� ������� ��������� �������� � ����� �������! �������� �������� � ������ ��������� ����������
			
		// ���������, ��������� �� ����������� ��������� ������ ��� �� ������� ��� ������ 
		if( ( 0 == SD_FAILSTAT_REPEAT_COUNT_MAX )		||	// ��������� ������ � ������� �� ������������
			( count > SD_FAILSTAT_COMP_SECTORS_COUNT )	||	// ������ ������� ������� ����� (��������������, ��� ��� �������� ����������� �������� ������� ���������� �������������� ��������� � ��������� ��������
			( RES_OK != Result ) )							// ������ ������� ������ �����������, ��� ������ ����������
			break;
		
		// ����������������, ������ �� ������� FAT ��� /root. ������� ����������� ������� �������� � ��������� ����� � �������.
		assert_param( sizeof( aCompareBuffer ) >= ( BLOCK_SIZE * count ) );
		SD_FailStat.pBuff = ( uint32_t * ) aCompareBuffer;
		int i;
		for( i = 0; i < SD_FAILSTAT_REPEAT_COUNT_MAX; i++ )
		{	// ������� ����������� ������� � ��������� �����
			Result = SD_ReadWrite( BSP_SD_ReadBlocks_DMA, &SD_FailStat, SD_FAILSTAT_REPEAT_COUNT_MAX );
			if( 0 != SD_FailStat.RepeatCount )
				SD_FailsAppend( SD_FailStat );				// �� ������� ��������� �������� � ����� �������! �������� �������� � ������ ��������� ����������
			if( RES_OK != Result )
				break;										// ������ �� �������. ����������
			// ������� �������� ��������� ������
			if( 0 == memcmp( buff, aCompareBuffer, BLOCK_SIZE * count ) )
				break;										// ������ � ������ ������ �������
			// ������ �� ������. ��������� ������
		}
		if( i > 0 )
		{	// �� ������� ������� ��������� ������ � ������� ����
			SD_FailStat.FailType = eSD_Fail_ReadCompare;
			SD_FailStat.RepeatCount = i;
			SD_FailsAppend( SD_FailStat );					// �������� ������� ���������� � ������ ��������� ����������
			if( i >= SD_FAILSTAT_REPEAT_COUNT_MAX )
				Result = RES_ERROR; 						// �� ������� ������� ��������� ������ �� ����
		}
	} while( 0 );
	SD_Stat.ReadsCount++;
	return Result;
}

/**
  * @brief  Writes Sector(s)
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
#if _USE_WRITE == 1
DRESULT SD_write(const BYTE *buff, DWORD sector, BYTE count)
{
	DRESULT Result;
	do
	{	// ��������� ���������
		if( ( NULL == buff ) || ( 0 == count ) || ( count > 128 ) )
		{
			Result = RES_PARERR;
			break;
		}
		// ��������� ������ �� ��������
		SD_FailStat_t SD_FailStat = { eSD_Fail_Write, 0, SD_Stat.WritesCount, ( uint32_t * ) buff, sector, count, 0 };

		if( ( count > SD_FAILSTAT_COMP_SECTORS_COUNT ) || ( 0 == SD_FAILSTAT_REPEAT_COUNT_MAX ) )
		{	// ����������������, ������ � ������� ������. ����������� ���������, ������ ������������
			Result = SD_ReadWrite( BSP_SD_WriteBlocks_DMA, &SD_FailStat, 0 );
			if( RES_OK != Result )
			{	// ������ �� �������. ��������� ���������� �� ������, �� ������ �� ����������
				SD_FailsAppend( SD_FailStat );					// �������� ������� ���������� � ������ ��������� ����������
				Result = RES_OK;
			}
			break;
		}

		// ����������������, ������ � ������� FAT ��� /root. �������� (��� �������������, � �������������),
		// ������� ������� � �������. ��� ��������� ������ ���������.
		int i;
		for( i = 0; i < SD_FAILSTAT_REPEAT_COUNT_MAX; i++ )
		{	// �������� ����������� �������
			SD_FailStat.FailType = eSD_Fail_Write;
			SD_FailStat.pBuff = ( uint32_t * ) buff;
			Result = SD_ReadWrite( BSP_SD_WriteBlocks_DMA, &SD_FailStat, SD_FAILSTAT_REPEAT_COUNT_MAX );
			if( 0 != SD_FailStat.RepeatCount )
				SD_FailsAppend( SD_FailStat );				// �� ������� ��������� �������� � ����� �������! �������� �������� � ������ ��������� ����������
			if( RES_OK != Result )
				break;										// ������ ������ �� �������. ����������

			// ������� ����������� ������� � ��������� �����
			assert_param( sizeof( aCompareBuffer ) >= ( BLOCK_SIZE * count ) );
			SD_FailStat.pBuff = ( uint32_t * ) aCompareBuffer;
			Result = SD_ReadWrite( BSP_SD_ReadBlocks_DMA, &SD_FailStat, SD_FAILSTAT_REPEAT_COUNT_MAX );
			if( 0 != SD_FailStat.RepeatCount )
				SD_FailsAppend( SD_FailStat );				// �� ������� ��������� �������� � ����� �������! �������� �������� � ������ ��������� ����������
			if( RES_OK != Result )
				break;										// ������ ������ �� �������. ����������
				
			// ������� �������� ��������� ������
			if( 0 == memcmp( buff, aCompareBuffer, BLOCK_SIZE * count ) )
				break;										// ������ � ������ ������ �������
			// ������ �� ������. ��������� ������, ������ � ������
		}
		if( i > 0 )
		{	// �� ������� ������� ��������� ������ � ������� ����
			SD_FailStat.FailType = eSD_Fail_ReadCompare;
			SD_FailStat.RepeatCount = i;
			SD_FailsAppend( SD_FailStat );					// �������� ������� ���������� � ������ ��������� ����������
			if( i >= SD_FAILSTAT_REPEAT_COUNT_MAX )
				Result = RES_ERROR; 						// �� ������� ������� ��������� ������ �� ����
		}
	} while( 0 );
	SD_Stat.WritesCount++;
	return Result;
}
#endif /* _USE_WRITE == 1 */

/**
  * @brief  I/O control operation
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
#if _USE_IOCTL == 1
DRESULT SD_ioctl(BYTE cmd, void *buff)
{
  DRESULT res = RES_ERROR;
  BSP_SD_CardInfo CardInfo;
  
  if (Stat & STA_NOINIT) return RES_NOTRDY;
  
  switch (cmd)
  {
  /* Make sure that no pending write process */
  case CTRL_SYNC :
    res = RES_OK;
    break;
  
  /* Get number of sectors on the disk (DWORD) */
  case GET_SECTOR_COUNT :
    BSP_SD_GetCardInfo(&CardInfo);
    assert_param( CardInfo.BlockSize == BLOCK_SIZE );
    *(DWORD*)buff = CardInfo.BlockNbr;
    res = RES_OK;
    break;
  
  /* Get R/W sector size (WORD) */
  case GET_SECTOR_SIZE :
    *(WORD*)buff = BLOCK_SIZE;
    res = RES_OK;
    break;
  
  /* Get erase block size in unit of sector (DWORD) */
  case GET_BLOCK_SIZE :
    *(DWORD*)buff = SD_RECORD_SIZE / BLOCK_SIZE;
    res = RES_OK;
    break;
  
  default:
    res = RES_PARERR;
  }
  
  return res;
}
#endif /* _USE_IOCTL == 1 */
  
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

