//  онфигуратор платформы, находитс€ в папке платформы и включаетс€ в stm32f3xx_hal_conf.h.
// ѕлатформа выбираетс€ препроцессором IAR EWARM константой USE_PLATFORM_xxx

// “ест-платформа для отработки библиотеки для STM32F373

#ifndef	PLATFORM_CONFIG_H
#define	PLATFORM_CONFIG_H

// ѕроверка выбранной платформы и опциональной модификации (настраивать в препроцессоре IAR EWARM)
#if !defined( USE_PLATFORM_TEST373 ) 
#error  онфигураци€ не соответствует выбранной платформе!
#endif
#if	!defined( PLATFORM_MODIFICATION_DEFAULT )
#define	PLATFORM_MODIFICATION_DEFAULT
#endif

// Platform & Target
#define PLATFORM_NAME	"Test373"		// Ќаименование платформы (используется в загрузчике)
// чип		STM32F373RCT6
// корпус	LQFP64
// flash	256K
// RAM		32K
// Temp		-40..+85 degC
#define	STM32F3					// Family
#define	STM32F373xC				// Series
#define	STM32F373RCT6			// Pins count and Flash size 

/*/ чип		STM32F373CCT6
// корпус	LQFP48
// flash	256K
// RAM		32K
// Temp		-40..+85 degC
#define	STM32F3					// Family
#define	STM32F373xC				// Series
#define	STM32F373CCT6			// Pins count and Flash size 
*/

/*/ чип		STM32F373CBT7
// корпус	LQFP48
// flash	128K
// RAM		24K
// Temp		-40..+105 degC
#define	STM32F3					// Family
#define	STM32F373xC				// Series
#define	STM32F373CBT7			// Pins count and Flash size 
*/

/* ########################## Module Selection ############################## */
// в ProjectSoftConfig.h

/* ########################## HSE/HSI Values adaptation ##################### */
#undef	HSE_VALUE	// затычка для SPL - HSE_VALUE (пустой) определить в IAR Preprocessor, и тогда SPL:stm32f4xx.h не станет подставлять свое значение по-умолчанию
#define HSE_VALUE				( 16384000ul )				// [Hz]	¬нешний кварц на плате (быстрый)
#undef	HSE_STARTUP_TIMEOUT
#define HSE_STARTUP_TIMEOUT		( ( uint32_t ) 100 )		// [ms]	“аймаут запуска основного осцилл€тора
#define LSE_VALUE	 			( ( uint32_t ) 0 )			// [Hz]	¬нешний кварц на плате (медленный)
#define EXTERNAL_CLOCK_VALUE	( ( uint32_t ) 0 )			// [Hz]	¬нешний кварц на плате (дл€ музыки)

/* ########################### System Configuration ######################### */
#define	VDD_VALUE				( 3300 )		// [mV]	Value of VDD
#define	VDDA_VALUE				( VDD_VALUE )	// [mV]	Value of VDDA
#define	VREFP_VALUE				( VDDA_VALUE )	// [mV]	Value of VRef+
#define	VREFN_VALUE				( 0 )			// [mV]	Value of VRef-

/* ########################## Assert Selection ############################## */
// skip

/* ################## Ethernet peripheral configuration ##################### */
// skip

/* #################### UART peripheral configuration ###################### */
// Modem SIG60 (UART2)		подключение параллельно с главным контроллером √√ѕ, который контролирует SIG60.HDC
#define	COM2_TX_PIN		GPIO_PIN_2			// TxD	PA2		LQFTP64.16	D6.1		(4)PLC_TX		SIG60.HDI
#define	COM2_TX_AF		GPIO_AF7_USART2
#define	COM2_TX_GPIO	GPIOA 
#define	COM2_RX_PIN		GPIO_PIN_3			// RxD	PA3		LQFTP64.17			(5)PLC_RX		sSIG60.HDO
#define	COM2_RX_AF		GPIO_AF7_USART2
#define	COM2_RX_GPIO	GPIOA

