// RebootUtils.c
// Утилиты, связанные с запуском и перезагрузкой
// Переработка с целью стабилизации процесса загрузки
#include "ProjectConfig.h"		// конфиг платформы, конфиг задачи.
#include "stm32xxxx_hal.h"		// дрова периферии
#include "RebootUtils.h"		// родной
#include "SKLP_Service.h"		// SKLP_Time
#include <stdio.h>				// snprintf()
#include "common_rcc.h"			// SystemClock_Config( )
#include "common_gpio.h"		// SystemClock_Config( )
#include "platform_common.h"	// DEVSIGN_UID
#include "FreeRTOS.h"			// xTaskGetTickCount()
#include "Timers.h"				// TimerHandle_t
#include "Logger.h"

// Определить, что делать в случае срабатывании assert() или Hard Fault - зависать или перезагружаться
#ifndef	ASSERTION_HALT					// зависнуть при срабатывании assert()
	#define	ASSERTION_RESET				// перезагрузиться при срабатывании assert()
#else
	#warning "!!! В результате Assert/Hard Fault происходит останов! УБРАТЬ ИЗ РЕЛИЗА!!!"
#endif	// ASSERTION_HALT

#ifdef	ASSERTON_WRITE2UART				// при срабатывании assert() отправить сообщение в последовательный канал
	#warning "!!! В результате Assert/Hard Fault будет передана посылка в SKL.UART! УБРАТЬ ИЗ РЕЛИЗА!!!"
#endif	// ASSERTON_WRITE2UART

#ifndef	APPLICATIONS
	#warning "Выполнить инициализацию APPLICATIONS в проекте!"
	#define	APPLICATIONS	{ FLASH_APPLICATION_BASE_BOOT, APPLICATION_BASE, FLASH_APPLICATION_BASE_FTPSERVER }
#endif	// APPLICATIONS

// ПорЯдок загрузки приложениЯ
// 1. Перехват SystemInitCallback() - вызываетсЯ из Reset->SystemInit(),
// либо __low_level_init() - вызываетсЯ из Reset->__iar_program_start().
// - Переход на другое приложение, если требуетсЯ;
// - ИнициализациЯ WDT;
// - ИнициализациЯ осциллЯтора и тактированиЯ шин;
// - ИнициализациЯ HAL;
// - ИнициализациЯ HAL_IncTick();
// - ИнициализациЯ DWT_Timer;
// 2. Работа основной программы, вызов SystemHardwareInit() длЯ идентификации причины сброса.
// 
// 3. Обеспечить вызов SysTick->HAL_IncTick() на частоте 1000 Гц как до запуска RTOS, так и после.

// ИнформациЯ, сохранЯемаЯ в RAM при программной перезагрузке.
// ОбъЯвлена __no_init, чтобы не потерЯть информацию при инициализации статических переменных.
// Размещена в самом конце основной RAM, чтобы в эту область случайно не залез загрузчик.
volatile __no_init ResetInfo_t ResetInfo @ ( SRAM_MAIN_END + 1 - sizeof( ResetInfo_t ) );		// Информация по перезагрузкам. !!Не размещать логгируемый текст в CCM, т.к. эта строка напрямую записывается в файл, а запись по DMA не работает с CCM

// ****************************
// IWDG, Сторожевой таймер
// ****************************
static __no_init IWDG_HandleTypeDef IwdgHandle;			// Хендлер сторожевого таймера 

// ИнициализациЯ сторожевого таймера на интервал 2..4 с
static void WatchdogInit( void )
{
	#ifndef	IWDG_DISABLE
	IwdgHandle.Instance			= IWDG;
	IwdgHandle.Init.Prescaler	= IWDG_PRESCALER_16;		// (32..64) eAo / 32 -> 1..2 eAo
	IwdgHandle.Init.Reload		= 0xFFF;					// 1..2 eAo / 4096 -> 0,25..0,5 Ao (2..4 n)
#ifdef	STM32L4
	// Ae? STM32L4xx i?aaoniio?ai ieiiiue ?a?ei. Anee na?in IWDG i?iecaiaeon? i?e ( Counter > Window ), oi i?ienoiaeo iaiaaeaiiue Reset.
	IwdgHandle.Init.Window		= IwdgHandle.Init.Reload;	// oaeei ia?acii, ieiiiue ?a?ei aoaao ioe??ai
#endif	// STM32L4
	// IWDG (Independed watchdog) Init
	assert_param( HAL_OK == HAL_IWDG_Init( &IwdgHandle ) );

#else
	#warning	"!!!IWDG ioee??ai! Oa?aou ec ?aeeca! OA?AOU EC ?AEECA!!!"
#endif	// IWDG_DISABLE
}

