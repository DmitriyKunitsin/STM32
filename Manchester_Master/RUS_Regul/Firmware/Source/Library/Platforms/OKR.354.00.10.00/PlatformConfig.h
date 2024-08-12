// Конфигуратор платформы, находитсЯ в папке платформы и включаетсЯ в stm32f4xx_hal_conf.h.
// Платформа выбираетсЯ препроцессором IAR EWARM константой USE_PLATFORM_xxx

// Платформа ЛУЧ.516.00.02.00 - Контроллер памЯти ИНГК (LWD PLC)
// В реальных приборах не используетсЯ и не планируетсЯ
// Применение в качестве макета

#ifndef	PLATFORM_CONFIG_H
#define	PLATFORM_CONFIG_H

#ifndef	USE_PLATFORM_OKR_354_10
#error "КонфигурациЯ не соответствует выбранной платформе!"
#endif

#if	!defined( PLATFORM_MODIFICATION_DEFAULT )
#define	PLATFORM_MODIFICATION_DEFAULT
#endif


// чип		STM32F407VGT
// корпус	LQFP100
// flash	1024K

#define STM32L4					// Family
#define STM32L433xx				// Series
#define STM32L433CC				// Pin count and flash size
#define TARGET_PACKAGE		DEVSIGN_PACKAGE_LQFP48

// Если будут разные конфигурации одной платформы (джампера, пайки и т.п.) -
// здесь надо будет подключить файл PlatformSubConfig.h, находЯщийсЯ в соответсвующей подпапке этой платформы.

/* ########################## Module Selection ############################## */
// в ProjectSoftConfig.h

/* ########################## HSE/HSI Values adaptation ##################### */
#undef	HSE_VALUE	// затычка длЯ SPL - HSE_VALUE (пустой) определить в IAR Preprocessor, и тогда SPL:stm32f4xx.h не станет подставлЯть свое значение по-умолчанию
#undef	HSE_STARTUP_TIMEOUT
#if		defined( PLATFORM_MODIFICATION_DEFAULT )
#define HSE_VALUE				( 16384000ul )				// [Hz]	Внешний кварц на плате (быстрый)
#endif	// PLATFORM_MODIFICATION
#define HSE_STARTUP_TIMEOUT		( ( uint32_t ) 5000 )		// [ms]	Таймаут запуска основного осциллятора
//#warning 'Убедиться в значении HSE_STARTUP_TIMEOUT'
#define LSE_VALUE	 			( ( uint32_t ) 0 )			// [Hz]	Внешний кварц на плате (медленный)
#define LSE_STARTUP_TIMEOUT		( ( uint32_t ) 5000 )		// [ms]	Таймаут запуска вспомогательного осциллятора
#define EXTERNAL_CLOCK_VALUE	( ( uint32_t ) 0 )			// [Hz]	Внешний кварц на плате (для музыки)

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

//#warning "Не доделано!"
/* #################### UART peripheral configuration ###################### */
// UART1 для связи с контроллером Теле
#if		defined( USE_HAL_DRIVER )
#define	COM1_TX_PIN		GPIO_PIN_9			// UART_TX	PA9		D6.4 
#define	COM1_TX_AF		GPIO_AF7_USART1
#define	COM1_TX_GPIO	GPIOA

#define	COM1_RX_PIN		GPIO_PIN_10			// UART_RX	PA10	D6.1	
#define	COM1_RX_AF		GPIO_AF7_USART1
#define	COM1_RX_GPIO	GPIOA

#define	COM1_TXEN_PIN	GPIO_PIN_12			// UART_DE	PA12	D6.2	
#define	COM1_TXEN_GPIO	GPIOA

// UART3 был добавлен больше для отладки (на плате висит на двух коннтрольных точках)
#define	COM3_TX_PIN		GPIO_PIN_10			// USART3_TX	PB10		КТ1
#define	COM3_TX_AF		GPIO_AF7_USART3
#define	COM3_TX_GPIO	GPIOB

#define	COM3_RX_PIN		GPIO_PIN_11			// USART3_RX	PB11		КТ2
#define	COM3_RX_AF		GPIO_AF7_USART3
#define	COM3_RX_GPIO	GPIOB
#else
#error "Select Perifery Driver!"
#endif


/* #################### SPI peripheral configuration ###################### */


/* #################### GPIO Common peripheral configuration ###################### */
//							Port	PinMask			PinPos	Port	X#		Name1		Name2		Name3

#define GPIO_MKP018			GPIOB,	GPIO_PIN_0,		0		// PB0	D1.4	Управление скоростью оцифровки внешнего АЦП (ADS1231ID)
#define GPIO_MKP019			GPIOB,	GPIO_PIN_1,		1		// PB1	D1.14	Управление питанием внешнего АЦП (ADS1231ID) (датчик давления) 
#define GPIO_MKP020			GPIOB,	GPIO_PIN_2,		2		// PB2	D5.2	Смена направления вращения мотора
#define GPIO_MKP041			GPIOB,	GPIO_PIN_5,		5		// PB5	KT6		Адресаня нога (пока что на КТ)
#define GPIO_MKP042			GPIOB,	GPIO_PIN_6,		6		// PB6	KT7		Адресаня нога (пока что на КТ)
#define GPIO_MKP004			GPIOC,	GPIO_PIN_15,	15		// PC15 R23.1	Подать пиатние на мотор



#define GPIO_ADC_SPD			GPIO_MKP018
#define GPIO_ADC_PWDWN			GPIO_MKP019
#define GPIO_MOTOR_REVERSE_ROT	GPIO_MKP020
#define GPIO_ADDRESS_0			GPIO_MKP041
#define GPIO_ADDRESS_1			GPIO_MKP042
#define GPIO_MOTOR_PWR			GPIO_MKP004

// Триггер ждя старта АЦП
#define ADC_TRIGGER_TIM		ADC_EXTERNALTRIGINJEC_T15_TRGO
#endif	// PLATFORM_CONFIG_H

