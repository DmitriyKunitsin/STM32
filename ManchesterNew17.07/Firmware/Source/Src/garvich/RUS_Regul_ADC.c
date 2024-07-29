// VIKPB_ADC.c
// »нициализация и запуск ј÷ѕ
/*****************************************************************************************************************************************/

#include "stm32l4xx_hal.h"		// дрова периферии
#include "ProjectConfig.h"		// конфиг платформы, конфиг задачи.
#include "platform_common.h"
#include "RUS_Regul_ADC.h"			// родной
#include "Stm32l4xx_hal_adc.h"	// HAL ADC driver
#include <string.h>				// memcpy()
#include "MathUtils.h"				// DIVIDE()

#include "FreeRTOS.h"
#include "RUS_Regul_Events.h"				// объявление √руппы событий прибора √√ѕ
#include "RUS_Regul_ADC.h"

#pragma pack( 1 )
typedef struct ADC_Channel_struct
{
	uint32_t		Channel;
	GPIO_TypeDef	*pPort;
	uint16_t		Pin;
	uint32_t		SingleDiff;		// ¬ыбор дифференциальный канал (текущий позитив, +1 негатив) или single-ended
} ADC_Channel_t;
#pragma pack( )

//  аналы дл€ корпуса LQFP48
const ADC_Channel_t aADC_Channels[] = 
{	
	{ ADC_CHANNEL_5,	GPIOA,	GPIO_PIN_0, 	ADC_SINGLE_ENDED }, 			// PA0						0	
	{ ADC_CHANNEL_6,	GPIOA,	GPIO_PIN_1, 	ADC_SINGLE_ENDED }, 			// PA1						1
	{ ADC_CHANNEL_7,	GPIOA,	GPIO_PIN_2, 	ADC_SINGLE_ENDED }, 			// PA2						2
	{ ADC_CHANNEL_8,	GPIOA,	GPIO_PIN_3, 	ADC_SINGLE_ENDED }, 			// PA3						3	OPAMP
	{ ADC_CHANNEL_9,	GPIOA,	GPIO_PIN_4, 	ADC_SINGLE_ENDED }, 			// PA4						4
	{ ADC_CHANNEL_10,	GPIOA,	GPIO_PIN_5, 	ADC_SINGLE_ENDED }, 			// PA5						5
	{ ADC_CHANNEL_11,	GPIOA,	GPIO_PIN_6, 	ADC_SINGLE_ENDED }, 			// PA6						6	AIN_UHV			Ќапряжение обратной связи с высоковольтника гаммы	
	{ ADC_CHANNEL_12,	GPIOA,	GPIO_PIN_7, 	ADC_SINGLE_ENDED }, 			// PA7						7	
	{ ADC_CHANNEL_15,	GPIOB,	GPIO_PIN_0, 	ADC_DIFFERENTIAL_ENDED },		// PB0						8	
	{ ADC_CHANNEL_16,	GPIOB,	GPIO_PIN_1, 	ADC_DIFFERENTIAL_ENDED },		// PB1						9
	// внутренние каналы
	{ ADC_CHANNEL_TEMPSENSOR,	NULL,	0,		ADC_SINGLE_ENDED },	 			// Temp Sensor				10
	{ ADC_CHANNEL_VREFINT,		NULL,	0,		ADC_SINGLE_ENDED }, 			// ADC_CHANNEL_VREFINT		11
	{ ADC_CHANNEL_VBAT, 		NULL,	0,		ADC_SINGLE_ENDED },		 		// ADC_CHANNEL_VBAT 		12
	{ ADC_CHANNEL_DAC1CH2, 		NULL,	0,		ADC_SINGLE_ENDED },				// ADC_CHANNEL_18			13
};
	
	// ќпределение порядка сканирования регулярной группы
	#define ADC1_REGULAR_CHANNEL_GROUP		{2, 3, 10, 11}			// DC напряжения
	#define iAIN_ADC1_PW_MANAGE		0							// индекс канала в массиве результатов	ј÷ѕ дл€ оцифровки напр€жен€ через делитель на плате 33к / 10к
	#define iAIN_ADC1_OPAMP 		1							// индекс канала в массиве результатов	ј÷ѕ дл€ оцифровки потреблени€ через шунт 0.1 ќм
	#define iAIN_ADC1_TEMPSENSOR	2							// индекс канала в массиве результатов	ј÷ѕ дл€ оцифровки внутренней температуры контроллера
	#define iAIN_ADC1_VREFINT		3							// индекс канала в массиве результатов	ј÷ѕ дл€ оцифровки опроноо напр€жени€
	#define iAIN_ADC1_TOTAL			4							// всего каналов в массиве


// *******************************************************************



// Static функции модуля
static void RUS_Regul_ADC1_DeInit( void );
static void HAL_ADC1_MspInit( void );
static void HAL_ADC1_MspDeInit( ADC_HandleTypeDef* hadc );


// √руппа аналоговых каналов не определена для неподготовленной платформы.
// »спользовать затычку, имитирующую работу ј÷ѕ

#if (!defined(ADC1_REGULAR_CHANNEL_GROUP))
void RUS_Regul_ADC_Init( void )						{ }
void RUS_Regul_ADC_DeInit( void )						{ }
void RUS_Regul_ADC_Start( void )									{ }
void RUS_Regul_ADC_Stop( void )									{ }
float RUS_Regul_ADC_GetSampleRate( void )					{ return 0; }
void RUS_Regul_ADC_GetVoltages( RUS_Regul_ADC_AI_t *pAI )		{ *pAI = ( RUS_Regul_ADC_AI_t ) { 0 }; }
#else

