// Driver_IMU_InvenSense.c
// ������� 9-������ �������� �������� (Inertial measurement unit) ������������ TDK-InvenSense:
// - ������ MPU-9250		(3xAccelerometer, 3xGyroscope and AK8963 3xMagnetometer)
// - ������ ICM-20948	(3xAccelerometer, 3xGyroscope and AK09916 3xMagnetometer)
// ����� ����� ���������������� ��������� I2C/SPI:
// - InitSerial/Read/Write
// - Poll/IT/DMA
// - IRQ
#include "ProjectConfig.h"			// ������ ���������
#include "stm32xxxx_hal.h"			// ����� ���������
#include "Platform_common.h"		// ���������-��������� ��������
#include "Driver_IMU_InvenSense.h"	// ������
//#include "MPU9250_RegisterMap.h"	// �������� ��������� MPU9250

// �������� ��� ������ � IMU ����� ���������������� ���������
#define IMU_SERIAL_TIMEOUT_ms		25	// [��]	�������� ���������� ��������
#define IMU_ACCESS_TIMEOUT_ms		10	// [��]	�������� ��� ���������� �������

#if		defined( IMU_USE_I2C )
// ****************************************************
// ****************************************************
// ���������� ������ � IMU MPU-9250/ICM-20948 ����� I2C
// ****************************************************
// ****************************************************
#if		defined( STM32F3 )
#include "stm32f3xx_hal_i2c.h"
#elif	defined( STM32F4 )
#include "stm32f4xx_hal_i2c.h"
#elif	defined( STM32L4 )
#include "stm32l4xx_hal_i2c.h"
#else
#error "Select Target STM32 Family!"
#endif	// STM32xx

// ����������, ����� ���������� �������� ������������
#if ( defined ( IMU_I2C_USE_POLLING ) && defined ( IMU_I2C_USE_EVENTS ) ) || ( !defined ( IMU_I2C_USE_POLLING ) && !defined ( IMU_I2C_USE_EVENTS ) )
#error "Select IMU_I2C_USE_POLLING _OR_ IMU_I2C_USE_EVENTS!"
#endif	// IMU_I2C_USE_XXX

// ����������� ������������������ ������ I2C ��� IMU.
// ��� ������������� ������ � ������� ������������ �� ������ ������� ����� ���� �����,
// ����������� ����������� ������ � �������������� ������������ ������.
static I2C_HandleTypeDef I2C_IMU_Hdl;
static I2C_HandleTypeDef * const pIMU_I2C_Hdl = &I2C_IMU_Hdl;

#ifdef	IMU_I2C_USE_EVENTS
#include "TaskConfig.h"		// FreeRTOS & EventGroup_System
static DMA_HandleTypeDef IMU_I2C_DMA_TX_Hdl, IMU_I2C_DMA_RX_Hdl;
#endif	// IMU_I2C_USE_EVENTS

// ������������� ����������������� ���������� I2C
HAL_StatusTypeDef IMU_SerialInit( void )
{
	pIMU_I2C_Hdl->Instance = IMU_I2C_INSTANCE;
	I2C_InitTypeDef *pInit = &pIMU_I2C_Hdl->Init;
#if		defined( STM32F4 )
	pInit->ClockSpeed		= IMU_I2C_CLOCK_SPEED;
	pInit->DutyCycle		= I2C_DUTYCYCLE_2;
#elif	defined( STM32L4 )
	pInit->Timing			= IMU_I2C_TIMING;
#else
	#error "I2C driver support only STM32F4xx & STM32L4xx!"
#endif	// STM32xx
	pInit->OwnAddress1		= 0;
	pInit->AddressingMode	= I2C_ADDRESSINGMODE_7BIT;
	pInit->DualAddressMode	= I2C_DUALADDRESS_DISABLE;
	pInit->OwnAddress2		= 0;
	pInit->GeneralCallMode	= I2C_GENERALCALL_DISABLE;
	pInit->NoStretchMode	= I2C_NOSTRETCH_DISABLE;
	HAL_StatusTypeDef Result = HAL_I2C_Init( pIMU_I2C_Hdl );

	// Enable the Analog I2C Filter
//	HAL_I2CEx_ConfigAnalogFilter( pIMU_I2C_Hdl, I2C_ANALOGFILTER_ENABLE );		// �����? ����� �� �������

	// ��������� ������ � ���� I2C
	assert_param( NULL != EventGroup_System );
	( void ) xEventGroupSetBits( EventGroup_System, EVENTSYSTEM_IMU_I2C_MUTEX );

	return Result;
}

