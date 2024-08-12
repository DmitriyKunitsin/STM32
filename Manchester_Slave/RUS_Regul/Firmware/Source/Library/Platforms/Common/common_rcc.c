#include "ProjectConfig.h"		// ������ ���������, ������ ������.
#include "stm32xxxx_hal.h"		// ����� ���������
#include "common_rcc.h"
#ifdef	USE_FREERTOS
#include "FreeRTOS.h"			// vTaskDelay()
#include "Task.h"				// xTaskGetSchedulerState()
#endif	// USE_FREERTOS

/** System Clock Configuration
*/
void SystemClock_Config( void )
{
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };;

	// Power interface clock enable
	__HAL_RCC_PWR_CLK_ENABLE( );

#if	defined( STM32F4 ) || defined( STM32L4 )
	// Regulator voltage scaling output selection
	__HAL_PWR_VOLTAGESCALING_CONFIG( PWR_REGULATOR_VOLTAGE_SCALE );
#endif

//	__HAL_RCC_BACKUPRESET_FORCE( );
//	__HAL_RCC_BACKUPRESET_RELEASE( );

	// Initializes the RCC Oscillators
	// ���������, ��� ���� PLL �� ������������, � ��� ����� ��������������������
	HAL_RCC_GetOscConfig( &RCC_OscInitStruct );
	assert_param( RCC_PLL_OFF == RCC_OscInitStruct.PLL.PLLState );
	RCC_OscInitStruct.OscillatorType	= PROJECTCONFIG_CLOCK_RCC_OSCILLATORTYPE;
#ifdef	PROJECTCONFIG_CLOCK_USE_LSE
	RCC_OscInitStruct.OscillatorType	|= RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.LSEState			= RCC_LSE_ON;
#endif

	RCC_OscInitStruct.HSEState			= PROJECTCONFIG_CLOCK_RCC_HSESTATE;
	RCC_OscInitStruct.PLL.PLLState		= PROJECTCONFIG_CLOCK_RCC_PLL_STATE;
	RCC_OscInitStruct.PLL.PLLSource		= PROJECTCONFIG_CLOCK_RCC_PLL_SOURCE;
#ifdef	STM32F3
	RCC_OscInitStruct.PLL.PLLMUL		= PROJECTCONFIG_CLOCK_RCC_PLL_MUL;
#else	// STM32F4, STM32L4
	RCC_OscInitStruct.PLL.PLLM			= PROJECTCONFIG_CLOCK_RCC_PLL_M;
	RCC_OscInitStruct.PLL.PLLN			= PROJECTCONFIG_CLOCK_RCC_PLL_N;
	RCC_OscInitStruct.PLL.PLLP			= PROJECTCONFIG_CLOCK_RCC_PLL_P;
	RCC_OscInitStruct.PLL.PLLQ			= PROJECTCONFIG_CLOCK_RCC_PLL_Q;
#ifdef	PROJECTCONFIG_CLOCK_RCC_PLL_R
	RCC_OscInitStruct.PLL.PLLR			= PROJECTCONFIG_CLOCK_RCC_PLL_R;
#endif	// PROJECTCONFIG_CLOCK_RCC_PLL_R
#endif	// STM32F3
	assert_param( HAL_OK == HAL_RCC_OscConfig( &RCC_OscInitStruct ) );

	// Initializes the CPU, AHB and APB busses clocks
	RCC_ClkInitStruct.ClockType			= RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_HCLK;
	RCC_ClkInitStruct.SYSCLKSource		= PROJECTCONFIG_CLOCK_SYSCLK_SOURCE;
	RCC_ClkInitStruct.AHBCLKDivider		= PROJECTCONFIG_CLOCK_AHB_DIVIDER;
	RCC_ClkInitStruct.APB1CLKDivider	= PROJECTCONFIG_CLOCK_APB1_DIVIDER;
	RCC_ClkInitStruct.APB2CLKDivider	= PROJECTCONFIG_CLOCK_APB2_DIVIDER;
	uint32_t FlashLatency = COMMON_RCC_PICK_FLASH_LATENCY( PROJECTCONFIG_CLOCK_FLASH_SLOWDOWN * COMMON_RCC_PLLCLK_DEFAULT ); 	// PROJECTCONFIG_CLOCK_FLASH_SLOWDOWN - ������ ���������� �������� � ����������� (������-�������������) ������ ��� ���������� ����������� ��������
	assert_param( HAL_OK == HAL_RCC_ClockConfig( &RCC_ClkInitStruct, FlashLatency ) );
	
/*	// STM32F405x/407x/415x/417x Revision Z devices: prefetch is supported
	if (HAL_GetREVID() == 0x1001)
	{
		// Enable the Flash prefetch
		__HAL_FLASH_PREFETCH_BUFFER_ENABLE();
	}
*/
}


// ***********************************
// ������������� ����������� ������� DWT ��� ��������� �����- � �������������� ��������.
// ���������� ������������ ��� �������� ��������, �.�. �� ����� �������� � ������������ ������, ����� ������������ ������ �����������.
// ����� ����� ��������� ��� �������� �� ���������� ��, � ��� ������� ����������� ��������� �������� ����������� vTaskDelay().
// ����� ������ ������ ��������� ��� ��������� ������� ��������� ������� ���� ���������.
// ��� ������� 168 ���, ������� ������������� �� 25.5 �, � ��� ��������� ������� ���������� ���������� ������������ ���������� �������,
// ������� ���������� ��������� � �������� �� ����� ��������� ������������ DWT.
// ***********************************

