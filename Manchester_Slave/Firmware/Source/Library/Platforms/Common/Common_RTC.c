// Common_RTC.c
// ������� ��� ������ � ������ ��������� �������
#include "ProjectConfig.h"		// ������ ���������
#include "stm32xxxx_hal.h"		// ����� ���������
#include "Common_RTC.h"			// ������
#include "STM32f4xx_hal_rtc.h"	// ������� RTC
#include "FreeRTOS.h"			// taskENTER_CRITICAL()
#include "Task.h"				// taskENTER_CRITICAL()
#include "Utils.h"				// Round_SI16()
#include "common_rcc.h"			// DWT_TimerGetLong()
#include "Logger.h"

#ifndef	STM32F4
#error "Only for STM32F4 Series!"
#endif

RTC_HandleTypeDef CommonRTC_hdl __PLACE_AT_RAM_CCM__;	// �������

// ������������� ������ RTC
// pbCalendarInitialized	- ���� ������������ ��������� ��������, ��� �� ��������������� ��������� �� ������ ������� �������
// ���������� true, ���� ������� ���������� �������������.
bool CommonRTC_Init( bool *pbCalendarInitialized )
{
	bool Result = false;
	do
	{
		RCC_PeriphCLKInitTypeDef RCC_PeriphCLKInit;
		RCC_PeriphCLKInit.PeriphClockSelection	= RCC_PERIPHCLK_RTC;
		RCC_PeriphCLKInit.RTCClockSelection 	= RCC_RTCCLKSOURCE;
		// ����� ���, ���� �������� RTC �� �������� �� ��������� � �������, ����������������� �� ������������
		if( HAL_OK != HAL_RCCEx_PeriphCLKConfig( &RCC_PeriphCLKInit ) )			// Reset the Backup domain only if the RTC Clock source selection is modified
			break;
		
		__HAL_RCC_RTC_CLKPRESCALER( RCC_RTCCLKSOURCE );
		__HAL_RCC_RTC_ENABLE( ); 
		
		CommonRTC_hdl.Instance = RTC;
		RTC_InitTypeDef * const pInit = &CommonRTC_hdl.Init;
		pInit->HourFormat 		= RTC_HOURFORMAT_24;
		pInit->AsynchPrediv		= RTC_ASINCH_PREDIV;
		pInit->SynchPrediv		= RTC_SINCH_PREDIV;
		pInit->OutPut 			= RTC_OUTPUT_DISABLE;
		pInit->OutPutPolarity	= RTC_OUTPUT_POLARITY_HIGH;
		pInit->OutPutType 		= RTC_OUTPUT_TYPE_OPENDRAIN;
/*
		// !!� ���������� ������������� ������������ ������� ����� ������!
		if( HAL_OK != HAL_RTC_Init( &CommonRTC_hdl ) )
			break;

		// ���� ����� �� ������������� ��������� � ������� ���������, ��������� ��� ����������
		if( HAL_OK != HAL_RTC_WaitForSynchro( &CommonRTC_hdl ) )
			break;
		// ���������� ��� �������������� ��������� ��� ��� ������ �� ������ ����������������, ����� ��������������� ����������� � ������� �������� �� ����������

		// ���������, ��������, ��������� ��� ��������������� - ���� �� ���� ������ �� �������
		if( NULL != pbCalendarInitialized )
			*pbCalendarInitialized = ( CommonRTC_hdl.Instance->ISR & RTC_ISR_INITS );
*/
		// !!!����!!!
		if( CommonRTC_hdl.Instance->ISR & RTC_ISR_INITS )
		{	// ��������� ��� ���������������, ����� ������������� �� ���������
			if( NULL != pbCalendarInitialized )
				*pbCalendarInitialized = true;
		}
		else
		{
			if( HAL_OK != HAL_RTC_Init( &CommonRTC_hdl ) )
				break;
			
			// ���� ����� �� ������������� ��������� � ������� ���������, ��������� ��� ����������
			if( HAL_OK != HAL_RTC_WaitForSynchro( &CommonRTC_hdl ) )
				break;
			// ���������� ��� �������������� ��������� ��� ��� ������ �� ������ ����������������, ����� ��������������� ����������� � ������� �������� �� ����������

			if( NULL != pbCalendarInitialized )
				*pbCalendarInitialized = false;
		}
		
		Result = true;
	} while( 0 );
	return Result;
}