// Перезапуск сторожевого таймера
void WatchdogReset( void )
{
	#ifndef	IWDG_DISABLE
	assert_param( HAL_OK == HAL_IWDG_Refresh( &IwdgHandle ) );		// Reset IWDG
#endif	// IWDG_DISABLE
}

// **********************************************
// Переход на приложение, слинкованное на указанный адрес
// **********************************************
static __noreturn void JumpToApplication( uint32_t ApplicationAddress )
{
	// Проверка, что адрес стека указывает на RAM
    assert_param( ( ( *( __IO uint32_t * ) ApplicationAddress ) & 0x2FFE0000 ) == 0x20000000 );
	// Инициализировать указатель стека
	__set_MSP( *( __IO uint32_t * ) ApplicationAddress );
	SCB->VTOR = ApplicationAddress;
	// Адрес перехода - вектор Reset
	FunctionVoid_t pApplicationReset = ( FunctionVoid_t ) *( ( __IO uint32_t * ) ( ApplicationAddress + 4 ) );
	pApplicationReset( );
	while( 1 );
}

// Коллбек из SystemInit(), перед передачей управлениЯ на инициализацию IAR.
// Немедленный переход на другое приложение, если требуетсЯ.
// ИнициализациЯ WDT, HSE, PLL, SYSCLK, SysTick.
// !!! Статические переменные еще не инициализированы, и будут инициализированы позже!
void SystemInitCallback( void )
{
	ResetInfo.ResetStage = ResetStage_SystemInitStart;
	// Проверка соответствиЯ программы целевому чипу
	// *********************************************
	// Проверить, что реальный чип соответствует целевому чипу проекта
	assert_param( DEVSIGN_DEVID == DBGMCU_IDCODE_DEVID );	// проверка чипа
#ifdef	TARGET_PACKAGE
	assert_param( TARGET_PACKAGE == DEVSIGN_PACKAGE );		// проверка корпуса
#endif	//TARGET_PACKAGE

	// Опциональный переход на другое приложение
	// *****************************************
	if( __HAL_RCC_GET_FLAG( RCC_FLAG_SFTRST ) )
	{	// Выполнен программный сброс.
		// Возможно, цель была сбросить периферию перед переходом на приложение/функцию
		if( ResetInfoTag_JumpToApp == ResetInfo.Tag )
		{	// Сброс был произведен с целью сбросить периферию и перейти на определенное приложение
			// Проверить, есть ли запрошенное приложение в числе объЯвленных
			const uint32_t aApplications[] = APPLICATIONS;
			for( int i = 0; i < SIZEOFARRAY( aApplications ); i++ )
				if( aApplications[i] == ResetInfo.xResetToApplication )
				{
#ifdef	FLASH_APPLICATION_BASE_BOOT
					if( FLASH_APPLICATION_BASE_BOOT == ResetInfo.xResetToApplication )
					{ 	// При переходе на загрузчик, сбросить все флаги перезагрузок.
						// ДлЯ загрузчика это ЯвлЯетсЯ признаком прЯмого перехода из основной микропрограммы,
						// что должно приводить к переводу загрузчика в режим идентификации на времЯ порЯдка единиц секунд.
						__HAL_RCC_CLEAR_RESET_FLAGS( );
					}
#endif	// FLASH_APPLICATION_BASE_BOOT
					ResetInfo.Tag = ResetInfoTag_Reboot;
					JumpToApplication( ResetInfo.xResetToApplication );
				}
			assert_param( 0 );
		}
		else if( ResetInfoTag_JumpToFunc == ResetInfo.Tag )
		{	// Сброс был произведен с целью сбросить периферию и перейти на определенную функцию
			ResetInfo.TagSaved = ResetInfo.Tag;
			ResetInfo.Tag = ResetInfoTag_Reboot;
			ResetInfo.ResetCounter++;
			( ( FunctionVoid_t ) ResetInfo.xResetToApplication ) ( );
		}
	}

	// ИнициализациЯ WDT
	// *******************************
	// Основное тактирование чипа еще не проинициализировано, но у IWDG свой независимый таймер.
	// После инициализации тактированиЯ, переинициализировать таймер не требуетсЯ.
	// ИнициализациЯ IWDG производитсЯ раньше инициализации тактированиЯ,
	// т.к. проблемы при запуске осциллЯторов могут вызвать зависание.
	WatchdogInit( );
	// IWDG запущен, и может теперь быть остановлен только в результате сброса MCU.
	
	// ************************
	// ПерваЯ инициализациЯ HAL driver
	// ************************
	// Далее будет произведен вызов HAL_Init()->HAL_InitTick(),
	// причем HAL_InitTick() использует статические переменные SystemCoreClock и uwTickFreq,
	// которые на данный момент еще не инициализированы по-умолчанию.
	SystemCoreClockUpdate( );					// инициализировать SystemCoreClock
#if		defined( STM32L4 )
	extern uint32_t uwTickFreq;
	uwTickFreq = HAL_TICK_FREQ_DEFAULT;			// инициализировать uwTickFreq на 1 кГц
#elif	defined( STM32F4 )
	extern HAL_TickFreqTypeDef uwTickFreq;
	uwTickFreq = HAL_TICK_FREQ_DEFAULT;			// инициализировать uwTickFreq на 1 кГц
#else
#error "Select Target Family!"
#endif
	assert_param( HAL_OK == HAL_Init( ) );
	// После перехода в main(), SystemCoreClock и uwTickFreq будут инициализированы по-умолчанию,
	// по-этому потребуетсЯ повторить процедуру!

	// ИнициализациЯ осциллЯтора и тактированиЯ шин
	// ********************************************
	// ИнициализациЯ HSE, PLL, SYSCLK, HCLK, PCLK, SysTick.
	// При вызове HAL_RCC_OscConfig() и HAL_RCC_ClockConfig() возможно зависание,
	// если не функционирует HAL_IncTick(). Расчет на то, что при таких проблемах спасет WDT.
	SystemClock_Config( );
	// В результате тактирование инициализировано,
	// RTC, если был запущен до сброса от HSE, продолжил работу на правильной частоте,
	// SystemCoreClock инициализирован,
	// SysTick запущен на 1000 Гц.
	// !!! Важно !!!
	// SystemCoreClock содержит частоту SYSCLK и ЯвлЯетсЯ глобальной переменной.
	// По-умолчанию обЯвлена, в расчете работы от HSI после сброса:
	// - System_stm32F4xx.c		uint32_t SystemCoreClock = 16000000;
	// - System_stm32L4xx.c		uint32_t SystemCoreClock = 4000000;
	// Здесь, при вызове HAL_RCC_ClockConfig(), переменнаЯ обновлЯетсЯ до реального значениЯ.
	// Но после перехода на main(), происходит инициализациЯ значением по-умолчанию.
	// Возможно объЯвить как __no_init, но не факт, что это объЯвление правильно расползетсЯ по проектам.
	// ДлЯ надежности требуетсЯ вызывать SystemCoreClockUpdate() после перехода на main().

	// ИнициализациЯ DWT_Timer
	// Ранее не имело смысла, т.к. тактирование еще производилось от HSI без PLL.
	DWT_TimerEnable( );
	// ?? Почему именно разрешение, а не инициализациЯ?
	// ?? Если DWT был задействован в BootLoader, он там порЯдком насчитает,
	// ?? затем при вызове SystemInit() замедлитсЯ, но все равно к этому времени накопит произвольный счет.
	DWT_AppendTimestampTag( "RCC Init" );

	// Напоследок, сбросить WDT
	WatchdogReset( );
	ResetInfo.ResetStage = ResetStage_SystemInitFinish;
}

