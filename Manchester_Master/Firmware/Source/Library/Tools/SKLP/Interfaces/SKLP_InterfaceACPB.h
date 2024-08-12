// SKLP_InterfaceACPB.h
// ��������� ��������� ������� ���(�) �� ���� ���:
// - ���.619.00.04.00 - ���������� ������������� ����������� � ������� ���(�)	���.619.00.00.00
// - ���.623.00.02.00 - ���������� ������������� ����������� � ������� ���		���.623.00.00.00
// - ���.624.00.04.00 - ���������� ������������� ����������� � ������� ���/���	���.624.00.00.00
// - ���.625.00.02.00 - ���������� ������������� ����������� � ������� ���		���.625.00.00.00
// - ���.513.00.02.00 - ���������� ������������� ����������� � ������� ���(�)	���.513.00.00.00
// �������� ���������� �:
// - ���.435.00.01.00 - ���������� ��������������� ����������� � ������� ���	���.679.00.00.00
#ifndef	SKLP_INTERFACE_ACPB_H
#define	SKLP_INTERFACE_ACPB_H

#include <stdint.h>
#include "SKLP_Service.h"
#include "SKLP_MS_Transport.h"
#include "SKLP_Time.h"

// ������ ��������� �� ���� ���, ������ � SKLP_MS_Transport.h
// SKLP_ADDRESS_ACP_679		// [0xB0]
// SKLP_ADDRESS_ACPB_623	// [0xB1]	� �.�. 625
// SKLP_ADDRESS_ACPB_619	// [0xB2]
// SKLP_ADDRESS_ACPB_624	// [0xB1]
// SKLP_ADDRESS_ACPA_513	// [0xB1]

// ��� ���������� ��� ������� [0x01].v2 (����������� � ���� �������	 �� �������� ����������)
#define SKLP_DEVICE_TYPE_ACPB_619		( ( LoochDeviceType_t ) 0x6190 )
#define SKLP_DEVICE_TYPE_ACPB_623		( ( LoochDeviceType_t ) 0x6230 )
#define SKLP_DEVICE_TYPE_ACPB_624		( ( LoochDeviceType_t ) 0x6240 )
// ���������� ���.435.00.01.00 �������� � ������� ��������� ���-� �� ���� RS-485, ��� ������������ ������� [0x01].v1, ��� ���� ����������
// ���������� ���.513.00.02.00 �������� � ������� ��������� ���-� �� ���� RS-485, ��� ������������ ������� [0x01].v1, ��� ���� ����������

// ��������� � ��������� _ACP - ��� ��������� ������� ������� ���(�) 619
// ��������� ��� �������� - ��� ���� �������� ����� �����, ��� ���(�), ��� � ���(�)
// ���������� ��������� � ������������� ������ �����
#define	ACPB_OSC_FRAME_SIZE				2048
// ���������� �������� ������������� �������
#define	ACPB_CHANNEL_COUNT_MAIN_679		8
#define	ACPB_CHANNEL_COUNT_MAIN_619		3
#define	ACPB_CHANNEL_COUNT_MAIN_623		1
#define	ACPB_CHANNEL_COUNT_MAIN_624		3
#define	ACPB_CHANNEL_COUNT_MAIN_513		3
#define	ACPB_CHANNEL_COUNT_MAIN_ACP		ACPB_CHANNEL_COUNT_MAIN_679		// 8
#define	ACPB_CHANNEL_COUNT_MAIN			ACPB_CHANNEL_COUNT_MAIN_619		// 3
// ���������� ������� �������
#define	ACPB_CHANNEL_COUNT_REF_679		1
#define	ACPB_CHANNEL_COUNT_REF_619		0
#define	ACPB_CHANNEL_COUNT_REF_623		0
#define	ACPB_CHANNEL_COUNT_REF_624		0
#define	ACPB_CHANNEL_COUNT_REF_513		0
#define	ACPB_CHANNEL_COUNT_REF_ACP		ACPB_CHANNEL_COUNT_REF_679		// 1
#define	ACPB_CHANNEL_COUNT_REF			ACPB_CHANNEL_COUNT_REF_619		// 0
// ���������� ��������� ������� (��� �������� �������������� ���������)
#define	ACPB_CHANNEL_COUNT_AUX_679		1
#define	ACPB_CHANNEL_COUNT_AUX_619		1
#define	ACPB_CHANNEL_COUNT_AUX_623		1
#define	ACPB_CHANNEL_COUNT_AUX_624		1
#define	ACPB_CHANNEL_COUNT_AUX_513		1
#define	ACPB_CHANNEL_COUNT_AUX_ACP		ACPB_CHANNEL_COUNT_AUX_679		// 1
#define	ACPB_CHANNEL_COUNT_AUX			ACPB_CHANNEL_COUNT_AUX_619		// 1
// ����� ����� ������� � ������
#define ACPB_CHANNEL_COUNT_ALL_ACP		( ACPB_CHANNEL_COUNT_MAIN_ACP + ACPB_CHANNEL_COUNT_REF_ACP + ACPB_CHANNEL_COUNT_AUX_ACP )	// 10
#define ACPB_CHANNEL_COUNT_ALL			( ACPB_CHANNEL_COUNT_MAIN + ACPB_CHANNEL_COUNT_REF + ACPB_CHANNEL_COUNT_AUX )				// 4
// ������������� ������� � ������� ��������� �������� ������
#define	ACPB_DATAHEADER_ID_ACP			0x01
#define	ACPB_DATAHEADER_ID				0x20

