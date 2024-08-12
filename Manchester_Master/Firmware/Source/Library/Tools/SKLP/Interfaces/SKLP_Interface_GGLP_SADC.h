// SKLP_Interface_GGLP_SADC.h
// ��������� ��������� ������������������� ��� ��� ���-�� �� ���� ���
// ���������� ���.638.00.02.00 � ������� ������ ���.638.00.00.00
#ifndef	SKLP_INTERFACE_GGLP_SADC_H
#define	SKLP_INTERFACE_GGLP_SADC_H

#include <stdint.h>
#include "SKLP_Time.h"				// SKLP_Time_t
//#include "NVM.h"					// NVM_Result_t
#include "SKLP_MS_Transport.h"		// SKLP_Signature
#include "SKLP_Service.h"			// SKLP_FlagsModule_t, SKLP_CommandResult_t
#include "MathUtils.h"				// Float16_t
#include <stdint.h>

// ������ �������������� ��������
#define	GGLP_SDAC_SPECTRUM_SIZE_SHORT			256
#define	GGLP_SDAC_SPECTRUM_SIZE_FULL			4096

// ������������� ������� ��������� ��� ��� ����� ������������������� ���
typedef enum SKLP_Command_GGLP_SADC_enum
{
	SKLP_COMMAND_GGLP_SADC_MODE_SET				= 0x11,	// ��������� ������ �������
	SKLP_COMMAND_GGLP_SADC_DATA_FAST_SPEC_GET	= 0x12, // ������ �������� ������ �� ���������, ���������� �� 25 ��
	SKLP_COMMAND_GGLP_SADC_DATA_FAST_GET		= 0x13, // ������ �������� ������, ���������� �� 25 ��
	SKLP_COMMAND_GGLP_SADC_DATA_TECH_GET		= 0x14, // ������ ��������������� ����������
	SKLP_COMMAND_GGLP_SADC_DATA_SLOW_SPEC_GET	= 0x15, // ������ �������� ������ �� ���������, ���������� �� 6000 ��
	SKLP_COMMAND_GGLP_SADC_DATA_SLOW_GET		= 0x16, // ������ �������� ������, ���������� �� 6000 ��
	SKLP_COMMAND_GGLP_SADC_DATA_ACCUM_SPEC_GET	= 0x17, // ������ ������������ ������� �� ���������� ������ �������
} SKLP_Command_GGLP_SADC_t;

// ����� ����� ������������������� ���
#pragma pack( 1 )
typedef union GGLP_SADC_FlagsModule_union
{
	uint8_t Byte;
	struct
	{
		uint8_t fEnabled			:1;		// ������� ������� ���������� ������
		uint8_t fReady				:1;		// ���������� ���������� �������������, ���������� � ������
		uint8_t fReserved			:6;		// ���������������
	};
} GGLP_SADC_FlagsModule_t;
#pragma pack( )

// ����� ���������� ���������� � ��������� ��������
#pragma pack( 1 )
typedef union GGLP_SADC_FlagsResult_union
{
	uint8_t Byte;
	struct
	{
		uint8_t fComplete			:1;		// ������ ��������
		uint8_t fFailCntLoss		:1;		// ������������ ������ ��� ��������� � �����
		uint8_t fFailCntOvfLow		:1;		// ������������ ����� � ������ ������ ������� (������� ����)
		uint8_t fFailCntOvfHigh		:1;		// ������������ ����� � ��������� ������ ������� (������� �����)
		uint8_t fFailCntOvfSpectrum	:1;		// ������������ ����� � �����-����� ������ �������, ����� ������ � ���������
		uint8_t fFailCntOvfPhoto	:1;		// ������������ �������� �����������
		uint8_t fFailCntOvfCompton	:1;		// ������������ �������� �������-�������
		uint8_t fFailCntOvfTotal	:1;		// ������������ �������� ���� ���������
	};
} GGLP_SADC_FlagsResult_t;
#pragma pack( )

// *****************************************************************************
// [0x13] ����� �� ������ �������� ������, ���������� �� 25 ��
// *****************************************************************************
#pragma pack( 1 )
typedef struct SKLP_GGLP_SADC_DataFastGet_Answer_struct
{
	GGLP_SADC_FlagsModule_t	FlagsModule;	// [1]		����� �����
	GGLP_SADC_FlagsResult_t	FlagsResult;	// [1]		����� ���������� ��������� �����
	uint16_t				TimeAccum_us;	// [2]		����� ���������� ����������
	uint16_t				CountTotal;		// [2]		���� ����� (������)
	uint16_t				CountPhoto;		// [2]		���� �� ��������� �����������
	uint16_t				CountCompton;	// [2]		���� �� ��������� �������-�������
} SKLP_GGLP_SADC_DataFastGet_Answer_t;	// [10]
#pragma pack( )