// **********************************************
// Когда-то здесь была инициализациЯ тактированиЯ и WDT,
// а также немедленный переход на требуемое приложение,
// но теперь этовсе перемещено в SystemInitCallback().
// Осталась обработка причины сброса.
// **********************************************
void SystemHardwareInit( void )
{
	ResetInfo.ResetStage = ResetStage_SystemHardwareInitStart;
	// ВтораЯ инициализациЯ HAL driver
	// ************************
	// Далее будет произведен вызов HAL_Init()->HAL_InitTick(),
	// причем HAL_InitTick() использует статические переменные SystemCoreClock и uwTickFreq,
	// которые на данный момент переинициализированы по-умолчанию.
	// Обновить SystemCoreClock
	SystemCoreClockUpdate( );
	// Инициализировать HAL driver
	assert_param( HAL_OK == HAL_Init( ) );

	// Анализ ResetInfo.Tag - поле, устанавливаемое перед программным сбросом
	switch( ResetInfo.Tag )
	{
	case ResetInfoTag_JumpToApp:	// перезагрузка с целью сбросить периферию и перейти на определенное приложение
	case ResetInfoTag_JumpToFunc:	// перезагрузка с целью сбросить периферию и перейти на определенную функцию
		// Эти признаки должны были быть обработаны еще в SystemInitCallback()
		assert_param( 0 );
		break;
		
	case ResetInfoTag_Assertion:	// перезагрузка в результате Assertion
	case ResetInfoTag_HardFault:	// перезагрузка в результате Hard Fault
	case ResetInfoTag_Reboot:		// перезагрузка в результате решениЯ приложениЯ
		ResetInfo.TagSaved = ResetInfo.Tag;
		ResetInfo.Tag = ResetInfoTag_Clear;
		ResetInfo.ResetCounter++;
		break;
		
	default:						// скорее всего, аппаратнаЯ перезагрузка
		ResetInfo.aRebootMessage[0] = '\0';
//		ResetInfo.TagSaved = ResetInfoTag_Hardware;
//		ResetInfo.Tag = ResetInfoTag_Clear;
		ResetInfo.ResetCounter = 0;
		ResetInfo.RTC_SubsecondCorrection = 0;
	}

	// Определение источников, вызвавших ресет - включение, WDT, программная перезагрузка
	char *apResetSources[] =
	// Обработать флаги, в которых накапливаются источники перезагрузок.
	// Флаги сбрасываютсЯ по команде __HAL_RCC_CLEAR_RESET_FLAGS()
	{
		( __HAL_RCC_GET_FLAG( RCC_FLAG_PINRST )		? "Pin"						: NULL ),
		( __HAL_RCC_GET_FLAG( RCC_FLAG_SFTRST )		? "Software"				: NULL ),
		( __HAL_RCC_GET_FLAG( RCC_FLAG_IWDGRST )	? "Independent Watchdog"	: NULL ),
		( __HAL_RCC_GET_FLAG( RCC_FLAG_WWDGRST )	? "Window Watchdog"			: NULL ),
		( __HAL_RCC_GET_FLAG( RCC_FLAG_LPWRRST )	? "Low Power"				: NULL ),
#if		defined( STM32F4 )
		( __HAL_RCC_GET_FLAG( RCC_FLAG_BORRST )		? "POR/PDR or BOR"			: NULL ),
		( __HAL_RCC_GET_FLAG( RCC_FLAG_PORRST )		? "POR/PDR"					: NULL ),
#elif	defined( STM32L4 )
		( __HAL_RCC_GET_FLAG( RCC_FLAG_BORRST )		? "POR/PDR or BOR"			: NULL ),
		( __HAL_RCC_GET_FLAG( RCC_FLAG_FWRST )		? "Firewall"				: NULL ),
		( __HAL_RCC_GET_FLAG( RCC_FLAG_OBLRST )		? "Option Byte Loader"		: NULL ),
#elif	defined( STM32F3 )
		( __HAL_RCC_GET_FLAG( RCC_FLAG_PORRST )		? "POR/PDR"					: NULL ),
		( __HAL_RCC_GET_FLAG( RCC_FLAG_OBLRST )		? "Option Byte Loader"		: NULL ),
#else
#error "Select Target Family!"
#endif
	};

	// Попытка перейти от набора флагов к истинной причине перезагрузки
	ResetInfo.ResetSource = ResetSource_Undefined;
	if( __HAL_RCC_GET_FLAG( RCC_FLAG_LPWRRST )
#if		defined( STM32F4 )
		|| __HAL_RCC_GET_FLAG( RCC_FLAG_BORRST ) || __HAL_RCC_GET_FLAG( RCC_FLAG_PORRST )
#elif	defined( STM32L4 )
		|| __HAL_RCC_GET_FLAG( RCC_FLAG_BORRST )
#elif	defined( STM32F3 )
		|| __HAL_RCC_GET_FLAG( RCC_FLAG_PORRST )
#endif
		)
		ResetInfo.ResetSource = ResetSource_Power;
	else if( __HAL_RCC_GET_FLAG( RCC_FLAG_IWDGRST ) || __HAL_RCC_GET_FLAG( RCC_FLAG_WWDGRST ) )
		ResetInfo.ResetSource = ResetSource_Watchdog;
	else if( __HAL_RCC_GET_FLAG( RCC_FLAG_SFTRST ) )
	{
		switch( ResetInfo.TagSaved )
		{
		case ResetInfoTag_Assertion:	ResetInfo.ResetSource = ResetSource_SoftAssertion;	break;
		case ResetInfoTag_HardFault:	ResetInfo.ResetSource = ResetSource_SoftFault;		break;
		case ResetInfoTag_Reboot:		ResetInfo.ResetSource = ResetSource_SoftReboot;		break;
		default:						ResetInfo.ResetSource = ResetSource_SoftUndefined;	break;
		}
	}
	else if( __HAL_RCC_GET_FLAG( RCC_FLAG_PINRST ) )
		ResetInfo.ResetSource = ResetSource_Pin;		// при софтовом ресете почему-то выставляется еще и Pin :-/
	__HAL_RCC_CLEAR_RESET_FLAGS( ); 	// сбросить все флаги перезагрузок

	// Формирование текстовой строки с перечислением источников сброса
	char aResetSourceText[25*4];		// строка, куда будут записаны все источники перезагрузок с момента последнего сброса флагов
	int TextLenght = 0;
	for( int i = 0; i < SIZEOFARRAY( apResetSources ); i++ )
		if( NULL != apResetSources[i] )
			TextLenght += snprintf( aResetSourceText + TextLenght, sizeof( aResetSourceText ) - TextLenght, " %s;", apResetSources[i] );

	// Получить идентификационный номер чипа
	uint32_t const * const pU_ID = DEVSIGN_UID_BASE;
	
	// Сформировать текстовый блок, который может быть сохранен на SD после запуска нормальной работы
	TextLenght = snprintf( ( char * ) ResetInfo.aLogMessage, sizeof( ResetInfo.aLogMessage ),
		"\r\n[RESET]\r\nSource(s):%s\r\nNote: %s\r\nReset Counter: %d\r\nProgram %s rev.%s for %s build at %s %s on %s\r\n",
		aResetSourceText, ResetInfo.aRebootMessage, ResetInfo.ResetCounter,
		aBuildInfo_ProjName, aBuildInfo_SVN_Revision, aBuildInfo_ConfigName, aBuildInfo_Date, aBuildInfo_Time, aBuildInfo_ComputerName );
	ResetInfo.aRebootMessage[0] = '\0';
	if( ResetSource_Power == ResetInfo.ResetSource )
		TextLenght += snprintf( TextLenght + ( char * ) ResetInfo.aLogMessage, sizeof( ResetInfo.aLogMessage ) - TextLenght,
			"Chip ID: %08lX %08lX %08lX\r\n", pU_ID[2], pU_ID[1], pU_ID[0] );

/*	// ИнициализациЯ SysTick на 1000 Гц уже произведена в SystemInitCallback()->SystemClock_Config()
	// и в SystemInitCallback()->HAL_Init().
	// Схема группировки вызовов прерываний установлена при вызове SystemInitCallback()->HAL_Init().
	// На всЯкий случай, продублировать установку схемы группировки прерываний длЯ RTOS:
	HAL_NVIC_SetPriorityGrouping( NVIC_PRIORITYGROUP_RTOS );
	// До запуска планировщика RTOS, Systick продолжает работать в режиме,
	// установленном при вызове SystemInitCallback().
	// При запуске планировщика RTOS, производитсЯ повторнаЯ инициализациЯ SysTick портом RTOS.
	// ПодразумеваетсЯ, что SysTick при этом будет запущен в том же режиме, на 1000 Гц.
*/

	WatchdogReset( );
	ResetInfo.ResetStage = ResetStage_SystemHardwareInitFinish;
	ResetInfo.ResetStage = ResetStage_InitComplete;
}


