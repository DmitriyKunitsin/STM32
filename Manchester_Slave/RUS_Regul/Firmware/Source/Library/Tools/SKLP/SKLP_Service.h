// SKLP_Service.h
// �������� ���������������� ���� ���, ������������ �������� �������.
#ifndef	SKLP_SERVICE_H
#define	SKLP_SERVICE_H

#include <stdint.h>
#include <stdbool.h>
#include "SKLP_Time.h"		// ���� � �����

// ************** ��������� ����� ������, ������������ � ��������� ��� **********************
// ������������ �����
typedef uint16_t SKLP_WorkTimeCounter_t;	// ������� �������������

// ����� ������
// ��������������, ��� ����� ������������� ������ �������� �������, � �� SD-�����
#pragma pack( 1 )
typedef struct SKLP_MemoryFlags_struct
{
	// ���� ������� ����������� ������ � ������
	uint8_t	SD0_DataPresent		:1;			// ��������� ������ � ������� �������� ������ (�� ��������� � ������� ����� � ��������������� ������)
	// ��������� ��������� �������� ��������
	uint8_t	SD0_ErrorMemInit	:1;			// ������������ �������� �������
	uint8_t	SD0_ErrorMemWrite	:1;			// ������ ��� ������ ����� ��� ���������� ����� �� ������
	uint8_t	SD0_ErrorMemRead	:1;			// ������ ��� ������ ����� ��� ���������� ����� �� ������
	// ��� ������ �������� �� ������������
	uint8_t	SD1_DataPresent		:1;
	uint8_t	SD1_ErrorMemInit	:1;
	uint8_t	SD1_ErrorMemWrite	:1;
	uint8_t	SD1_ErrorMemRead	:1;
} SKLP_MemoryFlags_t;
#pragma pack( )

// ����� ������������� ��������� ������
#pragma pack( 1 )
typedef struct SKLP_MemoryInit_struct
{
	uint8_t	SD0_ErrorRead		:1;
	uint8_t	SD0_ErrorCRC		:1;
	uint8_t	SD0_Reserved0		:1;
	uint8_t	SD0_Reserved1		:1;
	uint8_t	SD1_ErrorRead		:1;
	uint8_t	SD1_ErrorCRC		:1;
	uint8_t	SD1_Reserved0		:1;
	uint8_t	SD1_Reserved1		:1;
} SKLP_MemoryInit_t;
#pragma pack( )

// ����� ���������. ��������� � ����������� ����������, �� ������ ��� �����.
#pragma pack( 1 )
typedef struct SKLP_FlagsModule_struct
{
	uint8_t	DataSaving			:1;		// ���� ����������� ������ - �� ������� [0x13] ��������� ������ � ������
	uint8_t Reserved			:7;		// ���������� ���� ����� � ����������
} SKLP_FlagsModule_t;
#pragma pack( )

// ����� ������ ������.
// ��� ������� ��������� ���������� ������� �� EEPROM, ����� ������, � ����� ������ ���������� ������.
// � ���������� ������� ��������� ���� SKLP_FlagsModule.DataSaving, � ������������ ������ ���� �������.
// � ���������� ������� ��������� ���������� ��������� ������ �� SD.
// � ���������� ������� ������ ��� ���������� ����� �������.
typedef enum SKLP_ModuleMode_enum
{
	SKLP_ModuleMode_NotAuto			= 0,	// ������������ �����
	SKLP_ModuleMode_Auto			= 1,	// ���������� �����
	SKLP_ModuleMode_AutoDrilling	= 6,	// ���������� �����, ������� (���)
	SKLP_ModuleMode_AutoLogging		= 7,	// ���������� �����, ������� (���)
	SKLP_ModuleMode_Unknown			= 0xFF,	
} SKLP_ModuleMode_t;