// ѕорядок оцифровки каналов через регул€рную группу
static const uint8_t aADC1_RegularChannelGroup[] = ADC1_REGULAR_CHANNEL_GROUP;

#define	RUS_Regul_ADC1_CHANNEL_COUNT		SIZEOFARRAY( aADC1_RegularChannelGroup )
#define RUS_Regul_ADC1_DC_CHANNEL_COUNT		5

#define ADC1_IIR_DEPTH				16
#define	iADC_NOT_INIT				-1
#define ADC1_COPYBUFFER_HALFDEPTH	16
#define ADC1_ARRAY_CNT   			ADC1_COPYBUFFER_HALFDEPTH
#define ADC1_BETA					0.1f	//  оэффициент бета для фильтра усреднения оцифровок для получения постоянных значений каналов


// Ќаплатные делители напр€жений
#define RUS_Regul_AIN_DEVISOR		((float)(10.0f / (10.0f + 33.0f))) // ¬ кќм

// ÷иклический буфер, в который складывается оцифровка через DMA
// Ѕуфер двойного размера - при заполнении половины буфера и всего буфера возникают прерывания,
// по которым соответствующую половину буфера необходимо скопировать в "безопасное" место
static ADC1_Data_t	aADC1BufferTemp[ 2 ] [ ADC1_COPYBUFFER_HALFDEPTH ] [ RUS_Regul_ADC1_CHANNEL_COUNT ];	// Ѕуфер для хранения результатов оцифровок

//#define	DIVIDE( _VALUE_, _BASE_ )	( ( ( _VALUE_ ) + ( _BASE_ ) / 2 ) / ( _BASE_ ) )		// деление с округлением

// —водная структура данных ј÷ѕ –егул€тора
typedef struct RUS_Regul_ADC_struct
{
	// ADC & ADC.DMA handlers declaration
	
	ADC_HandleTypeDef ADC1_hdl;
	DMA_HandleTypeDef ADC1_DMA_hdl;
	
	// ”казатели на буферы, не размещенные в составе структуры
	uint8_t const	*pADC1RegularChannelGroup;
	ADC1_Data_t		*pADC1BufferDMA;

	// »ндексы выделенных каналов
	int8_t iChannelVref;
	int8_t iChannelTS;

	// Ѕуферы в составе структуры
	uint16_t	aBufferIIR[RUS_Regul_ADC1_CHANNEL_COUNT];	// Ѕуфер для фильтрации результатов оцифровки. ƒля экономии памяти - фильтр с бесконечной импульсной характеристикой
													// !! буфер обновляется в обработчике прерывания ADC1.DMA!
	float		aADC1AI_V[RUS_Regul_ADC1_CHANNEL_COUNT];			// [¬] ћассив для отнормированных измерений ј÷ѕ

	
	float  		aADC1_Value[RUS_Regul_ADC1_CHANNEL_COUNT];	// ћассив для усредненных значений ј÷ѕ 

	uint8_t		adcDCSignalStabilized;	// флаг установившихся показаний постоянных напряжений
	uint16_t	adcDCSignalStabilized_cnt;	// текущее значение счетчика циклов стабилизации (изначально 0, потом растет до CNTMAX, после чего поднимаетс флаг, что напряжение стабилизировалось)
	// Ѕуфер линейный относительно половины буфера DMA каждого канала постонного сигнала ј÷ѕ1
	ADC1_Data_t aADC1BufferDCSignalChannel[ 2 ] [ RUS_Regul_ADC1_CHANNEL_COUNT ] [ ADC1_COPYBUFFER_HALFDEPTH ];		// Ѕуфер для хранения результатов оцифровок постоянных сигналов (DC), используется при копировании из буфера DMA

	RUS_Regul_ADC_AI_t			AnalogInputs;			// ¬ыходные данные из драйвера ј÷ѕ (ADC)
	uint32_t	VPLCsumValue;		// [ед ј÷ѕ]. —умма значений ј÷ѕ за период.
	uint32_t	VPLCsumCnt;			// [ед] —четчик суммы дл€ усреднени€
	bool		VPLCsumEnable;		// –азрешение накопление измерений VPLC
	float		VPLCsumVoltage;		// [¬] ”средненное значение накопленных измерений VPLC
	uint32_t	VPLCminValue;		// [ед ј÷ѕ]. ћинимальное значение ј÷ѕ за период.
	float		VPLCminVoltage;		// [¬] ћинимальное значение измерений VPLC за период
} RUS_Regul_ADC_t;

static RUS_Regul_ADC_t RUS_Regul_ADC /*__PLACE_AT_RAM_CCM__*/ = { 0 };

static inline float ADC_GetFromIIR( uint16_t const *pBufferIIR, uint8_t i )
{
	assert_param( i < RUS_Regul_ADC1_CHANNEL_COUNT );
	return ( ( float ) pBufferIIR[i] ) / ADC1_IIR_DEPTH;
}

/*********************************************************
ѕрописать порядок оцифровки служебных сигналов через ADC1.
ѕрописать порядок оцифровки акселерометров через ADC2.
*********************************************************/

// Ќизкоуровневые инициализатор и деинициализатор ј÷ѕ.
// Ќеобходимо определить, для какого канала ј÷ѕ вызов, и вызвать соответствуеющий инициализатор
void HAL_ADC_MspInit( ADC_HandleTypeDef* hadc )
{
	if( hadc == &RUS_Regul_ADC.ADC1_hdl )
		HAL_ADC1_MspInit( );
	else
		assert_param( 0 );
}