// ���������� ������������ ��������� ����������� �� ���� ������
#define	ACPB_TF_STEP_COUNT				16		// ������ ������ �� 22.5 �������

// ������������������ �������
typedef enum SKLP_Command_ACPB_enum
{
	SKLP_COMMAND_ACPB_CONTROL				= 0x60,								// [0x60]	����������������� ������� ��� ��� �������, � ������ ��� ���� SKLP_COMMAND_AKP_CUSTOM
	SKLP_COMMAND_ACPB_DATA_GET_OSC			= SKLP_COMMAND_DATA_ACQ_GET_LONG,	// [0x03]	������ ������� ������ (������������)
	SKLP_COMMAND_ACPB_DATA_GET_OSC_FRAGMENT	= 0x67,								// [0x67]	������ ������� ������ (������������), �����������
	SKLP_COMMAND_ACPB_DATA_GET_INSTANT		= SKLP_COMMAND_DATA_ACQ_GET,		// [0x13]	������ ���������� ������
	SKLP_COMMAND_ACPB_DATA_GET_INTEGRAL 	= 0x15,						 		// [0x15]	������ ������� ������������ ������, � ����������� ������������
	SKLP_COMMAND_ACPB_DATA_GET_DIR_EXT		= 0x19,								// [0x19]	������ ����������� ������ � �������� ����������
} SKLP_Command_ACPB_t;

// ����� ������ ���(�)
#pragma pack( 1 )
typedef union SKLP_FlagsModuleACPB_union
{
	SKLP_FlagsModule_t Common;
	struct
	{
		uint8_t	fReserved0			:1; 	// ���� ����������� ������. �������������� ����������. ������ �� ������ ������� ���� ����, � �.�. ��������� (??!!)
		uint8_t	fDataNotReady		:1; 	// ���� ���������� �������������� ������ ��� ���������� �� ������� [0x13]. ������������ ��� ���������� ���������� ���������� ������. ������������ ��� ��������� ������ [0xFF] � [0x13].
		uint8_t	fDataAcqFail 		:1; 	// ���� ��������� ���������� ����� ��������� �� ������� [0xFF].
		uint8_t	fGenHV_Fail			:1; 	// ���� ��������� ������ � �������������� ����������� � ���� ���������� �����
		uint8_t	fReboot				:1; 	// ��������� ���������� �� assert() ��� HardFault
		uint8_t	fReserved			:1;
		uint8_t fTF_Ready			:1;		// ���� ���������� ����������� �����������
		uint8_t	fTFG_nTFM			:1;		// ���� ��������� �����������: 0 - TFM (���������), 1 - TFG (��������������)
	};
} SKLP_FlagsModuleACPB_t;
#pragma pack( )


// ���������� �������� � ������� �� ������� ������� �� ��������� ���

// *********************************************************************************
// [0x13] ������� ������ ���������� ������ ��� ���(�), ������ ���������� �� ������� ��� ���(�)
// *********************************************************************************
// SKLP.[0x13]	��������� ���������� ������ ��� ������� [0x13] ��� ������� ���(�)
#pragma pack( 1 )
typedef struct ACPA_ResultInstant13_struct
{
	uint16_t	aGaps[ACPB_CHANNEL_COUNT_ALL_ACP];	// [��*0.1]	������������ ���������� �� ������ ��������
	uint16_t	SpeedOfSound;						// [�/�]	�������� ����� � �����, ����������� �� �������� ������
	uint16_t	Diameter;							// [��*0.1]	������� ��������
} ACPA_ResultInstant13_t;	// [24]
#pragma pack( )

