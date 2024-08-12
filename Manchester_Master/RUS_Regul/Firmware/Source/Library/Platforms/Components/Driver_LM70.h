// Driver_LM70.h
// ������� ������������� �������� LM70
#ifndef DRIVER_LM70_H
#define DRIVER_LM70_H

#include <stdint.h>
#include <stdbool.h>

/******************************************************
�������� ������������ ������ ����������� ������� LM70:
-55 [���] - 0xE49F
  0 [���] - 0x001F
150 [���] - 0x4B1F
��� 0.25 [���] - 0x0001

������������ ������ ����������� (int16):
-55 [���] - 0xFF24	( -55 * 4 )
  0 [���] - 0x0000	( 0 )
150 [���] - 0x0258	( 150 * 4 )
��� 0.25 [���] - 0x0001
*******************************************************/

#define	LM70_TEMP_MAX		150
#define	LM70_TEMP_MIN		-55
#define LM70_CHECK_LIMITS	0							// �� ��������� ��������� �� ���������� �������
#define	LM70_NAN			( ( int16_t ) 0x8000 )		// ������������ �������� ��� ������
#define	LM70_TEMP_COEFF		4							// ����������� �������� �� ������� LM70_ReadTemp( ) � �������
inline int16_t LM70_ConvertToCelsium_int16( int16_t LM70_Temp )		{ return ( ( LM70_NAN != LM70_Temp ) ? ( LM70_Temp / LM70_TEMP_COEFF ) : ( LM70_NAN ) ); }
inline float LM70_ConvertToCelsium_float( int16_t LM70_Temp )		{ return ( ( LM70_NAN != LM70_Temp ) ? ( ( float ) LM70_Temp / LM70_TEMP_COEFF ) : ( 0.NaN ) ); }

//void LM70_CaptureSPI( void );			// ������ ������ SPI ��� ������� � ����������� �������������
bool LM70_Init( void );					// ������������� ��������� �����������

// ?? ������� �������� � �������������� ������� (��������) ??
//int16_t LM70_ReadTemp( uint8_t ChannelIndex );	// ����������� ������ �������
bool LM70_ReadTemp( int16_t *pTemp );		// ����������� ������ �������

#endif	// DRIVER_LM70_H

