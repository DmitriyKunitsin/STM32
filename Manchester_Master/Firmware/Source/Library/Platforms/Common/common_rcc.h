#ifndef	COMMON_RCC_H
#define	COMMON_RCC_H

#include "Platform_common.h"	// COMMON_RCC_PLLCLK_DEFAULT

// Прототипы функций
void SystemClock_Config( void );			// Конфигурирование системных частот

// Использование таймера из модулЯ Data Watchpoint Trigger (DWT) длЯ генерации задержек и измерениЯ времени
typedef uint32_t DWT_Ticks_t;
typedef uint64_t DWT_TicksLong_t;

#define	DWT_US2TICKS( __US__ )		( ( DWT_Ticks_t ) ( ( __US__ ) * ( COMMON_RCC_PLLCLK_DEFAULT / 1.0e6f ) ) )
#define	DWT_NS2TICKS( __NS__ )		( ( DWT_Ticks_t ) ( ( __NS__ ) * ( COMMON_RCC_PLLCLK_DEFAULT / 1.0e9f ) ) )
#define	DWT_TICKS2US( _TICKS_ )		( ( uint32_t ) ( ( _TICKS_ ) * ( 1.0e6f / COMMON_RCC_PLLCLK_DEFAULT ) ) )
#define	DWT_TICKS2NS( _TICKS_ )		( ( uint32_t ) ( ( _TICKS_ ) * ( 1.0e9f / COMMON_RCC_PLLCLK_DEFAULT ) ) )

void DWT_TimerEnable( void );
DWT_Ticks_t DWT_TimerGet( void );			// Считывание системного таймера высокого разрешения для точных отметок времени
void DWT_TimerDelay( DWT_Ticks_t Ticks );
void DWT_TimerDelayUntil( DWT_Ticks_t * const pxPreviousWakeTime, DWT_Ticks_t xTimeIncrement );
void Delay_us( uint32_t us );					// Микросекундная задержка на основе системного таймера высокого разрешения
// Работа с "удлиненным" счетчиком
void DWT_TimerLongUpdate( void );		// Вычисление удлиненного счетчика, вызывать чаще периода переполнениЯ DWT!
DWT_TicksLong_t DWT_TimerGetLong( void );	// Считывание удлиненного счетчика

void DWT_AppendTimestampTag( char *pTag );

#endif	// COMMON_RCC_H

