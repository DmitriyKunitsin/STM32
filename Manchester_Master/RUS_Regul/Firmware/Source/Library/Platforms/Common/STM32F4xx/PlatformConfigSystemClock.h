// PlatformConfigSystemClock.h
// “иповые конфигурации системных часов дл€ разных платформ
#ifndef	PLATFORM_CONFIG_SYSTEM_CLOCK_H
#define	PLATFORM_CONFIG_SYSTEM_CLOCK_H

/*******************************************************
 *	сокр.		название				источник					куда
 *	HSE_OSC								OSC							HSE
 *	HSE			High Speed External		HSE_OSC						SYSCLOCK, HSE_RTC, PLL
 *	HSE_RTC								HSE / HSE_RTC_DIV			RTCCLK
 *	PLL48CK															USB_OTG_FS, SDIO_CK
 *******************************************************/


#if		defined ( PROJECTCONFIG_CLOCK_25M_168M )
	// чипы STM32F405ххх, STM32F407ххх
	// HSE 25 MHz ->PLL-> SYSCLK/APB1/APB2 168/42/84 MHz (макс дл€ 405/407)
	//			k				MHz
	// HSE			[4..26] 	25		[4..26] 	 варц
	// PLLM 	25	[2..63] 	1		[1..2]		HSE / M
	// PLLN 	336 [0..511]	336 	[192..432]	HSE / M * N
	// PLLP 	2	[2,4,6,8]	168 	[24..168]	HSE / M * N / P -> PLLCLK
	// SYSCLK					168 	[<=168] 	PLLCLK -> SYSCLK
	// AHB		1	[1,2..512]	168 	[<=168] 	HCLK
	// APB1P	4	[1,2..16]	42		[<=42]		AHB / k1
	// APB1T					84					( k1 == 1 ) ? APB1P : APB1P * 2
	// APB2P	2	[1,2..16]	84		[<=84]		AHB / k2
	// APB2T					168 				( k2 == 1 ) ? APB2P : APB2P * 2
	// ADCCLK	4				21		[0,6..30]	APB2P / k
	// PLL48CK	7				48		[<=48]		HSE / M * N / Q
	// SDIO_CK	0				24		[<=25]		PLL48CK / ( SDIO_CLKDIV + 2 )
	// HSE_DIV	25	[2..31] 	1		[<=4]		HSE / HSE_DIV -> RTCCLK
	// RTC_ASIN 124 [0..127]						RTCCLK / ( RTC_ASIN + 1 )
	// RTC_SIN	7999[0..32K-1]	1Hz 				RTCCLK / ( RTC_ASIN + 1 ) / ( RTC_SIN + 1 ) -> RTC
	#if	( HSE_VALUE != 25000000ul )
	#error "HSE_VALUE Mismatch!"
	#endif
	// __HAL_PWR_VOLTAGESCALING_CONFIG()	// управление напряжением питания ядра, от которого зависит максимальная допустимая частота и потребление
	#define PWR_REGULATOR_VOLTAGE_SCALE 			PWR_REGULATOR_VOLTAGE_SCALE1	// SCALE1 - до 168 ћ√ц, SCALE2 - до 144 ћ√ц
	// HAL_RCC_OscConfig()
	#define PROJECTCONFIG_CLOCK_USE_HSE 										// «адействовать тактирование от HSE
	#define	PROJECTCONFIG_CLOCK_RCC_HSESTATE		RCC_HSE_ON					// »спользовать HSE в качестве генератора 
	#define	PROJECTCONFIG_CLOCK_RCC_PLL_STATE		RCC_PLL_ON					// »спользовать PLL
	#define	PROJECTCONFIG_CLOCK_RCC_PLL_SOURCE		RCC_PLLSOURCE_HSE			// “актировать PLL от HSE
	#define	PROJECTCONFIG_CLOCK_RCC_PLL_M			25							// [2..63]	ѕредделитель PLL
	#define	PROJECTCONFIG_CLOCK_RCC_PLL_N			( 168 * 2 )					// [192..432]	”множитель PLL
	#define PROJECTCONFIG_CLOCK_RCC_PLL_P			RCC_PLLP_DIV2				// [2,4,6,8]	ƒелитель после PLL на PLLCLK
	#define	PROJECTCONFIG_CLOCK_RCC_PLL_Q			7							// [4..15]	ƒелитель после PLL на PLL48CK
	// HAL_RCC_ClockConfig()
	#define	PROJECTCONFIG_CLOCK_SYSCLK_SOURCE		RCC_SYSCLKSOURCE_PLLCLK		// »спользовать PLL в качестве SYSCLK
	#define	PROJECTCONFIG_CLOCK_AHB_DIVIDER			RCC_SYSCLK_DIV1				// SYSCLK -> (AHBDIV) -> HCLK
	#define	PROJECTCONFIG_CLOCK_APB1_DIVIDER		RCC_HCLK_DIV4				// HCLK  -> (APB1DIV) -> APB1
	#define	PROJECTCONFIG_CLOCK_APB2_DIVIDER		RCC_HCLK_DIV2				// HCLK  -> (APB2DIV) -> APB2
	// HAL_ADC_Init()
	#define	PROJECTCONFIG_CLOCK_ADCCLK_PRESCALER	ADC_CLOCK_SYNC_PCLK_DIV4	// APB2 -> (ADCDIV) -> ADCCLK
	// RTC_Init( )
	#define RCC_RTCCLKSOURCE						RCC_RTCCLKSOURCE_HSE_DIV25	// [LSI, LSE, HSE/2..HSE/31]	»сточник тактировани€ RTC
	#define RTC_ASINCH_PREDIV						( 125 - 1 ) 				// [1..128] јсинхронный делитель RTC
	#define RTC_SINCH_PREDIV						( 8000 - 1 )				// [1..32K] —инхронный делитель RTC
