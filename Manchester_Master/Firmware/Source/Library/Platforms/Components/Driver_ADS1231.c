// Driver_ADS1231.c
// ƒрайвер ј÷ѕ ADS1231
#include "ProjectConfig.h"
#include "stm32xxxx_hal.h"		// дрова периферии
#include "Driver_ADS1231.h"		// родной
#include "Platform_Common.h"
#include "common_GPIO.h"
#include "FreeRTOS.h"
#include "task.h"

static SPI_HandleTypeDef ADS1231_SPI_hdl = { 0 };
static SPI_HandleTypeDef * const pADS1231_SPI_hdl = &ADS1231_SPI_hdl;

// ”становить темп сэплирования ј÷ѕ ADS1231 на 10/80 SPS
void ADS1231_SetSPS( ADS1231_SPS_t SPS )
{
	switch( SPS )
	{
	case ADS1231_SPS_10:	GPIO_Common_Write( iGPIO_ADC_SPD, GPIO_PIN_RESET );	break;
	case ADS1231_SPS_80:	GPIO_Common_Write( iGPIO_ADC_SPD, GPIO_PIN_SET );	break;
	default:				assert_param( 0 );
	}
}

// ”правление режимом работы с ADS1231.nDRDY/DOUT
// ( bIRQ_En == false )		ADS1231.DOUT	SPI.MOSI	прием данных через SPI в полудуплексном режиме
// ( bIRQ_En != false )		ADS1231.nDRDY	EXTI		ожидание заднего фронта для запуска оцифровки
void ADS1231_IRQ_Enable( bool bIRQ_En )
{
	GPIO_InitTypeDef  GPIO_InitStruct;
	GPIO_InitStruct.Pin			= ADS1231_SPI_MISO_PIN;
	GPIO_InitStruct.Pull		= GPIO_PULLDOWN;
	GPIO_InitStruct.Speed		= GPIO_SPEED_FREQ_HIGH;
	if( bIRQ_En )
	{	// ( bIRQ_En != false ) 	ADS1231.nDRDY	EXTI		ожидание заднего фронта для запуска оцифровки
		// ”становить режим запуска прерывания по заднему фронту
		GPIO_InitStruct.Mode		= GPIO_MODE_IT_FALLING;
		// ѕри вызове HAL_GPIO_Init() будет разрешено прерывание по пину, предварительно будет сброшен флаг накопившихся запросов
	}
	else
	{	// ( bIRQ_En == false )		ADS1231.DOUT	SPI.MOSI	прием данных через SPI в полудуплексном режиме
		// ”становить режим SPI.MOSI
		GPIO_InitStruct.Mode		= GPIO_MODE_AF_PP;
		GPIO_InitStruct.Alternate	= ADS1231_SPI_MISO_AF;
		// ѕри вызове HAL_GPIO_Init() воздействия на EXTI не производится, поэтому необходимо замаскировать запрос прерывания от пина
		EXTI->IMR1 &= ~ADS1231_SPI_MISO_PIN;
	}
	// ѕроизвести переконфигурирование пина
	//taskENTER_CRITICAL( );
	HAL_GPIO_Init( ADS1231_SPI_MISO_GPIO, &GPIO_InitStruct );	// операция HAL_GPIO_Init() содержит массу неатомарных операций конфигурирования понов в порту
	//taskEXIT_CRITICAL( );
	// ѕри необходимости разрешить прерывание EXTI#
	if( bIRQ_En )
	{
		HAL_NVIC_SetPriority( ADS1231_IRQn, ADS1231_IRQ_PREEMTPRIORITY, ADS1231_IRQ_SUBPRIORITY );
		HAL_NVIC_EnableIRQ( ADS1231_IRQn );
		// ƒля обработки прерывания необходимо переопределить пустой коллбек ADS1231_EXTI_Callback(),
		// который будет вызван из автоматического обработчика EXTI#.
	}
}

__weak void ADS1231_EXTI_Callback( void )
{
}

// »нициализация работы с ј÷ѕ ADS1231
// !!! ј÷ѕ расположено на плате 687_01, на которой собственный DCDC преобразователь питания.
// !!! ƒля включения ј÷ѕ необходимо включить преобразователь.
// !!! ”правление нагрузками модуля производится из задачи BKS_MainTask( ), здесь включение ј÷ѕ не производится.
bool ADS1231_Init( void )
{
	bool bResult = false;
	do
	{
		pADS1231_SPI_hdl->Instance = ADS1231_SPI;
		
		SPI_InitTypeDef *pInit = &pADS1231_SPI_hdl->Init;
		pInit->Direction			= SPI_DIRECTION_2LINES_RXONLY;
		pInit->CLKPhase 			= SPI_PHASE_2EDGE;
		pInit->CLKPolarity			= SPI_POLARITY_HIGH;
		pInit->CRCCalculation		= SPI_CRCCALCULATION_DISABLE;
		pInit->CRCPolynomial		= 1;
		pInit->DataSize 			= SPI_DATASIZE_8BIT;
		pInit->FirstBit 			= SPI_FIRSTBIT_MSB;
		pInit->NSS					= SPI_NSS_SOFT;
		pInit->TIMode				= SPI_TIMODE_DISABLE;
		pInit->Mode 				= SPI_MODE_MASTER;
		pInit->BaudRatePrescaler	= SPI_BAUDRATEPRESCALER_32;		// 1 ћ√ц или 2 ћ√ц
#warning "SPI Baudrate must set according to real CLK!"
		
		HAL_SPI_Init( pADS1231_SPI_hdl );
		
		// ¬ключение производится в процессе BKS_MainTask( );
//		ADS1231_PowerOn( true );
		// ”становить скорость сэмплирования ј÷ѕ
		ADS1231_SetSPS( ADS1231_SPS_80 );
		// ѕеревести пин, к которому подключен ADS1231.nDRDY/DOUT, в режим EXTI
		ADS1231_IRQ_Enable( true );

		bResult = true;
	} while( 0 );
	return bResult;	
}

