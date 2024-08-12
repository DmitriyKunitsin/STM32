// RebootUtils.c
// �������, ��������� � �������� � �������������
// ����������� � ����� ������������ �������� ��������
#include "ProjectConfig.h"		// ������ ���������, ������ ������.
#include "stm32xxxx_hal.h"		// ����� ���������
#include "RebootUtils.h"		// ������
#include "SKLP_Service.h"		// SKLP_Time
#include <stdio.h>				// snprintf()
#include "common_rcc.h"			// SystemClock_Config( )
#include "common_gpio.h"		// SystemClock_Config( )
#include "platform_common.h"	// DEVSIGN_UID
#include "FreeRTOS.h"			// xTaskGetTickCount()
#include "Timers.h"				// TimerHandle_t
#include "Logger.h"

// ����������, ��� ������ � ������ ������������ assert() ��� Hard Fault - �������� ��� ���������������
#ifndef	ASSERTION_HALT					// ��������� ��� ������������ assert()
	#define	ASSERTION_RESET				// ��������������� ��� ������������ assert()
#else
	#warning "!!! � ���������� Assert/Hard Fault ���������� �������! ������ �� ������!!!"
#endif	// ASSERTION_HALT

#ifdef	ASSERTON_WRITE2UART				// ��� ������������ assert() ��������� ��������� � ���������������� �����
	#warning "!!! � ���������� Assert/Hard Fault ����� �������� ������� � SKL.UART! ������ �� ������!!!"
#endif	// ASSERTON_WRITE2UART

#ifndef	APPLICATIONS
	#warning "��������� ������������� APPLICATIONS � �������!"
	#define	APPLICATIONS	{ FLASH_APPLICATION_BASE_BOOT, APPLICATION_BASE, FLASH_APPLICATION_BASE_FTPSERVER }
#endif	// APPLICATIONS

// ������� �������� ����������
// 1. �������� SystemInitCallback() - ���������� �� Reset->SystemInit(),
// ���� __low_level_init() - ���������� �� Reset->__iar_program_start().
// - ������� �� ������ ����������, ���� ���������;
// - ������������� WDT;
// - ������������� ����������� � ������������ ���;
// - ������������� HAL;
// - ������������� HAL_IncTick();
// - ������������� DWT_Timer;
// 2. ������ �������� ���������, ����� SystemHardwareInit() ��� ������������� ������� ������.
// 
// 3. ���������� ����� SysTick->HAL_IncTick() �� ������� 1000 �� ��� �� ������� RTOS, ��� � �����.

// ����������, ����������� � RAM ��� ����������� ������������.
// ��������� __no_init, ����� �� �������� ���������� ��� ������������� ����������� ����������.
// ��������� � ����� ����� �������� RAM, ����� � ��� ������� �������� �� ����� ���������.
volatile __no_init ResetInfo_t ResetInfo @ ( SRAM_MAIN_END + 1 - sizeof( ResetInfo_t ) );		// ���������� �� �������������. !!�� ��������� ����������� ����� � CCM, �.�. ��� ������ �������� ������������ � ����, � ������ �� DMA �� �������� � CCM

// ****************************
// IWDG, ���������� ������
// ****************************
static __no_init IWDG_HandleTypeDef IwdgHandle;			// ������� ����������� ������� 

// ������������� ����������� ������� �� �������� 2..4 �
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

// ���������� ����������� �������
void WatchdogReset( void )
{
	#ifndef	IWDG_DISABLE
	assert_param( HAL_OK == HAL_IWDG_Refresh( &IwdgHandle ) );		// Reset IWDG
#endif	// IWDG_DISABLE
}

