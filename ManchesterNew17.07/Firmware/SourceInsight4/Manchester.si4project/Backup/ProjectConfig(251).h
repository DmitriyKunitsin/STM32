// ProjectConfig.h
// ������������ ��������� ��� ������.
// ���������� ������������������� ��� ���.638.00.02.00 � ������� ������ ���-�� ���.638.00.00.00
// ���������� ������������������� ��� ���.641.00.05.00 � ���.641.00.06.00 � ������� ������ ����-�����-��� ���.641.00.00.00

// ����������� ���������������� ������ ���������� ����� (638_02) � ������ (641_05 � 641_06)
// ����������� ���������. �������� ���������:
// - ������� ���������� ����������� ������� VDD � VDDA (��������);
// - �� ������ ����� �������� ��� UART2.Rx � UART2 ����� ������������� � ������ �� ����� ����������,
// �.�. � ������ �����.641 �������� ������ ����� � ������ ����� ��/���, � �.�. ����� ���� ���������� ���������;
// - �������� ��� GPIO_SelectMCU ��� ��������� �������� �� �������� � �������� ������, �.�. �������������� ����������;

#ifndef	PROJECT_CONFIG_H
#define	PROJECT_CONFIG_H

/* ########################## Module Selection ############################## */
// ��������� ��������, ��������� � ���������� �������. ���� �� ����� ������ ���� ������ � IAR EWARM
#include "PlatformConfig.h"
//#include "Driver_ADS1231.h"	
// ��� �������
#if		defined ( USE_PLATFORM_OKR_354_10 ) || defined ( USE_PLATFORM_LOOCH_601_03 )
#define	PROJECT_NAME_MANCHESTER
#else
#error "Select platform!"
#endif	// USE_PLATFORM_XXX


/* ####################### System Clock Configuration ######################### */
// ����� ������������� ������������ � ����������� �� ���������
#if		defined ( USE_PLATFORM_OKR_354_10 )
#define	PROJECTCONFIG_CLOCK_16M4_65M5			// ����� 16.384 ���, SYSCLK/APB1/APB2 64/32/32 * 1.024 ���
#elif		defined ( USE_PLATFORM_LOOCH_601_03 )
#define	PROJECTCONFIG_CLOCK_16M0_64M0			// ����� 16.0 ���, SYSCLK/APB1/APB2 64/32/32 ���
#else
#error "Select platform!"
#endif	// USE_PLATFORM_XXX

//#define TEST_BOARD // ���� �������� ��� ������ ������� ����
//#define UART_DEBUG_LOG // ���� � �������, ���� �� ������ ������� ����� �� �������, ������� ������ � ����
// ��������� ������������� ������������
#include "PlatformConfigSystemClock.h"

// ���������� ���������� � DMA ��� ���������� ��������� SKL
#define	SKLP_IRQ_PREEMTPRIORITY				( NVIC_PRIORITY_LOWEST_WITHIN_RTOS - 1 )
#define	SKLP_IRQ_SUBPRIORITY				NVIC_SUBPRIORITY_DEFAULT
#define	SKLP_DMA_PRIORITY					DMA_PRIORITY_LOW

// ���������� ���������� � DMA ��� ���������� ��������� ���������� ������
#define	ADC_IRQ_PREEMTPRIORITY				( NVIC_PRIORITY_LOWEST_WITHIN_RTOS - 2 )
#define	ADC_IRQ_SUBPRIORITY					NVIC_SUBPRIORITY_DEFAULT
#define	ADC_DMA_PRIORITY					DMA_PRIORITY_LOW
#define	ADC_DMA_IRQ_PREEMTPRIORITY			ADC_IRQ_PREEMTPRIORITY
#define	ADC_DMA_IRQ_SUBPRIORITY				ADC_IRQ_SUBPRIORITY


/* ####################### UART Configuration ####################### */