// ***************************************************************************
// ***************************************************************************
// Различные варианты перезагрузок
// ***************************************************************************
// ***************************************************************************

#ifdef	ASSERTON_WRITE2UART
// Скинуть сообщение о перезагрузке в UART
// !! Использовать в крайних случаЯх, когда нет возможности отладитьсЯ иначе!
#include "Common_UART.h"
static void WriteRebootMessage2UART( void )
{
	UART_Ext_HandleTypeDef *pUartExtHdl = &COM_SKLP_485_UART_EXT_HDL;
	HAL_UART_DMAStop( &pUartExtHdl->Common );
	HAL_GPIO_WritePin( pUartExtHdl->TXEN_GPIO, pUartExtHdl->TXEN_Pin, GPIO_PIN_SET );
//	GPIO_Common_Write( pUartExtHdl->iGPIO_TxEn, GPIO_PIN_SET );
	HAL_UART_Transmit( &pUartExtHdl->Common, ( uint8_t * ) ResetInfo.aRebootMessage, strlen( ( char * ) ResetInfo.aRebootMessage ), 100 );
	HAL_GPIO_WritePin( pUartExtHdl->TXEN_GPIO, pUartExtHdl->TXEN_Pin, GPIO_PIN_RESET );
//	GPIO_Common_Write( pUartExtHdl->iGPIO_TxEn, GPIO_PIN_RESET );
}
#endif	// ASSERTON_WRITE2UART

