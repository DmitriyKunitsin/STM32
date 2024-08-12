// main.c
// ������������� ����� � ������ �����������.
#include "ProjectConfig.h"		// ������ ���������, ������ ������.
#include "stm32xxxx_hal.h"		// ����� ���������
#include "TaskConfig.h"			// ������������ ����� RTOS
#include "common_gpio.h"		// GPIO
#include "RebootUtils.h"		// JumpToApplication(), SystemHardwareInit()
//#include <stdio.h>	
#include "common_uart.h"
#include "stm32xxxx_hal_uart.h"
#include "Stm32xxxx_hal_uart_ext.h"// snprintf()
#include "stm32l4xx_hal_opamp.h"
#include "stm32l4xx_hal_comp.h"
#include "stm32l4xx_hal_dac.h"
#include "stm32l4xx_ll_tim.h"
#include "MemoryThread.h"		// MemoryThread_SprintfMutexGive()
#include "common_rcc.h"
#include "main.h"
#include "DRIVER_CM36-3650.h"
#include "DRIVER_Motor.h"
#include "Driver_ADS1231.h"
#include "RUS_Regul_ADC.h"
#include "RUS_Regul_Main.h"
#include "common_dac.h"
#include "RUS_Regul_Events.h"

extern RUS_Regul_t RUS_Regul;
extern Motor_Main_t Motor;

// ������ ��������� �������
EventGroupHandle_t EventGroup_System;


// ���������� ���� � ������� ��� "__no_init", ����� �� ������� ����� �� �������������� ������������� ������
// �� ����� SystemInit(), ������� ���������� �� ������ �������� ����.
// ��-��������, ������ ��� �� ���� ���������������� ����� ������� ����, �� ���� ���.
#if	( configAPPLICATION_ALLOCATED_HEAP == 1 )
__no_init uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#endif /* configAPPLICATION_ALLOCATED_HEAP */


 void main( void )
{
	// ������������, DWT, IWDG ��� ���������������� ��� ������ SystemInitCallback()
	// ���������� ����� ������������� ������
	SystemHardwareInit( );
	// ���������, ��� ������� DWT �������
	assert_param( 0 != DWT_TimerGet( ) );
	// ����� ��� ������ assert
#warning "NIKITA!!!! HERE IS EEPROM!!!! RUNAWAY!!! TRANSLATE: ������!!!! ��� EEPROM!!!! ����!!!"
	EE_initialize();

	// ������������� �����, ����������� ����� GPIO_Common_defs
	GPIO_Common_InitAll( );
	// �������� ��������� ������ �������
	assert_param( NULL != ( EventGroup_System = xEventGroupCreate( ) ) );
	MemoryThread_SprintfMutexGive( );
	// ������������� �����
	assert_param( SKLP_TaskInit( ) );		// �������� ���
	assert_param( RUS_Regul_TaskInit() );
	Motor_SpeedControl_Init();

	// ������������� ��������
	Motor_Calibration_Current_Create();
	Motor_Calibration_Press_Create();
	Motor_Stabilization_Press_Create();
	RUS_Regul_OPAMP_Timer_Create();
	
	GPIO_Common_Write(iGPIO_ADC_PWDWN, GPIO_PIN_SET);
	OPAMP_Init();

	//ADS1231_DataRead(&RUS.Data.Press);

	/*uint8_t a[] = "Hello!";
	while(1)
		{
			HAL_UART_Transmit((UART_HandleTypeDef*)(&COM_SKLP_AUX_UART_EXT_HDL), a, sizeof(a), 100);
			HAL_Delay(1000);
		}*/

	// ������ ������������


        WatchdogReset( );

	
	vTaskStartScheduler( );

	assert_param( 0 );
}

// ?????????? ?????????? ?? ????? ?????????? ??? ??????? ????????
void ADS1231_EXTI_Callback( void )
{
	// ��������� ���� ���������� �������� ��� � ������
	assert_param( NULL != RUS_Regul_EventGroup );
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xEventGroupSetBitsFromISR( RUS_Regul_EventGroup, EVENTS_RUS_REGUL_ADS1231_READY, &xHigherPriorityTaskWoken );
}

// ***************************************
// ���������� ���������� SysTick
// ***************************************
// ��������� ����� HAL
// ������������ FreeRTOS

/*void HAL_COMP_TriggerCallback(COMP_HandleTypeDef *hcomp)
{
	static int counter;
	static int comp_value;
	counter++;
	comp_value = HAL_COMP_GetOutputLevel(hcomp);
}*/