// **********************************************
// ������� �� ����������, ������������ �� ��������� �����
// **********************************************
static __noreturn void JumpToApplication( uint32_t ApplicationAddress )
{
	// ��������, ��� ����� ����� ��������� �� RAM
    assert_param( ( ( *( __IO uint32_t * ) ApplicationAddress ) & 0x2FFE0000 ) == 0x20000000 );
	// ���������������� ��������� �����
	__set_MSP( *( __IO uint32_t * ) ApplicationAddress );
	SCB->VTOR = ApplicationAddress;
	// ����� �������� - ������ Reset
	FunctionVoid_t pApplicationReset = ( FunctionVoid_t ) *( ( __IO uint32_t * ) ( ApplicationAddress + 4 ) );
	pApplicationReset( );
	while( 1 );
}

// ������� �� SystemInit(), ����� ��������� ���������� �� ������������� IAR.
// ����������� ������� �� ������ ����������, ���� ���������.
// ������������� WDT, HSE, PLL, SYSCLK, SysTick.
// !!! ����������� ���������� ��� �� ����������������, � ����� ���������������� �����!
void SystemInitCallback( void )
{
	ResetInfo.ResetStage = ResetStage_SystemInitStart;
	// �������� ������������ ��������� �������� ����
	// *********************************************
	// ���������, ��� �������� ��� ������������� �������� ���� �������
	assert_param( DEVSIGN_DEVID == DBGMCU_IDCODE_DEVID );	// �������� ����
#ifdef	TARGET_PACKAGE
	assert_param( TARGET_PACKAGE == DEVSIGN_PACKAGE );		// �������� �������
#endif	//TARGET_PACKAGE

	// ������������ ������� �� ������ ����������
	// *****************************************
	if( __HAL_RCC_GET_FLAG( RCC_FLAG_SFTRST ) )
	{	// �������� ����������� �����.
		// ��������, ���� ���� �������� ��������� ����� ��������� �� ����������/�������
		if( ResetInfoTag_JumpToApp == ResetInfo.Tag )
		{	// ����� ��� ���������� � ����� �������� ��������� � ������� �� ������������ ����������
			// ���������, ���� �� ����������� ���������� � ����� �����������
			const uint32_t aApplications[] = APPLICATIONS;
			for( int i = 0; i < SIZEOFARRAY( aApplications ); i++ )
				if( aApplications[i] == ResetInfo.xResetToApplication )
				{
#ifdef	FLASH_APPLICATION_BASE_BOOT
					if( FLASH_APPLICATION_BASE_BOOT == ResetInfo.xResetToApplication )
					{ 	// ��� �������� �� ���������, �������� ��� ����� ������������.
						// ��� ���������� ��� �������� ��������� ������� �������� �� �������� ��������������,
						// ��� ������ ��������� � �������� ���������� � ����� ������������� �� ����� ������� ������ ������.
						__HAL_RCC_CLEAR_RESET_FLAGS( );
					}
#endif	// FLASH_APPLICATION_BASE_BOOT
					ResetInfo.Tag = ResetInfoTag_Reboot;
					JumpToApplication( ResetInfo.xResetToApplication );
				}
			assert_param( 0 );
		}
		else if( ResetInfoTag_JumpToFunc == ResetInfo.Tag )
		{	// ����� ��� ���������� � ����� �������� ��������� � ������� �� ������������ �������
			ResetInfo.TagSaved = ResetInfo.Tag;
			ResetInfo.Tag = ResetInfoTag_Reboot;
			ResetInfo.ResetCounter++;
			( ( FunctionVoid_t ) ResetInfo.xResetToApplication ) ( );
		}
	}

	// ������������� WDT
	// *******************************
	// �������� ������������ ���� ��� �� �������������������, �� � IWDG ���� ����������� ������.
	// ����� ������������� ������������, �������������������� ������ �� ���������.
	// ������������� IWDG ������������ ������ ������������� ������������,
	// �.�. �������� ��� ������� ������������ ����� ������� ���������.
	WatchdogInit( );
	// IWDG �������, � ����� ������ ���� ���������� ������ � ���������� ������ MCU.
	
	// ************************
	// ������ ������������� HAL driver
	// ************************
	// ����� ����� ���������� ����� HAL_Init()->HAL_InitTick(),
	// ������ HAL_InitTick() ���������� ����������� ���������� SystemCoreClock � uwTickFreq,
	// ������� �� ������ ������ ��� �� ���������������� ��-���������.
	SystemCoreClockUpdate( );					// ���������������� SystemCoreClock
#if		defined( STM32L4 )
	extern uint32_t uwTickFreq;
	uwTickFreq = HAL_TICK_FREQ_DEFAULT;			// ���������������� uwTickFreq �� 1 ���
#elif	defined( STM32F4 )
	extern HAL_TickFreqTypeDef uwTickFreq;
	uwTickFreq = HAL_TICK_FREQ_DEFAULT;			// ���������������� uwTickFreq �� 1 ���
#else
#error "Select Target Family!"
#endif
	assert_param( HAL_OK == HAL_Init( ) );
	// ����� �������� � main(), SystemCoreClock � uwTickFreq ����� ���������������� ��-���������,
	// ��-����� ����������� ��������� ���������!

	// ������������� ����������� � ������������ ���
	// ********************************************
	// ������������� HSE, PLL, SYSCLK, HCLK, PCLK, SysTick.
	// ��� ������ HAL_RCC_OscConfig() � HAL_RCC_ClockConfig() �������� ���������,
	// ���� �� ������������� HAL_IncTick(). ������ �� ��, ��� ��� ����� ��������� ������ WDT.
	SystemClock_Config( );
	// � ���������� ������������ ����������������,
	// RTC, ���� ��� ������� �� ������ �� HSE, ��������� ������ �� ���������� �������,
	// SystemCoreClock ���������������,
	// SysTick ������� �� 1000 ��.
	// !!! ����� !!!
	// SystemCoreClock �������� ������� SYSCLK � �������� ���������� ����������.
	// ��-��������� ��������, � ������� ������ �� HSI ����� ������:
	// - System_stm32F4xx.c		uint32_t SystemCoreClock = 16000000;
	// - System_stm32L4xx.c		uint32_t SystemCoreClock = 4000000;
	// �����, ��� ������ HAL_RCC_ClockConfig(), ���������� ����������� �� ��������� ��������.
	// �� ����� �������� �� main(), ���������� ������������� ��������� ��-���������.
	// �������� �������� ��� __no_init, �� �� ����, ��� ��� ���������� ��������� ����������� �� ��������.
	// ��� ���������� ��������� �������� SystemCoreClockUpdate() ����� �������� �� main().

	// ������������� DWT_Timer
	// ����� �� ����� ������, �.�. ������������ ��� ������������� �� HSI ��� PLL.
	DWT_TimerEnable( );
	// ?? ������ ������ ����������, � �� �������������?
	// ?? ���� DWT ��� ������������ � BootLoader, �� ��� �������� ���������,
	// ?? ����� ��� ������ SystemInit() ����������, �� ��� ����� � ����� ������� ������� ������������ ����.
	DWT_AppendTimestampTag( "RCC Init" );

	// ����������, �������� WDT
	WatchdogReset( );
	ResetInfo.ResetStage = ResetStage_SystemInitFinish;
}