// *****************************************************************************
// [0x12] ����� �� ������ �������� ������ �� ���������, ���������� �� 25 ��
// *****************************************************************************
#pragma pack( 1 )
typedef struct SKLP_GGLP_SADC_DataFastSpectrumGet_Answer_struct
{
	SKLP_GGLP_SADC_DataFastGet_Answer_t	Summary;		// [10]	����� ������ �� ����������� ������, ��� ��������
	uint8_t	aSpectrum[GGLP_SDAC_SPECTRUM_SIZE_SHORT];	// [256]	������ �� ��������
} SKLP_GGLP_SADC_DataFastSpectrumGet_Answer_t;	// [266]
#pragma pack( )

// *****************************************************************************
// [0x16] ����� �� ������ �������� ������, ���������� �� 6000 ��
// *****************************************************************************
#pragma pack( 1 )
typedef struct SKLP_GGLP_SADC_DataSlowGet_Answer_struct
{
	GGLP_SADC_FlagsModule_t	FlagsModule;	// [1]		����� �����
	GGLP_SADC_FlagsResult_t	FlagsResult;	// [1]		����� ���������� ��������� �����
	uint16_t				TimeAccum_ms;	// [2]		����� ���������� ����������
	uint16_t				CountTotal;		// [2]		���� ����� (������)
	uint16_t				CountPhoto;		// [2]		���� �� ��������� �����������
	uint16_t				CountCompton;	// [2]		���� �� ��������� �������-�������
} SKLP_GGLP_SADC_DataSlowGet_Answer_t;	// [10]
#pragma pack( )
//typedef SKLP_GGLP_SADC_DataFastGet_Answer_t SKLP_GGLP_SADC_DataSlowGet_Answer_t;	// [10]

// *****************************************************************************
// [0x15] ����� �� ������ �������� ������ �� ���������, ���������� �� 6000 ��
// *****************************************************************************
#pragma pack( 1 )
typedef struct SKLP_GGLP_SADC_DataSlowSpectrumGet_Answer_struct
{
	SKLP_GGLP_SADC_DataSlowGet_Answer_t	Summary;		// [10]	����� ������ �� ����������� ������, ��� ��������
	uint16_t aSpectrum[GGLP_SDAC_SPECTRUM_SIZE_SHORT];	// [512]	������ �� ��������
} SKLP_GGLP_SADC_DataSlowSpectrumGet_Answer_t;	// [522]
#pragma pack( )


// *****************************************************************************
// [0x11] ������ �� ��������� ������ ������
// *****************************************************************************
#pragma pack( 1 )
typedef struct SKLP_GGLP_SADC_ModeSet_Query_struct
{
	uint8_t					Mode;			// [1]			����� (0x01 - ���������� ������)
	uint16_t				Vref_mV;		// [2]	[��]	��������� ���������� VDDA/Vref, 0xFFFF - auto
	uint16_t				Vcomp_mV;		// [2]	[��]	��������� ���������� ������ �����������, 0xFFFF - auto
	uint16_t				UHV;			// [2]	[�]		������������� ���������� ������� ��� (���)
} SKLP_GGLP_SADC_ModeSet_Query_t;		// [7]
#pragma pack( )

#pragma pack( 1 )
typedef struct SKLP_GGLP_SADC_ModeSet_Answer_struct
{
	GGLP_SADC_FlagsModule_t	Flags;			// [1]			�����
} SKLP_GGLP_SADC_ModeSet_Answer_t;		// [1]
#pragma pack( )

// *****************************************************************************
// [0x14] ����� �� ������� ������� ����� ��������������� ������
// *****************************************************************************
#pragma pack( 1 )
typedef struct SKLP_GGLP_SADC_DataTechGet_Answer_struct
{
	GGLP_SADC_FlagsModule_t	Flags;			// [1]			�����
	uint8_t					TempProbe;		// [1]	[?]		����������� �����������
	uint8_t					TempMicro;		// [1]	[?]		����������� ����������������
	uint16_t				Vref_mV;		// [2]	[��]	������������ ���������� VDDA/Vref
	uint16_t				Vcomp_mV;		// [2]	[��]	������������ ���������� ������ �����������
	// ������������ ��� ��������� ��������� ����� � ������������ �������������� �������
	float					KE;				// [4]	[���/���]	����������� �������� �� ���� ��� � �������
	float					Emin;			// [4]	[���]	������� � ������ ������ �������
	float					dE;				// [4]	[���]	�������� ������� � ����� ������ �������
	float					aEp[2];			// [8]	[���]	�������� ������� �����������
	float					aEc[2];			// [8]	[���]	�������� ������� �������-�������
} SKLP_GGLP_SADC_DataTechGet_Answer_t;	// [35]
#pragma pack( )

#endif	// SKLP_INTERFACE_GGLP_SADC_H
