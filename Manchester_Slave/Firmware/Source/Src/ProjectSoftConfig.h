// ProjectSoftConfig.h
// Конфигуратор модулей ПО под задачу, находится в исходниках проекта. Подключается в stm32xxxx_hal_conf.h.
// Проект спектрометрического АЦП для модулей ГГК-ЛП 638 и 641

#ifndef	PROJECT_SOFT_CONFIG_H
#define	PROJECT_SOFT_CONFIG_H

/* ########################## Module Selection ############################## */
#define HAL_MODULE_ENABLED  
#define HAL_GPIO_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_ADC_MODULE_ENABLED
#define HAL_DAC_MODULE_ENABLED 
#define HAL_EXTI_MODULE_ENABLED
#define HAL_IWDG_MODULE_ENABLED 
#define HAL_SPI_MODULE_ENABLED   
#define HAL_TIM_MODULE_ENABLED   
#define HAL_UART_MODULE_ENABLED
#define HAL_OPAMP_MODULE_ENABLED
#define HAL_COMP_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED


// Опции
/*	 
#define HAL_MODULE_ENABLED
#define HAL_ADC_MODULE_ENABLED
#define HAL_CAN_MODULE_ENABLED
//#define HAL_CAN_LEGACY_MODULE_ENABLED
#define HAL_COMP_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_CRC_MODULE_ENABLED
#define HAL_CRYP_MODULE_ENABLED
#define HAL_DAC_MODULE_ENABLED
#define HAL_DFSDM_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_DCMI_MODULE_ENABLED
#define HAL_DMA2D_MODULE_ENABLED
#define HAL_FIREWALL_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_HASH_MODULE_ENABLED
#define HAL_HCD_MODULE_ENABLED
#define HAL_NAND_MODULE_ENABLED
#define HAL_NOR_MODULE_ENABLED
#define HAL_SRAM_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_IRDA_MODULE_ENABLED
#define HAL_IWDG_MODULE_ENABLED
#define HAL_LCD_MODULE_ENABLED
#define HAL_LPTIM_MODULE_ENABLED
#define HAL_OPAMP_MODULE_ENABLED
#define HAL_PCD_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_QSPI_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_RNG_MODULE_ENABLED
#define HAL_RTC_MODULE_ENABLED
#define HAL_SAI_MODULE_ENABLED
#define HAL_SD_MODULE_ENABLED
#define HAL_SMARTCARD_MODULE_ENABLED
#define HAL_SMBUS_MODULE_ENABLED
#define HAL_SPI_MODULE_ENABLED
#define HAL_SWPMI_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_TSC_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_USART_MODULE_ENABLED
#define HAL_WWDG_MODULE_ENABLED
*/


// IRQ Prority
#define	NVIC_PRIORITYGROUP_RTOS				NVIC_PRIORITYGROUP_4		// 16 приоритетов, без субприоритетов
#define	NVIC_PRIORITYGROUP					NVIC_PRIORITYGROUP_RTOS
#define	NVIC_SUBPRIORITY_DEFAULT			0
#define	NVIC_PRIORITY_LOWEST				0x0F
#define	NVIC_PRIORITY_HIGHEST				0x00
#define	NVIC_PRIORITY_RTOS_KERNEL			NVIC_PRIORITY_LOWEST
#define	NVIC_PRIORITY_RTOS_APIMAXCALL		0x05
#define	NVIC_PRIORITY_LOWEST_WITHIN_RTOS	( NVIC_PRIORITY_RTOS_KERNEL - 1 )
#define	NVIC_PRIORITY_HIGHEST_WITHIN_RTOS	( NVIC_PRIORITY_RTOS_APIMAXCALL )
#define	NVIC_PRIORITY_LOWEST_ABOVE_RTOS		( NVIC_PRIORITY_RTOS_APIMAXCALL - 1 )
#if ( NVIC_PRIORITY_LOWEST_ABOVE_RTOS < NVIC_PRIORITY_HIGHEST ) || ( NVIC_PRIORITY_LOWEST_WITHIN_RTOS < NVIC_PRIORITY_RTOS_APIMAXCALL )
#error
#endif

