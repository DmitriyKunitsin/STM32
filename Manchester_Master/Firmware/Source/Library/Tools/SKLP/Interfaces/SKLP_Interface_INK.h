// SKLP_Interface_INK.h
// ��������� ��������� ��� (�����, ���������-���������� �������) �� ���� ���
// ���������� ���.636.00.01.00
#ifndef	SKLP_INTERFACE_INK_H
#define	SKLP_INTERFACE_INK_H

#include <stdint.h>
#include "SKLP_Time.h"				// SKLP_Time_t
//#include "NVM.h"					// NVM_Result_t
#include "SKLP_MS_Transport.h"		// SKLP_Signature
#include "SKLP_Service.h"			// SKLP_FlagsModule_t
#include <stdint.h>

#define	INK_TIMING_ND_WINDOWS_COUNT_SKLP	60	// 58?			���������� ���� ������ ��������� ��� �������� �� SKLP (����� ����� ���� ���������, ����� ������� � ������ ������)

// �������������� ���� ������
typedef uint16_t INK_ND_CntSumm_t;	// [�����]	����� ������ ��������� � �����
//typedef uint16_t INK_GK_ChargeSumm_t;	// [ADC]	����� ����������� �����

// ������� ������ ����� � ���������
typedef enum INK_Probes_enum
{
	iINK_ProbeNear = 0,		// ������� ����
	iINK_ProbeFar,			// ������� ����
	iINK_ProbesTotal,		// ����� ������
} INK_Probes_t;

// ������������� ������� ��������� ��� ��� ������ ��� 
typedef enum SKLP_Command_INK_enum
{
	SKLP_COMMAND_INK_DATA_TECH_SET			= 0x11,		// ��������� ��������������� ����������
	SKLP_COMMAND_INK_DATA_MAIN_GET			= 0x13,		// ������ ����� �������� ������, ����� ������ ����������� � �������� ������
	SKLP_COMMAND_INK_DATA_TECH_GET			= 0x14,		// ������ ����� ��������������� ������
	SKLP_COMMAND_INK_DATA_ACCUM_GET			= 0x15,		// ������ ����� ����������� ������
	SKLP_COMMAND_INK_DATA_ASCII_GET 		= 0x16, 	// ������ ����� ����������� ������ � ������� ASCII, ��  ������������� SKLP!
	SKLP_COMMAND_INK_START_SINGLE			= 0xFF, 	// ������ ������������� ����� ���������
} SKLP_Command_INK_t;

// ����� ���
#pragma pack( 1 )
typedef union SKLP_FlagsModuleINK_union
{
	uint8_t Byte;
	struct
	{
		uint8_t	fDataSaving			:1;		// ���������� ����� (�� ������������)
		uint8_t fWorkEnable			:1;		// ���������� ������ �� ������� ������������� �������
		uint8_t fTF_MnG				:1;		// ������� ���������� (0 - ��������������, 1 - ���������)
		uint8_t fFailPower			:1;		// ����� �� �������
		uint8_t fFailPulse			:1;		// ����� ������������� ���������� ���������
		uint8_t fFailGamma			:1;		// ����� ���������� �����
		uint8_t fFailNeutron		:1;		// ����� ���������� ���������
		uint8_t fFailACPTF			:1;		// ����� ������������� ����������� ��� ���������� ���� ����������� 
	};
} SKLP_FlagsModuleINK_t;
#pragma pack( )

// *****************************************************************************
// [0x13] ����� �� ������� ������� ����� �������� ������, ����� ������ ����������� � �������� ������
// *****************************************************************************
#pragma pack( 1 )
typedef struct SKLP_INK_DataMainGet_Answer
{
	SKLP_FlagsModuleINK_t	Flags;			//	[1]			�����
	uint16_t				RHOB;			// [2]	[??]	���������
	uint16_t				TRNP;			// [2]	[??]	�����������
	uint8_t					PulseCnt;		// [1]			������� ���������, �� ������� ��������� ������
	uint8_t					ACP_TF;			// [1]	[x2��]	��������� �����������
	uint8_t					ACP_R;			// [1]	[��]	������
	uint8_t					ACP_A;			// [1]	[�.�.]	��������� ������������� �������
	uint8_t					Temp;			// [1]	[���+55]	�����������
} SKLP_INK_DataMainGet_Answer_t;	// [10]
#pragma pack( )

// *****************************************************************************
// [0x11] ������ �� ��������� ��������������� ����������
// *****************************************************************************
#pragma pack( 1 )
typedef struct SKLP_INK_DataTechSet_Query
{
	uint16_t				UHV_Pulse;		// [2]	[�]		���������� ������� �� �������������� ��������� ���������� ���������
	uint16_t				UHV_GK;			// [2]	[�]		���������� ������� �� �������������� ��������� ��������� �����
	uint16_t				UHV_ND;			// [2]	[�]		���������� ������� �� �������������� ��������� �������� ���������
} SKLP_INK_DataTechSet_Query_t;	// [6]
#pragma pack( )

// *****************************************************************************
// [0x14] ����� �� ������� ������� ����� ��������������� ������
// *****************************************************************************
#pragma pack( 1 )
typedef struct SKLP_INK_DataTechGet_Answer
{
	SKLP_FlagsModuleINK_t	Flags;			//	[1]			�����
	uint16_t				UHV_Pulse;		// [2]	[�]		���������� ������� �� �������������� ��������� ���������� ���������
	uint16_t				UHV_GK;			// [2]	[�]		���������� ������� �� �������������� ��������� ��������� �����
	uint16_t				UHV_ND;			// [2]	[�]		���������� ������� �� �������������� ��������� �������� ���������
	uint8_t					TimeReady;		// [1]	[��]	����� ���������� ���������� ��������� � ������
	uint16_t				PulseCntTotal;	// [2]			������� ��������� �� ������� ���������
	uint8_t					Temp;			// [1]	[���+55]	�����������
} SKLP_INK_DataTechGet_Answer_t;	// [9]
#pragma pack( )

// *****************************************************************************
// [0x15] ����� �� ������� ������� ����� ����������� ������
// *****************************************************************************
#pragma pack( 1 )
typedef struct SKLP_INK_DataAccumGet_Answer
{
	SKLP_FlagsModuleINK_t	Flags;								//	[1]			�����
	uint8_t					PulseCnt;							// [1]			������� ���������, �� ������� ��������� ������
	uint16_t				aGammaIdle_mV[iINK_ProbesTotal];	// [4]	[��]	����������� ���������� �� ������������ ����� ����� ���������
	uint16_t				aGammaPulse_mV[iINK_ProbesTotal];	// [4]	[��]	����������� ���������� �� ������������ ����� ����� ��������
	INK_ND_CntSumm_t		aNeutronCnt[INK_TIMING_ND_WINDOWS_COUNT_SKLP][iINK_ProbesTotal];	// [240]	[�.�.]	����� ������ ��������� ��-������
} SKLP_INK_DataAccumGet_Answer_t;	// [250]
#pragma pack( )

//STATIC_ASSERT( sizeof( SKLP_INK_DataAccumGet_Answer_t ) <= SKLP_SIZE_DATA_MAX_ANSWER );

#endif	// SKLP_INTERFACE_INK_H