// ��������� ������ ������ ���(�) �� ���-������� [0x13] "������ ���������� ������"
#pragma pack( 1 )
typedef struct SKLP_ACPA_DataInstantAnswer_struct
{
	SKLP_FlagsModuleACPB_t		FlagsModule;
	SKLP_MemoryFlags_t			MemoryFlags;
	ACPA_ResultInstant13_t		Data;
 	int16_t						Temp;			// [���]		����������� ������
} SKLP_ACPA_DataInstantAnswer_t;
#pragma pack( )

// *********************************************************************************
// [0x13] ������� ������ ���������� ������ (������ ���������� �� ACP_FastResult_t ������� ��� 679)
// *********************************************************************************
// SKLP.[0x13]	��������� ���������� ������ ��� ������� [0x13]
#pragma pack( 1 )
typedef struct ACPB_ResultInstant13_struct
{
	uint8_t		TF;		// [2��/���]	���� �����������
	uint8_t		R0;		// [1��/���]	������ �� ������� �����
	uint8_t		A0;		// [�.�.]		��������� ������� �� ������� �����
	uint8_t		R1;		// [1��/���]	������ �� ������� �����
	uint8_t		A1;		// [�.�.]		��������� ������� �� ������� �����
	uint8_t		R2;		// [1��/���]	������ �� �������� �����
	uint8_t		A2;		// [�.�.]		��������� ������� �� ������� �����
	uint8_t		Time;	// [20��/���]	����� ���������� � ���� ������� ����������� (22.5��) � ���������� �������
} ACPB_ResultInstant13_t;	// [8]
#pragma pack( )

// SKLP.[0x13]	 ��������� ������ ������ ���(�) �� ������� "������ ���������� ������"
#pragma pack( 1 )
typedef struct SKLP_ACPB_DataInstantAnswer_struct
{
	SKLP_FlagsModuleACPB_t		FlagsModule;	// [1]
	SKLP_MemoryFlags_t			MemoryFlags;	// [1]
	ACPB_ResultInstant13_t		Data;			// [8]
} SKLP_ACPB_DataInstantAnswer_t;	// [10]
#pragma pack( )

// *********************************************************************************
// [0x15] ������� ������ ������� ������������ ������, � ����������� ������������
// *********************************************************************************
// SKLP.[0x15]	������� ������������ ������, � ��������� � ���� �����������
#pragma pack( 1 )
typedef struct ACPB_ResultIntergral_struct
{
	struct
	{
		uint16_t	RadiusAvg;	// [2]	[0.1��/���]	����������� ������, ����������� �� ����� ���������� ������ �� ���� �����������
		uint16_t	TimeSumm;	// [2]	[1��/���]	����� ���������� ������ �� ���� �����������
	} aTableByTF[ACPB_TF_STEP_COUNT];
} ACPB_ResultIntergral_t;	// [64]
#pragma pack( )

// SKLP.[0x15]	��������� ���������� ������ ��� ������� [0x15]
#pragma pack( 1 )
typedef struct ACPB_ResultInstant15_struct
{
	uint16_t	TF;				// [2]	[1��/���]	���� �����������
	uint16_t	Diam;			// [2]	[0.1��/���] ������� ��������
	uint16_t	R0;				// [2]	[0.1��/���] ������ ������
	uint16_t	R1;				// [2]	[0.1��/���] ������ ������
	uint16_t	R2;				// [2]	[0.1��/���] ������ ������
	uint16_t	SndSpd;			// [2]	[�/�]		�������� ����� � �����
	SKLP_Temp_t	Temp;			// [2]	[���]		����������� ������
} ACPB_ResultInstant15_t;	// [14]
#pragma pack( )

// SKLP.[0x15]	 ��������� ������ ������ ���(�) �� ���-������� "������ ������� ������������ ������"
#pragma pack( 1 )
typedef struct SKLP_ACPB_DataIntergralAnswer_struct
{
	SKLP_FlagsModuleACPB_t		FlagsModule;	// [1]
	SKLP_MemoryFlags_t			MemoryFlags;	// [1]
	ACPB_ResultIntergral_t		DataIntegral;	// [64]
	ACPB_ResultInstant15_t		DataInstant;	// [14]
} SKLP_ACPB_DataIntegralAnswer_t;		// [80]
#pragma pack( )
// *********************************************************************************