// ���������� ���� �� ������� ������� ���
// ����� ������� ������������ � RTC, � ���� ������� � ��������� ���������� *pCommonRTC_SubsecondCorrection ��� ���������� �����������
bool CommonRTC_SetSKLP( SKLP_Time_t NewTime )
{
	bool Result = false;
	do
	{
		if( !SKLP_TimeValidate( &NewTime ) )
			break;
			
		RTC_TimeTypeDef RTC_Time;
		RTC_DateTypeDef RTC_Date;
		
		RTC_Date.Year			= NewTime.YearFrom2000;
		RTC_Date.Month			= NewTime.Month;
		RTC_Date.Date			= NewTime.Day;
//		RTC_Date.WeekDay		= 0;	//RTC_WEEKDAY_WEDNESDAY;
		RTC_Date.WeekDay		= RTC_WEEKDAY_MONDAY;
		RTC_Time.Hours			= NewTime.Hour;
		RTC_Time.Minutes		= NewTime.Minute;
		RTC_Time.Seconds		= NewTime.Second;
		RTC_Time.SubSeconds 	= 0;
		*pCommonRTC_SubsCorrection = Time_SecondHundr2SubSecond( NewTime.SecondHundr );
		RTC_Time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
		RTC_Time.StoreOperation = RTC_STOREOPERATION_RESET;
		taskENTER_CRITICAL( );
		if( ( HAL_OK == HAL_RTC_SetDate( &CommonRTC_hdl, &RTC_Date, RTC_FORMAT_BIN ) ) &&
			( HAL_OK == HAL_RTC_SetTime( &CommonRTC_hdl, &RTC_Time, RTC_FORMAT_BIN ) ) )
			Result = true;
		taskEXIT_CRITICAL( );
		
		CommonRTC_AutoCalibr( true );
	} while( 0 );

	return Result;
}

// ������� ����� �� ����� ��������� ������� � ������� � ������� ��� (� ����������� �� 0.01 �������)
SKLP_Time_t CommonRTC_GetSKLP( void )
{
	TimeExt_t TimeExt = CommonRTC_GetTimeExt( );
	SKLP_Time_t Result;
	Result.Time6 = Time_Time2SKLP( TimeExt.Time );
	Result.SecondHundr = Time_SubSecond2SecondHundr( TimeExt.Subs );
	return Result;
}

// ������� ����� �� ����� ��������� ������� � ������� � ������� TimeExt_t (� ����������� ����������� �� ����� �������)
TimeExt_t CommonRTC_GetTimeExt( void )
{
	RTC_TimeTypeDef RTC_Time;
	RTC_DateTypeDef RTC_Date;

	// ������� ��������� �� RTC
	taskENTER_CRITICAL( );
	assert_param( HAL_OK == HAL_RTC_GetTime( &CommonRTC_hdl, &RTC_Time, RTC_FORMAT_BIN ) );
	assert_param( HAL_OK == HAL_RTC_GetDate( &CommonRTC_hdl, &RTC_Date, RTC_FORMAT_BIN ) );
	taskEXIT_CRITICAL( );

	// ��������� � ������ ���
	SKLP_Time6_t SKLP_Time6;
	SKLP_Time6.YearFrom2000	= RTC_Date.Year;
	SKLP_Time6.Month		= RTC_Date.Month;
	SKLP_Time6.Day			= RTC_Date.Date;
	SKLP_Time6.Hour 		= RTC_Time.Hours;
	SKLP_Time6.Minute		= RTC_Time.Minutes;
	SKLP_Time6.Second		= RTC_Time.Seconds;

	// ��������� � ������ TimeExt_t � ������� ���� ������ �� ��������� RTC
	TimeExt_t TimeExt;
	TimeExt.Time = Time_SKLP2Time( &SKLP_Time6 );
	TimeExt.Subs = ( TimeSubs_t ) ( TIME_SUBS_SPAN * ( ( float ) ( RTC_Time.SecondFraction - RTC_Time.SubSeconds ) ) / ( RTC_Time.SecondFraction + 1 ) );

	// �������� ����������� ����� ������
	TimeExt.Subs += *pCommonRTC_SubsCorrection;
	if( TimeExt.Subs < *pCommonRTC_SubsCorrection )
		TimeExt.Time++;		// ��������� ������������ ������������� ����, �������� ������� � ��������� �������

	return TimeExt;
}

#if	defined( USE_RTC_FROM_LSE ) && ( LSE_VALUE == 32768ul )
// ������� ����������� ���������� �� ������ � ������������� RTC �������� 32768 ��.
// ��� ������ �������, �������� ���� ����� ������������ ���
#define	RTC_CALR_MAX	( ( int32_t ) RTC_CALR_CALM	)	// 0x1FF			������������� ������������� ���������� -487.1 ppm
#define	RTC_CALR_MIN	( -RTC_CALR_MAX - 1 )			// CALP | 0x000	������������� ������������� ��������� +488.5 ppm
#define	RTC_CALR2PPM	( -487.1f / RTC_CALR_MAX )		// ����������� �������� �� CALR � [ppm]