void HAL_I2C_MspInit( I2C_HandleTypeDef *hi2c )
{
	if( pIMU_I2C_Hdl == hi2c )
	{
#ifdef	STM32L4
		// -1- Configure the I2C clock source. The clock is derived from the PCLK1
		RCC_PeriphCLKInitTypeDef  RCC_PeriphCLKInitStruct;
		IMU_I2C_RCC_PERIF_INIT_STRUCT( RCC_PeriphCLKInitStruct );
		assert_param( HAL_OK == HAL_RCCEx_PeriphCLKConfig( &RCC_PeriphCLKInitStruct ) );
#endif	// STM32L4

		// -2- Enable peripherals and GPIO Clocks
		GPIO_CLK_ENABLE( IMU_I2C_PORT );
		I2C_CLK_ENABLE( IMU_I2C_INSTANCE );

		// -3- Configure peripheral GPIO
		GPIO_InitTypeDef GPIO_InitStruct;
		GPIO_InitStruct.Pin			= IMU_I2C_PIN_SCL | IMU_I2C_PIN_SDA;
		GPIO_InitStruct.Mode		= GPIO_MODE_AF_OD;
		GPIO_InitStruct.Pull		= GPIO_NOPULL;				// GPIO_PULLUP (external pullup required) 
		GPIO_InitStruct.Speed		= GPIO_SPEED_FREQ_LOW;		// GPIO_SPEED_FREQ_VERY_HIGH
		GPIO_InitStruct.Alternate	= IMU_I2C_SCL_SDA_AF( IMU_I2C_INSTANCE );
		HAL_GPIO_Init( IMU_I2C_PORT, &GPIO_InitStruct );

#ifdef	IMU_I2C_USE_EVENTS
		// -4- Configure the DMA streams ##########################################
#ifndef	STM32L4
#error	"Only for STM32L4xx, need add code for other cores!"
#endif	// STM32L4
		DMA_CLK_ENABLE( IMU_I2C_DMA_TX_CHANNEL );
		DMA_HandleTypeDef *pDMA_hdl;
		DMA_InitTypeDef DmaInit;
		
		DmaInit.Request 			= IMU_I2C_DMA_TX_REQUEST;
		DmaInit.Direction			= DMA_MEMORY_TO_PERIPH;
		DmaInit.PeriphInc			= DMA_PINC_DISABLE;
		DmaInit.MemInc				= DMA_MINC_ENABLE;
		DmaInit.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		DmaInit.MemDataAlignment	= DMA_MDATAALIGN_BYTE;
		DmaInit.Mode				= DMA_NORMAL;
		DmaInit.Priority			= IMU_I2C_DMA_PRIORITY;
		pDMA_hdl			= &IMU_I2C_DMA_TX_Hdl;
		pDMA_hdl->Instance	= IMU_I2C_DMA_TX_CHANNEL;
		pDMA_hdl->Init		= DmaInit;
		assert_param( HAL_OK == HAL_DMA_Init( pDMA_hdl ) );
		__HAL_LINKDMA( pIMU_I2C_Hdl, hdmatx, *pDMA_hdl );

		DmaInit.Request 			= IMU_I2C_DMA_RX_REQUEST;
		DmaInit.Direction			= DMA_PERIPH_TO_MEMORY;
		pDMA_hdl			= &IMU_I2C_DMA_RX_Hdl;
		pDMA_hdl->Instance	= IMU_I2C_DMA_RX_CHANNEL;
		pDMA_hdl->Init		= DmaInit;
		assert_param( HAL_OK == HAL_DMA_Init( pDMA_hdl ) );
		__HAL_LINKDMA( pIMU_I2C_Hdl, hdmarx, *pDMA_hdl );

		// -5- Configure the NVIC for DMA #########################################
		HAL_NVIC_SetPriority( IMU_I2C_DMA_TX_IRQn, IMU_I2C_DMA_IRQ_PREEMTPRIORITY, IMU_I2C_DMA_IRQ_SUBPRIORITY );
		HAL_NVIC_EnableIRQ( IMU_I2C_DMA_TX_IRQn );
		HAL_NVIC_SetPriority( IMU_I2C_DMA_RX_IRQn, IMU_I2C_DMA_IRQ_PREEMTPRIORITY, IMU_I2C_DMA_IRQ_SUBPRIORITY );
		HAL_NVIC_EnableIRQ( IMU_I2C_DMA_RX_IRQn );

		// -6- Configure the NVIC for I2C ########################################
		HAL_NVIC_SetPriority( IMU_I2C_EV_IRQn, IMU_I2C_IRQ_PREEMTPRIORITY, IMU_I2C_IRQ_SUBPRIORITY );
		HAL_NVIC_EnableIRQ( IMU_I2C_EV_IRQn );
		HAL_NVIC_SetPriority( IMU_I2C_ER_IRQn, IMU_I2C_IRQ_PREEMTPRIORITY, IMU_I2C_IRQ_SUBPRIORITY );
		HAL_NVIC_EnableIRQ( IMU_I2C_ER_IRQn );
#endif	// IMU_I2C_USE_EVENTS
	}
	else
 		assert_param( 0 );
}