// **********************************************
// �����-�� ����� ���� ������������� ������������ � WDT,
// � ����� ����������� ������� �� ��������� ����������,
// �� ������ ������ ���������� � SystemInitCallback().
// �������� ��������� ������� ������.
// **********************************************
void SystemHardwareInit( void )
{
	ResetInfo.ResetStage = ResetStage_SystemHardwareInitStart;
	// ������ ������������� HAL driver
	// ************************
	// ����� ����� ���������� ����� HAL_Init()->HAL_InitTick(),
	// ������ HAL_InitTick() ���������� ����������� ���������� SystemCoreClock � uwTickFreq,
	// ������� �� ������ ������ �������������������� ��-���������.
	// �������� SystemCoreClock
	SystemCoreClockUpdate( );
	// ���������������� HAL driver
	assert_param( HAL_OK == HAL_Init( ) );

	// ������ ResetInfo.Tag - ����, ��������������� ����� ����������� �������
	switch( ResetInfo.Tag )
	{
	case ResetInfoTag_JumpToApp:	// ������������ � ����� �������� ��������� � ������� �� ������������ ����������
	case ResetInfoTag_JumpToFunc:	// ������������ � ����� �������� ��������� � ������� �� ������������ �������
		// ��� �������� ������ ���� ���� ���������� ��� � SystemInitCallback()
		assert_param( 0 );
		break;
		
	case ResetInfoTag_Assertion:	// ������������ � ���������� Assertion
	case ResetInfoTag_HardFault:	// ������������ � ���������� Hard Fault
	case ResetInfoTag_Reboot:		// ������������ � ���������� ������� ����������
		ResetInfo.TagSaved = ResetInfo.Tag;
		ResetInfo.Tag = ResetInfoTag_Clear;
		ResetInfo.ResetCounter++;
		break;
		
	default:						// ������ �����, ���������� ������������
		ResetInfo.aRebootMessage[0] = '\0';
//		ResetInfo.TagSaved = ResetInfoTag_Hardware;
//		ResetInfo.Tag = ResetInfoTag_Clear;
		ResetInfo.ResetCounter = 0;
		ResetInfo.RTC_SubsecondCorrection = 0;
	}

	// ����������� ����������, ��������� ����� - ���������, WDT, ����������� ������������
	char *apResetSources[] =
	// ���������� �����, � ������� ������������� ��������� ������������.
	// ����� ������������ �� ������� __HAL_RCC_CLEAR_RESET_FLAGS()
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

	// ������� ������� �� ������ ������ � �������� ������� ������������
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
		ResetInfo.ResetSource = ResetSource_Pin;		// ��� �������� ������ ������-�� ������������ ��� � Pin :-/
	__HAL_RCC_CLEAR_RESET_FLAGS( ); 	// �������� ��� ����� ������������

	// ������������ ��������� ������ � ������������� ���������� ������
	char aResetSourceText[25*4];		// ������, ���� ����� �������� ��� ��������� ������������ � ������� ���������� ������ ������
	int TextLenght = 0;
	for( int i = 0; i < SIZEOFARRAY( apResetSources ); i++ )
		if( NULL != apResetSources[i] )
			TextLenght += snprintf( aResetSourceText + TextLenght, sizeof( aResetSourceText ) - TextLenght, " %s;", apResetSources[i] );

	// �������� ����������������� ����� ����
	uint32_t const * const pU_ID = DEVSIGN_UID_BASE;
	
	// ������������ ��������� ����, ������� ����� ���� �������� �� SD ����� ������� ���������� ������
	TextLenght = snprintf( ( char * ) ResetInfo.aLogMessage, sizeof( ResetInfo.aLogMessage ),
		"\r\n[RESET]\r\nSource(s):%s\r\nNote: %s\r\nReset Counter: %d\r\nProgram %s rev.%s for %s build at %s %s on %s\r\n",
		aResetSourceText, ResetInfo.aRebootMessage, ResetInfo.ResetCounter,
		aBuildInfo_ProjName, aBuildInfo_SVN_Revision, aBuildInfo_ConfigName, aBuildInfo_Date, aBuildInfo_Time, aBuildInfo_ComputerName );
	ResetInfo.aRebootMessage[0] = '\0';
	if( ResetSource_Power == ResetInfo.ResetSource )
		TextLenght += snprintf( TextLenght + ( char * ) ResetInfo.aLogMessage, sizeof( ResetInfo.aLogMessage ) - TextLenght,
			"Chip ID: %08lX %08lX %08lX\r\n", pU_ID[2], pU_ID[1], pU_ID[0] );

/*	// ������������� SysTick �� 1000 �� ��� ����������� � SystemInitCallback()->SystemClock_Config()
	// � � SystemInitCallback()->HAL_Init().
	// ����� ����������� ������� ���������� ����������� ��� ������ SystemInitCallback()->HAL_Init().
	// �� ������ ������, �������������� ��������� ����� ����������� ���������� ��� RTOS:
	HAL_NVIC_SetPriorityGrouping( NVIC_PRIORITYGROUP_RTOS );
	// �� ������� ������������ RTOS, Systick ���������� �������� � ������,
	// ������������� ��� ������ SystemInitCallback().
	// ��� ������� ������������ RTOS, ������������ ��������� ������������� SysTick ������ RTOS.
	// ���������������, ��� SysTick ��� ���� ����� ������� � ��� �� ������, �� 1000 ��.
*/

	WatchdogReset( );
	ResetInfo.ResetStage = ResetStage_SystemHardwareInitFinish;
	ResetInfo.ResetStage = ResetStage_InitComplete;
}


