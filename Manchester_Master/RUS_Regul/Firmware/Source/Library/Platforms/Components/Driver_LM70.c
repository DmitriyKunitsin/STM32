// Driver_LM70.c
// ƒрайвер температурных датчиков LM70
//#include "DeviceConfig.h"
#include "stm32f4xx_hal.h"		// дрова периферии
#include "ProjectConfig.h"
#include "Driver_LM70.h"		// родной
//#include "spi_driver.h"
//#include "port_driver.h"
#include "Platform_Common.h"
#include "common_GPIO.h"
//#include "device_driver.h"	// assert()

//  оманды дл€ термодатчиков
#define LM70_CMD_NORM	0x0000		// continuous conversion mode
#define LM70_CMD_SHDWN	0xFFFF		// shutdown mode

// ¬ ProjectConfig.h
#define	LM70_SPI				SPI3
#define	LM70_SPI_hdl			SPI3_hdl
#define	LM70_SPI_MISO_GPIO		GPIOB
#define	LM70_SPI_MISO_PIN		GPIO_PIN_4
#define	LM70_SPI_MISO_AF		GPIO_AF6_SPI3
#define	LM70_SPI_MOSI_GPIO		GPIOB
#define	LM70_SPI_MOSI_PIN		GPIO_PIN_5
#define	LM70_SPI_MOSI_AF		GPIO_AF6_SPI3
#define	LM70_SPI_SCK_GPIO		GPIOB
#define	LM70_SPI_SCK_PIN		GPIO_PIN_3
#define	LM70_SPI_SCK_AF			GPIO_AF6_SPI3

//#define LM70_SPI_FRQMAX			6e6f
//#define LM70_SPI_FRQMAX			2e6f
#define LM70_SPI_FRQMAX			2000000


// ******************* ѕеренести в common_SPI.c *******************
typedef struct SPIext_Handle_struct
{
	SPI_HandleTypeDef	CommonHdl;
//	SemaphoreHandle_t	Mutex;
	bool				MutexTaken;
} SPIext_Handle_t;

SPIext_Handle_t SPI3_hdl /*__PLACE_AT_RAM_CCM__*/;

SPIext_Handle_t *pLM70_SPI_hdl = &LM70_SPI_hdl;
SPI_InitTypeDef LM70_SPI_Init /*__PLACE_AT_RAM_CCM__*/ = { 0 };

// «ахватить канал SPI
bool SPIext_Capture( SPIext_Handle_t *pSPIext_hdl )
{
	bool Result = false;
	ENTER_CRITICAL_SECTION( );
	if( !pSPIext_hdl->MutexTaken )
		if( HAL_SPI_STATE_RESET == HAL_SPI_GetState( &pSPIext_hdl->CommonHdl ) )
		{
			pSPIext_hdl->MutexTaken = true;
			Result = true;
		}
	EXIT_CRITICAL_SECTION( );

	return Result;
}

// ќсвободить канал SPI
void SPIext_Release( SPIext_Handle_t *pSPIext_hdl )
{
	ENTER_CRITICAL_SECTION( );
		pSPIext_hdl->MutexTaken = false;
	EXIT_CRITICAL_SECTION( );
}

// –ассчитать подходящий предделитель
bool SPI_SetupPrescaler( SPI_HandleTypeDef *pSPI_hdl, uint32_t FrqMax )
{
	bool Result = false;
	do
	{
		if( NULL == pSPI_hdl )
			break;

		uint32_t PCLKFreq;
		if( SPI1 == pSPI_hdl->Instance )
			PCLKFreq = HAL_RCC_GetPCLK1Freq( );
		else if( ( SPI2 == pSPI_hdl->Instance ) || ( SPI3 == pSPI_hdl->Instance ) )
			PCLKFreq = HAL_RCC_GetPCLK2Freq( );
		else
			break;

		uint32_t PrescalerValue = PCLKFreq / FrqMax + 1;
		uint32_t PrescalerConfig;
		if( PrescalerValue <= 2 )
			PrescalerConfig = SPI_BAUDRATEPRESCALER_2;
		else if( PrescalerValue <= 4 )
			PrescalerConfig = SPI_BAUDRATEPRESCALER_4;
		else if( PrescalerValue <= 8 )
			PrescalerConfig = SPI_BAUDRATEPRESCALER_8;
		else if( PrescalerValue <= 16 )
			PrescalerConfig = SPI_BAUDRATEPRESCALER_16;
		else if( PrescalerValue <= 32 )
			PrescalerConfig = SPI_BAUDRATEPRESCALER_32;
		else if( PrescalerValue <= 64 )
			PrescalerConfig = SPI_BAUDRATEPRESCALER_64;
		else if( PrescalerValue <= 128 )
			PrescalerConfig = SPI_BAUDRATEPRESCALER_128;
		else if( PrescalerValue <= 256 )
			PrescalerConfig = SPI_BAUDRATEPRESCALER_256;
		else
			break;

		pSPI_hdl->Init.BaudRatePrescaler = PrescalerConfig;
		Result = true;
	} while( 0 );

	return Result;
}

