// RUS_Regul_ADC.h
// ИнициализациЯ и запуск АЦП
// Проект ГГКП (Гамма-гамма каротаж плотностной) ЛУЧ.623.00.01.00
#ifndef	VIKPB_ADC_H
#define	VIKPB_ADC_H

#include <stdint.h>
#include <stdbool.h>

// Калибровочные регистры АЦП
#if	defined( STM32F401xC ) || defined( STM32F405xx ) || defined( STM32F407xx ) || defined( STM32F417xx )
#define	ADC_REFIN_CAL				( * ( uint16_t * ) 0x1FFF7A2A )			// Измеренный внутренний Vref при температуре 30 degC и питании VDDA 3.3 V
#define	ADC_REFIN_CAL_VDDA			( ( float ) 3.3 )
#define	ADC_REFIN_CAL_TEMP			( ( float ) 30 )
#define	ADC_TS_CAL1					( * ( uint16_t * ) 0x1FFF7A2C )			// Измеренный внутренний температурный датчик при температуре 30 degC и питании VDDA 3.3 V
#define	ADC_TS_TEMP1				( ( float ) 30. )
#define	ADC_TS_CAL2					( * ( uint16_t * ) 0x1FFF7A2E )			// Измеренный внутренний температурный датчик при температуре 110 degC и питании VDDA 3.3 V
#define	ADC_TS_TEMP2				( ( float ) 110. )
#endif
// Расчеты
//#define ADC_CALC_VDDA( __ADC_VREF__ )			( ( ADC_REFIN_CAL_VDDA * ( __ADC_VREF__ ) ) / ADC_REFIN_CAL )
#define ADC_CALC_VDDA( __ADC_VREF__ )			( ( ADC_REFIN_CAL_VDDA * ADC_REFIN_CAL ) / ( __ADC_VREF__ ) )
#define	ADC_CALC_TS_TEMP( __ADC_TS__ )			( ( ( __ADC_TS__ ) - ADC_TS_CAL1 ) * ( ( ADC_TS_TEMP2 - ADC_TS_TEMP1 ) / ( ADC_TS_CAL2 - ADC_TS_CAL1 ) ) + ADC_TS_TEMP1 )
#define ADC_V_TO_mV_FLOAT(__VOLTS__)			( (__VOLTS__) * 1000.0f )
// Структура для измеренных данных АЦП 
#pragma pack( 1 )
typedef struct RUS_Regul_ADC_AI_struct
{
	float VSUP;		// [В]		НапрЯжение питаниЯ (+ 12 V) (V Supply)
	float OPAMP_meas;	// [А]		Ток через шунт (Напряжение на внутреннем операционном усилителе)
	float VDDA;		// [В]		+VDDA
	float VREF;		// [В]		Int.VRef
	float Temp;		// [degC]		Int.Temp
} RUS_Regul_ADC_AI_t;
#pragma pack( )

#if defined (ADC1_USE)
// ОбъЯвление типа оцифрованных данных
#if		( ADC1_RESOLUTION_bits == 12 )
	#define	ADC1_RESOLUTION			ADC_RESOLUTION_12B
	typedef __packed uint16_t ADC1_Data_t;
#elif	( ADC1_RESOLUTION_bits == 10 )
	#define ADC1_RESOLUTION			ADC_RESOLUTION_10B
	typedef __packed uint16_t ADC1_Data_t;
#elif	( ADC1_RESOLUTION_bits == 8 )
	#define ADC1_RESOLUTION			ADC_RESOLUTION_8B
	typedef __packed uint8_t ADC1_Data_t;
#elif	( ADC1_RESOLUTION_bits == 6 )
	#define ADC1_RESOLUTION			ADC_RESOLUTION_6B
	typedef __packed uint8_t ADC1_Data_t;
#else
	#error "Select Resolution to 6, 8, 10, 12"
#endif
#endif	// #if defined (ADC1_USE)

#define ADC1_SPAN				ADC_SPAN( ADC1_RESOLUTION_bits )

// ОбъЯвление функций
void RUS_Regul_ADC1_Init( void );
void RUS_Regul_ADC1_DeInit( void );
void HAL_ADC1_MspInit( void );
void HAL_ADC1_MspDeInit( ADC_HandleTypeDef* hadc );
void RUS_Regul_ADC_Start( void );								// Запустить оцифровку
void RUS_Regul_ADC_Stop( void );								// Завершить оцифровку
void RUS_Regul_ADC_GetVoltages( RUS_Regul_ADC_AI_t *pAI );				// Рассчитать измеренные напрЯжениЯ
//float ADC_getDCSignal_VPLC (void);
float ADC_getDCSignal_VDDA (void);
float ADC_getDCSignal_UHV ( void );
// Копирование буфера DMA в буфер накоплениЯ результатов оцифровок постоЯнных сигналов АЦП1
void ADC1_ConvRes_DCSignal_Copy( bool bFirstBank, ADC1_Data_t *pBuffResult );
// Усреднение показаний по всем каналам АЦП за половину транзакций DMA
void ADC1_AllChAvgHalfBuffer( bool bFirstBank );
uint8_t ADC_getFlagDCSignalStabilized( void );
void ADC_clearFlagDCSignalStabilized( void );
void ADC_clearVPLCsum( void );
void ADC_calcVPLCsum( void );
float ADC_getVPLCsum( void );
float ADC_getVPLCmin( void );
uint32_t ADC_getVPLCsumCnt( void );
uint32_t ADC_getVPLCsumValue( void );
float ADC_OPAMP_Value_Get(void);
float ADC_VSUP_Value_Get(void);







#endif	// VIKPB_ADC_H

