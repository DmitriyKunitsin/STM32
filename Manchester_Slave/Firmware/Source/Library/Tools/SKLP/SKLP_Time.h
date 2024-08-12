// SKLP_Time.h
// ������� ������� � �������
#ifndef	SKLP_TIME_H
#define	SKLP_TIME_H

#include <time.h>		// time_t
#include <stdint.h>
#include <stdbool.h>

// ������� �������� �������
typedef uint16_t TimeSubs_t;		// ������������� ���������� - 0xFFFF ������������� ( 1 - 1/0x10000 ) �
#define	TIME_SUBS_SPAN			( ( uint32_t ) 0x10000 )

typedef double OADate_t;			// OLE Automation DateTime

// ********************* time_t *********************
// ����������� time_t - ����������� ������������ �� ��
typedef struct TimeExt_struct
{
	time_t			Time;		// [c]	common time_t
	TimeSubs_t		Subs;		//		���� ������� 0..0xFFFF
} TimeExt_t;

// ********************* ��� *********************
// ��������� �� 1 � ������ ������� �� ������� ��� - ������������ ��� ������ ��������� ������� � ��������� ������������
#pragma pack( 1 )
typedef struct SKLP_Time6_struct
{
	uint8_t	YearFrom2000;		// [0..255]
	uint8_t	Month;				// [1..12]
	uint8_t	Day;				// [1..31]
	uint8_t	Hour;				// [0..23]
	uint8_t	Minute;				// [0..59]
	uint8_t	Second;				// [0..59]
} SKLP_Time6_t;
#pragma pack( )

// ����� ������ ������� �� ������� ���
#pragma pack( 1 )
typedef union SKLP_Time_union
{
	struct
	{
		uint8_t YearFrom2000;		// [0..255]
		uint8_t Month;				// [1..12]
		uint8_t Day;				// [1..31]
		uint8_t Hour;				// [0..23]
		uint8_t Minute; 			// [0..59]
		uint8_t Second; 			// [0..59]
		uint8_t SecondHundr;		// [0..99]
	};
	SKLP_Time6_t Time6;
} SKLP_Time_t;
#pragma pack( )

#define	SKLP_TIME_NULL		( ( SKLP_Time_t ) { 0 } )
#define	SKLP_TIME6_NULL		( ( SKLP_Time6_t ) { 0 } )

// ********************* MS-DOS *********************
#pragma pack( 1 )
typedef union DosDateTime_union
{
	struct
	{
		uint32_t	Second2	:5; 	// [0..29]	second / 2
		uint32_t	Minute 	:6; 	// [0..59]
		uint32_t	Hour	:5; 	// [0..23]
		uint32_t	Day		:5; 	// [1..31]
		uint32_t	Month	:4; 	// [1..12]
		uint32_t	Year	:7; 	// [0..127] � 1980 �.
	};
	struct
	{
		uint16_t	Time;
		uint16_t	Date;
	};
	uint32_t		DateTime;
} DosDateTime_t;
#pragma pack( )


// ������� ������ �� ��������
bool SKLP_TimeValidate( SKLP_Time_t *pTime );				// �������� ������������ ������� �������
bool SKLP_Time6Validate( SKLP_Time6_t *pTime6 );			// �������� ������������ ������� �������

// ������� �������������� �������� ��������
inline TimeSubs_t Time_SecondHundr2SubSecond( uint8_t SecondHundr )	{ return ( TimeSubs_t ) ( SecondHundr * ( TIME_SUBS_SPAN / 100.0f ) ); }
inline uint8_t Time_SubSecond2SecondHundr( TimeSubs_t TimeSubs )		{ return ( uint8_t ) ( ( ( uint32_t ) TimeSubs * 100 ) / TIME_SUBS_SPAN ); }
time_t Time_SKLP2Time( SKLP_Time6_t *pSKLP_Time6 );
SKLP_Time6_t Time_Time2SKLP( time_t Time );
char const *Time_Time2String( time_t Time );
char const *Time_SKLP2String( SKLP_Time6_t *pSKLP_Time6 );
DosDateTime_t Time_SKLP2DOS( SKLP_Time6_t *pSKLP_Time6 );
SKLP_Time6_t Time_DOS2SKLP( DosDateTime_t DosDateTime );
OADate_t Time_TimeExt2OADate( TimeExt_t *pTimeExt );
OADate_t Time_SKLP2OADate( SKLP_Time_t *pSKLP );

// �������� ��� ��������
TimeExt_t Time_DiffTimeExt( TimeExt_t *pTimeExt1, TimeExt_t *pTimeExt2 );	// ������ ��������

#endif	// SKLP_TIME_H