static void HAL_ADC1_MspInit( void )
{
	// -1- Enable peripherals and GPIO Clocks #################################
	ADC_CLK_ENABLE( ADC1 );
	DMA_CLK_ENABLE( ADC1_DMA_INSTANCE );
	for( int i = 0; i < RUS_Regul_ADC1_CHANNEL_COUNT; i++ )
	{
		ADC_Channel_t const *pChannel = &aADC_Channels[ RUS_Regul_ADC.pADC1RegularChannelGroup[i] ];
		if( NULL != pChannel->pPort )
			GPIO_CLK_ENABLE( pChannel->pPort );
	}
	
	// -2- Configure peripheral GPIO ##########################################
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Mode	= GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull	= GPIO_NOPULL;		// подтяжки в аналоговом режиме не работают?
	for( int i = 0; i < RUS_Regul_ADC1_CHANNEL_COUNT; i++ )
	{
		ADC_Channel_t const *pChannel = &aADC_Channels[ RUS_Regul_ADC.pADC1RegularChannelGroup[i] ];
		if( NULL != pChannel->pPort )
		{
			GPIO_InitStruct.Pin = pChannel->Pin;
			HAL_GPIO_Init( pChannel->pPort, &GPIO_InitStruct );
		}
	}
	
	// -3- Configure the DMA streams ##########################################
	assert_param( sizeof( ADC1_Data_t ) == sizeof( uint16_t ) );		// DMA затачивается на работу с 16-битными словами
  	DMA_InitTypeDef DmaInit;
	DmaInit.Direction			= DMA_PERIPH_TO_MEMORY;
	DmaInit.PeriphInc			= DMA_PINC_DISABLE;
	DmaInit.MemInc				= DMA_MINC_ENABLE;
	DmaInit.PeriphDataAlignment	= DMA_PDATAALIGN_HALFWORD;
	DmaInit.MemDataAlignment	= DMA_MDATAALIGN_HALFWORD;
	DmaInit.Mode				= DMA_CIRCULAR;
	DmaInit.Priority			= ADC_DMA_PRIORITY;
	#if defined (STM32F4)
	DmaInit.FIFOMode			= DMA_FIFOMODE_DISABLE;         
	DmaInit.FIFOThreshold		= DMA_FIFO_THRESHOLD_HALFFULL;		// зачем?
	DmaInit.MemBurst			= DMA_MBURST_SINGLE;
	DmaInit.PeriphBurst			= DMA_PBURST_SINGLE; 
	DmaInit.Channel 			= ADC1_DMA_CHANNEL;
	#endif // STM32F4
	#if defined (STM32L4) 
	DmaInit.Request				= DMA_REQUEST_0;
	#endif // STM32L4
	RUS_Regul_ADC.ADC1_DMA_hdl.Instance	= ADC1_DMA_INSTANCE;
	RUS_Regul_ADC.ADC1_DMA_hdl.Init		= DmaInit;
	assert_param( HAL_OK == HAL_DMA_Init( &RUS_Regul_ADC.ADC1_DMA_hdl ) );
	__HAL_LINKDMA( &RUS_Regul_ADC.ADC1_hdl, DMA_Handle, RUS_Regul_ADC.ADC1_DMA_hdl );
	
	// -4- Configure the NVIC for DMA #########################################
	HAL_NVIC_SetPriority( ADC1_DMA_IRQn, ADC_DMA_IRQ_PREEMTPRIORITY, ADC_DMA_IRQ_SUBPRIORITY );
	HAL_NVIC_EnableIRQ( ADC1_DMA_IRQn );
}

void HAL_ADC_MspDeInit( ADC_HandleTypeDef* hadc )
{
	if( hadc == &RUS_Regul_ADC.ADC1_hdl )
		HAL_ADC1_MspDeInit( hadc );
	else
		assert_param( 0 );
}

static void HAL_ADC1_MspDeInit( ADC_HandleTypeDef* hadc )
{
	assert_param( hadc == &RUS_Regul_ADC.ADC1_hdl );
	if( HAL_DMA_GetState( &RUS_Regul_ADC.ADC1_DMA_hdl ) != HAL_DMA_STATE_READY )
		assert_param( HAL_OK == HAL_DMA_Abort( &RUS_Regul_ADC.ADC1_DMA_hdl ) );
	assert_param( HAL_OK == HAL_DMA_DeInit( &RUS_Regul_ADC.ADC1_DMA_hdl ) );
	HAL_NVIC_DisableIRQ( ADC1_DMA_IRQn );
}

// »нициализация программного модуля ADC 
void RUS_Regul_ADC_Init( void )
{
	RUS_Regul_ADC1_Init( );
	RUS_Regul_ADC.adcDCSignalStabilized = 0;
}

// ƒеинициализация программного модуля ADC
void RUS_Regul_ADC_DeInit( void )
{
	RUS_Regul_ADC1_DeInit( );
}


