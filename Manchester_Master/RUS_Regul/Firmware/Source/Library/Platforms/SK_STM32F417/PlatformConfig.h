//  онфигуратор платформы, находится в папке платформы и включается в stm32f4xx_hal_conf.h.
// ѕлатформа выбирается препроцессором IAR EWARM константой USE_PLATFORM_xxx

// ѕлатформа Starterkit SK-STM32F217/417 V1.0

#ifndef	PLATFORM_CONFIG_H
#define	PLATFORM_CONFIG_H

#ifndef	USE_PLATFORM_SK_STM32F417
#error  онфигурация не соответствует выбранной платформе!
#endif

#define	STM32F417xx											// ядро установленного чипа

// ≈сли будут разные конфигурации одной платформы (джампера, пайки и т.п.) -
// здесь надо будет подключить файл PlatformSubConfig.h, находящийся в соответсвующей подпапке этой платформы.

/* ########################## Module Selection ############################## */
// в ProjectSoftConfig.h

/* ########################## HSE/HSI Values adaptation ##################### */
#undef	HSE_VALUE	// затычка для SPL - HSE_VALUE (пустой) определить в IAR Preprocessor, и тогда SPL:stm32f4xx.h не станет подставлять свое значение по-умолчанию
#undef	HSE_STARTUP_TIMEOUT
#define HSE_VALUE				( ( uint32_t ) 25000000 )	// [Hz]	¬нешний кварц на плате (быстрый)
#define HSE_STARTUP_TIMEOUT		( ( uint32_t ) 5000 )		// [ms]	“аймаут запуска основного осциллятора
#define LSE_VALUE	 			( ( uint32_t ) 32768 )		// [Hz]	¬нешний кварц на плате (медленный)
#define EXTERNAL_CLOCK_VALUE	( ( uint32_t ) 0 )			// [Hz]	¬нешний кварц на плате (для музыки)

/* ########################### System Configuration ######################### */
#define	VDD_VALUE				( 3300 )		// [mV]	Value of VDD
#define	VDDA_VALUE				( 3300 )		// [mV]	Value of VDDA
#define	VREFP_VALUE				( 3300 )		// [mV]	Value of VRef+
#define	VREFN_VALUE				( 0 )			// [mV]	Value of VRef-

/* ########################## Assert Selection ############################## */
// skip

/* ################## Ethernet peripheral configuration ##################### */
// skip

/* #################### UART peripheral configuration ###################### */
// RS-232
#define	COM1_TX_PIN		GPIO_PIN_9			// TxD	PA9		LQFTP144.101	DA2.11	RS232.Tx
#define	COM1_TX_AF		GPIO_AF7_USART1
#define	COM1_TX_GPIO	GPIOA
#define	COM1_RX_PIN		GPIO_PIN_10			// RxD	PA10		LQFTP144.102	DA2.12	RS232.Rx
#define	COM1_RX_AF		GPIO_AF7_USART1
#define	COM1_RX_GPIO	GPIOA

// "GSM"
#define	COM3_TX_PIN		GPIO_PIN_8			// TxD	PD8		LQFTP144.77	X9.7		GSM_RX
#define	COM3_TX_AF		GPIO_AF7_USART3
#define	COM3_TX_GPIO	GPIOD
#define	COM3_RX_PIN		GPIO_PIN_9			// RxD	PD9		LQFTP144.78	X9.8		GSM_TX
#define	COM3_RX_AF		GPIO_AF7_USART3
#define	COM3_RX_GPIO	GPIOD
#define	COM3_TXEN_PIN	GPIO_PIN_10			// TxEn	PD10		LQFTP144.79	X9.11	GSM_PWR
#define	COM3_TXEN_GPIO	GPIOD

/*/ LOOCH.435.00.01 RS-485 (55, 56)
#define	COM3_TX_PIN		GPIO_PIN_8
#define	COM3_TX_AF		GPIO_AF7_USART3
#define	COM3_TX_GPIO	GPIOD
#define	COM3_RX_PIN		GPIO_PIN_9
#define	COM3_RX_AF		GPIO_AF7_USART3
#define	COM3_RX_GPIO	GPIOD
#define	COM3_TXEN_PIN	GPIO_PIN_14
#define	COM3_TXEN_GPIO	GPIOB
#define	COM_RS485		COM3
*/

// "GPS"
#define	COM6_TX_PIN		GPIO_PIN_6			// TxD	PC6		LQFTP144.96	X9.9		GPS_RX
#define	COM6_TX_AF		GPIO_AF8_USART6
#define	COM6_TX_GPIO	GPIOC
#define	COM6_RX_PIN		GPIO_PIN_7			// RxD	PC7		LQFTP144.97	X9.10	GPS_TX
#define	COM6_RX_AF		GPIO_AF8_USART6
#define	COM6_RX_GPIO	GPIOC
#define	COM6_TXEN_PIN	GPIO_PIN_10			// TxEn	PB10		LQFTP144.69	X9.12	GPS_PWR
#define	COM6_TXEN_GPIO	GPIOB