// ������������� (��������) �������, ������ BCD 0xNNCNCCYY
// YY	- ��� ������������
// CCC	- ����������� ����� �������
// NNN	- ���������� ����� ������ � ����
#pragma pack( 1 )
typedef	union LoochDeviceSerial_union
{
	char		aBytes[4];
	uint32_t	BCD;
	struct
	{
		uint32_t YY		:8;
		uint32_t CC_L	:8;
		uint32_t NN_L	:4;
		uint32_t CC_H	:4;
		uint32_t NN_H	:8;
	};
} LoochDeviceSerial_t;
#pragma pack( )

// ����������� ����� �������� ������ �������
typedef uint16_t LoochDeviceDecimalNumber_t;
//#define	LOOCH_DECIMALNUMBER_MP		265
//!! � �� ����� ���� ������ ����������� ������, �� ���������

/*typedef enum LoochDeviceDecimalNumber_enum
{
	LOOCH_DECIMALNUMBER_MP	= 265,
} LoochDeviceDecimalNumber_t;
*/

// ������������� (��������) �������, ������ ��������
typedef	struct LoochDeviceSerialBin_struct
{
	uint16_t					YearFrom2000;		// YY -> Bin
	LoochDeviceDecimalNumber_t	Decimal;			// CCC -> Bin
	uint16_t					Number;				// NNN -> Bin
} LoochDeviceSerialBin_t;

// ������ � ���� ��������, ������ 0xYYMMDDVV
#pragma pack( 1 )
typedef struct LoochSoftVersion_struct
{
	uint8_t Version;		// [?]
	uint8_t Day;			// [01..31]
	uint8_t Month;			// [01..12]
	uint8_t YearFrom2000;	// [15....]
} LoochSoftVersion_t;
#pragma pack( )

// ������������� ���� �������, ��� ������������� � �������� ��� � �.�., ������ �������
typedef	uint16_t LoochDeviceType_t;	// 0x123V, 123 - ����������� ����� ������� � BCD, V - ������ ������

/*typedef union LoochDeviceType_union
{
	uint16_t Bin;
	struct
	{
		uint16_t Version	:4;
		uint16_t Decimal	:12;
	};
} LoochDeviceType_t;
*/

// ********************** ���������� ��� ���.600 **********************
// ********************************************************************
// ����� ������������ ������� - ������, �������, ������ �����, ���������� �����
typedef uint32_t Looch600_ModulesMask_t;
#pragma pack( 1 )
typedef union Looch600_Modules_union
{
	Looch600_ModulesMask_t Mask;
	struct
	{								//		'*' - ���������� ���������� ���, '=' ������ �� ����� RS-485, '~' - ������ �� �������� �����
		// ������ ������� ��� �������� �� ����������� (��������)
		Looch600_ModulesMask_t Inclin 	:1; 	//			*	�����������
		Looch600_ModulesMask_t Gamma	:1; 	// ��		*	�����
		Looch600_ModulesMask_t VIKPB	:1; 	// �����	~	�����
		Looch600_ModulesMask_t GGKP		:1; 	// ����		~	�����-�����
		// ������ ������� ��� �������� �� ����������� (���������������)
		Looch600_ModulesMask_t NNKT		:1; 	// ����		=	�������-����������
		Looch600_ModulesMask_t BK 		:1; 	// ��		~	�������
		Looch600_ModulesMask_t AK 		:1; 	// ��		~	��������
		Looch600_ModulesMask_t INGK		:1; 	// ����		~	���������-����������
		// ������ �������, ������ �������
		Looch600_ModulesMask_t MP0		:1; 	// ��-0		=	������ �������
		Looch600_ModulesMask_t MP1		:1; 	// ��-1		=	������ �������
		Looch600_ModulesMask_t MP2		:1; 	// ��-2		=	������ �������
		Looch600_ModulesMask_t MP3		:1; 	// ��-3		=	������ �������
		// ��������� �������, ������ ������
		Looch600_ModulesMask_t AKP		:1; 	// ���		~	����������
		Looch600_ModulesMask_t NDM 		:1; 	// ���		~	�����������
		Looch600_ModulesMask_t MPI		:1; 	// ���		*	������ ������ � ������� (���������� ������ ���, ����� ���������� ���������� ��� ��� �������� �� �����������?)
		Looch600_ModulesMask_t MUP		:1; 	// ���		=	������ ���������� �����������
		// ����� ������� � �.�.
		Looch600_ModulesMask_t VIKPB_GK	:1; 	//			~	����� �����, ���� ��������� �� �� PLC
		Looch600_ModulesMask_t MP36_0	:1; 	// ��36-0	=	������ ������� 36 �
		Looch600_ModulesMask_t MP36_1	:1; 	// ��36-1	=	������ ������� 36 �
		Looch600_ModulesMask_t MP36_2	:1; 	// ��36-2	=	������ ������� 36 �
		Looch600_ModulesMask_t MP36_3	:1; 	// ��36-3	=	������ ������� 36 �
		// ������
//		Looch600_ModulesMask_t Reserved	:11;
		Looch600_ModulesMask_t NNKT2	:1; 	// ����2	~	�������-���������� �� ������
		Looch600_ModulesMask_t Reserved	:10;
	};
} Looch600_Modules_t;		// [4]
#pragma pack( )