void RUS_Regul_ADC1_Init( void )
{
	// Ќачальная инициализация основной структуры данных
	RUS_Regul_ADC.pADC1RegularChannelGroup = aADC1_RegularChannelGroup;
	RUS_Regul_ADC.pADC1BufferDMA = &aADC1BufferTemp[0][0][0];


	for ( uint8_t i = 0; i < RUS_Regul_ADC1_CHANNEL_COUNT; i++ )
	{
		RUS_Regul_ADC.aADC1_Value[i] = 0.0f;
	}

	ADC_ChannelConfTypeDef CConfig;
	ADC_InitTypeDef	Init;
	ADC_HandleTypeDef *pAdcHandle;

#if defined (STM32F3)
#if defined(STM32F373xC) || defined(STM32F378xx)
	Init.DataAlign				= ADC_DATAALIGN_LEFT;
	Init.ScanConvMode			= ADC_SCAN_ENABLE;
#if defined( ADC_TRIGGER_TIMER )
		Init.ContinuousConvMode		= DISABLE;
#elif defined( ADC_TRIGGER_SOFTWARE )
		Init.ContinuousConvMode		= ENABLE;
#endif // #if defined( ADC_TRIGGER )
	Init.DiscontinuousConvMode	= DISABLE;
	Init.NbrOfDiscConversion	= 0;
#if defined( ADC_TRIGGER_TIMER )
		Init.ExternalTrigConv		= ADC_TRIGGER_TIM;
#elif defined( ADC_TRIGGER_SOFTWARE )
		Init.ExternalTrigConv		= ADC_SOFTWARE_START;
#endif // #if defined( ADC_TRIGGER )
#elif 
	Init.ClockPrescaler			= ADC1_CLOCKPRESCALER;
	Init.Resolution				= ADC1_RESOLUTION;
	Init.ScanConvMode			= ENABLE;
	Init.ContinuousConvMode		= ENABLE;
	Init.DiscontinuousConvMode	= DISABLE;
	Init.NbrOfDiscConversion	= 0;
	Init.ExternalTrigConvEdge	= ADC_EXTERNALTRIGCONVEDGE_NONE;
	Init.ExternalTrigConv		= ADC_EXTERNALTRIGCONV_T1_CC1;		// т.к. ADC_EXTERNALTRIGCONVEDGE_NONE, то ве равно
	Init.DataAlign				= ADC_DATAALIGN;
	Init.DMAContinuousRequests	= ENABLE;
	Init.EOCSelection			= ADC_EOC_SEQ_CONV;
#endif // Platform-depended Init struct
#endif // #if defined (STM32F3)
	
#if defined (STM32L4)
	Init.ClockPrescaler 		= ADC1_CLOCKPRESCALER;
	Init.Resolution 			= ADC1_RESOLUTION;
	Init.DataAlign				= ADC1_DATAALIGN;
	Init.ScanConvMode			= ADC_SCAN_ENABLE;
	Init.EOCSelection			= ADC_EOC_SEQ_CONV;
	Init.LowPowerAutoWait		= DISABLE;
	// ¬ыбор триггера для старта преобразований
#if defined( ADC1_TRIGGER_TIMER )
		Init.ContinuousConvMode		= DISABLE;
#elif defined( ADC1_TRIGGER_SOFTWARE )
		Init.ContinuousConvMode		= ENABLE;
#endif // #if defined( ADC_TRIGGER )
	Init.DiscontinuousConvMode	= DISABLE;
	Init.NbrOfDiscConversion	= 0;
#if defined( ADC1_TRIGGER_TIMER )
		Init.ExternalTrigConv		= ADC_TRIGGER_TIM;
#elif defined( ADC1_TRIGGER_SOFTWARE )
		Init.ExternalTrigConv		= ADC_SOFTWARE_START;
#endif // #if defined( ADC_TRIGGER )
	Init.ExternalTrigConvEdge	= ADC_EXTERNALTRIGCONVEDGE_NONE;
	Init.DMAContinuousRequests	= ENABLE;
	Init.Overrun				= ADC_OVR_DATA_OVERWRITTEN;
	Init.OversamplingMode		= DISABLE;
	Init.Oversampling.Ratio 	= ADC_OVERSAMPLING_RATIO_2;
	Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_NONE;
	Init.Oversampling.TriggeredMode = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;
	Init.Oversampling.OversamplingStopReset = ADC_REGOVERSAMPLING_CONTINUED_MODE;
	#if defined(ADC_CFGR_DFSDMCFG) &&defined(DFSDM1_Channel0)
  	Init.DFSDMConfig			= ADC_DFSDM_MODE_DISABLE;           /*!< Specify whether ADC conversion data is sent directly to DFSDM.
                                       This parameter can be a value of @ref ADC_HAL_EC_REG_DFSDM_TRANSFER.
                                       Note: This parameter can be modified only if there is no conversion is ongoing (both ADSTART and JADSTART cleared). */
	#warning "Filed DFSDMConfig"
#endif

#endif // #if defined (STM32L4)

	// ADC1
	pAdcHandle				= &RUS_Regul_ADC.ADC1_hdl;
	pAdcHandle->Instance	= ADC1;
	Init.NbrOfConversion	= RUS_Regul_ADC1_CHANNEL_COUNT;
	pAdcHandle->Init		= Init;
	assert_param( HAL_OK == HAL_ADC_Init( pAdcHandle ) );

#if defined (STM32F3)
	// Config Channel Scan Sequence for ADC1
#if defined(STM32F373xC) || defined(STM32F378xx)
	CConfig.Rank			= 1;
	CConfig.SamplingTime	= ADC_SAMPLINGTIME;
#elif
	CConfig.Rank			= 1;
	CConfig.SamplingTime	= ADC_SAMPLINGTIME;
	CConfig.Offset			= 0;
#endif // Platform-depended CConfig struct 
#endif // #if defined (STM32F3)

#if defined (STM32L4)
	// Config Channel Scan Sequence for ADC1
	CConfig.Rank			= 1;
	CConfig.SamplingTime	= ADC1_SAMPLINGTIME;
	CConfig.OffsetNumber	= ADC_OFFSET_NONE;
	CConfig.Offset			= 0; 
#endif // #if defined (STM32L4)
	
	for( int i = 0; i < RUS_Regul_ADC1_CHANNEL_COUNT; i++ )
	{
		switch( i )
		{
		case 0:	CConfig.Rank = ADC_REGULAR_RANK_1;	break;
		case 1:	CConfig.Rank = ADC_REGULAR_RANK_2;	break;
		case 2:	CConfig.Rank = ADC_REGULAR_RANK_3;	break;
		case 3:	CConfig.Rank = ADC_REGULAR_RANK_4;	break;
		case 4:	CConfig.Rank = ADC_REGULAR_RANK_5;	break;
		case 5:	CConfig.Rank = ADC_REGULAR_RANK_6;	break;
		case 6:	CConfig.Rank = ADC_REGULAR_RANK_7;	break;
		case 7:	CConfig.Rank = ADC_REGULAR_RANK_8;	break;
		default:	assert_param( 0 );
		}
		CConfig.Channel = aADC_Channels[ RUS_Regul_ADC.pADC1RegularChannelGroup[i] ].Channel;
		CConfig.SingleDiff = aADC_Channels[ RUS_Regul_ADC.pADC1RegularChannelGroup[i] ].SingleDiff;
		assert_param( HAL_OK == HAL_ADC_ConfigChannel( pAdcHandle, &CConfig ) );

		// ƒополнительная обработка для выделенных каналов
		// !! Ќеобходимо проверять на минимально допустимое время семплирования при работе со встроенными каналами - (VrefInt/Vbat/TempSensor)
		switch( CConfig.Channel )
		{
		case ADC_CHANNEL_VREFINT:
			//assert_param( iADC_NOT_INIT == BKS_ADC.iChannelVref );
			RUS_Regul_ADC.iChannelVref = i;
			break;
		case ADC_CHANNEL_TEMPSENSOR:
			//assert_param( iADC_NOT_INIT == BKS_ADC.iChannelTS );
			RUS_Regul_ADC.iChannelTS = i;
			break;
		case ADC_CHANNEL_VBAT:
			break;
		/*case ADC_CHANNEL_DAC1CH2:
			{
				uint32_t tmp_config_internal_channel = LL_ADC_GetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(pAdcHandle->Instance));
				LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(pAdcHandle->Instance), ADC_CHANNEL_DAC1CH2 | tmp_config_internal_channel);
			}
			break;*/
		default:
			break;
		}

		// 18.09.2020 ѕопытка добавить калибровку для получения более точных значений оцифровки
		assert_param( HAL_OK == HAL_ADCEx_Calibration_Start(pAdcHandle, ADC_SINGLE_ENDED) );
	}
}

