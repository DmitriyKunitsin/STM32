// Driver_ADS1231.h
// ������� ��� ADS1231
#ifndef	DRIVER_ADS1231_H
#define	DRIVER_ADS1231_H

#include <stdbool.h>
#include <stdint.h>

// ���������� ����� ������������� � ������ �������������� ���������
typedef enum ADS1231_SPS_enum
{
	ADS1231_SPS_10,
	ADS1231_SPS_80,
} ADS1231_SPS_t;

// ���������� � ��� ���������
#define	ADS1231_GAIN		128
// ������� ������������ ������� (24 ���� �� ������), ������������ � SINT32
#define	ADS1231_ADCMAX		( ( int32_t ) 0x007FFFFF )								// +8388607	������������� ( +0.5 Vref / 128 )
#define	ADS1231_ADCMIN		( ( int32_t ) 0xFF800000 )								// -8388608	������������� ( -0.5 Vref / 128 )
#define	ADS1231_ADCSPAN		( ( uint32_t ) ( ADS1231_ADCMAX - ADS1231_ADCMIN ) )	// 16777216
// ������ ������������ ������� Vin,			[�]		�� ( -0.5 Vref / 128 )	�� ( +0.5 Vref / 128 )
// ������ ������������ ������� Vin/Vref,	[�/�]	�� ( -3.096e-3 )			�� ( +3.096e-3 )

bool ADS1231_Init( void );
bool ADS1231_DataRead( int32_t *pDataADC );
void ADS1231_SetSPS( ADS1231_SPS_t SPS );
void ADS1231_IRQ_Enable( bool bIRQ_En );
extern void ADS1231_EXTI_Callback( void );

// ������� ������ ��� � ��������� Vin/Vref
inline float ADS1231_ADC2Float( int32_t DataADC )	{ return DataADC / ( ADS1231_ADCSPAN * ( float ) ADS1231_GAIN ); }

#endif	// DRIVER_ADS1231_H

