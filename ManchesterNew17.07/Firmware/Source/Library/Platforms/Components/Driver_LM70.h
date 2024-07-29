// Driver_LM70.h
// Драйвер температурных датчиков LM70
#ifndef DRIVER_LM70_H
#define DRIVER_LM70_H

#include <stdint.h>
#include <stdbool.h>

/******************************************************
Исходный возвращаемый формат температуры датчика LM70:
-55 [грЦ] - 0xE49F
  0 [грЦ] - 0x001F
150 [грЦ] - 0x4B1F
шаг 0.25 [грЦ] - 0x0001

Возвращаемый формат температуры (int16):
-55 [грЦ] - 0xFF24	( -55 * 4 )
  0 [грЦ] - 0x0000	( 0 )
150 [грЦ] - 0x0258	( 150 * 4 )
шаг 0.25 [грЦ] - 0x0001
*******************************************************/

#define	LM70_TEMP_MAX		150
#define	LM70_TEMP_MIN		-55
#define LM70_CHECK_LIMITS	0							// Не проверЯть показаниЯ на допустимые пределы
#define	LM70_NAN			( ( int16_t ) 0x8000 )		// Возвращаемое значение при ошибке
#define	LM70_TEMP_COEFF		4							// Коэффициент перехода из формата LM70_ReadTemp( ) в градусы
inline int16_t LM70_ConvertToCelsium_int16( int16_t LM70_Temp )		{ return ( ( LM70_NAN != LM70_Temp ) ? ( LM70_Temp / LM70_TEMP_COEFF ) : ( LM70_NAN ) ); }
inline float LM70_ConvertToCelsium_float( int16_t LM70_Temp )		{ return ( ( LM70_NAN != LM70_Temp ) ? ( ( float ) LM70_Temp / LM70_TEMP_COEFF ) : ( 0.NaN ) ); }

//void LM70_CaptureSPI( void );			// Захват канала SPI длЯ доступа к микросхемам термодатчиков
bool LM70_Init( void );					// Инициализация измерения температуры

// ?? Неверно работает с отрицательными числами (возможно) ??
//int16_t LM70_ReadTemp( uint8_t ChannelIndex );	// Температура одного датчика
bool LM70_ReadTemp( int16_t *pTemp );		// Температура одного датчика

#endif	// DRIVER_LM70_H

