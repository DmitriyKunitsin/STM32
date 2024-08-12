// PlatformConfigSystemClock.h
// “иповые конфигурации системных часов дл€ разных платформ на основе STM32Lxxxx
#ifndef	PLATFORM_CONFIG_SYSTEM_CLOCK_H
#define	PLATFORM_CONFIG_SYSTEM_CLOCK_H

/*******************************************************
 *	сокр.		название				источник					куда
 *	HSE_OSC								OSC							HSE
 *	HSE			High Speed External		HSE_OSC						SYSCLOCK, HSE_RTC, PLL
 *	HSE_RTC								HSE / HSE_RTC_DIV			RTCCLK
 *	PLL48CK															USB_OTG_FS, SDIO_CK
 *******************************************************/

#ifndef	STM32L4
#error "Configuration for STM32L4xx only!"
#endif

#if		defined ( PROJECTCONFIG_CLOCK_16M4_65M5 )
	// чип STM32L496хх, Voltage scaling Range 1
	// HSE 16.384 MHz ->PLL-> SYSCLK/APB1/APB2 65.5x/32.8x/32.8x MHz
	//			k				MHz
	// HSE			[4..48] 	16.4x	[4..48] 	 варц, генератор
	// PLL.M 	2	[1..8] 		8.2x	[4..16]		HSE / M							предделитель всех PLL
	// PLL.N	16	[8..86]		131.0x	[64..344]	HSE / M * N						умножитель PLL
	// PLLCLK 	2	[2,4,6,8]	65.5x	[8..80]		HSE / M * N / R					делитель PLL
	// PLLQ 	4	[2,4,6,8]	32.8x	[8..80]		HSE / M * N / Q					делитель PLL
	// PLLP 	2	[2..31]		65.5x	[2.065..80]	HSE / M * N / P					делитель PLL
	// SYSCLK					65.5x	[0..80]	 	PLLCLK -> SYSCLK
	// AHB		1	[1,2..512]	65.5x	[0..80] 	HCLK
	// SYSTICK	8	[8]			8.2x	[0..10] 	SYSTICK
	// APB1P	2	[1,2..16]	32.8x	[0..80]		AHB / k1
	// APB1T					65.5x				??? ( k1 == 1 ) ? APB1P : APB1P * 2		TIM[2..7]
	// APB2P	2	[1,2..16]	32.8x	[0..80]		AHB / k2
	// APB2T					65.5x				??? ( k2 == 1 ) ? APB2P : APB2P * 2		TIM[1,8,15,16,17]
	// ADCCLK	1				65.5x	[0..80]		AHB / k
	// ----------------------------------------
	// ѕредполагаемая разводка тактирования:
	// HSE		/2		-> PLL		8.2x MHz
	// PLL		x16/2	-> PLLCLK	65.5x MHz
	// PLLCLK			-> SYSCLK	65.5x MHz
	// SYSCLK			-> HCLK		65.5x MHz	ADC
	// HCLK		/2		-> PCLK1	32.8x MHz	UART, I2C2
	// HCLK		/2		-> PCLK2	32.8x MHz
	// PLL		x16/4	-> PLLQ		32.8x MHz
	// PLLQ				-> CLK48	32.8x MHz	SDMMC1, !!!USB!!!

	#if	( HSE_VALUE != 16384000ul )
	#error "HSE_VALUE Mismatch!"
	#endif
	// __HAL_PWR_VOLTAGESCALING_CONFIG()	// управление напряжением питания ядра, от которого зависит максимальная допустимая частота и потребление
	#define PWR_REGULATOR_VOLTAGE_SCALE 			PWR_REGULATOR_VOLTAGE_SCALE1	// SCALE1 - до 80 ћ√ц, SCALE2 - до 26 ћ√ц
	// HAL_RCC_OscConfig( )
	#define PROJECTCONFIG_CLOCK_USE_HSE 										// «адействовать тактирование от HSE
	#define	PROJECTCONFIG_CLOCK_RCC_HSESTATE		RCC_HSE_ON					// »спользовать HSE в качестве генератора 
	#define	PROJECTCONFIG_CLOCK_RCC_PLL_STATE		RCC_PLL_ON					// »спользовать PLL
	#define	PROJECTCONFIG_CLOCK_RCC_PLL_SOURCE		RCC_PLLSOURCE_HSE			// “актировать PLL от HSE
	#define	PROJECTCONFIG_CLOCK_RCC_PLL_M			2							// [1..8]		ѕредделитель PLL
	#define	PROJECTCONFIG_CLOCK_RCC_PLL_N			16							// [8..86]	”множитель PLL
	#define PROJECTCONFIG_CLOCK_RCC_PLL_P			RCC_PLLP_DIV2				// [2..31]	ƒелитель после PLL на PLLP
	#define PROJECTCONFIG_CLOCK_RCC_PLL_Q			RCC_PLLQ_DIV4				// [2,4,6,8]	ƒелитель после PLL на PLLQ
	#define	PROJECTCONFIG_CLOCK_RCC_PLL_R			RCC_PLLR_DIV2				// [2,4,6,8]	ƒелитель после PLL на PLLCLK
	// HAL_RCC_ClockConfig( )
	#define	PROJECTCONFIG_CLOCK_SYSCLK_SOURCE		RCC_SYSCLKSOURCE_PLLCLK		// »спользовать PLL в качестве SYSCLK
	#define	PROJECTCONFIG_CLOCK_AHB_DIVIDER			RCC_SYSCLK_DIV1				// SYSCLK -> (AHBDIV) -> HCLK
	#define	PROJECTCONFIG_CLOCK_APB1_DIVIDER		RCC_HCLK_DIV2				// HCLK  -> (APB1DIV) -> APB1
	#define	PROJECTCONFIG_CLOCK_APB2_DIVIDER		RCC_HCLK_DIV2				// HCLK  -> (APB2DIV) -> APB2
	// HAL_ADC_Init( )
	#ifndef	PROJECTCONFIG_CLOCK_ADCCLK_PRESCALER
	#define	PROJECTCONFIG_CLOCK_ADCCLK_PRESCALER	ADC_CLOCK_SYNC_PCLK_DIV1	// AHB -> (ADCDIV) -> ADCCLK
	#endif