#elif	defined ( PROJECTCONFIG_CLOCK_25M_96M )
	// чипы STM32F405ххх, STM32F407ххх
	// HSE 25 MHz ->PLL-> SYSCLK/APB1/APB2 96/24/48 MHz (примерно половина от макс дл€ 405/407)
	//			k				MHz
	// HSE			[4..26] 	25		[4..26] 	 варц
	// PLLM 	25	[2..63] 	1		[1..2]		HSE / M
	// PLLN 	192 [0..511]	192 	[192..432]	HSE / M * N
	// PLLP 	2	[2,4,6,8]	96	 	[24..168]	HSE / M * N / P -> PLLCLK
	// SYSCLK					96	 	[<=168] 	PLLCLK -> SYSCLK
	// AHB		1	[1,2..512]	96	 	[<=168] 	HCLK
	// APB1P	4	[1,2..16]	24		[<=42]		AHB / k1
	// APB1T					48					( k1 == 1 ) ? APB1P : APB1P * 2
	// APB2P	2	[1,2..16]	48		[<=84]		AHB / k2
	// APB2T					96 					( k2 == 1 ) ? APB2P : APB2P * 2
	// ADCCLK	2				24		[0,6..30]	APB2P / k
	// PLL48CK	6				32		[<=48]		HSE / M * N / Q
	// SDIO_CK	0				16		[<=25]		PLL48CK / ( SDIO_CLKDIV + 2 )
	// HSE_DIV	25	[2..31] 	1		[<=4]		HSE / HSE_DIV -> RTCCLK
	// RTC_ASIN 124 [0..127]						RTCCLK / ( RTC_ASIN + 1 )
	// RTC_SIN	7999[0..32K-1]	1Hz 				RTCCLK / ( RTC_ASIN + 1 ) / ( RTC_SIN + 1 ) -> RTC
	#if ( HSE_VALUE != 25000000ul )
	#error "HSE_VALUE Mismatch!"
	#endif
	// __HAL_PWR_VOLTAGESCALING_CONFIG()	// управление напряжением питания ядра, от которого зависит максимальная допустимая частота и потребление
	#define PWR_REGULATOR_VOLTAGE_SCALE 			PWR_REGULATOR_VOLTAGE_SCALE1	// SCALE1 - до 168 ћ√ц, SCALE2 - до 144 ћ√ц
	// HAL_RCC_OscConfig( )
	#define PROJECTCONFIG_CLOCK_USE_HSE 										// «адействовать тактирование от HSE
	#define PROJECTCONFIG_CLOCK_RCC_HSESTATE		RCC_HSE_ON					// »спользовать HSE в качестве генератора 
	#define PROJECTCONFIG_CLOCK_RCC_PLL_STATE		RCC_PLL_ON					// »спользовать PLL
	#define PROJECTCONFIG_CLOCK_RCC_PLL_SOURCE		RCC_PLLSOURCE_HSE			// “актировать PLL от HSE
	#define PROJECTCONFIG_CLOCK_RCC_PLL_M			25							// [2..63]	ѕредделитель PLL
	#define PROJECTCONFIG_CLOCK_RCC_PLL_N			( 96 * 2 ) 					// [192..432]	”множитель PLL
	#define PROJECTCONFIG_CLOCK_RCC_PLL_P			RCC_PLLP_DIV2				// [2,4,6,8]	ƒелитель после PLL на PLLCLK
	#define PROJECTCONFIG_CLOCK_RCC_PLL_Q			6							// [4..15]	ƒелитель после PLL на PLL48CK
	// HAL_RCC_ClockConfig( )
	#define PROJECTCONFIG_CLOCK_SYSCLK_SOURCE		RCC_SYSCLKSOURCE_PLLCLK 	// »спользовать PLL в качестве SYSCLK
	#define PROJECTCONFIG_CLOCK_AHB_DIVIDER 		RCC_SYSCLK_DIV1 			// SYSCLK -> (AHBDIV) -> HCLK
	#define PROJECTCONFIG_CLOCK_APB1_DIVIDER		RCC_HCLK_DIV4				// HCLK  -> (APB1DIV) -> APB1
	#define PROJECTCONFIG_CLOCK_APB2_DIVIDER		RCC_HCLK_DIV2				// HCLK  -> (APB2DIV) -> APB2
	// HAL_ADC_Init( )
	#define PROJECTCONFIG_CLOCK_ADCCLK_PRESCALER	ADC_CLOCK_SYNC_PCLK_DIV2	// APB2 -> (ADCDIV) -> ADCCLK
	// RTC_Init( )
	#define RCC_RTCCLKSOURCE						RCC_RTCCLKSOURCE_HSE_DIV25	// [LSI, LSE, HSE/2..HSE/31]	»сточник тактировани€ RTC
	#define RTC_ASINCH_PREDIV						( 125 - 1 ) 				// [1..128] јсинхронный делитель RTC
	#define RTC_SINCH_PREDIV						( 8000 - 1 )				// [1..32K] —инхронный делитель RTC