// ***************************************************************************
// ***************************************************************************
// ��������� �������� ������������
// ***************************************************************************
// ***************************************************************************

#ifdef	ASSERTON_WRITE2UART
// ������� ��������� � ������������ � UART
// !! ������������ � ������� �������, ����� ��� ����������� ���������� �����!
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
// ��������� �������������� ������������
// ****************************************
static ASSERT_ATTRIBUTE /*__noreturn*/ void RebootUtils_Reset( void )
{
	// ��������� ��� ����������
	__disable_irq( );
	ResetInfo.ResetStage = ResetStage_SoftReset;
	
#ifdef	ASSERTON_WRITE2UART
	#warning "!!! ������ �� ������ !!!"
	// ������ ��� assert(), � ����� �������, ����������� ��������� ��������� � ������������ � UART.
	// !! ������������ � ������� �������, ����� ��� ����������� ���������� �����!
	if( ResetInfoTag_Assertion == ResetInfo.Tag )
		WriteRebootMessage2UART( );
#endif	// ASSERTON_WRITE2UART

	// ������� ��������-��������� _���������_ �������
	FaultCallback( );

#ifdef	ASSERTION_HALT	
	#warning "!!! ������ �� ������ !!!"
	// ������ ��� assert(), � ����� �������, ����������� �������.
	// !! ������������ � �������� ������� ���������!
	if( ResetInfoTag_Assertion == ResetInfo.Tag )
	{	// ������� �������������� ����������� ������ � ���������� ������� IWDG
		while( 1 )
			WatchdogReset( );
	}
#endif	// ASSERTION_HALT	

	// ��������� ����������� ������������
	NVIC_SystemReset( );
}