// ****************************************
// Выполнить принудительную перезагрузку
// ****************************************
static ASSERT_ATTRIBUTE /*__noreturn*/ void RebootUtils_Reset( void )
{
	// Запретить все прерываниЯ
	__disable_irq( );
	ResetInfo.ResetStage = ResetStage_SoftReset;
	
#ifdef	ASSERTON_WRITE2UART
	#warning "!!! УБРАТЬ ИЗ РЕЛИЗА !!!"
	// Только длЯ assert(), в целЯх отладки, допускаетсЯ скидывать сообщение о перезагрузке в UART.
	// !! Использовать в крайних случаЯх, когда нет возможности отладитьсЯ иначе!
	if( ResetInfoTag_Assertion == ResetInfo.Tag )
		WriteRebootMessage2UART( );
#endif	// ASSERTON_WRITE2UART

	// Вызвать проектно-зависимый _последний_ коллбек
	FaultCallback( );

#ifdef	ASSERTION_HALT	
	#warning "!!! УБРАТЬ ИЗ РЕЛИЗА !!!"
	// Только длЯ assert(), в целЯх отладки, допускаетсЯ останов.
	// !! Использовать в релизных версиЯх рисковано!
	if( ResetInfoTag_Assertion == ResetInfo.Tag )
	{	// Останов обеспечиваетсЯ бесконечным циклом с постоЯнным сбросом IWDG
		while( 1 )
			WatchdogReset( );
	}
#endif	// ASSERTION_HALT	

	// Выполнить программную перезагрузку
	NVIC_SystemReset( );
}

