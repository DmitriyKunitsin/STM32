#ifndef	UTILS_H
#define	UTILS_H

#include <stdint.h>
#include <stdbool.h>

// BCD32_t
typedef uint32_t BCD32_t;
bool BCD32_to_UInt32( BCD32_t BCD32, uint32_t *pUInt32 );
bool UInt32_to_BCD32( uint32_t UInt32, BCD32_t *pBCD32 );
bool UInt8_to_BCD8( uint8_t UInt8, uint8_t *pBCD8 );
uint8_t BCD8_2Bin( uint8_t BCD8 );

// ѕобайтово помен€ть тетрады в байтах
uint32_t UInt32_SwapTetrades( uint32_t Source );

// ѕомен€ть пор€док следовани€ байтов в слове
#include <intrinsics.h>
inline uint16_t ReverseByteOrder16( uint16_t Value )	{ return __REV16( Value ); }
inline uint32_t ReverseByteOrder32( uint32_t Value )	{ return __REV( Value ); }

// CRC8
uint8_t CalcCRC8SKLP( uint8_t *pData, uint32_t Size );
uint8_t CalcCRC8SKLP_Load( uint8_t *pData, uint32_t Size, uint8_t CRC8 );

// CRC16
uint16_t CalcCRC16( uint8_t *pData, uint32_t Size );
uint16_t CalcCRC16_Load( uint8_t *pData, uint32_t Size, uint16_t CRC16 );

// CRC16_SKLP
uint16_t CalcCRC16SKLP( uint8_t *pData, uint32_t Size );
uint16_t CalcCRC16SKLP_Load( uint8_t *pData, uint32_t Size, uint16_t CRC16 );

#define TRANC( VAL, TRESH )		( ( ( VAL ) > ( TRESH ) ) ? ( TRESH ) : ( VAL ) )
//uint16_t TrancateFloatUI16( const float Value, const uint16_t MargLow, const uint16_t MargHigh );

// *********************************************************************
// Rounding float to int utils
// *********************************************************************
uint8_t Round_UI8( const float Val );
uint16_t Round_UI16( const float Val );
uint32_t Round_UI32( const float Val );
int8_t Round_SI8( const float Val );
int16_t Round_SI16( const float Val );
int32_t Round_SI32( const float Val );

#endif	// UTILS_H