// �������� ������� ����� � ���� ��������� ������
// �� ������������ ���������� ������, �.�. ����� ��� ������ ����������� � �������� ����������
static char const *GetLocalTimeStr( void )
{
	return Time_SKLP2String( ( SKLP_Time6_t * ) &SKLP_Time.Time6 );
}

// ****************************************
// ������������ � ����������.
// ����� ����� �������� � ResetInfo, � ����������� ����� ������ � ��� (���� ����) ����� ������������.
// ****************************************
__noreturn void Reboot( const char *pNote )
{
	// ��������� ��� ����������
	__disable_irq( );
	// ��������� ���������� � ����������� ������������ � �������������� ������� ���
	ResetInfo.Tag = ResetInfoTag_Reboot;
	if( NULL == pNote )
		pNote = "Unknown";
	snprintf( ( char * ) ResetInfo.aRebootMessage, sizeof( ResetInfo.aRebootMessage ),
			"[%08X] %s Software reboot: %s", xTaskGetTickCount( ), GetLocalTimeStr( ), pNote );
	// ��������� ����������� ������������
	RebootUtils_Reset( );
#ifdef	ASSERT_USE_CALL_STACK
	while( 1 );
#endif
}

// **********************************************
// ��������� ������������ ����� �������� �������
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
// ������������ � ��������� �� ��������� ����������.
// ��������� ��������� � ����������� �������� � ���.
// ��������� ���������� � ����� ����� ����� � ResetInfo � ��������� ������������ (����� �������� ��� ���������)
// **********************************************
__noreturn void ResetToApplication( uint32_t xApplication )
{
	// �� ��������� ���������� �� ���������� ������������ ��������� � ���!
	// ��������� ���������� � ����������� ������������ � �������������� ������� ���
	snprintf( ( char * ) ResetInfo.aRebootMessage, sizeof( ResetInfo.aRebootMessage ),
			"Reset to Application at 0x%08lX", xApplication );
	// ������� ��������� � ��� �� SD (���� ����) � ��������� ���������� ������
	assert_param( Logger_WriteRecord( ( char * ) ResetInfo.aRebootMessage, LOGGER_FLAGS_WAITEFORCOMPLETE | LOGGER_FLAGS_APPENDTIMESTAMP ) );
	// ���������� ���������� ���������� � ����������� ������������ � �������������� ������� ���
	ResetInfo.Tag = ResetInfoTag_JumpToApp;
	ResetInfo.xResetToApplication = xApplication;
	// ��������� ����������� ������������
	RebootUtils_Reset( );
#ifdef	ASSERT_USE_CALL_STACK
	while( 1 );
#endif
}