/*/ LOOCH.435.00.01 AUX (63, 64)
#define	COM6_TX_PIN		GPIO_PIN_6
#define	COM6_TX_AF		GPIO_AF8_USART6
#define	COM6_TX_GPIO	GPIOC
#define	COM6_RX_PIN		GPIO_PIN_7
#define	COM6_RX_AF		GPIO_AF8_USART6
#define	COM6_RX_GPIO	GPIOC
#define	COM_AUX			COM6
*/

/* #################### GPIO Common peripheral configuration ###################### */
//							Port	PinMask			PinPos	Port	X#		Name1		Name2		Name3
#define	GPIO_MKP007			GPIOC,	GPIO_PIN_13,	13		// PC13	X1.8					LED1	
#define	GPIO_MKP019			GPIOF,	GPIO_PIN_7,		7		// PF7	X1.3		Reset
#define	GPIO_MKP020			GPIOF,	GPIO_PIN_8,		8		// PF8	X8.40	TS_CLK					AmpAMuxAddr0
#define	GPIO_MKP021			GPIOF,	GPIO_PIN_9,		9		// PF9	X8.39	TS_DOUT					AmpAMuxEn0
#define	GPIO_MKP022			GPIOF,	GPIO_PIN_10,	10		// PF10	X8.38	LCD_ON					AmpAMuxEn1
#define	GPIO_MKP028			GPIOC,	GPIO_PIN_2,		2		// PC2	X8.36							AmpGate
#define	GPIO_MKP064			GPIOE,	GPIO_PIN_11,	11		// PE11	X8.22				LED4			GenHV_Start
#define	GPIO_MKP065			GPIOE,	GPIO_PIN_12,	12		// PE12	X8.21				LED3			GenHV_Ready
#define	GPIO_MKP069			GPIOB,	GPIO_PIN_10,	10		// PB10	X9.12	GPS_PWR		COM6.TxEn		COM6.TxEn	
#define	GPIO_MKP079			GPIOD,	GPIO_PIN_10,	10		// PD10	X9.11	GSM_PWR		COM3.TxEn		COM3.TxEn	
#define	GPIO_MKP122			GPIOD,	GPIO_PIN_6,		6		// PD6	X1.18				Key
#define	GPIO_MKP126			GPIOG,	GPIO_PIN_11,	11		// PG11	X1.16				Tamper
#define	GPIO_MKP127			GPIOG,	GPIO_PIN_12,	12		// PG12	X1.14				Wake
#define	GPIO_MKP132			GPIOG,	GPIO_PIN_15,	15		// PG15	X1.10				TP3			TP3
#define	GPIO_MKP135			GPIOB,	GPIO_PIN_5,		5		// PB05	X1.10				LED2			TP2
#define	GPIO_MKP139			GPIOB,	GPIO_PIN_8,		8		// PB08	X1.6		SCL			TP0			TP0
#define	GPIO_MKP140			GPIOB,	GPIO_PIN_9,		9		// PB09	X1.5		SDA			TP1			TP1

#define	GPIO_AMux_port		GPIOF
#define	GPIO_AMuxAddr0_pin	GPIO_PIN_8
#define	GPIO_AMuxAddr1_pin	GPIO_PIN_8
#define	GPIO_AMuxAddr2_pin	GPIO_PIN_8
#define	GPIO_AMuxEn0_pin	GPIO_PIN_9
#define	GPIO_AMuxEn1_pin	GPIO_PIN_10
#define	GPIO_AMuxAll_pin	( GPIO_AMuxAddr0_pin | GPIO_AMuxAddr1_pin | GPIO_AMuxAddr2_pin | GPIO_AMuxEn0_pin | GPIO_AMuxEn1_pin )

#define	GPIO_TestPin0		GPIO_MKP139
#define	GPIO_TestPin1		GPIO_MKP140
#define	GPIO_TestPin2		GPIO_MKP135
#define	GPIO_TestPin3		GPIO_MKP132
#define	GPIO_LedPin1		GPIO_MKP007
#define	GPIO_LedPin2		GPIO_LedPin1
#define	GPIO_LedPin3		GPIO_LedPin1
#define	GPIO_COM3_TxEn		GPIO_MKP079
#define	GPIO_COM6_TxEn		GPIO_MKP069
#define	GPIO_GenHV_Start	GPIO_MKP064
#define	GPIO_GenHV_Ready	GPIO_MKP065
#define	GPIO_AmpAMux		GPIO_AMux_port, GPIO_AMuxAll_pin, 0
#define	GPIO_AmpGate		GPIO_MKP028

#endif	// PLATFORM_CONFIG_H