void HAL_I2C_MspDeInit( I2C_HandleTypeDef *hi2c )
{
	assert_param( 0 );
}

#if		defined ( IMU_I2C_USE_POLLING )
// ***************************************************************************
// ***************************************************************************
// ����� � IMU MPU-9250/ICM-20948 ����� I2C ����� ������� � ����������� ������
// ***************************************************************************
// ***************************************************************************

// �������� ����� � IMU ����� I2C � ����������� ������
// ������ ���� � ������ ������ ���� ������� ��������
HAL_StatusTypeDef IMU_WriteBuffer( IMU_Address_t DevAddress, void *pBuffer, uint16_t BufferSize )
{
	while( HAL_I2C_GetState( pIMU_I2C_Hdl ) != HAL_I2C_STATE_READY );
	return HAL_I2C_Master_Transmit( pIMU_I2C_Hdl, DevAddress.I2C, pBuffer, BufferSize, IMU_SERIAL_TIMEOUT_ms );
}

// ��������� ����� �� IMU ����� I2C � ����������� ������
// Address		- ����� ������� ��������
// pBuffer		- ����� ��� ���������� ����������
// BufferSize	- ���������� ���� ��� ������
HAL_StatusTypeDef IMU_ReadBuffer( IMU_Address_t DevAddress, uint8_t Address, uint8_t *pBuffer, uint16_t BufferSize )
{
	HAL_StatusTypeDef Result;
  
	do
	{
		if( NULL == pBuffer )
		{
			Result = HAL_ERROR;
			break;
		}

		// �������� �����
		while( HAL_I2C_GetState( pIMU_I2C_Hdl ) != HAL_I2C_STATE_READY );
		Result = HAL_I2C_Master_Transmit( pIMU_I2C_Hdl, DevAddress.I2C, &Address, sizeof( Address ), IMU_SERIAL_TIMEOUT_ms );
		if( HAL_OK != Result )
			break;
		
		// ������� �����
		while( HAL_I2C_GetState( pIMU_I2C_Hdl ) != HAL_I2C_STATE_READY );
		Result = HAL_I2C_Master_Receive( pIMU_I2C_Hdl, DevAddress.I2C, pBuffer, BufferSize, IMU_SERIAL_TIMEOUT_ms );
		if( HAL_OK != Result )
			break;
	} while( 0 );
  return Result;
}

#elif	defined ( IMU_I2C_USE_EVENTS )
// *******************************************************************
// *******************************************************************
// ����� � IMU MPU-9250/ICM-20948 ����� I2C ����� DMA � RTOS
// *******************************************************************
// *******************************************************************