static void RUS_Regul_ADC1_DeInit( void )
{
	assert_param( HAL_OK == HAL_ADC_DeInit( &RUS_Regul_ADC.ADC1_hdl ) );
}
/*
void MPI_ADC_DeInit( void )
{
	assert_param( HAL_OK == HAL_ADC_DeInit( &RUS_Regul_ADC.ADC_hdl ) );
}
*/
// ќбработчики прерываний от ADC DMA
// Ѕуфер оцифровки каждого канала заполняется от начала до конца и далее снова по циклу.
// ѕрерывания возникают при заполнении половины буфера и всего буфера.
// ѕервая и вторая половины буфера условно считаются первым и банком. ѕока один банк заполняется через DMA, второй обрабатывается процессором.
// ѕри этом важно, чтобы DMA и CPU не получали одновременного доступа к одному банку.
//  онтроль доступа к буферам:
// - в начале и в конце обработки буфера, процессор проверяет, что DMA в текущий момент контролирует другой буфер.
// - в начале обработки буфера процессор выставляет флаг доступа к этому буферу, в конце обработки - сбрасывает. ≈сли в начале обработки флаг выставлен, значит предыдущая обработка еще не завершилась, и состоялся конфликт.
//static bool bADC1_HalfConvCplt	= false;
//static bool bADC1_ConvCplt		= false;

//  опирование буфера DMA в буфер IRR с фильтрацией
// !! вызывается из обработчиков прерываний ADC.DMA.Cmplt и ADC.DMA.HalfCmplt
void ADC_IIR( bool bFirstBank )
{
	assert_param( ( ADC_LIMIT_MAX(ADC1_RESOLUTION_bits) * ADC1_IIR_DEPTH ) < 0xFFFF );
	ADC1_Data_t *pBuffDMA = ( bFirstBank ) ? ( RUS_Regul_ADC.pADC1BufferDMA ) : ( RUS_Regul_ADC.pADC1BufferDMA + RUS_Regul_ADC1_CHANNEL_COUNT );
	static bool bADC_IIR_InitComplete = false;
	if( bADC_IIR_InitComplete )
	{
		for( int i = 0; i < RUS_Regul_ADC1_CHANNEL_COUNT; i++ )
			RUS_Regul_ADC.aBufferIIR[i] = RUS_Regul_ADC.aBufferIIR[i] - DIVIDE( RUS_Regul_ADC.aBufferIIR[i], ADC1_IIR_DEPTH ) + pBuffDMA[i];
	}
	else
	{	// ѕервичная инициализация фильтра первым оцифрованным значением
		for( int i = 0; i < RUS_Regul_ADC1_CHANNEL_COUNT; i++ )
			RUS_Regul_ADC.aBufferIIR[i] = pBuffDMA[i] * ADC1_IIR_DEPTH;
		bADC_IIR_InitComplete = true;
	}
}