// Service (UART3)		подключение к главному контроллеру √√ѕ, напрямую
#define	COM3_TX_PIN		GPIO_PIN_10			// TxD	PB10		LQFTP64.29	DD7.1	(6)SLAVE_TX
#define	COM3_TX_AF		GPIO_AF7_USART3
#define	COM3_TX_GPIO	GPIOB 
#define	COM3_RX_PIN		GPIO_PIN_11			// RxD	PB11		LQFTP64.30			(8)SLAVE_RX
#define	COM3_RX_AF		GPIO_AF7_USART3
#define	COM3_RX_GPIO	GPIOB
#define	COM3_TXEN_PIN	GPIO_PIN_2			// TxEn	PB2		LQFTP64.28			(7)SLAVE_DE	резерв
#define	COM3_TXEN_GPIO	GPIOB
/*/  онфигурация UART RS-485 для STM32F4_StdPerifLib, используемого в загрузчике
#define	BOOTCOM_TX_PIN			GPIO_Pin_2
#define BOOTCOM_TX_SOURCE		GPIO_PinSource2
#define	BOOTCOM_TX_AF			GPIO_AF_USART2
#define	BOOTCOM_TX_GPIO			GPIOA
#define	BOOTCOM_TX_GPIO_CLK		RCC_AHB1Periph_GPIOA
#define	BOOTCOM_RX_PIN			GPIO_Pin_3
#define BOOTCOM_RX_SOURCE		GPIO_PinSource3
#define	BOOTCOM_RX_AF			GPIO_AF_USART2
#define	BOOTCOM_RX_GPIO			GPIOA
#define	BOOTCOM_RX_GPIO_CLK		RCC_AHB1Periph_GPIOA
#define	BOOTCOM_TXEN_PIN		GPIO_Pin_10
#define BOOTCOM_TXEN_SOURCE		GPIO_PinSource10
#define	BOOTCOM_TXEN_GPIO		GPIOB
#define	BOOTCOM_TXEN_GPIO_CLK	RCC_AHB1Periph_GPIOB
#define	BOOTCOM_USART			USART2
#define	BOOTCOM_CLK_EN( )		RCC_APB1PeriphClockCmd( RCC_APB1Periph_USART2, ENABLE )
#define	BOOTCOM_IRQn			USART2_IRQn
#define	BootCOM_IRQHandler		USART2_IRQHandler
*/

/* #################### SPI peripheral configuration ###################### */


/* #################### GPIO Common peripheral configuration ###################### */
//							Port	PinMask			PinPos	Port	X#		Name1		Name2		Name3
#define	GPIO_MKP003			GPIOC,	GPIO_PIN_14,	14		// PC14	HL1		LED_TEST1		LED1
#define	GPIO_MKP004			GPIOC,	GPIO_PIN_15,	15		// PC15	HL2		LED_TEST2		LED2
#define	GPIO_MKP028			GPIOB,	GPIO_PIN_2,		2		// PB2	(7)		SLAVE_DE
#define	GPIO_MKP033			GPIOB,	GPIO_PIN_12,	12		// PB12	D3.12	GYRO_INT
#define	GPIO_MKP037			GPIOC,	GPIO_PIN_6,		6		// PC6	D3.22	GYRO_CS
#define	GPIO_MKP039			GPIOC,	GPIO_PIN_8,		8		// PC8	D4.2		H1			PULSE_H1
#define	GPIO_MKP040			GPIOC,	GPIO_PIN_9,		9		// PC9	D5.2		L1			PULSE_L1
#define	GPIO_MKP041			GPIOA,	GPIO_PIN_8,		8		// PA8	KT8		Test0
#define	GPIO_MKP042			GPIOA,	GPIO_PIN_9,		9		// PA9	KT9		Test1
#define	GPIO_MKP043			GPIOA,	GPIO_PIN_10,	10		// PA10	KT10		Test2
#define	GPIO_MKP044			GPIOA,	GPIO_PIN_11,	11		// PA11	D2.2		V_PWM		PWM_BQV
#define	GPIO_MKP045			GPIOA,	GPIO_PIN_12,	12		// PA12	KT11		Test3

#define	GPIO_TestPin0		GPIO_MKP041
#define	GPIO_TestPin1		GPIO_MKP042
#define	GPIO_TestPin2		GPIO_MKP043
#define	GPIO_TestPin3		GPIO_MKP045
#define	GPIO_Led1			GPIO_MKP003
#define	GPIO_Led2			GPIO_MKP004
#define	GPIO_Led3			GPIO_NULL
#define	GPIO_GyroCS			GPIO_MKP037
#define	GPIO_GyroInt		GPIO_MKP033
#define	GPIO_PulseLow		GPIO_MKP040
#define	GPIO_PulseHigh		GPIO_MKP039
#define	GPIO_PWM_BQV		GPIO_MKP044

/*/  онфигурация UART RS-485 для STM32F4_StdPerifLib, используемого в загрузчике
#define	GPIO_RS485_TxEn_port	GPIOB
#define	GPIO_RS485_TxEn_pin		GPIO_PIN_10
#define	GPIO_LedPinBusy_port	GPIOC
#define	GPIO_LedPinBusy_pin		GPIO_PIN_13
*/

// Ў»ћ для управления "высоковольтным" источником
//#define	GPIO_GENHV_PWM_PIN			GPIO_PIN_11
//#define	GPIO_GENHV_PWM_PORT			GPIOA


#endif	// PLATFORM_CONFIG_H