#elif	defined ( PROJECTCONFIG_CLOCK_16M0_64M0 )
	// чип STM32L496хх, Voltage scaling Range 1
	// HSE 16.0 MHz ->PLL-> SYSCLK/APB1/APB2 64.0/32.0/32.0 MHz
	//			k	MHz
	// HSE			[4..48] 	16.0	[4..48] 	 варц, генератор
	// PLL.M 	2	[1..8] 	8.0		[4..16]	HSE / M							предделитель всех PLL
	// PLL.N	16	[8..86]	128.0	[64..344]	HSE / M * N						умножитель PLL
	// PLLCLK 	2	[2,4,6,8]	64.0	[8..80]	HSE / M * N / R					делитель PLL
	// PLLQ 	4	[2,4,6,8]	32.0	[8..80]	HSE / M * N / Q					делитель PLL
	// PLLP 	2	[2..31]	64.0	[2.065..80]	HSE / M * N / P				делитель PLL
	// SYSCLK					64.0	[0..80] 	PLLCLK -> SYSCLK
	// AHB		1	[1,2..512]	64.0	[0..80] 	HCLK
	// APB1P	2	[1,2..16]	32.0	[0..80]	AHB / k1
	// APB1T					64.0				??? ( k1 == 1 ) ? APB1P : APB1P * 2		TIM[2..7]
	// APB2P	2	[1,2..16]	32.0	[0..80]	AHB / k2
	// APB2T					64.0				??? ( k2 == 1 ) ? APB2P : APB2P * 2		TIM[1,8,15,16,17]
	// ADCCLK	1				64.0	[0..80]	AHB / k
	// ----------------------------------------
	// ѕредполагаемая разводка тактирования:
	// HSE		/2		-> PLL		8.0 MHz
	// PLL		x16/2	-> PLLCLK	64.0 MHz
	// PLLCLK			-> SYSCLK	64.0 MHz
	// SYSCLK			-> HCLK	64.0 MHz	ADC
	// HCLK	/2		-> PCLK1	32.0 MHz	UART, I2C2
	// HCLK	/2		-> PCLK2	32.0 MHz
	// PLL		x16/4	-> PLLQ	32.0 MHz
	// PLLQ			-> CLK48	32.8x MHz	SDMMC1, !!!USB!!!

	#if	( HSE_VALUE != 16000000ul )
	#error "HSE_VALUE Mismatch!"
	#endif
	// __HAL_PWR_VOLTAGESCALING_CONFIG()	// управление напряжением питания ядра, от которого зависит максимальная допустимая частота и потребление
	#define PWR_REGULATOR_VOLTAGE_SCALE 			PWR_REGULATOR_VOLTAGE_SCALE1	// SCALE1 - до 80 ћ√ц, SCALE2 - до 26 ћ√ц
	// HAL_RCC_OscConfig( )
	#define PROJECTCONFIG_CLOCK_USE_HSE 										// «адействовать тактирование от HSE
	#define	PROJECTCONFIG_CLOCK_RCC_HSESTATE		RCC_HSE_ON					// »спользовать HSE в качестве генератора 
	#define	PROJECTCONFIG_CLOCK_RCC_PLL_STATE		RCC_PLL_ON					// »спользовать PLL
	#define	PROJECTCONFIG_CLOCK_RCC_PLL_SOURCE		RCC_PLLSOURCE_HSE			// “актировать PLL от HSE
	#define	PROJECTCONFIG_CLOCK_RCC_PLL_M			2							// [1..8]		ѕредделитель PLL
	#define	PROJECTCONFIG_CLOCK_RCC_PLL_N			16							// [8..86]	”множитель PLL
	#define PROJECTCONFIG_CLOCK_RCC_PLL_P			RCC_PLLP_DIV7				// [2..31]	ƒелитель после PLL на PLLP
	#define PROJECTCONFIG_CLOCK_RCC_PLL_Q			RCC_PLLQ_DIV2				// [2,4,6,8]	ƒелитель после PLL на PLLQ
	#define	PROJECTCONFIG_CLOCK_RCC_PLL_R			RCC_PLLR_DIV2				// [2,4,6,8]	ƒелитель после PLL на PLLCLK
	// HAL_RCC_ClockConfig( )
	#define	PROJECTCONFIG_CLOCK_SYSCLK_SOURCE		RCC_SYSCLKSOURCE_PLLCLK		// »спользовать PLL в качестве SYSCLK
	#define	PROJECTCONFIG_CLOCK_AHB_DIVIDER			RCC_SYSCLK_DIV1				// SYSCLK -> (AHBDIV) -> HCLK
	#define	PROJECTCONFIG_CLOCK_APB1_DIVIDER		RCC_HCLK_DIV2				// HCLK  -> (APB1DIV) -> APB1
	#define	PROJECTCONFIG_CLOCK_APB2_DIVIDER		RCC_HCLK_DIV2				// HCLK  -> (APB2DIV) -> APB2
	// HAL_ADC_Init( )
	#ifndef PROJECTCONFIG_CLOCK_ADCCLK_PRESCALER
	#define PROJECTCONFIG_CLOCK_ADCCLK_PRESCALER	ADC_CLOCK_SYNC_PCLK_DIV1	// AHB -> (ADCDIV) -> ADCCLK
	#endif
#else
#error "Select Project Clock Config!"
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