// �������������� ����� ��������� ���.600
// ����� ������ ����������� ���, ����� ������ ��� (*)
#pragma pack( 1 )
typedef union Looch600_FlagsInfo_union
{
	uint16_t Mask;
	struct
	{
		uint16_t Reserved0		:1;		// 0	
		uint16_t Static			:1;		// 1		����-�����
		uint16_t Rotor			:1;		// 2*	�������� �������
		uint16_t TF_MnG			:1;		// 3*	����������� ����������� (0 - ��������������, 1 - ���������)
		uint16_t Reserved4_7	:4;		// 4..7
		uint16_t Flow1			:1;		// 8		������� ������ (��� �������� �� ����� �����)
		uint16_t Flow2			:1;		// 9		������� ������
		uint16_t Reserved10_15	:6;		// 10..15
	};
} Looch600_FlagsInfo_t;		// [2]
#pragma pack( )


// ************** ���������� �������� � ������� �� ������� �� ��������� ��� ****************

// [0x01]	����� �� ������ �������������� ������
#pragma pack( 1 )
typedef struct SKLP_GetLoochID_Answer_struct
{
	SKLP_FlagsModule_t	FlagsModule;
	LoochDeviceSerial_t Serial;
	LoochSoftVersion_t	SoftVersion;
} SKLP_GetLoochID_Answer_t;
#pragma pack( )

#pragma pack( 1 )
typedef struct SKLP_GetLoochID_v2_Answer_struct
{
	SKLP_FlagsModule_t	FlagsModule;
	LoochDeviceSerial_t Serial;
	LoochDeviceType_t	Type;
	LoochSoftVersion_t	SoftVersion;
} SKLP_GetLoochID_v2_Answer_t;
#pragma pack( )

// [0x02]	������ �� ��������� �������������� ������
#pragma pack( 1 )
typedef struct SKLP_SetLoochID_Query_struct
{
	LoochDeviceSerial_t Serial;			// ����� ��������
	LoochSoftVersion_t	SoftVersion;	// ������ �� ������ ������������
} SKLP_SetLoochID_Query_t;
#pragma pack( )

// [0x02]	����� �� ������ �� ��������� �������������� ������
typedef SKLP_GetLoochID_Answer_t SKLP_SetLoochID_Answer_t;
typedef SKLP_GetLoochID_v2_Answer_t SKLP_SetLoochID_v2_Answer_t;