void SysTick_Handler( void )
{
	// ���������������� ����, ������������ ����������� HAL.
	// ����� �������������� ������������� ���������, SysTick ��������������� �� 1000 ��.
	// ����� ������� ������������ FreeRTOS, SysTick ������������������� �� ������� configTICK_RATE_HZ.
	assert_param( 1000 == configTICK_RATE_HZ );		// ������ configTICK_RATE_HZ == 1000, ��������� ���������� ���� ��������������.
	HAL_IncTick( );
	HAL_SYSTICK_IRQHandler( );

	// �������� ������������ � FreeRTOS, ���� ��� ��� ��������
	if( ( ResetStage_InitComplete == ResetInfo.ResetStage ) && ( xTaskGetSchedulerState( ) != taskSCHEDULER_NOT_STARTED ) )
	{
		// ���� FreeRTOS �������� ���, xPortSysTickHandler() �� ����� ���� �������� SysTick_Handler().
		// ����� �������������� ������ SysTick �� ������� FreeRTOS,
		// ���������� SysTick_Handler() ���������� � ����������,
		// � ����� ������� FreeRTOS ������ ���������� xPortSysTickHandler(), ������������� � �����.
		extern void xPortSysTickHandler( );		// FreeRTOS->Port->STM32F4xx->Port.c
		xPortSysTickHandler( );
	}
}

// ***************************************
// �������� FreeRTOS
// ***************************************

// �������� �������� �� ������ IdleTask
// - ��������� CPU � ��� �� ������������ SysTick
// - �������� WDT
void vApplicationIdleHook( void )
{
	static bool bIdleTaskInitComplete = false;
	if( !bIdleTaskInitComplete )
	{	// ��� ������ ������ �������� � ������ ������������� ����� ������� ������ FreeRTOS,
		// ������� ���������������� ��������������� ����� �������� ������������
		SysState_AppendTaskHandler( xTaskGetIdleTaskHandle( ) );			// Idle
		SysState_AppendTaskHandler( xTimerGetTimerDaemonTaskHandle( ) );	// Timer
		bIdleTaskInitComplete = true;
	}

	// !!! �����!
	// !!! �� ������������, ��� �������� � Sleep ������������ ������������ ���� CPU.
	// !!! SysTick �������� �� ���� �� ������������, � ��� �� ������ ������������.
	// !!! ������, � STM32 SysTick ���������� ��������� �������� � ������ Sleep,
	// !!! ��� ������ ��������� ������������ ��� ���������� ��� ������ ���� �� Sleep ����� ������ ���.
	HAL_PWR_EnterSLEEPMode( PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI );

	// ����� WDT ��������� ���� �� �������� ������������ �������,
	// �.�. ��� ���������� ��������� �����-���� ������ � ����������� ���� �������,
	// WDT ��������� ��������� ������������.
	// ������� ������ WDT � IdleTask() ������ ��������:
	// ���� ������ � ����������� ���� Idle ��������� ������, ��� ������ Idle ������ �� ��������,
	// � ������� ������� �� 2 � - WDT ����� ���������� �����.
	// ��������������, ��� �� ������� ������� �������� ��� � ���� ������ �� ������,
	// �.�. � ���� ������ �� ������ ��������� �������� ������, ������������� SD-�����.
	WatchdogReset( );
}

// ���������� ������ ��� ������������ ����� ������
void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
	assert_param( 0 );
}

// ���������� ������ ��� ������������ ��������� ������
void vApplicationMallocFailedHook( void )
{
	assert_param( 0 );
}

// ************************************************
// �������
// ************************************************
#include "eeprom_emul.h"
/*uint16_t EE_Init( void )
{
	return HAL_OK;
}*/

#if 0
uint16_t EE_ReadVariable(uint16_t VirtAddress, void *Data, uint16_t Size)
{
	return EEPROM_READ_FAIL;
}

uint16_t EE_WriteVariable(uint16_t VirtAddress, void *Data, uint16_t Size)
{
	return EEPROM_READ_FAIL;
}
#endif

bool Logger_WriteRecord( char const *pText, uint32_t Flags )
{
	return true;
}

bool SKLP_BlackBox_WriteRecord( char const *pText )
{
	return true;
}

void SysState_AppendTaskHandler( TaskHandle_t xTaskHandle )
{
	return;
}

// ������� ��� ������������� ������� � snprintf()
void MemoryThread_SprintfMutexGive( void )
{
	assert_param( NULL != EventGroup_System );
	( void ) xEventGroupSetBits( EventGroup_System, EVENTSYSTEM_SPRINTF_READY );
}

bool MemoryThread_SprintfMutexTake( TickType_t xTicksToWait )
{
	assert_param( NULL != EventGroup_System );
	const EventBits_t EventToWait = EVENTSYSTEM_SPRINTF_READY;
	return ( EventToWait & xEventGroupWaitBits( EventGroup_System, EventToWait, pdTRUE, pdTRUE, xTicksToWait ) );
}

#if 1
void FaultCallback( void )
{
  static volatile int FaultCallbackCnt = 0;
  FaultCallbackCnt++;
	// ����������� ����������� �������� � ��������-��������� �����
}
#endif
#ifdef	USE_FULL_ASSERT
ASSERT_ATTRIBUTE void assert_failed( const char *pFile, uint32_t Line, const char *pFunc )
{
  static volatile int AssertFailedCnt = 0;
  AssertFailedCnt++;
}
#endif	// USE_FULL_ASSERT