// ********** COM_SKLP_XXX ********************************************
// Platform-Common
#define	COM_SKLP_IRQ_PREEMTPRIORITY			SKLP_IRQ_PREEMTPRIORITY
#define	COM_SKLP_IRQ_SUBPRIORITY			SKLP_IRQ_SUBPRIORITY
#define	COM_SKLP_DMATX_PRIORITY				SKLP_DMA_PRIORITY
#define	COM_SKLP_DMATX_IRQ_PREEMTPRIORITY	SKLP_IRQ_PREEMTPRIORITY
#define	COM_SKLP_DMATX_IRQ_SUBPRIORITY		SKLP_IRQ_SUBPRIORITY
#define	COM_SKLP_DMARX_PRIORITY				SKLP_DMA_PRIORITY
#define	COM_SKLP_DMARX_IRQ_PREEMTPRIORITY	SKLP_IRQ_PREEMTPRIORITY
#define	COM_SKLP_DMARX_IRQ_SUBPRIORITY		SKLP_IRQ_SUBPRIORITY

// Platform-Depended Service COM
#if	defined( USE_PLATFORM_OKR_354_10 ) || defined ( USE_PLATFORM_LOOCH_601_03 )
	#define COM_SKLP_SERVICE				COM1
	#define COM_SKLP_SERVICE_UART_EXT_HDL	COM1_UART_Ext_hdl
	#define COM1_USE
	#define COM1_USE_IRQ
	#define COM1_USE_DMATX
	#define COM1_USE_DMARX
	#ifdef	COM1_TXEN_PIN
	#define COM1_USE_TXEN		// ����������� ������������ �������� �������� RS-485
	#endif
	// ������������� DMA
	#define COM1_DMATX_CHANNEL				DMA1_Channel4
	#define COM1_DMATX_REQUEST				DMA_REQUEST_2
	#define COM1_DMATX_IRQHandler			DMA1_Channel4_IRQHandler
	#define COM1_DMARX_CHANNEL				DMA1_Channel5
	#define COM1_DMARX_REQUEST				DMA_REQUEST_2
	#define COM1_DMARX_IRQHandler			DMA1_Channel5_IRQHandler
	// ����������� COM_SKLP_XXX->COM1
	#define COM1_IRQ_PREEMTPRIORITY 		COM_SKLP_IRQ_PREEMTPRIORITY
	#define COM1_IRQ_SUBPRIORITY			COM_SKLP_IRQ_SUBPRIORITY
	#define COM1_DMATX_PRIORITY 			COM_SKLP_DMATX_PRIORITY
	#define COM1_DMATX_IRQ_PREEMTPRIORITY	COM_SKLP_DMATX_IRQ_PREEMTPRIORITY
	#define COM1_DMATX_IRQ_SUBPRIORITY		COM_SKLP_DMATX_IRQ_SUBPRIORITY
	#define COM1_DMARX_PRIORITY 			COM_SKLP_DMARX_PRIORITY
	#define COM1_DMARX_IRQ_PREEMTPRIORITY	COM_SKLP_DMARX_IRQ_PREEMTPRIORITY
	#define COM1_DMARX_IRQ_SUBPRIORITY		COM_SKLP_DMARX_IRQ_SUBPRIORITY
#else
#error Select platform!
#endif	// Platform-Depended Service COM


// ********** COM_AUX *************************************************
// Platform-Depended Aux COM
#if	defined( USE_PLATFORM_OKR_354_10 ) || defined ( USE_PLATFORM_LOOCH_601_03 )
#define COM_SKLP_AUX						COM3
#define COM_SKLP_AUX_UART_EXT_HDL			COM3_UART_Ext_hdl
#define COM3_USE
#define COM3_USE_IRQ
#define COM3_USE_DMATX
#define COM3_USE_DMARX

// ������������� DMA
#define COM3_DMATX_CHANNEL				DMA1_Channel2
#define COM3_DMATX_REQUEST				DMA_REQUEST_2
#define COM3_DMATX_IRQHandler			DMA1_Channel2_IRQHandler
#define COM3_DMARX_CHANNEL				DMA1_Channel3
#define COM3_DMARX_REQUEST				DMA_REQUEST_2
#define COM3_DMARX_IRQHandler			DMA1_Channel3_IRQHandler