/*/ !! ����!!
// [0x04], [0x05] ��������� ���������� ������ ������ � ������ NVM
typedef enum NVM_Result_enum
{
	NVM_Result_Ok			= 0,	// �������� ��������� �������
	NVM_Result_FailBlockID	= 1,	// ������������ ������������� �����
	NVM_Result_FailData		= 2,	// ������������ ������ ������ ����� (������, CRC, ����������)
	NVM_Result_FailEEPROM	= 3,	// ������ ��� ������� � EEPROM
} NVM_Result_t;
*/
// [0x07]	������ ������������� �������
#pragma pack( 1 )
typedef struct SKLP_WorkTimeSetQuery_struct
{
	char					Signature;
	SKLP_WorkTimeCounter_t	Counter;
} SKLP_WorkTimeSetQuery_t;
#pragma pack( )

// [0x07]	����� �� ������ ������������� �������
#pragma pack( 1 )
typedef struct SKLP_WorkTimeAnswer_struct
{
	SKLP_FlagsModule_t		FlagsModule;
	char					Signature;
	SKLP_WorkTimeCounter_t	WorkTimeCounter;
} SKLP_WorkTimeAnswer_t;
#pragma pack( )

#pragma pack( 1 )
typedef struct SKLP_WorkTimeAnswer_v2_struct
{
	SKLP_FlagsModule_t		FlagsModule;
	char					Signature;
	SKLP_WorkTimeCounter_t	WorkTimeCounter;
	uint16_t				CRC16;
} SKLP_WorkTimeAnswer_v2_t;
#pragma pack( )

// [0x21]	����� �� ������ �������� ��������� ������
#pragma pack( 1 )
typedef struct SKLP_MemoryGetStateAnswer_struct
{
	SKLP_FlagsModule_t	FlagsModule;
	SKLP_MemoryFlags_t	MemoryFlags;
} SKLP_MemoryGetStateAnswer_t;
#pragma pack( )

// [0x37, 0x3F]	������ �� ������������� �������
#pragma pack( 1 )
typedef struct SKLP_TimeSinchronization1_Query_struct
{
	SKLP_Time_t	Time;
} SKLP_TimeSinchronization1_Query_t;
#pragma pack( )

#pragma pack( 1 )
typedef struct SKLP_TimeSinchronization2_Query_struct
{
	SKLP_Time_t	Time;
	uint8_t		aDummy[5];
} SKLP_TimeSinchronization2_Query_t;
#pragma pack( )

// [0x3F]	������ �� ������������� �������, ������ ���������� ������� � �������� ����� ������
// ��� ��������� ���.600
#pragma pack( 1 )
typedef struct SKLP_TimeSinchronization600_Query_struct
{
	SKLP_Time_t				Time;			// [7]	����� ���������
	Looch600_Modules_t		ModulesWork;	// [4]	������, ������� ������ ���� ��������
	// �������������
	Looch600_FlagsInfo_t	FlagsInfo;		// [2]	����� ��������������
	int16_t					FrqRot;			// [2]	[0.1 ��/���]	������� �������� �������
	uint16_t				Toolface;		// [2]	[0.01 deg]	�������� �����������
} SKLP_TimeSinchronization600_Query_t;	//	[17]
#pragma pack( )

// [0x46]	������ �� ��������� �������� ������
#pragma pack( 1 )
typedef struct SKLP_BaudSetQuery_struct
{
	uint8_t				Speed;
} SKLP_BaudSetQuery_t;
#pragma pack( )

// [0x46]	����� �� ������ �� ��������� �������� ������
#pragma pack( 1 )
typedef struct SKLP_BaudSetAnswer_struct
{
	SKLP_FlagsModule_t	FlagsModule;
	SKLP_MemoryFlags_t	MemoryFlags;
} SKLP_BaudSetAnswer_t;
#pragma pack( )

// [0x32, 0x36]	������ �� ��������� ������� �������/��������� ������������
#pragma pack( 1 )
typedef struct SKLP_SetTimeLogStartStop_Query_struct
{
//	char			aSignature[ sizeof( SKLP_SIGNATURE_COMMAND_NVM_SET ) - 1 ];
	SKLP_Time6_t	Time;
} SKLP_SetTimeLogStartStop_Query_t;
#pragma pack( )