// ��������� ������ �������
static void DWT_TimerInit( void )
{
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; 	// *SCB_DEMCR = *SCB_DEMCR | 0x01000000;
	DWT->CYCCNT = 0;									// *DWT_CYCCNT = 0; // reset the counter
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;				// *DWT_CONTROL = *DWT_CONTROL | 1 ; // enable the counter
}

void DWT_TimerEnable( void )
{
	// !!! ��� ���������� ��������� ��-�������, ����� ���� ���������� ������ ������� ����,
	// !!! ��� ���� ��������� ������ ������ DWT!
	if( ( 0 == ( CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk ) ) ||
		( 0 == ( DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk ) ) )
		DWT_TimerInit( );
}

DWT_Ticks_t DWT_TimerGet( void )
{
	return DWT->CYCCNT;
}

// ��������� ��������. �������� ���������� ��� ����������� � ������������� �� ������ ������!
void DWT_TimerDelay( DWT_Ticks_t Ticks )
{
	DWT_Ticks_t TickStart = DWT->CYCCNT;
	while( ( DWT->CYCCNT - TickStart ) < Ticks );
}

// ��������� �������� �� ���������� �������.
// *pxPreviousWakeTime	- ������� �������, �� �������� ����������� ��������.
//						���� �� ������������ ������� ������� ���������� ��������.
// xTimeIncrement		- ��������� �������� �� �������
void DWT_TimerDelayUntil( DWT_Ticks_t * const pxPreviousWakeTime, DWT_Ticks_t xTimeIncrement )
{
	while( 1 )
	{
		DWT_Ticks_t TicksCurrent = DWT->CYCCNT;
		if( ( TicksCurrent - *pxPreviousWakeTime ) >= xTimeIncrement )
		{
			*pxPreviousWakeTime = TicksCurrent;
			break;
		}
	}
}

// ������ � "����������" ���������
static DWT_TicksLong_t DWT_CounterLong = 0;

// ���������� ����������� ��������
DWT_TicksLong_t DWT_TimerGetLong( void )
{
	DWT_TicksLong_t Result;
	ATOMIC_WRITE( Result, DWT_CounterLong );
	return Result;
}

// ���������� ����������� ��������, �������� �� ���� ������� ������������ DWT!
void DWT_TimerLongUpdate( void )
{
	static DWT_Ticks_t DWT_CounterPrev = 0;
	ENTER_CRITICAL_SECTION( );
	DWT_Ticks_t DWT_CounterNew = DWT_TimerGet( );
	DWT_CounterLong += DWT_CounterNew - DWT_CounterPrev;
	DWT_CounterPrev = DWT_CounterNew;
	EXIT_CRITICAL_SECTION( );
}


void Delay_us( uint32_t us )
{
	DWT_TimerEnable( );
#if	( ( COMMON_RCC_PLLCLK_DEFAULT / 1000000 ) < 10 )
#warning "�� ����� ��������� ���� �������� ����� ������ ������������ �����������!"
#endif
//#warning "�� ����� ��������� ���� �������� ����� ������ ������������ �����������!"
/* �������������� ������ ������, ����� �������� �� �������� �� ������� ����
   � ����� ��������� �������� ������������ �������� (������� ��).
   ��� ������� ������� ��������� �������� ���������.
*/
	DWT_TimerDelay( DWT_US2TICKS( us ) );
}

// ������ ���������� HAL_Delay()
void HAL_Delay( __IO uint32_t Delay_ms )
{
#ifdef	USE_FREERTOS
	if( taskSCHEDULER_RUNNING == xTaskGetSchedulerState( ) )
	{	// ����������� ��������. ������� ������������ ���������� ��������
		vTaskDelay( pdMS_TO_TICKS( Delay_ms ) + 1 );	// �.�. ��� ������� �������� �� N ����� �������� �������� ����� ���� �� N-1 �� N, ��������� ����� ��������� �����
	}
	else
	{	// ����������� �� ��������, ��� ����������� ����������.
		Delay_us( Delay_ms * 1000 );
/*		if( Delay_ms < 5 )
		{	// ��� ����� �������� ������������ DWT, ����� ������ �������� ��������� ������� �������
			Delay_us( Delay_ms * 1000 );
		}
		else
		{	// ��������� ���������� HAL_Delay(). ���������, ����� ���-�� (������ ��� SysTick_Handler()) ������� HAL_IncTick() �� ������� 100 ��
			uint32_t TickStart = HAL_GetTick( );
			while( ( HAL_GetTick( ) - TickStart ) < ( Delay_ms + 1 ) );
		}
*/
	}
#else
	Delay_us( Delay_ms * 1000 );
#endif	// USE_FREERTOS

}


#ifndef	DWT_TIMESTAMPTAGS_COUNT
void DWT_AppendTimestampTag( char *pTag )	{}

#else
#include <string.h>

typedef struct DWT_TimestampTag_struct
{
	DWT_Ticks_t		Timestamp;
	char			aTag[12];
} DWT_TimestampTag_t ;

static DWT_TimestampTag_t aDWT_TimestampTags[DWT_TIMESTAMPTAGS_COUNT] = { 0 };
static int DWT_TimestampTagsCount = 0;

void DWT_AppendTimestampTag( char *pTag )
{
	if( DWT_TimestampTagsCount >= SIZEOFARRAY( aDWT_TimestampTags ) )
		return;
	DWT_TimestampTag_t * const pTimestampTag = &aDWT_TimestampTags[DWT_TimestampTagsCount];
	pTimestampTag->Timestamp = DWT_TimerGet( );
	if( NULL != pTag )
		strncpy( pTimestampTag->aTag, pTag, sizeof( pTimestampTag->aTag ) );
	DWT_TimestampTagsCount++;
}
#endif	// DWT_TIMESTAMPTAGS_COUNT