// ��������� �������� ��� �������� � ����� ������� ����� I2C �������������,
// �� ������� ��������� ������� ��� ������ ����� ������� RTOS,
// ����� ���������� ������� ����� ��� ��� ������, ��� � ��� ������.
typedef HAL_StatusTypeDef ( *HAL_I2C_Master_Operation_DMA_t ) ( I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size );
static HAL_StatusTypeDef IMU_I2C_Operation( HAL_I2C_Master_Operation_DMA_t xOperation, EventBits_t BitsToDetect, uint16_t DevAddress, uint8_t *pData, uint16_t Size )
{
	assert_param( NULL != EventGroup_System );
	assert_param( ( HAL_I2C_Master_Transmit_DMA == xOperation ) || ( HAL_I2C_Master_Receive_DMA == xOperation ) );
	assert_param( 0 != ( EVENTSYSTEM_IMU_I2C_ALL & BitsToDetect ) );
	assert_param( ( NULL != pData ) && ( 0 != Size ) );
	
	HAL_StatusTypeDef Result;
	do
	{
		// ��������� ���������� ������ I2C
		if( HAL_I2C_STATE_READY != HAL_I2C_GetState( pIMU_I2C_Hdl ) )
		{
			Result = HAL_BUSY;
			break;
		}
		EventBits_t BitsToWait = EVENTSYSTEM_IMU_I2C_ALL;
		( void ) xEventGroupClearBits( EventGroup_System, BitsToWait );
		// ��������� ���������� ��������
		Result = xOperation( pIMU_I2C_Hdl, DevAddress, pData, Size );
		if( HAL_OK != Result )
		{
			if( HAL_I2C_ERROR_DMA & pIMU_I2C_Hdl->ErrorCode )
			{
				HAL_DMA_Abort_IT( pIMU_I2C_Hdl->hdmarx );
				HAL_DMA_Abort_IT( pIMU_I2C_Hdl->hdmatx );
			}
			break;
		}
		// ��������� ���������� ���������� ��������
		EventBits_t EventBits = BitsToWait & xEventGroupWaitBits( EventGroup_System, BitsToWait, pdTRUE, pdFALSE, pdMS_TO_TICKS( IMU_SERIAL_TIMEOUT_ms ) );
		if( 0 == EventBits )
			Result = HAL_TIMEOUT;
		else if( BitsToDetect != EventBits )
			Result = HAL_ERROR;
		else
			Result = HAL_OK;
	} while( 0 );
	
	return Result;
}

// �������� ����� � IMU ����� I2C
// DevAddress	- ����� IMU �� ���� I2C
// pBuffer		- ����� ��� ��������, ������ ���� � ������ ������ ���� ������� ��������
// BufferSize	- ���������� ���� ��� ������
HAL_StatusTypeDef IMU_WriteBuffer( IMU_Address_t DevAddress, void *pBuffer, uint16_t BufferSize )
{
	HAL_StatusTypeDef Result = HAL_BUSY;
	do
	{
		// �������� ������ � ������ I2C
		assert_param( NULL != EventGroup_System );
		if( 0 == ( EVENTSYSTEM_IMU_I2C_MUTEX & xEventGroupWaitBits( EventGroup_System, EVENTSYSTEM_IMU_I2C_MUTEX, pdTRUE, pdFALSE, pdMS_TO_TICKS( IMU_ACCESS_TIMEOUT_ms ) ) ) )
			break;
		// ���������� ������ �� ������ � I2C
		Result = IMU_I2C_Operation( HAL_I2C_Master_Transmit_DMA, EVENTSYSTEM_IMU_I2C_TX_COMPLETE, DevAddress.I2C, pBuffer, BufferSize );
		// ���������� ������ � ������ I2C
		( void ) xEventGroupSetBits( EventGroup_System, EVENTSYSTEM_IMU_I2C_MUTEX );
	}
	while( 0 );
	return Result;
}

// ��������� ����� �� IMU ����� I2C
// DevAddress	- ����� IMU �� ���� I2C
// Address		- ����� ������� ��������
// pBuffer		- ����� ��� ���������� ����������
// BufferSize	- ���������� ���� ��� ������
HAL_StatusTypeDef IMU_ReadBuffer( IMU_Address_t DevAddress, uint8_t Address, uint8_t *pBuffer, uint16_t BufferSize )
{
	HAL_StatusTypeDef Result = HAL_BUSY;
	bool bMutexTaken = false;
	do
	{
		// �������� ������ � ������ I2C
		assert_param( NULL != EventGroup_System );
		if( 0 == ( EVENTSYSTEM_IMU_I2C_MUTEX & xEventGroupWaitBits( EventGroup_System, EVENTSYSTEM_IMU_I2C_MUTEX, pdTRUE, pdFALSE, pdMS_TO_TICKS( IMU_ACCESS_TIMEOUT_ms ) ) ) )
			break;
		bMutexTaken = true;
		// �������� ����� ��������
		if( HAL_OK != ( Result = IMU_I2C_Operation( HAL_I2C_Master_Transmit_DMA, EVENTSYSTEM_IMU_I2C_TX_COMPLETE, DevAddress.I2C, &Address, sizeof( Address ) ) ) )
			break;
		// ������� �������� � �����
		if( HAL_OK != ( Result = IMU_I2C_Operation( HAL_I2C_Master_Receive_DMA, EVENTSYSTEM_IMU_I2C_RX_COMPLETE, DevAddress.I2C, pBuffer, BufferSize ) ) )
			break;
	} while( 0 );
	// ���������� ������ � ����
	if( bMutexTaken )
		( void ) xEventGroupSetBits( EventGroup_System, EVENTSYSTEM_IMU_I2C_MUTEX );
	return Result;
}