#elif	defined ( PROJECTCONFIG_CLOCK_4M19_67M1 )
	// чипы STM32F401RCT6, STM32F405ххх, STM32F407ххх
	// HSE 4.194304 MHz ->PLL-> SYSCLK/APB1/APB2 67.x/33.x/67 MHz (близко к макс дл€ 401)
	//			k				MHz 
	// HSE_OSC					4.19x	2^22	[4..26] 	 варц 4.194304 MHz
	// HSE					 	4.19x	2^22	[4..26] 	HSE_OSC
	// PLLM 	4	[2..63] 	1.05x	2^20	[1..2]		HSE / M
	// PLLN 	256 [0..511]	268.4x	2^28	[192..432]	HSE / M * N
	// PLLP 	4	[2,4,6,8]	67.1x	2^26	[..84]		HSE / M * N / P
	// SYSCLK					67.1x	2^26	[<=84]		PLLCLK
	// AHB		1	[1,2..512]	67.1x	2^26	[<=84]		HCLK
	// APB1P	2	[1,2..16]	33.5x	2^25	[<=42]		AHB / k1
	// APB1T					67.1x	2^26				( k1 == 1 ) ? APB1P : APB1P * 2
	// APB2P	1	[1,2..16]	67.1x	2^26	[<=84]		AHB / k2
	// APB2T					67.1x	2^26				( k2 == 1 ) ? APB2P : APB2P * 2
	// ADCCLK	4				16.8x	2^24	[0,6..30]	APB2P / k
	// PLL48CK	8				33.5x	2^25	[<=48]		HSE / M * N / Q
	// SDIO_CK	0				16.8x	2^24	[<=48]		PLL48CK / ( SDIO_CLKDIV + 2 )
	// “актирование RTC от HSE
	// HSE_DIV	16	[2..31] 	0.26x	2^18	[<=4]		HSE / HSE_DIV -> RTCCLK
	// RTC_ASIN 127 [0..127]			2^11				RTCCLK / ( RTC_ASIN + 1 )
	// RTC_SIN	2K-1[0..32K-1]	1Hz 	2^0 				RTCCLK / ( RTC_ASIN + 1 ) / ( RTC_SIN + 1 ) -> RTC
	// “актирование RTC от LSE
	// LSE_DIV	1	[1] 		32KHz	2^15	[<=4M]		LSE -> RTCCLK
	// RTC_ASIN 127 [0..127]			2^8					RTCCLK / ( RTC_ASIN + 1 )
	// RTC_SIN	255	[0..32K-1]	1Hz 	2^0 				RTCCLK / ( RTC_ASIN + 1 ) / ( RTC_SIN + 1 ) -> RTC
	#if ( HSE_VALUE != 4194304ul )
	#error "HSE_VALUE Mismatch!"
	#endif
	// __HAL_PWR_VOLTAGESCALING_CONFIG()	// управление напряжением питания ядра, от которого зависит максимальная допустимая частота и потребление
	#define PWR_REGULATOR_VOLTAGE_SCALE 			PWR_REGULATOR_VOLTAGE_SCALE1	// SCALE1 - до 168 ћ√ц, SCALE2 - до 144 ћ√ц
	// HAL_RCC_OscConfig( )
	#define	PROJECTCONFIG_CLOCK_USE_HSE											// «адействовать тактирование от HSE
	#define PROJECTCONFIG_CLOCK_RCC_PLL_STATE		RCC_PLL_ON					// »спользовать PLL
	#define PROJECTCONFIG_CLOCK_RCC_PLL_SOURCE		RCC_PLLSOURCE_HSE			// “актировать PLL от HSE
	#define PROJECTCONFIG_CLOCK_RCC_PLL_M			4							// [2..63]	ѕредделитель PLL
	#define PROJECTCONFIG_CLOCK_RCC_PLL_N			256 						// [192..432]	”множитель PLL
	#define PROJECTCONFIG_CLOCK_RCC_PLL_P			RCC_PLLP_DIV4				// [2,4,6,8]	ƒелитель после PLL на PLLCLK
	#define PROJECTCONFIG_CLOCK_RCC_PLL_Q			8							// [4..15]	ƒелитель после PLL на PLL48CK
	// HAL_RCC_ClockConfig( )
	#define PROJECTCONFIG_CLOCK_SYSCLK_SOURCE		RCC_SYSCLKSOURCE_PLLCLK 	// »спользовать PLL в качестве SYSCLK
	#define PROJECTCONFIG_CLOCK_AHB_DIVIDER 		RCC_SYSCLK_DIV1 			// SYSCLK -> (AHBDIV) -> HCLK
	#define PROJECTCONFIG_CLOCK_APB1_DIVIDER		RCC_HCLK_DIV2				// HCLK  -> (APB1DIV) -> APB1
	#define PROJECTCONFIG_CLOCK_APB2_DIVIDER		RCC_HCLK_DIV1				// HCLK  -> (APB2DIV) -> APB2
	// HAL_ADC_Init( )
	#define PROJECTCONFIG_CLOCK_ADCCLK_PRESCALER	ADC_CLOCK_SYNC_PCLK_DIV4	// APB2 -> (ADCDIV) -> ADCCLK
	// RTC_Init( )
	#if		defined( USE_RTC_FROM_HSE )
		#define	RCC_RTCCLKSOURCE					RCC_RTCCLKSOURCE_HSE_DIV16	// [LSI, LSE, HSE/2..HSE/31]	»сточник тактировани€ RTC
		#define	RTC_ASINCH_PREDIV					( 128 - 1 )					// [1..128]	јсинхронный делитель RTC
		#define	RTC_SINCH_PREDIV					( 2 * 1024 - 1 )			// [1..32K]	—инхронный делитель RTC
	#elif	defined( USE_RTC_FROM_LSE )
		#if ( LSE_VALUE != 32768ul )
		#error "LSE_VALUE Mismatch!"
		#endif
		#define PROJECTCONFIG_CLOCK_USE_LSE 									// «адействовать тактирование от LSE
		#define RCC_RTCCLKSOURCE					RCC_RTCCLKSOURCE_LSE		// [LSI, LSE, HSE/2..HSE/31]	»сточник тактировани€ RTC
		#define RTC_ASINCH_PREDIV					( 128 - 1 ) 				// [1..128] јсинхронный делитель RTC
		#define RTC_SINCH_PREDIV					( 256 - 1 )					// [1..32K] —инхронный делитель RTC
	#endif	// USE_RTC_FROM_XXX
