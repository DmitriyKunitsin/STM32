// main.c
// Инициализация задач и запуск операционки.
#include "ProjectConfig.h"		// конфиг платформы, конфиг задачи.
#include "stm32xxxx_hal.h"		// дрова периферии
#include "TaskConfig.h"			// конфигуратор задач RTOS
#include "common_gpio.h"		// GPIO
#include "RebootUtils.h"		// JumpToApplication(), SystemHardwareInit()
//#include <stdio.h>	
#include "common_uart.h"
#include "stm32xxxx_hal_uart.h"
#include "Stm32xxxx_hal_uart_ext.h"// snprintf()
#include "stm32l4xx_ll_tim.h"
#include "MemoryThread.h"		// MemoryThread_SprintfMutexGive()
#include "common_rcc.h"


// Группа системных событий
EventGroupHandle_t EventGroup_System;


// ОбъЯвление кучи в проекте как "__no_init", чтобы не тратить времЯ на автоматическую инициализацию нулЯми
// во времЯ SystemInit(), котораЯ происходит на низкой скорости Ядра.
// По-хорошему, нулЯми все же надо инициализировать после разгона Ядра, но пока нет.
#if	( configAPPLICATION_ALLOCATED_HEAP == 1 )
__no_init uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#endif /* configAPPLICATION_ALLOCATED_HEAP */


 void main( void )
{
	// Тактирование, DWT, IWDG уже инициализированы при вызове SystemInitCallback()
	// Завершение общей инициализации железа
	SystemHardwareInit( );
	// Проверить, что счетчик DWT завелсЯ
	assert_param( 0 != DWT_TimerGet( ) );
	// Место для вашего assert
//#warning "NIKITA!!!! HERE IS EEPROM!!!! RUNAWAY!!! TRANSLATE: НИКИТА!!!! ТУТ EEPROM!!!! БЕГИ!!!"
//	EE_initialize();

	// ИнициализациЯ пинов, объЯвленных через GPIO_Common_defs
	GPIO_Common_InitAll( );
	// Создание системной группы событий
	assert_param( NULL != ( EventGroup_System = xEventGroupCreate( ) ) );
	MemoryThread_SprintfMutexGive( );
	// Инициализация задач
	assert_param( SKLP_TaskInit( ) );		// Протокол СКЛ

	// Инициализация таймеров


        WatchdogReset( );
	
	vTaskStartScheduler( );

	assert_param( 0 );
}

// ?????????? ?????????? ?? ????? ?????????? ??? ??????? ????????
/*void ADS1231_EXTI_Callback( void )
{
	// Выставить флаг готовности внешнего АЦП к опросу
	assert_param( NULL != RUS_Regul_EventGroup );
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xEventGroupSetBitsFromISR( RUS_Regul_EventGroup, EVENTS_RUS_REGUL_ADS1231_READY, &xHigherPriorityTaskWoken );
}*/

// ***************************************
// Обработчик прерываниЯ SysTick
// ***************************************
// Инкремент тиков HAL
// Тактирование FreeRTOS

/*void HAL_COMP_TriggerCallback(COMP_HandleTypeDef *hcomp)
{
	static int counter;
	static int comp_value;
	counter++;
	comp_value = HAL_COMP_GetOutputLevel(hcomp);
}*/


void SysTick_Handler( void )
{
	// Инкрементировать тики, используемые библиотекой HAL.
	// После первоначальной инициализации периферии, SysTick инициализирован на 1000 Гц.
	// После запуска планировщика FreeRTOS, SysTick переинициализиуетсЯ на частоту configTICK_RATE_HZ.
	assert_param( 1000 == configTICK_RATE_HZ );		// Обычно configTICK_RATE_HZ == 1000, проверить соблюдение этой договоренности.
	HAL_IncTick( );
	HAL_SYSTICK_IRQHandler( );

	// Передать тактирование в FreeRTOS, если она уже запущена
	if( ( ResetStage_InitComplete == ResetInfo.ResetStage ) && ( xTaskGetSchedulerState( ) != taskSCHEDULER_NOT_STARTED ) )
	{
		// Порт FreeRTOS настроен так, xPortSysTickHandler() на самом деле заменЯет SysTick_Handler().
		// Чтобы контролировать работу SysTick до запуска FreeRTOS,
		// обработчик SysTick_Handler() реализован в приложении,
		// и после запуска FreeRTOS отсюда вызываетсЯ xPortSysTickHandler(), реализованный в порте.
		extern void xPortSysTickHandler( );		// FreeRTOS->Port->STM32F4xx->Port.c
		xPortSysTickHandler( );
	}
}

// ***************************************
// Коллбеки FreeRTOS
// ***************************************

// Перехват коллбека из задачи IdleTask
// - перевести CPU в сон до срабатываниЯ SysTick
// - сбросить WDT
void vApplicationIdleHook( void )
{
	static bool bIdleTaskInitComplete = false;
	if( !bIdleTaskInitComplete )
	{	// При первом вызове добавить в список отслеживаемых задач теневые задачи FreeRTOS,
		// которые инициализируютсЯ непосредственно перед запуском планировщика
		SysState_AppendTaskHandler( xTaskGetIdleTaskHandle( ) );			// Idle
		SysState_AppendTaskHandler( xTimerGetTimerDaemonTaskHandle( ) );	// Timer
		bIdleTaskInitComplete = true;
	}

	// !!! Важно!
	// !!! По спецификации, при переводе в Sleep прекращаетсЯ тактирование Ядра CPU.
	// !!! SysTick работает от того же тактированиЯ, и так же должен остановитьсЯ.
	// !!! Однако, у STM32 SysTick продолжает нормально работать в режиме Sleep,
	// !!! что делает возможным использовать его прерывание длЯ вывода Ядра из Sleep через каждый тик.
	HAL_PWR_EnterSLEEPMode( PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI );

	// Сброс WDT перенесен сюда из коллбека программного таймера,
	// т.к. при длительном зависании какой-либо задачи с приоритетом ниже таймера,
	// WDT продолжит нормально сбрасыватьсЯ.
	// Перенос сброса WDT в IdleTask() чреват обратным:
	// если задачи с приоритетом выше Idle настолько занЯты, что задача Idle вообще не работает,
	// в течении времени от 2 с - WDT может произвести сброс.
	// ПредполагаетсЯ, что на столько плотной загрузки нет и быть вообще не должно,
	// т.к. в этом случае не сможет нормально работать задача, обслуживающаЯ SD-карту.
	WatchdogReset( );
}

// Обработчик ошибок при переполнении стека задачи
void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
	assert_param( 0 );
}

// Обработчик ошибок при динамическом выделении памЯти
void vApplicationMallocFailedHook( void )
{
	assert_param( 0 );
}

// ************************************************
// Затычки
// ************************************************
//#include "eeprom_emul.h"
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

// Функции длЯ разграничениЯ доступа к snprintf()
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
	// Реализовать необходимые действия в проектно-зависимой части
}
#endif
#ifdef	USE_FULL_ASSERT
ASSERT_ATTRIBUTE void assert_failed( const char *pFile, uint32_t Line, const char *pFunc )
{
  static volatile int AssertFailedCnt = 0;
  AssertFailedCnt++;
}
#endif	// USE_FULL_ASSERT