// [0x32, 0x36]	����� �� ������ �� ��������� ������� �������/��������� ������������
#pragma pack( 1 )
typedef struct SKLP_SetTimeLogStartStop_Answer_struct
{
	SKLP_Time6_t	Time;
} SKLP_SetTimeLogStartStop_Answer_t;
#pragma pack( )

// [0x35, 0x37]	����� �� ������ �������/��������� ������������
typedef SKLP_SetTimeLogStartStop_Answer_t SKLP_GetTimeLogStartStop_Answer_t;

// [0x34]	����� �� ������ ������ �������������� �������
#pragma pack( 1 )
typedef struct SKLP_TimeFrozenGet_Answer_struct
{
	SKLP_Time_t	Time;
} SKLP_TimeFrozenGet_Answer_t;
#pragma pack( )

// ######################### ������ #########################
#define	SKLP_MEMSECTORSIZE		512			// ������ ������������� ������� ��� ��������� � ������
#ifndef	SKLP_MEMBLOCKSIZEMAX				// ��������������, ��� ���������� ������ ���������� ������� �� SKLP_MEMSECTORSIZE * N, ��� N <= SKLP_MEMBLOCKSIZEMAX
#define	SKLP_MEMBLOCKSIZEMAX	64			// ���������� ����������� ������ �������� ������ �� �������, ������� SKLP_MEMSECTORSIZE * SKLP_MEMBLOCKSIZEMAX
#endif	// SKLP_MEMBLOCKSIZEMAX
											
typedef struct SKLP_Sector_struct
{
	uint8_t aSector[SKLP_MEMSECTORSIZE];
} SKLP_Sector_t;

typedef uint32_t SKLP_SectorIndex_t;		// ������ ������� ������
typedef uint16_t SKLP_ByteIndex_t;		// ������ ����� � �������

// ��������� �������� ����� ������ "������ ����"
#pragma pack( 1 )
typedef struct SKLP_MemInfo_StructLog_struct
{
	SKLP_SectorIndex_t	iSectorFirst;		// [4]	������ ������� �������
	SKLP_SectorIndex_t	iSectorLast;		// [4]	������ ���������� �������
	SKLP_ByteIndex_t	iLastByte;			// [2]	������ ���������� ����� � ��������� �������
} SKLP_MemInfo_StructLog_t;	// [10]
#pragma pack( )

// ��������� �������� ����� ������ "������"
#pragma pack( 1 )
typedef struct SKLP_MemInfo_StructData_struct
{
	SKLP_SectorIndex_t	iSectorFirst;		// [4]	������ ������� �������
	SKLP_SectorIndex_t	iSectorLast;		// [4]	������ ���������� �������
	union
	{
		uint32_t		Mark;				// [4]	?? ����� ����� �������? ������ �������� ������� ���������� ������?
		uint32_t		v1_Alias;			// [4]	!!! ����! ������� ����� ��� ������������� �����, ��� MemInfo_v1
	};
} SKLP_MemInfo_StructData_t;	// [12]
#pragma pack( )