#elif	defined ( PROJECTCONFIG_CLOCK_16M384_65M536)
			// чип STM32F405ххх
			// HSE 16.384 MHz ->PLL-> SYSCLK/APB1/APB2 65.536/32.768/65.536 MHz
			//			k				MHz
			// HSE			[4..26] 	16.384	[4..26] 	 варц
			// PLLM 	8	[2..63] 	2.048	[0.95..2.1] HSE / M
			// PLLN 	64	[0..511]	131.072 [100..432]	HSE / M * N
			// PLLP 	2	[2,4,6,8]	65.536	[24..168]	HSE / M * N / P -> PLLCLK
			// SYSCLK					65.536	[<=168] 	PLLCLK -> SYSCLK
			// AHB		1	[1,2..512]	65.536	[<=168] 	HCLK
			// APB1P	2	[1,2..16]	32.768	[<=42]		AHB / k1
			// APB1T					65.536				( k1 == 1 ) ? APB1P : APB1P * 2
			// APB2P	1	[1,2..16]	65.536	[<=84]		AHB / k2
			// APB2T					65.536				( k2 == 1 ) ? APB2P : APB2P * 2
			// ADCCLK	8				8.192	[0,6..30]	APB2P / k
			// HSE_DIV	20	[2..31] 	0.8192	[<=4]		HSE / HSE_DIV -> RTCCLK
			// RTC_ASIN 127 [0..127]						RTCCLK / ( RTC_ASIN + 1 )
			// RTC_SIN	6399[0..32K-1]	1Hz 				RTCCLK / ( RTC_ASIN + 1 ) / ( RTC_SIN + 1 ) -> RTC
	#if ( HSE_VALUE != 16384000ul )
	#error "HSE_VALUE Mismatch!"
	#endif
	// __HAL_PWR_VOLTAGESCALING_CONFIG()	// управление напряжением питания ядра, от которого зависит максимальная допустимая частота и потребление
	#define PWR_REGULATOR_VOLTAGE_SCALE 			PWR_REGULATOR_VOLTAGE_SCALE1	// SCALE1 - до 168 ћ√ц, SCALE2 - до 144 ћ√ц
	// HAL_RCC_OscConfig( )
	#define PROJECTCONFIG_CLOCK_USE_HSE 										// «адействовать тактирование от HSE
	#define PROJECTCONFIG_CLOCK_RCC_HSESTATE		RCC_HSE_ON					// »спользовать HSE в качестве генератора 
	#define PROJECTCONFIG_CLOCK_RCC_PLL_STATE		RCC_PLL_ON					// »спользовать PLL
	#define PROJECTCONFIG_CLOCK_RCC_PLL_SOURCE		RCC_PLLSOURCE_HSE			// “актировать PLL от HSE
	#define PROJECTCONFIG_CLOCK_RCC_PLL_M			8							// [2..63]	ѕредделитель PLL
	#define PROJECTCONFIG_CLOCK_RCC_PLL_N			64		 					// [192..432]	”множитель PLL
	#define PROJECTCONFIG_CLOCK_RCC_PLL_P			RCC_PLLP_DIV2				// [2,4,6,8]	ƒелитель после PLL на PLLCLK
	#define PROJECTCONFIG_CLOCK_RCC_PLL_Q			4							// [4..15]	ƒелитель после PLL на PLL48CK
			// HAL_RCC_ClockConfig( )
	#define PROJECTCONFIG_CLOCK_SYSCLK_SOURCE		RCC_SYSCLKSOURCE_PLLCLK 	// »спользовать PLL в качестве SYSCLK
	#define PROJECTCONFIG_CLOCK_AHB_DIVIDER 		RCC_SYSCLK_DIV1 			// SYSCLK -> (AHBDIV) -> HCLK
	#define PROJECTCONFIG_CLOCK_APB1_DIVIDER		RCC_HCLK_DIV2				// HCLK  -> (APB1DIV) -> APB1
	#define PROJECTCONFIG_CLOCK_APB2_DIVIDER		RCC_HCLK_DIV1				// HCLK  -> (APB2DIV) -> APB2
			// HAL_ADC_Init( )
	#define PROJECTCONFIG_CLOCK_ADCCLK_PRESCALER	ADC_CLOCK_SYNC_PCLK_DIV8	// APB2 -> (ADCDIV) -> ADCCLK
			// RTC_Init( )
	#define RCC_RTCCLKSOURCE						RCC_RTCCLKSOURCE_HSE_DIV20	// [LSI, LSE, HSE/2..HSE/31]	»сточник тактировани€ RTC
	#define RTC_ASINCH_PREDIV						( 128 - 1 ) 				// [1..128] јсинхронный делитель RTC
	#define RTC_SINCH_PREDIV						( 6400 - 1 )				// [1..32K] —инхронный делитель RTC
	