// ����������� COM_SKLP_XXX->COM3
#define COM3_IRQ_PREEMTPRIORITY 		COM_SKLP_IRQ_PREEMTPRIORITY
#define COM3_IRQ_SUBPRIORITY			COM_SKLP_IRQ_SUBPRIORITY
#define COM3_DMATX_PRIORITY 			COM_SKLP_DMATX_PRIORITY
#define COM3_DMATX_IRQ_PREEMTPRIORITY	COM_SKLP_DMATX_IRQ_PREEMTPRIORITY
#define COM3_DMATX_IRQ_SUBPRIORITY		COM_SKLP_DMATX_IRQ_SUBPRIORITY
#define COM3_DMARX_PRIORITY 			COM_SKLP_DMARX_PRIORITY
#define COM3_DMARX_IRQ_PREEMTPRIORITY	COM_SKLP_DMARX_IRQ_PREEMTPRIORITY
#define COM3_DMARX_IRQ_SUBPRIORITY		COM_SKLP_DMARX_IRQ_SUBPRIORITY

#endif	// Platform-Depended Service COM


/* #################### GPIO Common peripheral configuration ###################### */
// ������������� ��� ���� ����� ��-���������
// ��. �������� � Common_GPIO.h
#if	defined( USE_PLATFORM_OKR_354_10 ) 
#define	GPIO_Common_defs																												\
{																																		\
	{ GPIO_ADC_SPD,					GPIO_MODE_OUTPUT_PP,	GPIO_PULLDOWN	| GPIO_PULL_INITSTATE_RESET,	GPIO_SPEED_FREQ_LOW},		\
	{ GPIO_ADC_PWDWN,				GPIO_MODE_OUTPUT_PP,	GPIO_PULLDOWN	| GPIO_PULL_INITSTATE_RESET,	GPIO_SPEED_FREQ_LOW},		\
	{ GPIO_MOTOR_PWR,				GPIO_MODE_OUTPUT_PP,	GPIO_PULLDOWN	| GPIO_PULL_INITSTATE_RESET,	GPIO_SPEED_FREQ_LOW},		\
}
#elif	defined( USE_PLATFORM_LOOCH_601_03 ) 
#define	GPIO_Common_defs																												\
{																																		\
	{ GPIO_KT1,					GPIO_MODE_OUTPUT_PP,	GPIO_PULLDOWN	| GPIO_PULL_INITSTATE_RESET,	GPIO_SPEED_FREQ_LOW},		\
	{ GPIO_KT2,		                	GPIO_MODE_OUTPUT_PP,	GPIO_PULLDOWN	| GPIO_PULL_INITSTATE_RESET,	GPIO_SPEED_FREQ_LOW},		\
	{ GPIO_KT3,			               	GPIO_MODE_OUTPUT_PP,	GPIO_PULLDOWN	| GPIO_PULL_INITSTATE_RESET,	GPIO_SPEED_FREQ_LOW},		\
	{ 	} \
}
#else
#error Select platform!
#endif	// Platform-Depended
	
// ������� ��� ���� �����
typedef enum GPIO_CommonIndex_enum
{
	// iGPIO_NULL - ������ ������. ��������� � ���� � ���� �������� ������������ (�� �� ����������)
	iGPIO_NULL = -1,

	// �������� �������, ������� � 0
	iGPIO_KT1,
	iGPIO_KT2,
	iGPIO_KT3,

	// iGPIO_Total - ������ �������
	iGPIO_Total,

	// ��������������� �������-��������
	iGPIO_TestPinSKLP_Transport = iGPIO_NULL,
	iGPIO_TestPinSKLP_Service	= iGPIO_NULL,
} GPIO_CommonIndex_t;

//#define	IS_GPIO_COMMONINDEX_LED( Index )	( ( Index ) == iGPIO_LED1 )

/* #################### Timer peripheral configuration ###################### */
// ********** SKLP_TIMER ************************************************
// ������� �������� �������� ����� ������ ���������� ����� �� ���

#define	SKLP_TIMER_IRQ_PREEMTPRIORITY	SKLP_IRQ_PREEMTPRIORITY		// ��������� ����� ��, ��� � ���������� UART
#define	SKLP_TIMER_IRQ_SUBPRIORITY		SKLP_IRQ_SUBPRIORITY
#define	SKLP_TIMER_CLOCKDIVISION		TIM_CLOCKDIVISION_DIV4
// Platform-Depended
#if	defined(USE_PLATFORM_OKR_354_10) || defined( USE_PLATFORM_LOOCH_601_03 )
	#define	SKLP_TIMER_SERVICE							TIM7
	#define	SKLP_TIMER_SERVICE_hdl						TIM7_hdl
	#define SKLP_TIMER_SERVICE_IRQn 					TIM7_IRQn
	#define	SKLP_Timer_Service_MspInit					TIM7_MspInit
	#define SKLP_Timer_Service_MspDeInit				TIM7_MspDeInit
	#define SKLP_Timer_Service_PeriodElapsedCallback	TIM7_PeriodElapsedCallback
	#define	TIM7_USE
	#define	TIM7_USE_IRQ