// Получить текущее времЯ в виде текстовой строки
// Не используетсЯ защищенный доступ, т.к. здесь все вызовы выполнЯютсЯ с запретом прерываний
static char const *GetLocalTimeStr( void )
{
	return Time_SKLP2String( ( SKLP_Time6_t * ) &SKLP_Time.Time6 );
}

// ****************************************
// Перезагрузка с пояснением.
// Текст будет сохранен в ResetInfo, и опционально будет скинут в лог (если есть) после перезагрузки.
// ****************************************
__noreturn void Reboot( const char *pNote )
{
	// Запретить все прерываниЯ
	__disable_irq( );
	// Сохранить информацию о программной перезагрузке в неповреждаемую область ОЗУ
	ResetInfo.Tag = ResetInfoTag_Reboot;
	if( NULL == pNote )
		pNote = "Unknown";
	snprintf( ( char * ) ResetInfo.aRebootMessage, sizeof( ResetInfo.aRebootMessage ),
			"[%08X] %s Software reboot: %s", xTaskGetTickCount( ), GetLocalTimeStr( ), pNote );
	// Выполнить программную перезагрузку
	RebootUtils_Reset( );
#ifdef	ASSERT_USE_CALL_STACK
	while( 1 );
#endif
}

// **********************************************
// Запросить перезагрузку через заданный таймаут
// **********************************************
static const char *pDelayedRebootNote = NULL;
static __noreturn void TimerRebootCallbackFunction( TimerHandle_t pTimer )
{
	Reboot( pDelayedRebootNote );
}