void HAL_SPI_MspInit( SPI_HandleTypeDef *hspi )
{
	assert_param( hspi == &pLM70_SPI_hdl->CommonHdl );

	GPIO_InitTypeDef  GPIO_InitStruct;
	
	/*##-1- Enable peripherals and GPIO Clocks #################################*/
	/* Enable GPIO TX/RX clock */
	GPIO_CLK_ENABLE( LM70_SPI_MISO_GPIO );
	GPIO_CLK_ENABLE( LM70_SPI_MOSI_GPIO );
	GPIO_CLK_ENABLE( LM70_SPI_SCK_GPIO );
	/* Enable SPI clock */
	SPI_CLK_ENABLE( LM70_SPI ); 
	
	/*##-2- Configure peripheral GPIO ##########################################*/	
	/* SPI SCK GPIO pin configuration  */
	GPIO_InitStruct.Pin			= LM70_SPI_SCK_PIN;
	GPIO_InitStruct.Mode		= GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull		= GPIO_PULLUP;
	GPIO_InitStruct.Speed		= GPIO_SPEED_FREQ_MEDIUM;
	GPIO_InitStruct.Alternate	= LM70_SPI_SCK_AF;
	
	HAL_GPIO_Init( LM70_SPI_SCK_GPIO, &GPIO_InitStruct );
	  
	/* SPI MISO GPIO pin configuration	*/
	GPIO_InitStruct.Pin			= LM70_SPI_MISO_PIN;
	GPIO_InitStruct.Alternate	= LM70_SPI_MISO_AF;
	
	HAL_GPIO_Init( LM70_SPI_MISO_GPIO, &GPIO_InitStruct );
	
	/* SPI MOSI GPIO pin configuration	*/
	GPIO_InitStruct.Pin			= LM70_SPI_MOSI_PIN;
	GPIO_InitStruct.Alternate	= LM70_SPI_MOSI_AF;
	
	HAL_GPIO_Init( LM70_SPI_MOSI_GPIO, &GPIO_InitStruct );
	
	/*##-3- Configure the NVIC for SPI #########################################*/
	/* NVIC for SPI */
//	HAL_NVIC_SetPriority(SPIx_IRQn, 0, 1);
//	HAL_NVIC_EnableIRQ(SPIx_IRQn);
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi)
{
	assert_param( hspi == &pLM70_SPI_hdl->CommonHdl );

  /*##-1- Reset peripherals ##################################################*/
  SPI_FORCE_RESET( LM70_SPI );
  SPI_RELEASE_RESET( LM70_SPI );

  /*##-2- Disable peripherals and GPIO Clocks ################################*/
  /* Configure SPI SCK as alternate function  */
  HAL_GPIO_DeInit( LM70_SPI_SCK_GPIO, LM70_SPI_SCK_PIN );
  /* Configure SPI MISO as alternate function  */
  HAL_GPIO_DeInit( LM70_SPI_MISO_GPIO, LM70_SPI_MISO_PIN );
  /* Configure SPI MISO as alternate function  */
  HAL_GPIO_DeInit( LM70_SPI_MISO_GPIO, LM70_SPI_MISO_PIN );
  
  /*##-3- Disable the NVIC for SPI ###########################################*/
//  HAL_NVIC_DisableIRQ(SPIx_IRQn);
}