#else
#error "Select platform!"
#endif	// Platform-Depended

#if	defined(USE_PLATFORM_OKR_354_10) || defined( USE_PLATFORM_LOOCH_601_03 )
	#define	SKLP_TIMER_AUX								TIM6
	#define	SKLP_TIMER_AUX_hdl							TIM6_hdl
	#define SKLP_TIMER_AUX_IRQn 						TIM6_IRQn
	#define	SKLP_Timer_Aux_MspInit						TIM6_MspInit
	#define SKLP_Timer_Aux_MspDeInit					TIM6_MspDeInit
	#define SKLP_Timer_Aux_PeriodElapsedCallback		TIM6_PeriodElapsedCallback
	#define	TIM6_USE
	#define	TIM6_USE_IRQ
#endif

/*/ ������-������� ��������� �� �������� ����� ������ (�������� ��������)
#if	defined(USE_PLATFORM_OKR_354_10)
	#define SPEED_COUNTER_TIM							TIM2
	#define SPEED_COUNTER_TIM_HDL						TIM2_hdl
	#define SPEED_COUNTER_TIM_MspInit					TIM2_MspInit
	#define SPEED_COUNTER_TIM_MspDeInit					TIM2_MspDeInit
	#define TIM2_USE
	#define TIM2_USE_IRQ_OC
	#define SPEED_COUNTER_TIM_OC_DelayElapsedCallback	TIM2_OC_DelayElapsedCallback
	#define	SPEED_COUNTER_TIM_IRQ_PREEMTPRIORITY		NVIC_PRIORITY_LOWEST_WITHIN_RTOS
	#define	SPEED_COUNTER_TIM_IRQ_SUBPRIORITY			NVIC_SUBPRIORITY_DEFAULT
	#define SPEED_COUNTER_TIM_GPIO_PORT					GPIOA
	#define SPEED_COUNTER_TIM_GPIO_PIN					GPIO_PIN_15
	#define SPEED_COUNTER_TIM_AF						GPIO_AF2_TIM2
#endif

// ������-��������� ��������� ��� ��� ���������� ��������� ���������
#if	defined(USE_PLATFORM_OKR_354_10)
	#define MOTOR_SPEED_CONTROL_TIM						TIM1
	#define MOTOR_SPEED_CONTROL_TIM_HDL					TIM1_hdl
	#define MOTOR_SPEED_CONTROL_TIM_MspInit				TIM1_MspInit
	#define MOTOR_SPEED_CONTROL_TIM_MspDeInit			TIM1_MspDeInit
	#define TIM1_USE
	#define MOTOR_SPEED_CONTROL_TIM_GPIO_PORT			GPIOA
	#define MOTOR_SPEED_CONTROL_TIM_GPIO_PIN			GPIO_PIN_8
	#define MOTOR_SPEED_CONTROL_TIM_AF					GPIO_AF1_TIM1
#endif
*/
/*
// ������ ��� ����������� (COMP2 Blinking)
#if	defined(USE_PLATFORM_OKR_354_10)
	#define RUS_REGUL_COMP_TIMER						TIM15
	#define RUS_REGUL_COMP_TIMER_CHANNEL				TIM_CHANNEL_1
	#define RUS_REGUL_COMP_TIMER_HDL					TIM15_hdl
	#define RUS_REGUL_COMP_TIMER_MspInit				TIM15_MspInit
	#define RUS_REGUL_COMP_TIMER_MspDeInit				TIM15_MspDeInit
	#define TIM15_USE
	#define RUS_REGUL_COMP_TIMER_GPIO_PORT				GPIOB
	#define RUS_REGUL_COMP_TIMER_GPIO_PIN				GPIO_PIN_14
	#define RUS_REGUL_COMP_TIMER_ALTERNTAE				GPIO_AF14_TIM15
#endif*/