// ��������� �������� ������ MemInfo_v0
// !!! ����!!! �� ������ ���� ��������� �������� MemInfo_v1,
// !!! ����� ���������� ������������� ���������� ����� v1_aAuxStructData[] ����� ����� CRC8
#pragma pack( 1 )
typedef struct SKLP_MemInfo_struct
{
	uint8_t						Format;				// [1]	������ ��������� (0 - v0, 1 - v1, ...)
	uint8_t						TypeBlackBox;		// [1]	������ "������� �����"
	uint8_t						TypeTechData;		// [1]	������ ����������� ������
	uint8_t						TypeData;			// [1]	������ �������� ������
	uint16_t					MemSize;			// [2]	[��]	������� ����� ������
	uint16_t					SectorSize;			// [2]	[�]	������ �������
	uint8_t						DiskCount;			// [1]	���������� ���� ������
	SKLP_MemoryFlags_t			Flags[2];			// [2]	����� ��������� ���� ������
	SKLP_MemInfo_StructLog_t	StructBlackBox;		// [10]	��������� ������� �����
	SKLP_MemInfo_StructLog_t	StructTechData;		// [10]	��������� ����������� ������
	SKLP_MemInfo_StructData_t	StructData;			// [12]	��������� �������� ������
//	SKLP_MemInfo_StructData_t	StructDataZip;		// ��������� ������ ������
	SKLP_MemInfo_StructData_t	StructDataAsync;	// [12]	��������� ����������� ������ (������� ������)
	union
	{
		uint32_t	SegmentSize;					// [4]	?? ������ ������������ ����� ������ (������������� ������������� ������� �����)
		struct
		{
			uint8_t	AuxDataStructCount;				// 		���������� �������������� �������� ������, ������ ��� MemInfo_v1
			uint8_t	AuxReadBufferSize;				//		������ ������ ��� ������ ������, (� ��������)
			uint8_t	Reserved2;
			uint8_t	Reserved3;
		} v1_Aux;									// [4]
	};
//	SKLP_MemInfo_StructData_t	v1_aAuxStructData[N];	// [12*N]	����� �������������� �������� ������, ������ ��� MemInfo_v1
	uint8_t			CRC8;							// [1]	???
} SKLP_MemInfo_t;		// [60] ��� v0, [60 + 12*N] ��� v1
#pragma pack( )

// ����� �� ������ ��������� ������
#pragma pack( 1 )
typedef struct SKLP_MemInfoAnswer_struct
{
	SKLP_MemoryInit_t	InitFlags;
	SKLP_MemoryFlags_t	MemoryFlags;
	SKLP_MemInfo_t		Info;
} SKLP_MemInfoAnswer_t;
#pragma pack( )
	
// ������ �� ������ ������
#pragma pack( 1 )
typedef struct SKLP_MemReadQuery_struct
{
	uint8_t				DiskNum;				// ����� ����� (SD-�����, Flash � �.�.)
	uint16_t			SectorsCount;			// ���������� ������������� ��������
	SKLP_SectorIndex_t	iSectorStart;			// ������ ������� �������������� �������
} SKLP_MemReadQuery_t;
#pragma pack( )
	
// ��������� ������ �� ������ ������ ������
#pragma pack( 1 )
typedef struct SKLP_MemReadAnswerFrame_struct
{
	uint8_t 			Start;
	uint16_t			SectorsCount;
	SKLP_MemoryFlags_t	MemoryFlags;
	SKLP_Sector_t		aBuffer[SKLP_MEMBLOCKSIZEMAX];			// ����� ������������� ��������������� �������, ������������� ����� ����� ���� ������
	uint8_t 			CRC8;
} SKLP_MemReadAnswerFrame_t;
#pragma pack( )

// ���������, ������� ���������� �����, ����������� � ������
#pragma pack( 1 )
typedef struct SKLP_MemSaveSign_struct
{
//	char aBuff[ sizeof( SKLP_SIGNATURE_SAVING_START ) - 1 ];	// [5]	"START"
	char aBuff[5];			// [5]	"START"
} SKLP_MemSaveSign_t;		// [5 ����]
#pragma pack( )

#define	SKLP_SIGNATURE_MEMSAVING_START		( ( SKLP_MemSaveSign_t ) { SKLP_SIGNATURE_SAVING_START } )

// ��������� ����� �������� ������ (��� ������ �� SD-����� ��� ������ �� ������� 0x03), �������� � ��������� ���������� ������ ���
#pragma pack( 1 )
typedef struct SKLP_MemStruct_DataHeader_struct
{
	char					aSignature[5];		// [5]	"START"
//	SKLP_MemSaveSign_t		Signature;			// [5]	"START"
	uint8_t					ID;					// [1]	??
	SKLP_Time_t				Time;				// [7]
	SKLP_FlagsModule_t		FlagsModule;		// [1]
	SKLP_MemoryFlags_t		MemoryFlags;		// [1]
} SKLP_MemStruct_DataHeader_t;				// [15 ����]
#pragma pack( )