// ќбработчик прерываний от ADC1.DMA (оцифровка служебных сигналов)
// ¬ызвать библиотечный обработчик, дальнейшая обработка в коллбеках HAL_ADC_ConvHalfCpltCallback() и HAL_ADC_ConvCpltCallback()
void ADC1_DMA_IRQHandler( void )
{
	HAL_NVIC_ClearPendingIRQ( ADC1_DMA_IRQn );
	HAL_DMA_IRQHandler( &RUS_Regul_ADC.ADC1_DMA_hdl );
}

//  оллбеки из библиотечного обработчика прерываний от ADC#.DMA - HAL_DMA_IRQHandler()
void HAL_ADC_ConvHalfCpltCallback( ADC_HandleTypeDef* hadc )
{
	if( hadc == &RUS_Regul_ADC.ADC1_hdl )
	{	// ѕрерывание от ADC1.DMA (оцифровка служебных сигналов)
		// «авершено заполнение первого банка через DMA и уже начал заполняться второй банк.
		// ѕередать первый банк на обработку процессором.
		//ADC_IIR( true );
		// !! где-то здесь генерить событие для основной задачи?
		
		//  опирование из буфера DMA в буфер обработки оцифровок переменных сигналов
		//  опируются сразу все каналы измерения переменных сигналов
		ADC1_ConvRes_DCSignal_Copy( true, &RUS_Regul_ADC.aADC1BufferDCSignalChannel[0][0][0] );

		assert_param( NULL != RUS_Regul_EventGroup);
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xEventGroupSetBitsFromISR( RUS_Regul_EventGroup, EVENTS_RUS_REGUL_ADC1_FIRSTHALFBUFFER_READY, &xHigherPriorityTaskWoken );
	}
	else
		assert_param( 0 );
}

void HAL_ADC_ConvCpltCallback( ADC_HandleTypeDef* hadc )
{
	if( hadc == &RUS_Regul_ADC.ADC1_hdl )
	{	// ѕрерывание от ADC1.DMA (оцифровка служебных сигналов)
		// «авершено заполнение второго банка через DMA и уже начал заполняться первый банк.
		// ѕередать второй банк на обработку процессром.
		//ADC_IIR( false );

		//  опирование из буфера DMA в буфер обработки оцифровок переменных сигналов
		//  опируются сразу все каналы измерения переменных сигналов
		ADC1_ConvRes_DCSignal_Copy( false, &RUS_Regul_ADC.aADC1BufferDCSignalChannel[0][0][0] );

		assert_param( NULL != RUS_Regul_EventGroup );
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xEventGroupSetBitsFromISR( RUS_Regul_EventGroup, EVENTS_RUS_REGUL_ADC1_SECONDHALFBUFFER_READY, &xHigherPriorityTaskWoken );
	}
	else
		assert_param( 0 );
}

void HAL_ADC_ErrorCallback( ADC_HandleTypeDef *hadc )
{
	assert_param( hadc == &RUS_Regul_ADC.ADC1_hdl );
	assert_param( 0 );
	// !! где-то здесь генерить событие для основной задачи?
}

//  опирование буфера DMA в буфер накопления результатов оцифровок постоянных сигналов
// !! вызывается из обработчиков прерываний ADC.DMA.Cmplt и ADC.DMA.HalfCmplt
// ADC_Data_t *pBuffResult - указатель на буффер, куда сложить результаты оцифровок нужного канала (половина буфера DMA)
// uint8_t ChannelNmbr - индекс (номер) канала, результаты оцифровок которого будут скопированы из буфера DMA в буфер результата
void ADC1_ConvRes_DCSignal_Copy( bool bFirstBank, ADC1_Data_t *pBuffResult )
{
	uint8_t CurHalf;
	if ( bFirstBank )
		CurHalf = 0;
	else CurHalf = 1;
		
	for( uint16_t i = 0; i < ADC1_COPYBUFFER_HALFDEPTH; i++ )
	{
		for ( uint8_t j = 0; j < RUS_Regul_ADC1_CHANNEL_COUNT; j++ )
		{
			pBuffResult[CurHalf * RUS_Regul_ADC1_CHANNEL_COUNT * ADC1_COPYBUFFER_HALFDEPTH + i + j * ADC1_COPYBUFFER_HALFDEPTH ] = aADC1BufferTemp[CurHalf][i][j];
		}
	}
}