void HAL_SPI_MspInit( SPI_HandleTypeDef *hspi )
{
	assert_param( hspi == pADS1231_SPI_hdl );

	GPIO_InitTypeDef  GPIO_InitStruct;
	
	/*##-1- Enable peripherals and GPIO Clocks #################################*/
	/* Enable GPIO TX/RX clock */
//	GPIO_CLK_ENABLE( ADS1231_SPI_MISO_GPIO );
	GPIO_CLK_ENABLE( ADS1231_SPI_MISO_GPIO );
	GPIO_CLK_ENABLE( ADS1231_SPI_SCK_GPIO );
	/* Enable SPI clock */
	SPI_CLK_ENABLE( ADS1231_SPI ); 
	
	/*##-2- Configure peripheral GPIO ##########################################*/	
	/* SPI SCK GPIO pin configuration  */
	GPIO_InitStruct.Pin			= ADS1231_SPI_SCK_PIN;
	GPIO_InitStruct.Mode		= GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull		= GPIO_PULLDOWN;
	GPIO_InitStruct.Speed		= GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate	= ADS1231_SPI_SCK_AF;
	HAL_GPIO_Init( ADS1231_SPI_SCK_GPIO, &GPIO_InitStruct );
	  
	/* SPI MISO GPIO pin configuration	*/
//	GPIO_InitStruct.Pin			= ADS1231_SPI_MISO_PIN;
//	GPIO_InitStruct.Alternate	= ADS1231_SPI_MISO_AF;
//	HAL_GPIO_Init( ADS1231_SPI_MISO_GPIO, &GPIO_InitStruct );
	
	/* SPI MOSI GPIO pin configuration	*/
	GPIO_InitStruct.Pin			= ADS1231_SPI_MISO_PIN;
	GPIO_InitStruct.Alternate	= ADS1231_SPI_MISO_AF;
	HAL_GPIO_Init( ADS1231_SPI_MISO_GPIO, &GPIO_InitStruct );
	
	/*##-3- Configure the NVIC for SPI #########################################*/
	/* NVIC for SPI */
//	HAL_NVIC_SetPriority(SPIx_IRQn, 0, 1);
//	HAL_NVIC_EnableIRQ(SPIx_IRQn);
}

// ѕрочитать данные из внешнего ј÷ѕ по SPI
// ≈сли не удалось считать, вернуть ранее считанное значение
// *pDataADC	- результат в единицах ј÷ѕ 24 бит, со знаком
bool ADS1231_DataRead( int32_t *pDataADC )
{
	bool bResult = false;
	static volatile int32_t ADS1231_DataADC_Saved = 0;	// сохраненное предыдущее значение ј÷ѕ для возврата в случае отказа
	GPIO_Common_Write( iGPIO_TestPinADS1231, GPIO_PIN_RESET );		// контроль операции чтения, можно использовать как CS при парсинге SPI в анализаторе
	do
	{
		// ѕроверить готовность данных в ј÷ѕ (ADS1231.nDRDY/DOUT сброшен)
		if( GPIO_PIN_RESET != HAL_GPIO_ReadPin( ADS1231_SPI_MISO_GPIO, ADS1231_SPI_MISO_PIN ) )
			break;
		// ѕеревести пин, к которому подключен ADS1231.nDRDY/DOUT, в режим SPI.MISO
		ADS1231_IRQ_Enable( false );
		// ѕроизвести чтение данных через SPI.
		// ѕолное слово ј÷ѕ составляет 24 бита, но необходимо считать хотя бы 25 бит, чтобы ADS1231.nDRDY/DOUT установился в 1.
		// “огда при завершении сэмплирования ADS1231.nDRDY/DOUT будет сброшен как признак готовности ј÷ѕ к считыванию новых данных.
		// ѕри заявке на чтение 3-х байт производится выдвижение 32 SCLK.
		// ѕри скорости SPI 1 ћ√ц, на всю операцию уходит до 40 мкс.
		uint32_t DataRaw = 0;
		HAL_StatusTypeDef ReceiveResult = HAL_SPI_Receive( pADS1231_SPI_hdl, ( uint8_t * ) &DataRaw, 3, 3 );
		// ѕеревести пин, к которому подключен ADS1231.nDRDY/DOUT, в режим EXTI
		ADS1231_IRQ_Enable( true );
		if( HAL_OK != ReceiveResult )
			break;
		//  онвертировать считанные данные: развернуть порядок следования и перейти от 3-х байт к 4-м с сохранением знака
		int32_t DataBin = ( ( int32_t ) __REV( DataRaw ) ) >> 8;
		if( ( DataBin > ADS1231_ADCMAX ) || ( DataBin < ADS1231_ADCMIN ) )
			break;
		ADS1231_DataADC_Saved = DataBin;
		bResult = true;
	} while( 0 );
	assert_param( NULL != pDataADC );
	*pDataADC = ADS1231_DataADC_Saved;
	GPIO_Common_Write( iGPIO_TestPinADS1231, GPIO_PIN_SET );
	return bResult;
}