// *********************************************************************************
// [0x19] ������� ������ ����������� ������ � �������� ����������
// *********************************************************************************
// SKLP.[0x19]	��������� ������ ������ ���(�) �� ���-������� "������ ����������� ������ � �������� ����������"
#pragma pack( 1 )
typedef struct ACPB_DataGetDirExt_Answer_struct
{
	SKLP_FlagsModuleACPB_t	FlagsModule;	// [1]
	SKLP_MemoryFlags_t		MemoryFlags;	// [1]
	float	aAccel[3];		// [12]	[G]		���������
	float	aGyro[3];		// [12]	[rps]	�������� ��������
	float	aMagnet[3];		// [12]	[uT]	�������� ���������� ����
	float	Temp;			// [4]	[degC]	����������� MPU9250
	float	aDirQuat[4];	// [16]	[+-1]	���������� �����������
	float	ZENI;			// [4]	[0-180]	���� ��������
	float	AZIM;			// [4]	[0-360]	���� ������������
	float	TFG;			// [4]	[0-360]	���� ����������� ���������������
	float	TFM;			// [4]	[0-360]	���� ����������� ����������
} ACPB_DataGetDirExt_Answer_t;	// [74]
#pragma pack( )
// *********************************************************************************


// *********************************************************************************
// ���� �������� ������
// *********************************************************************************
// ����� ��� ������������ ������������ ������ ���� ������� � ������� ����� �������� ������,
// ��� �������� �� �������� [0x03] � [0x67], � ��� ���������� �� SD.
// ��������� ������������ ���������� ��������� ACPB_OscBuffer_t,
// �� ����� �� ������������ ��������� ��� ��������������� ��������� ���������.
// ��� ����������� ACPB_OscBuffer_t -> ACPB_OscBufferPacket_t ���������� ��������������, ��� �� ������� ���������.
#pragma pack( 1 )
typedef struct ACPB_OscBufferPacket_struct
{
	uint16_t	aBuffer[ACPB_CHANNEL_COUNT_ALL][ACPB_OSC_FRAME_SIZE];	// [2*4*2048]
} ACPB_OscBufferPacket_t;	// [16K]
#pragma pack( )

// ��������� ����� �������� ������
#pragma pack( 1 )
typedef struct ACPB_DataSaveHeader_struct
{
	// ����, ��������� ��������������� ��������� ACP_679
	uint8_t					ID;					// [1]	0x20	������ ���������
	uint16_t				HeaderDataSize;		// [2]			������ ��������� � ����� ������ - ��� ��������� ��� � CRC16
	uint32_t				CycleCounter;		// [4]	[0++]	������� ������ ��������� - ���������� ��� ��������� ������
	OADate_t				DateTime;			// [8]	[��.]	����� ������ ��������� ����������� ����� �����
	// ���� ��� �������� ACPB_6xx
	uint32_t				TickCounter;		// [2]	[��]	���� ����������� �� �������
	// - ������� � ������ �������
	// - ������� �� ������������
	// - ������������ ����, � �.�. �������� ������?
	// - ������� � ������ �������
	// - ������ ���������������
	// - ����������
	// - ???
} ACPB_DataSaveHeader_t;	// [??]
#pragma pack( )

// ���� �������� ������ ��� ���������� �� SD � ������ �� ������� [0x03] � [0x67]
#pragma pack( 1 )
typedef struct ACPB_DataSave_struct
{
	SKLP_MemStruct_DataHeader_t	SKLP_Header;	// [15]		��������� �� ��������� ���
	ACPB_DataSaveHeader_t		ACPB_Header;	// [??]		��������� ���(�)
	uint8_t						aPadding[ SKLP_MEMSECTORSIZE - sizeof( SKLP_Header ) - sizeof( ACPB_Header ) - sizeof( uint16_t ) ];
	ACPB_OscBufferPacket_t		ACPB_Osc;		// [16K]	�������������
	uint16_t					CRC16;			// [2]
} ACPB_DataSave_t;	// [16.5K]
#pragma pack( )

// *********************************************************************************
// [0x03] ������� ������ ������� ������ (������������)
// *********************************************************************************
// SKLP.[0x03]	������ ����� �� �������, ������� ����������
#pragma pack( 1 )
typedef struct SKLP_ACPB_DataOscAnswerFrame_struct
{
	uint8_t				SKLP_Start;		// [1]
	uint8_t				SKLP_Size;		// [1]
	ACPB_DataSave_t		Data;			// [16.5K]
	uint8_t				SKLP_CRC8;		// [1]
} SKLP_ACPB_DataOscAnswerFrame_t;		// [16'899]
#pragma pack( )