// ”среднение показаний по всем каналам ј÷ѕ за половину транзакций DMA
// ¬ызывается в основной программе по событию любой половинной передачи DMA (HT и TC)
// –езультат усреднения добавляется через комплементарный фильтр в усредненные значения по каналам в структуре BKS_ADC (в единицах ј÷ѕ)
void ADC1_AllChAvgHalfBuffer( bool bFirstBank )
{
	uint8_t CurHalf;
	if ( bFirstBank )
		CurHalf = 0;
	else CurHalf = 1;
	static bool ADC1_firsttime = true;
	const uint16_t CNTMAX = (uint16_t)( 1.0f / ADC1_BETA );
 	//static uint16_t cnt = 0;

	// ћассив сумм для каждого канала оцифровки
	uint32_t sum_ch[RUS_Regul_ADC1_CHANNEL_COUNT] = {0};
	uint32_t vlpc_avg_local = 0;
		
	for ( uint8_t j = 0; j < RUS_Regul_ADC1_CHANNEL_COUNT; j++ )
	{
		for( uint16_t i = 0; i < ADC1_COPYBUFFER_HALFDEPTH; i++ )
		{
			sum_ch[j] += aADC1BufferTemp[CurHalf][i][j];
		}
		if ( iAIN_ADC1_PW_MANAGE == j )
		{
			if ( RUS_Regul_ADC.VPLCsumEnable )
			{
				// ”средненное значение ј÷ѕ по каналу VPLC
				vlpc_avg_local = sum_ch[j] / ADC1_COPYBUFFER_HALFDEPTH;
				RUS_Regul_ADC.VPLCsumValue += vlpc_avg_local;
				RUS_Regul_ADC.VPLCsumCnt++;
				if ( vlpc_avg_local < RUS_Regul_ADC.VPLCminValue )
					RUS_Regul_ADC.VPLCminValue = vlpc_avg_local;
			}
		}
		if ( ADC1_firsttime )
		{
			RUS_Regul_ADC.aADC1_Value[j] = ( (float)sum_ch[j] ) / ADC1_COPYBUFFER_HALFDEPTH;
			ADC1_firsttime = false;
		}
		else
		{
			RUS_Regul_ADC.aADC1_Value[j] = ( RUS_Regul_ADC.aADC1_Value[j] * ( 1.0f - ADC1_BETA ) + ( ADC1_BETA * (float)sum_ch[j] / ADC1_COPYBUFFER_HALFDEPTH ) );
		}
	}

	// счетчик установшегося режима. «ависит от бета.  огда показания установились, выставить флаг
	if ( RUS_Regul_ADC.adcDCSignalStabilized_cnt < CNTMAX )
	{
		RUS_Regul_ADC.adcDCSignalStabilized_cnt++;
		if ( RUS_Regul_ADC.adcDCSignalStabilized_cnt == CNTMAX )
		{
			RUS_Regul_ADC.adcDCSignalStabilized = 1;
		}
	}
}

// «апустить оцифровку в циклический буфер
void RUS_Regul_ADC_Start( void )
{
	// ѕроверка адреса буфера на допустимость - должен быть выровнен по размеру слова обмена DMA (2 байта)
	assert_param( VALIDATE_ALIGN( RUS_Regul_ADC.pADC1BufferDMA, sizeof( *RUS_Regul_ADC.pADC1BufferDMA ) ) );
	// ѕроверка адреса буфера на допустимость - должен быть в пространстве ќ«”, доступном для записи по DMA
	assert_param( IS_DMA_WRITABLE_MEMORY( RUS_Regul_ADC.pADC1BufferDMA ) && IS_DMA_WRITABLE_MEMORY( RUS_Regul_ADC.pADC1BufferDMA + SIZEOFARRAY( RUS_Regul_ADC.pADC1BufferDMA ) ) );

	// ѕроверить готовность ј÷ѕ
	uint32_t AdcState = HAL_ADC_GetState( &RUS_Regul_ADC.ADC1_hdl );
	assert_param( ( HAL_ADC_STATE_READY == AdcState ) || ( HAL_ADC_STATE_REG_EOC == AdcState ) );

	// «апустить циклическую оцифровку
	HAL_ADC_Start_DMA( &RUS_Regul_ADC.ADC1_hdl, ( uint32_t * ) RUS_Regul_ADC.pADC1BufferDMA, RUS_Regul_ADC1_CHANNEL_COUNT * ADC1_COPYBUFFER_HALFDEPTH * 2 );
}

// ѕрекратить оцифровку
void RUS_Regul_ADC_Stop( void )
{
	assert_param( HAL_OK == HAL_ADC_Stop_DMA( &RUS_Regul_ADC.ADC1_hdl ) );
}


#if defined (USE_PLATFORM_OKR_354_10)
#define RUS_Regul_DEVISOR_VSUP	( (float) (33.0f + 10.0f) / 10.0f )		// ¬/¬
#define RUS_Regul_DEVISOR_OPAMP	( 1.0f / 16.0f / 0.1f )					// [ј/¬]	шунт 0.1 ќм, PGA x16

#else
#error Select Platform!
#endif