static void HAL_IMU_I2C_MasterCallback( EventBits_t BitsToWake )
{
	assert_param( NULL != EventGroup_System );
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	assert_param( pdPASS == xEventGroupSetBitsFromISR( EventGroup_System, BitsToWake, &xHigherPriorityTaskWoken ) );
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

// ������� �� ����������� I2C.IRQ.Tx
void HAL_I2C_MasterTxCpltCallback( I2C_HandleTypeDef *I2cHandle )
{
	assert_param( pIMU_I2C_Hdl == I2cHandle );
	if( HAL_I2C_ERROR_NONE == I2cHandle->ErrorCode )
		HAL_IMU_I2C_MasterCallback( EVENTSYSTEM_IMU_I2C_TX_COMPLETE );
	else
		HAL_IMU_I2C_MasterCallback( EVENTSYSTEM_IMU_I2C_ERROR );
}

// ������� �� ����������� I2C.IRQ.Rx
void HAL_I2C_MasterRxCpltCallback( I2C_HandleTypeDef *I2cHandle )
{
	assert_param( pIMU_I2C_Hdl == I2cHandle );
	if( HAL_I2C_ERROR_NONE == I2cHandle->ErrorCode )
		HAL_IMU_I2C_MasterCallback( EVENTSYSTEM_IMU_I2C_RX_COMPLETE );
	else
		HAL_IMU_I2C_MasterCallback( EVENTSYSTEM_IMU_I2C_ERROR );
}

// ������� �� ����������� I2C.IRQ.Err
void HAL_I2C_ErrorCallback( I2C_HandleTypeDef *I2cHandle )
{
	assert_param( pIMU_I2C_Hdl == I2cHandle );
	HAL_IMU_I2C_MasterCallback( EVENTSYSTEM_IMU_I2C_ERROR );
}

// IMU I2C IRQ Handlers
void IMU_I2C_EV_IRQHandler( void )			{	HAL_I2C_EV_IRQHandler( pIMU_I2C_Hdl );	}
void IMU_I2C_ER_IRQHandler( void )			{	HAL_I2C_ER_IRQHandler( pIMU_I2C_Hdl );	}
void IMU_I2C_DMA_TX_IRQHandler( void )		{	HAL_DMA_IRQHandler( pIMU_I2C_Hdl->hdmatx );	}
void IMU_I2C_DMA_RX_IRQHandler( void )		{	HAL_DMA_IRQHandler( pIMU_I2C_Hdl->hdmarx );	}

#endif	// IMU_I2C_USE_POLLING or IMU_I2C_USE_EVENTS

#elif	defined( IMU_USE_SPI )
// ****************************************************
// ****************************************************
// ���������� ������ � IMU MPU-9250/ICM-20948 ����� SPI
// ****************************************************
// ****************************************************
#if 	defined( STM32F3 )
#include "stm32f3xx_hal_spi.h"
#elif	defined( STM32F4 )
#include "stm32f4xx_hal_spi.h"
#elif	defined( STM32L4 )
#include "stm32l4xx_hal_spi.h"
#else
#error "Select Target STM32 Family!"
#endif	// STM32xx

#error "IMU via SPI not supported!"

#endif	// IMU_USE_I2C or IMU_USE_SPI

// ��������� ���� ���� �� IMU
// DevAddress	- ����� IMU �� ����
// Address		- ����� ��������
// pByte		- ����� ��� ���������� ����������
HAL_StatusTypeDef IMU_ReadByte( IMU_Address_t DevAddress, uint8_t Address, uint8_t *pByte )
{
	return IMU_ReadBuffer( DevAddress, Address, pByte, sizeof( *pByte ) );
}

// �������� ���� ���� � IMU
// DevAddress	- ����� IMU �� ����
// Address		- ����� ��������
// pByte		- ����� ��� ���������� ����������
HAL_StatusTypeDef IMU_WriteByte( IMU_Address_t DevAddress, uint8_t Address, uint8_t Byte )
{
	uint8_t aBuffer[] = { Address, Byte };
	return IMU_WriteBuffer( DevAddress, aBuffer, sizeof( aBuffer ) );
}

/*/ ***************************************************
// ***************************************************
// ���������� ������ � AK8963
// ����� ������������ ��������� ����� ��������������� ����� I2C � MPU9250
// ***************************************************
// ***************************************************

// ��������� ��� ���������������� ������ MPU9250 � ����������� ����������� ����� ��������������� I2C
#pragma	pack( 1 )
typedef struct MPU9250_SlavePacketConfig_struct
{
	uint8_t	ConfigAddress;		// ����� ��������, ��� ��������� ��� ����� ������������
	uint8_t	SlaveAddress;		// ����� ������, ���� ������/������
	uint8_t	SlaveRegister;		// ����� ������� �������� ��� �����/�������� ������
	uint8_t	Control;			// ���������� �����������
} MPU9250_SlavePacketConfig_t;
#pragma	pack( )

// ��������� ����� �� ������������ AK8963
// SubAddress	- ����� ������� ��������
// pBuffer		- ����� ��� ���������� ����������
// BufferSize	- ���������� ���� ��� ������
HAL_StatusTypeDef AK8963_ReadBuffer( uint8_t SubAddress, uint8_t *pBuffer, uint8_t BufferSize )
{
	HAL_StatusTypeDef Result;
	do
	{
		// Config read buffer transaction from Slave0
		MPU9250_SlavePacketConfig_t PacketConfig =
		{
			MPU9250_I2C_SLV0_ADDR,													// Config transaction for Slave0
			MPU9250_MAG_ADDRESS | MPU9250_I2C_SLV0_RNW_MASK,						// Set slave 0 to the AK8963 and set for read
			SubAddress,																// Set the register to the desired AK8963 sub address
			MPU9250_I2C_SLV0_EN_MASK | ( BufferSize & MPU9250_I2C_SLV0_LENG_MASK )	// Enable I2C and request the bytes
		};
		if( HAL_OK != ( Result = MPU9250_WriteBuffer( &PacketConfig, sizeof( PacketConfig ) ) ) )
			break;
		// Wait for end of slave transaction
		HAL_Delay( 1 );
		// Read the bytes off the MPU9250 EXT_SENS_DATA registers
		if( HAL_OK != ( Result = MPU9250_ReadBuffer( MPU9250_EXT_SENS_DATA_00, pBuffer, BufferSize ) ) )
			break;
	}
	while( 0 );
	return Result;
}

// �������� ���� � ����������� AK8963 ����� I2C
// SubAddress	- ����� ��������
// Byte			- ���� ��� ������ � �������
HAL_StatusTypeDef AK8963_WriteByte( uint8_t SubAddress, uint8_t Byte )
{
	HAL_StatusTypeDef Result;
	do
	{
		// Config write buffer transaction to Slave0
		MPU9250_SlavePacketConfig_t PacketConfig =
		{
			MPU9250_I2C_SLV0_ADDR,														// Config transaction for Slave0
			MPU9250_MAG_ADDRESS | ( 0 & MPU9250_I2C_SLV0_RNW_MASK ),					// Set slave 0 to the AK8963 and set for write
			SubAddress,																	// Set the register to the desired AK8963 sub address
			MPU9250_I2C_SLV0_EN_MASK | ( sizeof( Byte ) & MPU9250_I2C_SLV0_LENG_MASK )	// Enable I2C and request the bytes
		};
		if( HAL_OK != ( Result = MPU9250_WriteBuffer( &PacketConfig, sizeof( PacketConfig ) ) ) )
			break;
		// Store the data for write
		if( HAL_OK != ( Result = MPU9250_WriteByte( MPU9250_I2C_SLV0_DO, Byte ) ) )
			break;
		// Wait for write complete
		HAL_Delay( 1 );
		// Read the register and confirm
		uint8_t WrittenByte;
		if( HAL_OK != ( Result = AK8963_ReadBuffer( SubAddress, &WrittenByte, sizeof( WrittenByte ) ) ) )
			break;
		if( WrittenByte != Byte )
			break;
	}
	while( 0 );
	return Result;
}
*/