bool RebootDelayed( const float Delay_s, const char *pNote )
{
	static TimerHandle_t RebootTimerHandle = NULL;
	bool Result = false;
	do
	{
		if( RebootTimerHandle != NULL )
			break;
		if( pDelayedRebootNote != NULL )
			break;
		RebootTimerHandle = xTimerCreate( "RebootTmr", ( TickType_t ) ( Delay_s * configTICK_RATE_HZ ), pdTRUE, NULL, TimerRebootCallbackFunction );
		if( NULL == RebootTimerHandle )
			break;
		if( pdPASS != xTimerStart( RebootTimerHandle, 0 ) )
			break;
		Result = true;
		pDelayedRebootNote = pNote;

		char aMsg[80];
		snprintf( aMsg, sizeof( aMsg ), "Delayed for %f s reboot query: %s", Delay_s, pNote );
		assert_param( Logger_WriteRecord( aMsg, LOGGER_FLAGS_APPENDTIMESTAMP ) );
	} while( 0 );
	return Result;
}


// **********************************************
// Перезагрузка с переходом на выбранное приложение.
// Оставляет сообщение о планируемом переходе в лог.
// Оставляет информацию о новой точке входа в ResetInfo и выполняет перезагрузку (чтобы сбросить всю перефирию)
// **********************************************
__noreturn void ResetToApplication( uint32_t xApplication )
{
	// Не запрещать прерываниЯ до завершениЯ формированиЯ сообщениЯ в лог!
	// Сохранить информацию о программной перезагрузке в неповреждаемую область ОЗУ
	snprintf( ( char * ) ResetInfo.aRebootMessage, sizeof( ResetInfo.aRebootMessage ),
			"Reset to Application at 0x%08lX", xApplication );
	// Скинуть сообщение в лог на SD (если есть) и дождатьсЯ завершениЯ записи
	assert_param( Logger_WriteRecord( ( char * ) ResetInfo.aRebootMessage, LOGGER_FLAGS_WAITEFORCOMPLETE | LOGGER_FLAGS_APPENDTIMESTAMP ) );
	// Продолжить сохранение информации о программной перезагрузке в неповреждаемую область ОЗУ
	ResetInfo.Tag = ResetInfoTag_JumpToApp;
	ResetInfo.xResetToApplication = xApplication;
	// Выполнить программную перезагрузку
	RebootUtils_Reset( );
#ifdef	ASSERT_USE_CALL_STACK
	while( 1 );
#endif
}

// **********************************************
// Перезагрузка с последующим переходом на выбранную функцию
// **********************************************
__noreturn void ResetToFunction( FunctionVoid_t xFunction )
{
	// Сохранить информацию о программной перезагрузке в неповреждаемую область ОЗУ
	ResetInfo.Tag = ResetInfoTag_JumpToFunc;
	ResetInfo.xResetToApplication = ( uint32_t ) xFunction;
	// Выполнить программную перезагрузку
	RebootUtils_Reset( );
#ifdef	ASSERT_USE_CALL_STACK
	while( 1 );
#endif
}