// –ассчитать измеренные напряжения
void RUS_Regul_ADC_GetVoltages( RUS_Regul_ADC_AI_t *pAI )
{	
	assert_param ( NULL != pAI );

	// —труктура для промежуточных результатов
	RUS_Regul_ADC_AI_t AI;
	float * const pVSUP = &AI.VSUP;
	float * const pChipTemp = &AI.Temp;

	// ѕересчитать коды ј÷ѕ в натуральные единицы
	// [V]		VDDA
	if( iADC_NOT_INIT == RUS_Regul_ADC.iChannelVref )
		*pVSUP = ADC_REFIN_CAL_VDDA;											// ќпора не оцифрована, взять по умолчанию
	else
	{
		float VDDA_ADC = RUS_Regul_ADC.aADC1_Value[RUS_Regul_ADC.iChannelVref];				// принять код ј÷ѕ из структуры данных (усредненные значения измерения постоянных сигналов) 
		*pVSUP = ADC_CALC_VDDA( VDDA_ADC );										// –ассчитать VDDA по опоре
		// ≈сли рассчитанное значение VDDA отличается от ожидаемой опоры (на 10%), то считать, что VDDA равно ожидаемой опоре
		// сделано, чтобы в начальные моменты не портить остальные показания, которые корректируются на текущее VDDA
		if ( ( *pVSUP > 1.1f * VDDA_VALUE_V ) || ( *pVSUP < 0.9f * VDDA_VALUE_V ) )
		{
			*pVSUP = VDDA_VALUE_V;
		}
	}
	// [degC]	“емпература чипа
	if( iADC_NOT_INIT == RUS_Regul_ADC.iChannelTS )
		*pChipTemp = -100;														// “ермодатчик не оцифрован, взять по умолчанию
	else
	{
		float TS_ADC = RUS_Regul_ADC.aADC1_Value[RUS_Regul_ADC.iChannelTS];					// принять код ј÷ѕ из структуры данных (усредненные значения измерения постоянных сигналов)
		TS_ADC *= *pVSUP / ADC_REFIN_CAL_VDDA;									// коррекция термодатчика при отклонении VDDA от 3,3 ¬ 
		*pChipTemp = ADC_CALC_TS_TEMP( TS_ADC );								// –ассчитать температуру по термодатчику
	}
	// [mV]		ќбычные каналы измерения напряжения
	for( int i = 0; i < RUS_Regul_ADC1_CHANNEL_COUNT; i++ )
	{
		float AIN_ADC = RUS_Regul_ADC.aADC1_Value[i];									// принять код ј÷ѕ из структуры данных (усредненные значения измерения постоянных сигналов)
		RUS_Regul_ADC.aADC1AI_V[i] = ADC_CALC_AIN( AIN_ADC, *pVSUP, ADC1_SPAN );		// [V]	AINx
	}

	// –ассчитать исходные сигналы по измеренным напряжениям
	AI.VDDA = *pVSUP;
	AI.VSUP	= RUS_Regul_ADC.aADC1AI_V[iAIN_ADC1_PW_MANAGE] * RUS_Regul_DEVISOR_VSUP;// / RUS_Regul_AIN_DEVISOR_UHV;
	AI.OPAMP_meas	= RUS_Regul_ADC.aADC1AI_V[iAIN_ADC1_OPAMP] * RUS_Regul_DEVISOR_OPAMP;// / RUS_Regul_AIN_DEVISOR_UHV;
	AI.VREF		= ( iADC_NOT_INIT == RUS_Regul_ADC.iChannelVref ) ? 0 : RUS_Regul_ADC.aADC1AI_V[ RUS_Regul_ADC.iChannelVref ];

	// —копировать во внутреннюю структуру программного модуля (необязательно, но сейчас DAC завязан VDDA из нее)
	RUS_Regul_ADC.AnalogInputs = AI;
	// —копировать промежуточные результаты
	ATOMIC_WRITE( *pAI, AI );
}

float ADC_OPAMP_Value_Get(void)
{
	return RUS_Regul_ADC.AnalogInputs.OPAMP_meas;
}

// ѕолучить величину постоянного сигнала VPLC в [¬]
float ADC_VSUP_Value_Get(void)
{
	return RUS_Regul_ADC.AnalogInputs.VSUP;
}

// ќтдать изеренное значение VDDA в [¬]
float ADC_getDCSignal_VDDA (void)
{
	return RUS_Regul_ADC.AnalogInputs.VDDA;
}

// ѕолучить текущее состояние флага установившихся показаний постоянных сигналов
uint8_t ADC_getFlagDCSignalStabilized( void )
{
	return RUS_Regul_ADC.adcDCSignalStabilized;
}

// ќпустить флаг стабилизации, сбросить счетчик
void ADC_clearFlagDCSignalStabilized( void )
{
	RUS_Regul_ADC.adcDCSignalStabilized = 0;
	RUS_Regul_ADC.adcDCSignalStabilized_cnt = 0;
}

// VPLC. ќбнулить накопившуюс€ сумму значений ј÷ѕ, сбросить счетчик сумм, разрешить накопление
void ADC_clearVPLCsum( void )
{
	RUS_Regul_ADC.VPLCsumValue = 0;
	RUS_Regul_ADC.VPLCsumCnt = 0;
	RUS_Regul_ADC.VPLCsumEnable = true;
	RUS_Regul_ADC.VPLCminValue = UINT16MAX;
}
// VPLC. –ассчитать усредненное значение VPLC в вольтах за врем€ накоплени€
// ƒопущение, что VDDA сильно не мен€етс€ за врем€ накоплени€
void ADC_calcVPLCsum( void )
{
	RUS_Regul_ADC.VPLCsumEnable = false;	// «апретить дальнейшее накопление
	float VPLCavg = RUS_Regul_ADC.VPLCsumValue / RUS_Regul_ADC.VPLCsumCnt;
	float vdda = ADC_CALC_VDDA( RUS_Regul_ADC.aADC1_Value[RUS_Regul_ADC.iChannelVref] );
	RUS_Regul_ADC.VPLCsumVoltage = ADC_CALC_AIN( VPLCavg, vdda, ADC1_SPAN ) * RUS_Regul_DEVISOR_VSUP;
	RUS_Regul_ADC.VPLCminVoltage = ADC_CALC_AIN( RUS_Regul_ADC.VPLCminValue, vdda, ADC1_SPAN ) * RUS_Regul_DEVISOR_VSUP;
}
// VPLC. ѕолучить усредненное значение VPLC в вольтах
float ADC_getVPLCsum( void )
{
	return RUS_Regul_ADC.VPLCsumVoltage;
}
// VPLC. ѕолучить минимальное значение VPLC в вольтах
float ADC_getVPLCmin( void )
{
	return RUS_Regul_ADC.VPLCminVoltage;
}

uint32_t ADC_getVPLCsumCnt( void )
{
	return RUS_Regul_ADC.VPLCsumCnt;
}
uint32_t ADC_getVPLCsumValue( void )
{
	return RUS_Regul_ADC.VPLCsumValue;
}
#endif // ADC1_REGULAR_CHANNEL_GROUP