// ������� ������� ���������� RTC
// ���������� ��������� � [ppm]
// ���������� ���������� �������� RTC.CALR
int32_t CommonRTC_GetCalibration( int16_t *pCalibr_ppm )
{
	// !!��������� �����������!!
	int32_t CALR = CommonRTC_hdl.Instance->CALR;
	if( NULL != pCalibr_ppm )
	{
		int32_t CALR2 = CALR;
		if( CALR2 & RTC_CALR_CALP )
			CALR2 |= ~RTC_CALR_CALM;
		*pCalibr_ppm = Round_SI16( CALR2 * RTC_CALR2PPM );
	}
	return CALR;
}

// ���������� ����� ���������� RTC
// �������� � [ppm]
// ���������� ���������� �������� RTC.CALR
int32_t CommonRTC_SetCalibration( int16_t Calibr_ppm )
{
	int32_t CALR = Round_SI16( Calibr_ppm / RTC_CALR2PPM );
	if( CALR > RTC_CALR_MAX )
		CALR = RTC_CALR_MAX;
	else if( CALR < RTC_CALR_MIN )
		CALR = RTC_CALR_MIN;
	
	ENTER_CRITICAL_SECTION( );
	__HAL_RTC_WRITEPROTECTION_DISABLE( &CommonRTC_hdl );
	WRITE_BIT_MASKED( CommonRTC_hdl.Instance->CALR, CALR, RTC_CALR_CALM | RTC_CALR_CALP );
	__HAL_RTC_WRITEPROTECTION_ENABLE( &CommonRTC_hdl );
	CALR = CommonRTC_hdl.Instance->CALR;
	EXIT_CRITICAL_SECTION( );

	return CALR;
}

// ���������� ����� LSE (RTC) � HSE (SYSTICK)
// ������ ����� - ������������� ���������.
// ����������� ������ ��������� ���������� RTC � SYSTICK.
// ��� ������ ������ ����� ��������, ��� ������ ���������� (���� ������� �� ������ �� ��������).
// ����������������� ����������� ������ ����� �������� 10..30 �����.
void CommonRTC_AutoCalibr( bool bReset )
{
	static TimeExt_t TimeStampSaved_RTC = { 0 };
	static uint64_t TimeStampSaved_DWT = 0;
	if( 0 == TimeStampSaved_RTC.Time )
		bReset = true;

	taskENTER_CRITICAL( );
	TimeExt_t TimeStampNew_RTC = CommonRTC_GetTimeExt( );
	uint64_t TimeStampNew_DWT = DWT_TimerGetLong( );
	taskEXIT_CRITICAL( );

	char aMsg[130];
	aMsg[0] = '\0';
	int16_t Calibr_ppm;
	int32_t CALR_old = CommonRTC_GetCalibration( &Calibr_ppm );
	do
	{
		if( bReset )
		{
			snprintf( aMsg, sizeof( aMsg ), "RTC Autocalibration Reset (CALR = 0x%08X).", CALR_old );
			break;
		}
		
		uint32_t DiffTimeDWT_ms = ( uint32_t ) ( ( ( TimeStampNew_DWT - TimeStampSaved_DWT ) * 1000 ) / COMMON_RCC_PLLCLK_DEFAULT );
		if( DiffTimeDWT_ms > ( 10 * 60 * 1000 ) )
		{
			TimeExt_t DiffTimeRTC_s = Time_DiffTimeExt( &TimeStampNew_RTC, &TimeStampSaved_RTC );
			uint32_t DiffTimeRTC_ms = DiffTimeRTC_s.Time * 1000 + ( DiffTimeRTC_s.Subs * ( uint32_t ) 1000 ) / TIME_SUBS_SPAN;
			
			
			int32_t Dispersion_ms = DiffTimeRTC_ms - DiffTimeDWT_ms;
			int32_t Dispersion_ppm = Round_SI32( ( 1000000 * Dispersion_ms ) / ( float ) DiffTimeDWT_ms );
			
			Calibr_ppm -= Dispersion_ppm;
			int32_t CALR_new = CommonRTC_SetCalibration( Calibr_ppm );
			
			snprintf( aMsg, sizeof( aMsg ), "dT RTC = %d ms; dT DWT = %d ms; dispT = %+d ms (%+d ppm). Set calibr to %hd (CALR 0x%08X -> 0x%08X).",
					DiffTimeRTC_ms, DiffTimeDWT_ms, Dispersion_ms, Dispersion_ppm, Calibr_ppm, CALR_old, CALR_new );
			break;
		}
	} while( 0 );
	if( '\0' != aMsg[0] )
	{
		assert_param( Logger_WriteRecord( aMsg, LOGGER_FLAGS_APPENDTIMESTAMP ) );
		TimeStampSaved_RTC = TimeStampNew_RTC;
		TimeStampSaved_DWT = TimeStampNew_DWT;
	}
}
#else
void CommonRTC_AutoCalibr( bool bReset ) {}
#endif	// defined( USE_RTC_FROM_LSE ) && ( LSE_VALUE == 32768ul )