// **********************************************
// ������������ � ����������� ��������� �� ��������� �������
// **********************************************
__noreturn void ResetToFunction( FunctionVoid_t xFunction )
{
	// ��������� ���������� � ����������� ������������ � �������������� ������� ���
	ResetInfo.Tag = ResetInfoTag_JumpToFunc;
	ResetInfo.xResetToApplication = ( uint32_t ) xFunction;
	// ��������� ����������� ������������
	RebootUtils_Reset( );
#ifdef	ASSERT_USE_CALL_STACK
	while( 1 );
#endif
}

// **********************************************
// ���������� assert()
// !!! ����� ����������� __noreturn, ����� ��� ��������� � assert() ��� �������,
// !!! � ����� ������� ���������� �������� assert()
// **********************************************
/*#ifdef	USE_FULL_ASSERT
ASSERT_ATTRIBUTE void assert_failed( const char *pFile, uint32_t Line, const char *pFunc )
{
	// ��������� ��� ����������
	__disable_irq( );
	// ��������� ���������� � ����������� ������������ � �������������� ������� ���
	ResetInfo.Tag = ResetInfoTag_Assertion;
	snprintf( ( char * ) ResetInfo.aRebootMessage, sizeof( ResetInfo.aRebootMessage ),
			"[%08X] %s Assertion failed: %s at %lu (%s).", xTaskGetTickCount( ), GetLocalTimeStr( ), pFile, Line, pFunc );
	// ��������� ����������� ������������
	RebootUtils_Reset( );
}
#endif	// USE_FULL_ASSERT*/

// **********************************************
// ���������� Hard Fault
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

	// ��������� ���������� � ����������� ������������ � �������������� ������� ���
	ResetInfo.Tag = ResetInfoTag_HardFault;
	snprintf( ( char * ) ResetInfo.aRebootMessage, sizeof( ResetInfo.aRebootMessage ),
//		"[%08X] %s Hard Fault!", xTaskGetTickCount( ), GetLocalTimeStr( ) );
		"[%08X] %s %s Fault! LR=%08X PC=%08X", xTaskGetTickCount( ), GetLocalTimeStr( ), pHardFaultReason, pFaultState->LR, pFaultState->PC );
	// ��������� ����������� ������������
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

	// ��������� �����������, ��� ������� �� ������� �� ���������
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

// ������� �� ����������� assert() � Hard Fault - ����� ���������� ����������� �������� (������� ������) � ������ ������
__weak void FaultCallback( void )
{
	// ����������� ����������� �������� � ��������-��������� �����
}


