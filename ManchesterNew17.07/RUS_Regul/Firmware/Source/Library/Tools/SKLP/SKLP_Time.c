// SKLP_Time.c
// Форматы времени и утилиты
#include "ProjectConfig.h"		// конфиг платформы, конфиг задачи.
#include "stm32xxxx_hal.h"		// дрова периферии
#include "SKLP_Time.h"
#include <string.h>				// strlen{}

// Проверка допустимости формата времени
// Количество дней по месЯцам не контролируется
bool SKLP_Time6Validate( SKLP_Time6_t *pSKLP_Time6 )
{
	assert_param( NULL != pSKLP_Time6 );

	bool Result = false;
	do
	{
		if( pSKLP_Time6->YearFrom2000 > 50 )
			break;
		if( ( pSKLP_Time6->Month < 1  ) || ( pSKLP_Time6->Month > 12  ) )
			break;
		if( ( pSKLP_Time6->Day < 1  ) || ( pSKLP_Time6->Day > 31  ) )
			break;
		if( pSKLP_Time6->Hour > 23  )
			break;
		if( pSKLP_Time6->Minute > 59  )
			break;
		if( pSKLP_Time6->Second > 59  )
			break;
		Result = true;
	} while( 0 );

	return Result;
}

bool SKLP_TimeValidate( SKLP_Time_t *pSKLP_Time )
{
	if( SKLP_Time6Validate( &pSKLP_Time->Time6 ) )
		if( pSKLP_Time->SecondHundr <= 99 )
			return true;
	return false;
}

// ******************************
// Функции преобразования времени
time_t Time_SKLP2Time( SKLP_Time6_t *pSKLP_Time6 )
{
	if( !SKLP_Time6Validate( pSKLP_Time6 ) )
		return -1;
	
	struct tm Time;
	Time.tm_year	= ( uint16_t ) ( pSKLP_Time6->YearFrom2000 ) + ( 2000 - 1900 );
	Time.tm_mon 	= pSKLP_Time6->Month - 1;
	Time.tm_mday	= pSKLP_Time6->Day;
	Time.tm_hour	= pSKLP_Time6->Hour;
	Time.tm_min 	= pSKLP_Time6->Minute;
	Time.tm_sec 	= pSKLP_Time6->Second;
	Time.tm_isdst	= 0;	// не смещать на час

	return mktime( &Time );
}

SKLP_Time6_t Time_Time2SKLP( time_t Time )
{
	struct tm TM;
#warning "Возможны проблемы при параллельном вызове! Закрыть критической секцией."
//	taskENTER_CRITICAL( );
	TM = *localtime( &Time );
//	taskEXIT_CRITICAL( );
	
	SKLP_Time6_t Result;
	Result.YearFrom2000	= TM.tm_year - ( 2000 - 1900 );
	Result.Month		= TM.tm_mon + 1;
	Result.Day			= TM.tm_mday;
	Result.Hour 		= TM.tm_hour;
	Result.Minute		= TM.tm_min;
	Result.Second		= TM.tm_sec;
	return Result;
}

char const *Time_Time2String( time_t Time )
{
	if( ( 0 != Time ) && ( -1 != Time ) )
	{
		char *pTimeString = ctime( &Time );
		pTimeString[ strlen( pTimeString ) - 1 ] = '\0';
		return pTimeString;
	}
	else
		return "<Unknown time>";
}

char const *Time_SKLP2String( SKLP_Time6_t *pSKLP_Time6 )
{
	return Time_Time2String( Time_SKLP2Time( pSKLP_Time6 ) );
}

DosDateTime_t Time_SKLP2DOS( SKLP_Time6_t *pSKLP_Time6 )
{
	if( !SKLP_Time6Validate( pSKLP_Time6 ) )
		return ( DosDateTime_t ) { 0 };

	DosDateTime_t DosDateTime;
	DosDateTime.Second2 = pSKLP_Time6->Second / 2;
	DosDateTime.Minute	= pSKLP_Time6->Minute;
	DosDateTime.Hour	= pSKLP_Time6->Hour;
	DosDateTime.Day 	= pSKLP_Time6->Day;
	DosDateTime.Month	= pSKLP_Time6->Month;
	DosDateTime.Year	= pSKLP_Time6->YearFrom2000 + ( 2000 - 1980 );

	return DosDateTime;
}

SKLP_Time6_t Time_DOS2SKLP( DosDateTime_t DosDateTime )
{
	SKLP_Time6_t SKLP_Time6;
	SKLP_Time6.Second		= DosDateTime.Second2 * 2;
	SKLP_Time6.Minute		= DosDateTime.Minute;
	SKLP_Time6.Hour			= DosDateTime.Hour;
	SKLP_Time6.Day			= DosDateTime.Day;
	SKLP_Time6.Month		= DosDateTime.Month;
	SKLP_Time6.YearFrom2000	= DosDateTime.Year + 1980 - 2000;
	return SKLP_Time6;
}

OADate_t Time_TimeExt2OADate( TimeExt_t *pTimeExt )
{
	assert_param( NULL != pTimeExt );
	if( -1 == pTimeExt->Time )
		return -1.;
	// time_t		( uint32_t )	- секунды		с 00:00:00 01.01.1970
	// TimeSubs_t	( uint16_t )	- доли секунды	0..0xFFFF
	// OADate_t		( double )		- дни			с 00:00:00 29.12.1899
	// 25569					- дней между двумя отправными датами.
	return ( pTimeExt->Time + pTimeExt->Subs / ( double ) TIME_SUBS_SPAN ) / ( double ) ( 24 * 60 * 60 ) + 25569;
}

OADate_t Time_SKLP2OADate( SKLP_Time_t *pSKLP )
{
	TimeExt_t TimeExt;
	TimeExt.Time = Time_SKLP2Time( &pSKLP->Time6 );
	TimeExt.Subs = Time_SecondHundr2SubSecond( pSKLP->SecondHundr );
	return Time_TimeExt2OADate( &TimeExt );
}

// Операции над временем
// ВзЯтие разности
TimeExt_t Time_DiffTimeExt( TimeExt_t *pTimeExt1, TimeExt_t *pTimeExt2 )
{
	assert_param( ( NULL != pTimeExt1 ) && ( NULL != pTimeExt2 ) );

	TimeExt_t Result;
	Result.Time = pTimeExt1->Time - pTimeExt2->Time;
	Result.Subs = pTimeExt1->Subs - pTimeExt2->Subs;
	if( pTimeExt1->Subs < pTimeExt2->Subs )
		Result.Time--;
	return Result;
}