// **********************************************
// Обработчик assert()
// !!! Убран модификатор __noreturn, чтобы при попадании в assert() при отладке,
// !!! в стеке вызовов сохранЯлсЯ источник assert()
// **********************************************
/*#ifdef	USE_FULL_ASSERT
ASSERT_ATTRIBUTE void assert_failed( const char *pFile, uint32_t Line, const char *pFunc )
{
	// Запретить все прерываниЯ
	__disable_irq( );
	// Сохранить информацию о программной перезагрузке в неповреждаемую область ОЗУ
	ResetInfo.Tag = ResetInfoTag_Assertion;
	snprintf( ( char * ) ResetInfo.aRebootMessage, sizeof( ResetInfo.aRebootMessage ),
			"[%08X] %s Assertion failed: %s at %lu (%s).", xTaskGetTickCount( ), GetLocalTimeStr( ), pFile, Line, pFunc );
	// Выполнить программную перезагрузку
	RebootUtils_Reset( );
}
#endif	// USE_FULL_ASSERT*/

// **********************************************
// Обработчик Hard Fault
// **********************************************
static const char *pHardFaultReason = NULL;
ASSERT_ATTRIBUTE /*__noreturn*/ void hard_fault_handler_c (unsigned int * hardfault_args)
{
	typedef struct FaultState_struct
	{
		uint32_t	R0;
		uint32_t	R1;
		uint32_t	R2;
		uint32_t	R3;
		uint32_t	R12;
		uint32_t	*LR;
		uint32_t	*PC;
		uint32_t	PSR;
	} FaultState_t;

	volatile FaultState_t *pFaultState = ( FaultState_t * ) hardfault_args;
	volatile SCB_Type *pSCB = SCB;

	( void ) pFaultState;
	( void ) pSCB;

	// Сохранить информацию о программной перезагрузке в неповреждаемую область ОЗУ
	ResetInfo.Tag = ResetInfoTag_HardFault;
	snprintf( ( char * ) ResetInfo.aRebootMessage, sizeof( ResetInfo.aRebootMessage ),
//		"[%08X] %s Hard Fault!", xTaskGetTickCount( ), GetLocalTimeStr( ) );
		"[%08X] %s %s Fault! LR=%08X PC=%08X", xTaskGetTickCount( ), GetLocalTimeStr( ), pHardFaultReason, pFaultState->LR, pFaultState->PC );
	// Выполнить программную перезагрузку
	RebootUtils_Reset( );
}

ASSERT_ATTRIBUTE /*__noreturn*/ void HardFault_CommonHandler(void)
{
	asm volatile
	(
		"TST LR, #4			\n"
		"ITE EQ				\n"
		"MRSEQ R0, MSP		\n"
		"MRSNE R0, PSP		\n"
		"B hard_fault_handler_c  "
	);

	// ОбъЯснить компилЯтору, что возврат из функции не требуетсЯ
	while( 1 );
}

ASSERT_ATTRIBUTE /*__noreturn*/ void HardFault_Handler( void )
{
	pHardFaultReason = "Hard";
	HardFault_CommonHandler( );
}

ASSERT_ATTRIBUTE /*__noreturn*/ void MemManage_Handler( void )
{
	pHardFaultReason = "MemManage";
	HardFault_CommonHandler( );
}

ASSERT_ATTRIBUTE /*__noreturn*/ void BusFault_Handler( void )
{
	pHardFaultReason = "Bus";
	HardFault_CommonHandler( );
}

ASSERT_ATTRIBUTE /*__noreturn*/ void UsageFault_Handler( void )
{
	pHardFaultReason = "Usage";
	HardFault_CommonHandler( );
}

ASSERT_ATTRIBUTE /*__noreturn*/ void FLASH_IRQHandler( void )
{
	pHardFaultReason = "FLASH";
	HardFault_CommonHandler( );
}

// Коллбек из обработчика assert() и Hard Fault - можно произвести необходимые действия (дернуть пинами) в случае отказа
__weak void FaultCallback( void )
{
	// Реализовать необходимые действия в проектно-зависимой части
}


