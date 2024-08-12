// PlatformConfigSystemClock.h
// ������� ������������ ��������� ����� ��� ������ �������� �� ������ STM32F3xxx
#ifndef	PLATFORM_CONFIG_SYSTEM_CLOCK_H
#define	PLATFORM_CONFIG_SYSTEM_CLOCK_H

/*******************************************************
 *	����.		��������				��������					����
 *	HSE_OSC								OSC							HSE
 *	HSE			High Speed External		HSE_OSC						SYSCLOCK, HSE_RTC, PLL
 *	HSE_RTC								HSE / HSE_RTC_DIV			RTCCLK
 *******************************************************/

#ifndef	STM32F3
#error "Configuration for STM32F3xx only!"
#endif

#if		defined ( PROJECTCONFIG_CLOCK_16M_64M )
	// ��� STM32F373xx
	// HSE 16.384 MHz ->PLL-> SYSCLK/APB1/APB2 64K/32K/64K MHz
	//			k				MHz
	// HSE			[4..32] 	16K		[4..32] 	�����, ���������
	// PLLCLK	4	[2..16]		64K		[16..72]	HSE * PLLMUL					���������� PLL
	// SYSCLK					64K		[0..72]	 	PLLCLK -> SYSCLK
	// AHB		1	[1,2..512]	64K		[0..72] 	HCLK
	// SYSTICK	8	[8]			8K		[0..10] 	SYSTICK
	// APB1P	2	[1,2..16]	32K		[0..36]		AHB / k1
	// APB1T					64K					??? ( k1 == 1 ) ? APB1P : APB1P * 2		TIM[2..7]
	// APB2P	1	[1,2..16]	64K		[0..72]		AHB / k2
	// APB2T					64K 				??? ( k2 == 1 ) ? APB2P : APB2P * 2		TIM[1,8,15,16,17]
	// ADCCLK	6	[2,4,6,8]	13K		[0,6..14]	?? APB2P / k
	// SDADCCLK	16	[1..24]		4K		[0,5..6,3]	?? SYSTICK / k
	// ----------------------------------------
	#if	( HSE_VALUE != 16384000ul )
	#error "HSE_VALUE Mismatch!"
	#endif
	// __HAL_PWR_VOLTAGESCALING_CONFIG()	// ���������� ����������� ������� ����, �� �������� ������� ������������ ���������� ������� � �����������
	#define PWR_REGULATOR_VOLTAGE_SCALE 			PWR_REGULATOR_VOLTAGE_SCALE1	// SCALE1 - �� 80 ���, SCALE2 - �� 26 ���
	// HAL_RCC_OscConfig( )
	#define PROJECTCONFIG_CLOCK_USE_HSE 										// ������������� ������������ �� HSE
	#define	PROJECTCONFIG_CLOCK_RCC_HSESTATE		RCC_HSE_ON					// ������������ HSE � �������� ���������� 
	#define	PROJECTCONFIG_CLOCK_RCC_PLL_STATE		RCC_PLL_ON					// ������������ PLL
	#define	PROJECTCONFIG_CLOCK_RCC_PLL_SOURCE		RCC_PLLSOURCE_HSE			// ����������� PLL �� HSE
	#define PROJECTCONFIG_CLOCK_RCC_PLL_MUL			RCC_PLL_MUL4				// [2..16]	���������� PLL
	// HAL_RCC_ClockConfig( )
	#define	PROJECTCONFIG_CLOCK_SYSCLK_SOURCE		RCC_SYSCLKSOURCE_PLLCLK		// ������������ PLL � �������� SYSCLK
	#define	PROJECTCONFIG_CLOCK_AHB_DIVIDER			RCC_SYSCLK_DIV1				// SYSCLK -> (AHBDIV) -> HCLK
	#define	PROJECTCONFIG_CLOCK_APB1_DIVIDER		RCC_HCLK_DIV2				// HCLK  -> (APB1DIV) -> APB1
	#define	PROJECTCONFIG_CLOCK_APB2_DIVIDER		RCC_HCLK_DIV1				// HCLK  -> (APB2DIV) -> APB2
	// HAL_ADC_Init( )
//	#define	PROJECTCONFIG_CLOCK_ADCCLK_PRESCALER	ADC_CLOCK_SYNC_PCLK_DIV16	// AHB -> (ADCDIV) -> ADCCLK
#else
#error "Select Project Clock Config!"
#endif	// PROJECTCONFIG_CLOCK_XXX

// ������������� HSE
#ifdef	PROJECTCONFIG_CLOCK_USE_HSE
	#ifndef	HSE_VALUE
	#error	"HSE_VALUE not defined!"
	#else
	#define PROJECTCONFIG_CLOCK_RCC_OSCILLATORTYPE	RCC_OSCILLATORTYPE_HSE		// ����������� SYSCLCK �� HSE
	#ifdef	HSE_USE_EXTERNAL_OSC
	#define PROJECTCONFIG_CLOCK_RCC_HSESTATE		RCC_HSE_BYPASS				// ������������� HSE. �� ������������ ���������� ����������, �.�. � OSC_IN ��������� ������� ���������
	#else
	#define PROJECTCONFIG_CLOCK_RCC_HSESTATE		RCC_HSE_ON					// ������������� HSE. ������������ ���������� ����������, �.�. � OSC_IN � OSC_OUT ��������� ������� �����
	#endif	// HSE_EXTERNAL_OSC
	#endif	// HSE_VALUE
#else	// !PROJECTCONFIG_CLOCK_USE_HSE
	#define PROJECTCONFIG_CLOCK_RCC_HSESTATE		RCC_HSE_OFF					// �� ������������ HSE
	#error	"Only USE_HSE configuration defined :("
#endif	// PROJECTCONFIG_CLOCK_USE_HSE

// ������������ ���������� �������� ��� ������ � Flash,
// ��� ���������� ���������� ������ �� ������� �����������
#ifndef	PROJECTCONFIG_CLOCK_FLASH_SLOWDOWN
#define	PROJECTCONFIG_CLOCK_FLASH_SLOWDOWN		1		// ��������� ��������, ���� �� ���������� � ProjectConfig.h
#endif

#endif	// PLATFORM_CONFIG_SYSTEM_CLOCK_H