#else
#error Select Project Clock Config!
#endif	// PROJECTCONFIG_CLOCK_XXX

// »спользование HSE
#ifdef	PROJECTCONFIG_CLOCK_USE_HSE
	#ifndef	HSE_VALUE
	#error	"HSE_VALUE not defined!"
	#else
	#define PROJECTCONFIG_CLOCK_RCC_OSCILLATORTYPE	RCC_OSCILLATORTYPE_HSE		// “актировать SYSCLCK от HSE
	#ifdef	HSE_USE_EXTERNAL_OSC
	#define PROJECTCONFIG_CLOCK_RCC_HSESTATE		RCC_HSE_BYPASS				// «адействовать HSE. Ќе использовать внутренний осциллятор, т.к. к OSC_IN подключен внешний генератор
	#else
	#define PROJECTCONFIG_CLOCK_RCC_HSESTATE		RCC_HSE_ON					// «адействовать HSE. »спользовать внутренний осциллятор, т.к. к OSC_IN и OSC_OUT подключен внешний кварц
	#endif	// HSE_EXTERNAL_OSC
	#endif	// HSE_VALUE
#else	// !PROJECTCONFIG_CLOCK_USE_HSE
	#define PROJECTCONFIG_CLOCK_RCC_HSESTATE		RCC_HSE_OFF					// Ќе использовать HSE
	#error	"Only USE_HSE configuration defined :("
#endif	// PROJECTCONFIG_CLOCK_USE_HSE

// ќпциональное увеличение задержек при работе с Flash,
// дл€ увеличени€ надежности работы на высокой температуре
#ifndef	PROJECTCONFIG_CLOCK_FLASH_SLOWDOWN
#define	PROJECTCONFIG_CLOCK_FLASH_SLOWDOWN		1		// ƒефолтна€ задержка, если не определено в ProjectConfig.h
#endif

#endif	// PLATFORM_CONFIG_SYSTEM_CLOCK_H