// ��������� ���������� ������������ - ���������� ���� � �������������� ������
typedef int32_t SKLP_CommandResult_t;		// ��� 8-������� ���� ����� ���� int16_t
#define	SKLP_COMMAND_RESULT_RETURN			( ( SKLP_CommandResult_t ) 1 )		// (� �����) - ������� ��������� ����������, ���� ������ ������ ��������� ��������� ����������� ������
#define	SKLP_COMMAND_RESULT_NO_REPLY		( ( SKLP_CommandResult_t ) 0 )		// ������� ��������� ����������, �������� �� ���������
#define	SKLP_COMMAND_RESULT_ERROR			( ( SKLP_CommandResult_t ) -1 )		// �������� ������ ��� ��������� ������� (��� ���������)
#define	SKLP_COMMAND_RESULT_ERROR_FORMAT	( ( SKLP_CommandResult_t ) -2 )		// - ������ � ������� ������ (Start, Size, CRC ��� ���������� ��������� ������)
#define	SKLP_COMMAND_RESULT_ERROR_NOCB		( ( SKLP_CommandResult_t ) -3 )		// - �� ������ ���������� ���������� �������
#define	SKLP_COMMAND_RESULT_ERROR_CB		( ( SKLP_CommandResult_t ) -4 )		// - ������ ������ �����������
#define	SKLP_COMMAND_RESULT_ERROR_TM		( ( SKLP_CommandResult_t ) -5 )		// - ������������ ������ �� ��������� � ���������� �������

// ��� �������� ��� ��������� ������� ���
// pQuery	- ����� ������ � ��������� ��������, ������� � �� ������. ����� �������� �� ����� 256 ����.
// ppAnswer	- �����, ���� ������� ������� �����: ��-��������� (��� ��������� �������), *ppAnswer == pQuery,
//			�� ������� ����� ����� ������� � ������ �����.
//			����� ����� ppAnswer � ������� ���������� ����� ����������, ������ �������� ������, ��. SKLP_ProcessPacket() � SKLP_ProcessCommand_BaudSet()
// Return	- ���������� ���� � ������ ��� ��� ������
typedef SKLP_CommandResult_t ( *SKLP_Callback_t )( uint8_t *pQuery, uint8_t **ppAnswer );

typedef int16_t SKLP_Temp_t;	// � ������ ������� ����������� ����������� � ��������� 1 ��� � 0.01 ���
#define	SKLP_TEMP_UNKNOWN		( ( SKLP_Temp_t ) 0xFFFF )
#define	SKLP_TEMP_MIN			( ( SKLP_Temp_t ) -56 )		// !!�����!! ��� ���������� ����������� ����� ������ (����., � ���) - �������� 1 ��� � �������� +56 ��� (�����, �������� �� -56 �� 199 ���)
#define	SKLP_TEMP_MAX			( ( SKLP_Temp_t ) +155 )
#define	SKLP_TEMP_MAX_EEPROM1	( ( SKLP_Temp_t ) +85 )
#define	SKLP_TEMP_MAX_EEPROM2	( ( SKLP_Temp_t ) +90 )
#define	SKLP_TEMP_MAX_SD1		( ( SKLP_Temp_t ) +120 )
#define	SKLP_TEMP_MAX_SD2		( ( SKLP_Temp_t ) +125 )
#define	SKLP_TEMP_VALIDATE( __TEMP__ )		( ( ( __TEMP__ ) >= SKLP_TEMP_MIN ) && ( ( __TEMP__ ) <= SKLP_TEMP_MAX ) )