// *********************************************************************************
// [0x67] ������� ������ ������� ������ (������������) �����������.
// ������� ��������� ���������� ������� ����� ����������� ��� ������ ��������� ����� ������,
// ��� ���������� ��� ���������� ������ �����.
// *********************************************************************************
#define	SKLP_LONGDATAANSWER_FRAGMENT_SIZE		512		// ������ ���������

// SKLP.[0x67]	��������� ������� ������� "������� ����� ������� [0x03] �� ������"
#pragma pack( 1 )
typedef struct SKLP_LongDataFragmentQuery_struct
{
	uint8_t	iFragment;
} SKLP_LongDataFragmentQuery_t;
#pragma pack( )

// SKLP.[0x67]	��������� ������ �� ������� "������� ����� ������� [0x03] �� ������", ������� ����������
#pragma pack( 1 )
typedef struct SKLP_LongDataFragmentAnswerFrame_struct
{
	uint8_t	Start;
	uint8_t	Size;
	uint8_t aData[SKLP_LONGDATAANSWER_FRAGMENT_SIZE];
	uint8_t	CRC8;
} SKLP_LongDataFragmentAnswerFrame_t;
#pragma pack( )

// *********************************************************************************
// [0x04]	������� ������ ������������� ���������
// [0x05]	������� ������ ������������� ���������
// *********************************************************************************
// ��������� ��������, ��������� � ��������� ���������.
// �� �������� � EEPROM.
#pragma pack( 1 )
typedef struct ACPB_Parameters_struct
{
	// ��������� �� ������� ACP.679, �������������� � ������� ACPB.6xx
	uint8_t		ID;						// [1]			0x20	������
	uint8_t		Size;					// [1]	[����]	?28?	������ ���������
	uint16_t	SampleRate_kSPS;		// [2]	[kSPS]	4xxx	������� ������������� ���  (������� �� �������� ������ �����������)
	uint16_t 	SamplesCount;			// [2]			2048	���������� ��������� �� ������ �����
	uint8_t 	ChannelCount;			// [1]			4		���������� ������� ��������� (3+1)
	uint8_t		SampleDataSize;			// [1]	[���]	12		������ ������������ ���������
	// ��������� �� ������� ACP.679, ����������� ��� ��������� ��
	int16_t		StartSamplingDelay_ns[ACPB_CHANNEL_COUNT_ALL_ACP];		// [2x10]	[��]	�������� ����� ����������� ��������� � �������� ��������� (� ������ ������� ���� �������������� � ������� ��� ������ �������, � ���� ������� ������������� � ���������� ��� ���� �������
} ACPB_Parameters_t;			// [28]
#pragma pack( )

// ��������� ������������� ������������� ��� ���������� ��������
#pragma pack( 1 )
typedef union ACPB_Calibration_union
{
	float aVector[2*ACPB_CHANNEL_COUNT_MAIN];
	struct
	{
		float aK[2];		// ������� 1-� ������� ( K0 + K1*x ) 
	} aChannels[ACPB_CHANNEL_COUNT_MAIN];	// ���������� �� ��� ������, � ACP.679 ���� �� 8 �������
} ACPB_Calibration_t;			// [24]
#pragma pack( )

// SKLP.[0x04]	��������� ������ ������ ���(�) �� ������� "������ �������������"
#pragma pack( 1 )
typedef struct SKLP_ACPB_NVMGetAnswer_struct
{
	SKLP_FlagsModuleACPB_t	FlagsModule;	// [1]
	ACPB_Parameters_t		Parameters;		// [28]
	ACPB_Calibration_t		Calibration;	// [24]
} SKLP_ACPB_NVMGetAnswer_t;	// [53]
#pragma pack( )

// SKLP.[0x05]	��������� ������� ������� "������ �������������"
#pragma pack( 1 )
typedef struct SKLP_ACPB_NVMSetQuery_struct
{
	char					aSignature[ sizeof( SKLP_SIGNATURE_COMMAND_NVM_SET ) - 1 ];
	ACPB_Parameters_t		Parameters;		// ������������
	ACPB_Calibration_t		Calibration;	// �������� � EEPROM
} SKLP_ACPB_NVMSetQuery_t;
#pragma pack( )

// SKLP.[0x05]	��������� ������ ���(�) �� ������� "������ �������������"
typedef SKLP_ACPB_NVMGetAnswer_t SKLP_ACPB_NVMSetAnswer_t;

#endif	// SKLP_INTERFACE_ACPB_H