// System Tick IRQ (до запуска RTOS?)
#define	SYSTICK_IRQ_PREEMTPRIORITY			NVIC_PRIORITY_LOWEST
#define	SYSTICK_IRQ_SUBPRIORITY				NVIC_SUBPRIORITY_DEFAULT
#define	TICK_INT_PRIORITY					SYSTICK_IRQ_PREEMTPRIORITY		// tick interrupt priority
#define	SYSTICK_FRQ							( 1000 )						// при запуске RTOS может измениться!

// Использование Flash-памЯти
// Каждый сектор (страница) по 2кБ во всем диапазоне Flash памЯти
#define	FLASH_APPLICATION_BASE_NOBOOT			( ( uint32_t ) 0x08000000 )		// 0x08000000 - 0x0802FFFF
#define	FLASH_APPLICATION_BASE_BOOTOPENBLT		( ( uint32_t ) 0x08004000 )		// 
#define	FLASH_EEPROM_PAGE0					( ( uint32_t ) 0x08030000 )		// Bank2.Page256		2k (0x08080000 - 0x08080800)
// Количество занимаемых страниц Flash зависит от количества переменных (расчет через количество байт в блоке и количество блоков)
//#define	FLASH_EEPROM_PAGE0_END				( ( uint32_t ) 0x08080800 - 1 )	// Bank2.Page		2k (0x08080000 - 0x08080800)
//#define	FLASH_EEPROM_PAGE1					( ( uint32_t ) 0x08080800 )		// Bank2.Page260		2k (0x08080800 - 0x08081000)
//#define	FLASH_EEPROM_PAGE1_END				( ( uint32_t ) 0x08081000 - 1 )	// Bank2.Page		2k (0x08080800 - 0x08081000)
#define	FLASH_APPLICATION_BASE_DEFAULT		        FLASH_APPLICATION_BASE_NOBOOT	
#define	APPLICATION_BASE				FLASH_APPLICATION_BASE_DEFAULT
//#define	APPLICATION_BASE					FLASH_APPLICATION_BASE_BOOT   // Таблица векторов прерываний остаетсЯ по начальному адресу Boot
#define	APPLICATIONS						{ APPLICATION_BASE } //{ FLASH_APPLICATION_BASE_BOOT, APPLICATION_BASE }



// Резервирование места под дополнительные обработчики команд СКЛ
#define	SKLP_CALLBACKS_AUX_COUNT			20
// Скорость UART по-умолчанию
#define	SKLP_BAUD_RUS_REGUL_DEFAULT			115200		// [бод]	bare / RS-485

// Идентификаторы микропрограммы

#ifdef	PROJECT_CONFIG_H
#ifdef	PROJECT_NAME_MANCHESTER		// базоваЯ версиЯ
	#define SKLP_ADDRESS_MYSELF				0x96		// Адрес модуля на шине СКЛ (широковещательный)
//	#define SKLP_ADDRESS_CUSTOM 										// Адрес модуля на шине СКЛ зависит от джамперов
	#define	SKLP_DEVICE_TYPE_DEFAULT		0x6013						// Тип устройства длЯ команды [0x01]
	#define SKLP_DEVICE_TYPE				"Manchgester"					// Тип устройства, отображаемый в логе
	#define SKLP_SOFT_VERSION				( ( uint8_t ) 0x00 )
#endif	// PROJECT_NAME_RUS_PUMP
#endif	// PROJECT_CONFIG_H


#ifdef __ICCARM__

// Идентификаторы EEPROM
typedef enum EEPROM_ID_enum 
{
	EEPROM_ID_Serial = 1,
	EEPROM_PRESS_CALIBR,
	EEPROM_MOTOR_CALIBR,
	EEPROM_ID_TOTALNUMBER,
} EEPROM_ID_t;


#define UINT16MAX		(0xFFFF)


#endif // __ICCARM__

// Определить поведение при исключении - по умолчанию, перезагрузка
//#define	ASSERTION_HALT		// останов при исключении, исключить из релиза!!!

#define	DWT_TIMESTAMPTAGS_COUNT				8
#define	SKLP_DEVICE_SERIAL_DEFAULT	0//{ .aBytes = { 0x23, 0x35, 0x00, 0x00 } }	// LoochDeviceSerial_t
//#warning "ZATYCHKA SDELAT NORM NE PONYANTNO STRUCTURA ABytes"

#endif	// PROJECT_SOFT_CONFIG_H

