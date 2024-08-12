#ifndef	MATH_UTILS_H
#define	MATH_UTILS_H

// ��������� ������� � ����������� ���������� ��������������� ������������� �������
typedef struct DSP_IIR_Instace_f32_struct
{
	uint8_t			Order;			// Order		�������
	float * const	pZnum;			// [Order+1]	Numerator coefficients
	float * const	pZden;			// [Order]		Denumerator coefficients
	float			*pStates;		// [Order+1]	������������� �����
	float			InitCoeff;		// ����������� ��� ������������� �������������� ������
	uint16_t		iDelay;			// ��������, ������� ������������ ������ (� ��������� �� �������)
} DSP_IIR_Instace_f32_t;

float DSP_IIR_AppendFirst_f32( DSP_IIR_Instace_f32_t const *pInstatnce, float NewValue );		// �������� ������ �������� � ������, � ��������� ����������
float DSP_IIR_Append_f32( DSP_IIR_Instace_f32_t const *pInstatnce, float NewValue );			// �������� ��������� �������� � ������

// ****************************************************************
// ������ � �������� ���������� ���������������
// �� ������������� ��������� uint8_t � ������ uint16_t
typedef struct DSP_FIR_UI8_UI16_struct
{
	uint16_t	Depth;
	uint16_t	Index;
	uint16_t	Summ;
	uint8_t		*pBuffer;
} DSP_FIR_UI8_UI16_t;

void DSP_FIR_UI8_UI16_Setup( DSP_FIR_UI8_UI16_t *pFIR, uint8_t *pBuffer, uint16_t Depth, uint8_t InitValue );
uint16_t DSP_FIR_UI8_UI16_Append( DSP_FIR_UI8_UI16_t *pFIR, uint8_t NewValue );

// ****************************************************************
// ������ � �������� ���������� ���������������
// �� ������������� ��������� uint16_t � ������ uint32_t
typedef struct DSP_FIR_UI16_UI32_struct
{
	uint16_t	Depth;
	uint16_t	Index;
	uint32_t	Summ;
	uint16_t	*pBuffer;
} DSP_FIR_UI16_UI32_t;

void DSP_FIR_UI16_UI32_Setup( DSP_FIR_UI16_UI32_t *pFIR, uint16_t *pBuffer, uint16_t Depth, uint16_t InitValue );
uint32_t DSP_FIR_UI16_UI32_Append( DSP_FIR_UI16_UI32_t *pFIR, uint16_t NewValue );

// ������ � �������� ���������� ���������������
// �� ������������� ��������� int16_t � ������ int32_t
typedef struct DSP_FIR_SI16_SI32_struct
{
	uint16_t	Depth;
	uint16_t	Index;
	int32_t		Summ;
	int16_t		*pBuffer;
} DSP_FIR_SI16_SI32_t;

void DSP_FIR_SI16_SI32_Setup( DSP_FIR_SI16_SI32_t *pFIR, int16_t *pBuffer, uint16_t Depth, int16_t InitValue );
int32_t DSP_FIR_SI16_SI32_Append( DSP_FIR_SI16_SI32_t *pFIR, int16_t NewValue );

// ****************************************************************
// ������ � �������� ���������� ���������������
// �� ������������� ��������� int32_t � ������ int32_t
typedef struct DSP_FIR_SI32_SI32_struct
{
	uint16_t	Depth;
	uint16_t	Index;
	int32_t		Summ;
	int32_t		*pBuffer;
} DSP_FIR_SI32_SI32_t;

void DSP_FIR_SI32_SI32_Setup( DSP_FIR_SI32_SI32_t *pFIR, int32_t *pBuffer, uint16_t Depth, int32_t InitValue );
int32_t DSP_FIR_SI32_SI32_Append( DSP_FIR_SI32_SI32_t *pFIR, int32_t NewValue );


// �������
#ifndef	SQR
#define SQR( __VAL__ )				( ( __VAL__ ) * ( __VAL__ ) )
#endif
// ������ (���������� ��������
#ifndef	ABS
#define	ABS( __VAL__ )				( ( ( __VAL__ ) >= 0 ) ? ( __VAL__ ) : -( __VAL__ ) )
#endif
// ������� � �����������
#ifndef	DIVIDE
#define	DIVIDE( __VAL__, __BASE__ )	( ( ( __VAL__ ) + ( __BASE__ ) / 2 ) / ( __BASE__ ) )
#endif
// ��������/�������
#ifndef	MAX
#define	MAX( __VAL1__, __VAL2__ )	( ( ( __VAL1__ ) > ( __VAL2__ ) ) ? ( __VAL1__ ) : ( __VAL2__ ) )
#endif
#ifndef	MIN
#define	MIN( __VAL1__, __VAL2__ )	( ( ( __VAL1__ ) < ( __VAL2__ ) ) ? ( __VAL1__ ) : ( __VAL2__ ) )
#endif
// ��
#ifndef	M_PI
#define	M_PI		3.1415927f
#define	M_PI_2		( M_PI / 2 )
#define	M_PI_4		( M_PI / 4 )
#endif

// ����� ������������ �����, ������ ��� ������ ���������, � ���������� �������� ������
inline uint32_t GetGreatestPowerOf2( uint32_t Number )	{ return __RBIT( 1 << __CLZ( Number ) ); }


// ****************************************************************
// ****** Inline Assembler for Float Single/Half convertion *******
// ****************************************************************
typedef uint16_t Float16_t;
typedef float Float32_t;

#ifndef __ICCARM__
#error "This file should only be compiled by ICCARM"
#endif

#ifdef	__ARMVFP__
#define __IAR_FT _Pragma("inline=forced") __intrinsic

__IAR_FT Float16_t Float32to16( Float32_t Float32 )
{
	uint32_t Result;
	__asm volatile( "VCVTB.F16.F32 %[Dest], %[Src]" : [Dest] "=t" ( Result ) : [Src] "t" ( Float32 ) );
	return ( Float16_t ) Result;
}

__IAR_FT Float32_t Float16to32( Float16_t Float16 )
{
	Float32_t Result;
	__asm volatile( "VCVTB.F32.F16 %[Dest], %[Src]" : [Dest] "=t" ( Result ) : [Src] "t" ( Float16 ) );
	return Result;
}

#undef	__IAR_FT
#endif	// __ARMVFP__
// ****************************************************************


#endif	// MATH_UTILS_H

