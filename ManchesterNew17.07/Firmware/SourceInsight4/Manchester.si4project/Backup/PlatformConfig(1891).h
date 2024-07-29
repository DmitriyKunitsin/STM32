// ������������ ���������, ��������� � ����� ��������� � ���������� � stm32f4xx_hal_conf.h.
// ��������� ���������� �������������� IAR EWARM ���������� USE_PLATFORM_xxx

// ��������� ���.516.00.02.00 - ���������� ������ ���� (LWD PLC)
// � �������� �������� �� ������������ � �� �����������
// ���������� � �������� ������

#ifndef	PLATFORM_CONFIG_H
#define	PLATFORM_CONFIG_H

#ifndef	USE_PLATFORM_LOOCH_601_03
#error "������������ �� ������������� ��������� ���������!"
#endif

#if	!defined( PLATFORM_MODIFICATION_DEFAULT )
#define	PLATFORM_MODIFICATION_DEFAULT
#endif


// ���		STM32F407VGT
// ������	LQFP100
// flash	1024K

#define STM32L4					// Family
#define STM32L433xx				// Series
#define STM32L433CC				// Pin count and flash size
#define TARGET_PACKAGE		DEVSIGN_PACKAGE_LQFP48

// ���� ����� ������ ������������ ����� ��������� (��������, ����� � �.�.) -
// ����� ���� ����� ���������� ���� PlatformSubConfig.h, ����������� � �������������� �������� ���� ���������.

/* ########################## Module Selection ############################## */
// � ProjectSoftConfig.h

/* ########################## HSE/HSI Values adaptation ##################### */
#undef	HSE_VALUE	// ������� ��� SPL - HSE_VALUE (������) ���������� � IAR Preprocessor, � ����� SPL:stm32f4xx.h �� ������ ����������� ���� �������� ��-���������
#undef	HSE_STARTUP_TIMEOUT
#if		defined( PLATFORM_MODIFICATION_DEFAULT )
#define HSE_VALUE				( 16000000ul )				// [Hz]	������� ����� �� ����� (�������)
#endif	// PLATFORM_MODIFICATION
#define HSE_STARTUP_TIMEOUT		( ( uint32_t ) 5000 )		// [ms]	������� ������� ��������� �����������
//#warning '��������� � �������� HSE_STARTUP_TIMEOUT'
#define LSE_VALUE	 			( ( uint32_t ) 0 )			// [Hz]	������� ����� �� ����� (���������)
#define LSE_STARTUP_TIMEOUT		( ( uint32_t ) 5000 )		// [ms]	������� ������� ���������������� �����������
#define EXTERNAL_CLOCK_VALUE	( ( uint32_t ) 0 )			// [Hz]	������� ����� �� ����� (��� ������)

/* ########################### System Configuration ######################### */
#define	VDD_VALUE				( 3300 )				// [mV]	Value of VDD
#define	VDDA_VALUE				( 3300 )				// [mV]	Value of VDDA
#define	VREFP_VALUE				( VDDA_VALUE )			// [mV]	Value of VRef+
#define	VREFN_VALUE				( 0 )					// [mV]	Value of VRef-
#define VDDA_VALUE_V			(VDDA_VALUE / 1000.0f) 	// [V]	Value of VDDA

/* ########################## Assert Selection ############################## */
// skip

/* ################## Ethernet peripheral configuration ##################### */
// skip

//#warning "�� ��������!"
/* #################### UART peripheral configuration ###################### */
// UART1 ��� ����� � ������������ ����
#if		defined( USE_HAL_DRIVER )
#define	COM1_TX_PIN		GPIO_PIN_9			// UART_TX	PA9		D6.4 
#define	COM1_TX_AF		GPIO_AF7_USART1
#define	COM1_TX_GPIO	GPIOA

#define	COM1_RX_PIN		GPIO_PIN_10			// UART_RX	PA10	D6.1	
#define	COM1_RX_AF		GPIO_AF7_USART1
#define	COM1_RX_GPIO	GPIOA

#define	COM1_TXEN_PIN	GPIO_PIN_12			// UART_DE	PA12	D6.2	
#define	COM1_TXEN_GPIO	GPIOA

// UART3 ��� �������� ������ ��� ������� (�� ����� ����� �� ���� ������������ ������)
#define	COM3_TX_PIN		GPIO_PIN_10			// USART3_TX	PB10		��1
#define	COM3_TX_AF		GPIO_AF7_USART3
#define	COM3_TX_GPIO	GPIOB

#define	COM3_RX_PIN		GPIO_PIN_11			// USART3_RX	PB11		��2
#define	COM3_RX_AF		GPIO_AF7_USART3
#define	COM3_RX_GPIO	GPIOB
#else
#error "Select Perifery Driver!"
#endif


/* #################### SPI peripheral configuration ###################### */


/* #################### GPIO Common peripheral configuration ###################### */
//							Port	PinMask			PinPos	Port	X#		Name1		Name2		Name3

#define GPIO_MKP002			GPIOC,	GPIO_PIN_13,		13		// PB0	D1.4	���������� ��������� ��������� �������� ��� (ADS1231ID)
#define GPIO_MKP003			GPIOC,	GPIO_PIN_14,		14		// PB0	D1.4	���������� ��������� ��������� �������� ��� (ADS1231ID)
#define GPIO_MKP004			GPIOC,	GPIO_PIN_15,		15		// PB0	D1.4	���������� ��������� ��������� �������� ��� (ADS1231ID)
#define GPIO_MKP045			GPIOB,	GPIO_PIN_8,		8		// PB0	D1.4	���������� ��������� ��������� �������� ��� (ADS1231ID)
#define GPIO_MKP046			GPIOB,	GPIO_PIN_9,		9		// PB0	D1.4	���������� ��������� ��������� �������� ��� (ADS1231ID)


#define GPIO_KT1			GPIO_MKP045
#define GPIO_KT2			GPIO_MKP046
#define GPIO_KT3			GPIO_MKP002
#define GPIO_KT4			GPIO_MKP003
#define GPIO_KT5			GPIO_MKP004


// ������� ��� ������ ���
//#define ADC_TRIGGER_TIM		ADC_EXTERNALTRIGINJEC_T15_TRGO
#endif	// PLATFORM_CONFIG_H