// ���������� ����������, ��������� ��� �������� ���������
extern volatile SKLP_FlagsModule_t		SKLP_FlagsModule;			// ����� ������
extern volatile SKLP_MemoryFlags_t		SKLP_MemoryFlags;			// ����� ������
extern volatile SKLP_Time_t				SKLP_Time;					// ��������� ����� ���
extern volatile SKLP_WorkTimeCounter_t	SKLP_WorkTimeCounter;		// ������� ������������� ������� (������������)
extern volatile SKLP_Temp_t				TemperatureBoard;			// ����������� �����
extern LoochDeviceSerial_t				LoochDeviceSerial;			// �������� ����� ������
extern LoochSoftVersion_t				LoochSoftVersion;			// ���� � ������ ��������������
//extern ModuleMode_t						ModuleMode;					// ����� ������ ������

// ���������� �������
bool SKLP_ServiceInit( void );							// �������������
SKLP_Callback_t SKLP_ServiceSetCallback( uint8_t CommandNumber, SKLP_Callback_t xNewCallback );		// ������������� ������� � ����������
SKLP_CommandResult_t SKLP_ProcessCommand_Common( uint8_t *pQuery, uint8_t **ppAnswer );		// ����� ����������� ����������� ������� � ������ � ����� ���

// �������� ����������� �����������.
// ��������� � SKLP_ServiceDefault.c ��� __weak,
// ����� ���� �������������� � ����������, �� �������������.
// �������� ��������� ��. ����, ��� ���������� SKLP_Callback_t.
SKLP_CommandResult_t SKLP_ProcessCommand_ID_Get( uint8_t *pQuery, uint8_t **ppAnswer ); 							// [0x01]	������� ������������� �������
SKLP_CommandResult_t SKLP_ProcessCommand_ID_Set( uint8_t *pQuery, uint8_t **ppAnswer ); 							// [0x02]	���������� ������������� �������
SKLP_CommandResult_t SKLP_ProcessCommand_TimeSinchronization( uint8_t *pQuery, uint8_t **ppAnswer );			// [0x37]	������������� ������� ���������
SKLP_CommandResult_t SKLP_ProcessCommand_WorkTime_Get( uint8_t *pQuery, uint8_t **ppAnswer );					// [0x07]	������� ������������ �����
SKLP_CommandResult_t SKLP_ProcessCommand_WorkTime_Inc( uint8_t *pQuery, uint8_t **ppAnswer );					// [0x08]	���������������� ������������ �����
SKLP_CommandResult_t SKLP_ProcessCommand_WorkTime_Set( uint8_t *pQuery, uint8_t **ppAnswer );					// [0x09]	���������� ������������ �����
SKLP_CommandResult_t SKLP_ProcessCommand_MemoryGetState( uint8_t *pQuery, uint8_t **ppAnswer ); 				// [0x21]	������� ��������� ������
SKLP_CommandResult_t SKLP_ProcessCommand_BaudSet( uint8_t *pQuery, uint8_t **ppAnswer );							// [0x46]	���������� �������� ������
SKLP_CommandResult_t SKLP_ProcessCommand_GoTo_BootLoader( uint8_t *pQuery, uint8_t **ppAnswer );			// [0xB1]	������� �� BootLoader

SKLP_CommandResult_t SKLP_ProcessCommand_BaudSetLocal( void *pTransportInterface, uint32_t NewBaudRate );		// [0x46]	��������� ��������� ��������� ��������
//SKLP_Time_t Time_LoochSoftVersion2SKLP( LoochSoftVersion_t LoochSoftVersion );			// ������� ������� �������� �������� �� ����� ���
SKLP_Time6_t Time_LoochSoftVersion2SKLP( LoochSoftVersion_t LoochSoftVersion );			// ������� ������� �������� �������� �� ����� ���
LoochDeviceSerialBin_t SKLP_LoochDeviceSerial2Bin( LoochDeviceSerial_t SerialBCD );		// ������� ��������� �� BCD � Bin
LoochDeviceSerial_t SKLP_LoochDeviceSerial2BCD( LoochDeviceSerialBin_t SerialBin );		// ������� ��������� �� Bin � BCD

#endif	// SKLP_SERVICE_H