// ######## ��� ADS1231
// ����� ADS1231.nDRDY/DOUT ������� �� PA6.
// ������������ ��� ������ ������ ����� SPI.MISO (����� � �������������� ������)
// � ��� �������� ���������� ��� ����� EXTI.12
/*#if defined(USE_PLATFORM_OKR_354_10)
#define ADS1231_IRQn				EXTI9_5_IRQn
#define GPIO_USE_IRQ_EXTI6
#define ADS1231_EXTI_Callback		GPIO_PIN_6_Callback
#define	ADS1231_IRQ_PREEMTPRIORITY	NVIC_PRIORITY_LOWEST_WITHIN_RTOS
#define	ADS1231_IRQ_SUBPRIORITY		NVIC_SUBPRIORITY_DEFAULT
#else 
#error "Select platform!"
#endif
*/
/* #################### SPI peripheral configuration ###################### */
/*#if defined(USE_PLATFORM_OKR_354_10)
#define ADS1231_SPI					SPI1
#define	ADS1231_SPI_MISO_GPIO		GPIOA
#define	ADS1231_SPI_MISO_PIN		GPIO_PIN_6		// ��� �� ����� ������������ ��� ���� ���������� �� ������� ������ ���������� ���, �.�. ������� ����� ��������� �������������
#define	ADS1231_SPI_MISO_AF			GPIO_AF5_SPI1
#define	ADS1231_SPI_SCK_GPIO		GPIOA
#define	ADS1231_SPI_SCK_PIN			GPIO_PIN_5
#define	ADS1231_SPI_SCK_AF			GPIO_AF5_SPI1
#else
#error "Select platform!"
#endif	// Platform-Depended
*/
/* ############################################################################################################## */
/* ######################################## ADC peripheral configuration ######################################## */
/* ############################################################################################################## */

#define ADC_CLOCK_SOURCE			RCC_ADCCLKSOURCE_SYSCLK

// ��� ��� ��������� ���������� 12 �

#define ADC1_USE
#define USE_ADC1
#define ADC1_CLOCKPRESCALER			PROJECTCONFIG_CLOCK_ADCCLK_PRESCALER
#define ADC1_RESOLUTION_bits		12
#define ADC1_SAMPLINGTIME			ADC_SAMPLETIME_640CYCLES_5
#define ADC1_DATAALIGN				ADC_DATAALIGN_RIGHT
#define ADC1_TRIGGER_SOFTWARE

#if defined(STM32L4)
#define ADC1_DMA_REQUEST			DMA_REQUEST_0
#define ADC1_DMA_INSTANCE			DMA2_Channel3
#define ADC1_DMA_IRQn				DMA2_Channel3_IRQn
#define ADC1_DMA_IRQHandler			DMA2_Channel3_IRQHandler
#else
#error Select Platform!
#endif


/*/ ������������ ��������� ��� ��������� ���� (����� ������������� �������� �� ���������� ���, ������� ������� ���������� ������)
#if defined(USE_PLATFORM_OKR_354_10)
#define RUS_REGUL_OPAMP				OPAMP1
#define RUS_REGUL_OPAMP_GPIO		GPIOA
#define RUS_REGUL_OPAMP_PINS		GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3
#define RUS_REGUL_OPAMP_MODE		GPIO_MODE_ANALOG
#else
#error "Select Platform!"
#endif
*/
// ���������� ��� ��������� ������� ���������� �� ������ OPAMP � DAC1 (�������� �����������)
/*#if defined(USE_PLATFORM_OKR_354_10)
#define RUS_REGUL_COMP				COMP2
#define RUS_REGUL_COMP_GPIO			GPIOB
#define RUS_REGUL_COMP_PIN			GPIO_PIN_4
#define RUS_REGUL_COMP_MODE			GPIO_MODE_ANALOG
#define USE_COMP_IRQ
#define RUS_REGUL_COMP_HDL			COMP_hdl
#define RUS_REGUL_COMP_IRQN			COMP_IRQn
#else
#error "Select Platform!"
#endif

#if defined(USE_PLATFORM_OKR_354_10)
#define USE_DAC_CHANNEL_1
#define RUS_REGUL_DAC				DAC1
#define RUS_REGUL_DAC_HDL			DAC1_hdl
#define RUS_REGUL_DAC_CHANNEL		DAC_CHANNEL_1
#else
#error "Select Platform!"
#endif*/


#endif	// PROJECT_CONFIG_H