bool LM70_Init( void )
{
	bool Result = false;
	do
	{
		SPI_HandleTypeDef *pSPI_hdl = &pLM70_SPI_hdl->CommonHdl;
		SPI_InitTypeDef *pInit = &pSPI_hdl->Init;

		pSPI_hdl->Instance = LM70_SPI;
		
//		pInit->Direction 		= SPI_DIRECTION_1LINE;				// не работает, т.к. в однопроводном режиме используется MOSI, а LM70 на плате Ћ”„.435.00.01.00 и клонах подключен к MISO
		pInit->Direction 		= SPI_DIRECTION_2LINES;
		pInit->CLKPhase			= SPI_PHASE_1EDGE;
		pInit->CLKPolarity		= SPI_POLARITY_LOW;
		pInit->CRCCalculation	= SPI_CRCCALCULATION_DISABLE;
//		pInit->CRCPolynomial 	= 7;								// ?? было в примере
		pInit->CRCPolynomial 	= 1;
//		pInit->DataSize			= SPI_DATASIZE_16BIT;				// не работает - передает 4 байта вместо двух :-/
		pInit->DataSize			= SPI_DATASIZE_8BIT;
		pInit->FirstBit			= SPI_FIRSTBIT_MSB;
		pInit->NSS				= SPI_NSS_SOFT;
		pInit->TIMode			= SPI_TIMODE_DISABLE;
		pInit->Mode				= SPI_MODE_MASTER;
		if( !SPI_SetupPrescaler( pSPI_hdl, LM70_SPI_FRQMAX ) )		// при расчете предделителя учитывается канал SPI, поэтому нельзя передавать только pInit
			break;
		// !! важно - инициализация сначала прописывается в pLM70_SPI_hdl->Init, затем сохранятеся в LM70_SPI_Init для последующего использования
		LM70_SPI_Init = *pInit;

		if( !SPIext_Capture( pLM70_SPI_hdl ) )
			break;

//		aGPIO_Common[iGPIO_SPI_CS_Temp].Mode = GPIO_MODE_OUTPUT_PP;
//		GPIO_Common_Init( iGPIO_SPI_CS_Temp );
		GPIO_Common_Write( iGPIO_SPI_CS_Temp, GPIO_PIN_RESET );
		do
		{
			if( HAL_OK != HAL_SPI_Init( pSPI_hdl ) )
				break;

/*			uint16_t Command = LM70_CMD_NORM;
			if( HAL_OK != HAL_SPI_Transmit( pSPI_hdl, ( uint8_t * ) &Command, sizeof( Command ), 2 ) )
				break;*/

			// ѕередать команду 0x0000 - перевод в режим измерения - вручную управляя MISO на вывод 0
			GPIO_InitTypeDef  GPIO_InitStruct;
			GPIO_InitStruct.Pin 		= LM70_SPI_MISO_PIN;
			GPIO_InitStruct.Mode		= GPIO_MODE_OUTPUT_PP;
			GPIO_InitStruct.Pull		= GPIO_PULLUP;
			GPIO_InitStruct.Speed		= GPIO_SPEED_FREQ_MEDIUM;
			GPIO_InitStruct.Alternate	= LM70_SPI_MISO_AF;
			HAL_GPIO_Init( LM70_SPI_MISO_GPIO, &GPIO_InitStruct );

			HAL_GPIO_WritePin( LM70_SPI_MISO_GPIO, LM70_SPI_MISO_PIN, GPIO_PIN_RESET );
			int32_t Dummy;
			assert_param( HAL_OK == HAL_SPI_Receive( pSPI_hdl, ( uint8_t * ) &Dummy, sizeof( Dummy ), 2 ) );

			GPIO_InitStruct.Mode		= GPIO_MODE_AF_PP;
			HAL_GPIO_Init( LM70_SPI_MISO_GPIO, &GPIO_InitStruct );

			if( HAL_OK != HAL_SPI_DeInit( pSPI_hdl ) )
				break;

			Result = true;
		} while( 0 );

		GPIO_Common_Write( iGPIO_SPI_CS_Temp, GPIO_PIN_SET );
		SPIext_Release( pLM70_SPI_hdl );
	} while( 0 );

	return Result;
}

// “емпература одного датчика
uint8_t LM70_ErrorCode = 0;
bool LM70_ReadTemp( int16_t *pTemp )
{
	bool Result = false;
	LM70_ErrorCode = 0;
	do
	{
		SPI_HandleTypeDef *pSPI_hdl = &pLM70_SPI_hdl->CommonHdl;
		SPI_InitTypeDef *pInit = &LM70_SPI_Init;

		if( !SPIext_Capture( pLM70_SPI_hdl ) )
			break;
		LM70_ErrorCode++;

		do
		{
			pSPI_hdl->Init = *pInit;
			if( HAL_OK != HAL_SPI_Init( pSPI_hdl ) )
				break;
			LM70_ErrorCode++;
			GPIO_Common_Write( iGPIO_SPI_CS_Temp, GPIO_PIN_RESET );

			int16_t ADC_Data;
			if( HAL_OK != HAL_SPI_Receive( pSPI_hdl, ( uint8_t * ) &ADC_Data, sizeof( ADC_Data ), 2 ) )
				break;
			LM70_ErrorCode++;
			ADC_Data = ( ( ADC_Data & 0x00FF ) << 8 ) | ( ( ADC_Data & 0xFF00 ) >> 8 );

			GPIO_Common_Write( iGPIO_SPI_CS_Temp, GPIO_PIN_SET );
			if( HAL_OK != HAL_SPI_DeInit( pSPI_hdl ) )
				break;
			LM70_ErrorCode++;

			*pTemp = LM70_NAN;
			if( ( ADC_Data & 0x001F ) == 0x001F )
			{
				ADC_Data >>= 5;
#if LM70_CHECK_LIMITS != 0
				if( ( ADC_Data <= LM70_TEMP_MAX * LM70_TEMP_COEFF ) && ( ADC_Data >= LM70_TEMP_MIN * LM70_TEMP_COEFF ) )
#endif
				{
					*pTemp = ADC_Data;
				}
			}
			Result = true;
		} while( 0 );

		SPIext_Release( pLM70_SPI_hdl );
	} while( 0 );

	return Result;
}

