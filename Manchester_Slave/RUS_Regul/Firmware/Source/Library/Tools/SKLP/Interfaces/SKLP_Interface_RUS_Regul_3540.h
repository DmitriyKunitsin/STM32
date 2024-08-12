// SKLP_Interface_GGLP_SADC.h
// ��������� ��������� ������������������� ��� ��� ���-�� �� ���� ���
// ���������� ���.638.00.02.00 � ������� ������ ���.638.00.00.00
#ifndef	SKLP_INTERFACE_RUS_REGUL_V2_H
#define	SKLP_INTERFACE_RUS_REGUL_V2_H

#include <stdint.h>
#include "SKLP_Time.h"				// SKLP_Time_t
#include "NVM.h"					// NVM_Result_t
#include "SKLP_MS_Transport.h"		// SKLP_Signature
#include "SKLP_Service.h"			// SKLP_FlagsModule_t, SKLP_CommandResult_t
#include "MathUtils.h"				// Float16_t
#include <stdint.h>

// ������� ����������
typedef enum SKLP_Command_RUS_Regul_enum
{
	// ����������� ������ ������
	//SKLP_COMMAND_RUS_REGUL_ID_GET					= 0x01, // ����������� �������: ������ ID ������ (����� �����? (��������� � ������� ���, � ���� ID, � ���������� �����))
	SLKP_COMMAND_RUS_REGUL_NVM_GET					= 0x04, // ����������� �������: ��������� ������ �� NVM (���������� ������� ��������, �������� �����) 
	SLKP_COMMAND_RUS_REGUL_NVM_SET					= 0x05, // ����������� �������: �������� ������ � NVM (���������� ������� ��������) (�� ����, ��� ������������)
	SKLP_COMMAND_RUS_REGUL_MAIN_DATA_GET			= 0x13, // ����������� �������: ������ ���� �������� ������ (!!!�� ������������!!!)
	SKLP_COMMAND_RUS_REGUL_EXTENDEED_DATA_GET		= 0x14, // ����������� �������: ������ ����������� ���� ������ (��� �����������)

	// ������� ���������� ����������� � �������� ������� (��)
	SKLP_COMMAND_RUS_REGUL_MOTOR_POWER_MANAGE		= 0x60, // ������ ������� �� �����
	SKLP_COMMAND_RUS_REGUL_MAX_CURRENT_SET			= 0x61, // ���������� ����� �� ���� ����������� ������
	SKLP_COMMAND_RUS_REGUL_OVERLAP_SET				= 0x62, // ��������� ������� ���������� �������� 
	SKLP_COMMAND_RUS_REGUL_CALIBR_PRESS_SET			= 0x63, // ��������� ���������� �� ��������					
	SKLP_COMMAND_RUS_REGUL_CALIBR_CURRENT_SET		= 0x64, // ��������� ���������� �� ���� ����������� (���� �������� � �������)
	SKLP_COMMAND_RUS_REGUL_PRESS_STABILIZATION		= 0x65, // ��������� ������� ������������ �������� (��������� ����������)
	SKLP_COMMAND_RUS_REGUL_ADVANCED_MANAGMENT		= 0x70, // ������ � ������������ ����� ������ (������� � ��������� EX_) (���� ��� ������� �������� �����)

	EX_SKLP_COMMAND_RUS_REGUL_ANGLE_SET				= 0x71, // ������� ������������ ����������: ���������� ������ ����
	EX_SKLP_COMMAND_RUS_REGUL_SPEED_SET				= 0x72, // ����������� ��������  [%]	[0-100]
	
} SKLP_Command_RUS_Regul_t;

// [0x04] ������� ������ ����� �� ����������������� ������	
// BlockID ��� ������������� ����� �� ������\������ ������ � EEPROM
#pragma pack(1)
typedef enum SKLP_NVM_ID_enum
{
	eSKLP_NVM_PressCoeff	= 0,	// ������������ ������� ��������
	eSKLP_NVM_MotorCalibr	= 1,	// ������������ �������� �������� � ����������� �� ����
	eSKLP_NVM_NotDefined	= 0xFF,
} SKLP_NVM_ID_t;
#pragma pack()

// ��������� ���� ������� ������ ����� �� ����������������� ������
#pragma pack(1)
typedef struct SKLP_Command_RUS_Regul_NVM_Get_Query
{
	SKLP_NVM_ID_t SKLP_NVM_ID;
} SKLP_Command_RUS_Regul_NVM_Get_t;
#pragma pack() 

#pragma pack(1)
typedef struct SKLP_Command_RUS_Regul_NVM_GetSet_Answer_struct
{
	NVM_Result_t	Result;
	uint8_t			xData;			// !! � ����� ������ ����������� ���� ������ ������������� ������� !!
} SKLP_Command_RUS_Regul_NVM_GetSet_Answer_t;
#pragma pack()

// eSKLP_NVM_PressCoeff = 0 ������������ ������� ��������
#pragma pack(1)
typedef struct RUS_Regul_Calibr_Press_struct
{
	// ������������� ������������ ��. �� �������� C2 * ADC^2 + C1 * ADC + C0
	SKLP_Time6_t date;				//	[6]
	// ��� ������������ ���� ��� � ����������
	float Press_Calibr_Coef_C2;		//	[4]		
	float Press_Calibr_Coef_C1;		//	[4]
	float Press_Calibr_Coef_C0;		//	[4]
} RUS_Regul_Calibr_Press_t;			//	[18]
#pragma pack()

#pragma pack(1)
typedef struct RUS_Regul_Calibr_Motor_struct
{
	SKLP_Time6_t date;	//	[6]	
	float MaxCurrent; 	// 	[4]	[�]		 ������������ ��� �����������
	float MaxRotSpeed; 	//	[4]	[0 - 100%] 	 ������������ �������� �������� ������
} RUS_Regul_Calibr_Motor_t;	//	[14]
#pragma pack()

#pragma pack(1)
typedef struct SKLP_Command_RUS_Regul_NVM_Get_Answer
{
	RUS_Regul_Calibr_Press_t PressCoef;	//	[18] 
} SKLP_Command_RUS_Regul_NVM_Get_Answer_t;	//	[18]
#pragma pack()

// [0x13]
#pragma pack(1)
typedef struct SKLP_Command_RUS_Regul_Main_Data_Answer_struct
{
	int16_t	Press;	// [2]	[���]		[,00]	���������� ��������	
} SKLP_Command_RUS_Regul_Main_Data_get_t; // [���� ��� 2]
#pragma pack()

#pragma pack(1)
typedef struct SKLP_RUS_Regul_NVM_Set_Query_struct
{
	SKLP_NVM_ID_t 	SKLP_NVM_ID;
	uint8_t 		xData;				// !! � ����� ������ ����������� ���� ������ ������������� �������!!
	uint16_t 		CRC16;				// !! ���������� ���� CRC16 ������� �� ������� ���� ������!!
} SKLP_RUS_Regul_NVM_Set_Query_t;
#pragma pack()


// ��������� ������������� ����
#pragma pack(1)
typedef struct RUS_Regul_Positioning_Data_struct
{
	uint16_t		MinAngle;				// [2]	[����]		[,00]	����������� ���� �� ����������
	uint16_t		MaxAngle;				// [2]	[����]		[,00]	������������ ���� �� ����������
	int16_t			MinPress;				// [2]	[���]		[,00]	����������� �������� �� ����������
	int16_t			MaxPress;				// [2]	[���]		[,00]	������������ �������� �� ����������
	uint16_t		WorkZone;				// [2]	[����]		[,00]	������� ���� (maxAngle - minAngle)
} RUS_Regul_Positioning_Data_t;
#pragma pack()

#pragma pack(1)
typedef struct SKLP_Command_RUS_Regul_Positioning_Data_Set_Answer
{
	uint8_t 	error;			//	[1]		0 = OK, 1 = ERROR
	uint16_t	LogicFlags;		//	[2]		����� ������
	uint8_t		ErrorFlags;		//	[1]		����� ������
} RUS_Regul_Positioning_Data_Answer_t;
#pragma pack()

//[???]
#pragma pack(1)
typedef struct SKLP_Command_Positioning_Data_GetSet_struct
{
	RUS_Regul_Positioning_Data_t PositioniongData;
} SKLP_Command_Positioning_Data_GetSet_struct_t;
#pragma pack()


#pragma pack(1)
typedef enum RUS_Regul_Motor_Workstate_enum
{
	Standing = 0,				// 	[0]		����
	Rotating					// 	[1]		�������� ����������
} RUS_Regul_Motor_Workstate_t;
#pragma pack()


#pragma pack(1)
typedef enum RUS_Regul_Motor_Workmode_enum
{
	Ready = 0,					//	[0]		����� � ���������� ���������� ������
	CalibratingPress,			//	[1]		���������� �� ��������
	CalibratingConsumption,		//	[2]		���������� �� �����������
	RotatingByAngle,			// 	[3]		�������� �� ������������ ����	
	HoldingPressure,			//	[4]		��������� ��������
	SettingPressure,			//	[5]		��������� ��������
	SettingOverlap,				//	[6]		��������� ����������
	SettingCalibrNullPos,		//	[7]		��� � ���������� �������� ����� ����������
	DiscoverOverCurrent			//	[8]		���������� ���������� ���� �����������
} RUS_Regul_Motor_Workmode_t;
#pragma pack()


//[0x14]
#pragma pack(1)
typedef struct SKLP_Command_RUS_Regul_Extendeed_Data_Get_Answer
{
	union
	{
		uint8_t 		ErrorFlag;				//	[1] ����� ������
		struct
		{
			uint8_t fNotCalibr			:1; 	// �� ���� ��������� �������, ������ ��� �� ������������
			uint8_t fBadCalibr			:1; 	// ������ ���������� �� ��� ��� ���� �������� (��� � ���� �������� ���������� ��� ���������� ���� ��� ��� � ���� ��������)	
			uint8_t fNoCNT				:1; 	// ��� �������� �������� �������� ����� ��� ��������. ��������� ���� ������. ����� �������� ������ � ������ ������ ����������� ��������
			uint8_t fNoMotorPower		:1; 	// ��� ���������� �� ������
			uint8_t fNoPressure 		:1; 	// ��� ��������� ������� ��������
			uint8_t fERReserved 		:3; 	// ���������������� 
		};
	};		// [1]
	
	
	union
	{
		uint16_t		LogicalFlag;			// 	[2]	���������� �����, �������������� ��������� ������ ���������
		struct
		{
			uint16_t fReverseLogic		:1; 	// ���� ����������� ������. 1 ���� ��� ���������� ����������� ���� ����� � 360, ��� ������������, � 0 ���� ��������
			uint16_t fWorkOverCurrent	:1; 	// ���� ���������� �������� �����������
			uint16_t fExOverCurrent 	:1; 	// ����������� ���� ���������� �����������, ����������� ������ �� ������� ����� ���� �������� ������ �� �����������
			uint16_t fCalibrStart		:1; 	// ���� ������ ����������
			uint16_t fCalibrated		:1; 	// ���� ����, ��� ���������� ���� ��������� �������
			uint16_t fHoldPress 		:1; 	// ���� ����������� ������ ��������� ��������
			uint16_t fCheckOPAMPValue	:1; 	// ���� ���������� �������� �������� �����������
			uint16_t fFoundLimit		:1; 	// ���� ���������� ������� ��� ���������� �� �����������
			uint16_t fClearCNTValue 	:1; 	// ���� ������ �������� �������� CNT
			uint16_t fSetCalNullPos 	:1; 	// ���� ����������� ������� � "�������"(= � ��������� � ����������� ���������) ����� ���������� ����������
			uint16_t fPowerManage		:1;		// ���� ������ ������� �� �����
			uint16_t fLFReserved		:5; 	// ���������������� 
		};	// [2]
	};

	int8_t			SpeedSet;				// [1]	[0-100%]	[0]		������������ ��������
	int16_t			SpeedMeas;				// [2]	[��/���]	[,00]	���������� �������� ��/���
	uint16_t		AngleSet;				// [2]	[����]		[,00]	������������ ���� ��������
	uint16_t		CurPosition;			// [2]	[����]		[,00]	������� �������
	uint16_t		TimerCNTValue;			// [2]	[-]			[0]		�������� �������� �������
	int16_t			Press;					// [2]	[���]		[,00]	���������� ��������
	int32_t			ADC_code;				// [4]  [-]			[0]		�������� - ��� ���
	int16_t			CurPress;				// [2]	[���]		[,00]	���������� ��������
	uint16_t		TarPress;				// [2]	[���]		[0]		������������ �������� ��� ���������
	uint8_t			MotorPowerSupply;		// [1]	[�]			[,0]	���������� ���������� + 12�
	uint8_t			MotorCurrent;			// [1]	[��]		[0]		���������� ��� �����������
	uint16_t		CalibrNull;				// [2]	[����]		[,00]	���������� �������� �������� ��������� �� ����������� ���������� (0 ��������� � ����� �������� ��������)
	uint16_t		MinAngle;				// [2]	[����]		[,00]	����������� ���� �� ����������
	uint16_t		MaxAngle;				// [2]	[����]		[,00]	������������ ���� �� ����������
	int16_t			MinPress;				// [2]	[���]		[,00]	����������� �������� �� ����������
	int16_t			MaxPress;				// [2]	[���]		[,00]	������������ �������� �� ����������
	uint16_t		WorkZone;				// [2]	[����]		[,00]	������� ���� (maxAngle - minAngle)
	
	RUS_Regul_Motor_Workmode_t			WorkState;				// [1]  [-]			[0]		����������� ��������� (�������� ��� ����������)
	RUS_Regul_Motor_Workstate_t			WorkMode;				// [1]	[-]			[0]		����� ������
} SKLP_Command_RUS_Regul_Extendeed_Data_Get_Answer_t;	// [38] - ����� ������ �������
#pragma pack()

#pragma pack(1)
typedef struct SKLP_Command_RUS_Regul_Motor_Power_Managment_Query
{
	uint8_t 	pwrOn; 		// 	[1]		�������� �������. ����  1 - ������ �������, ���� 0 - ����� �������
} SKLP_Command_RUS_Regul_Motor_Power_Managment_Query_t;
#pragma pack()

#pragma pack(1)
typedef struct SKLP_Command_RUS_Regul_Motor_Power_Managment_Answer
{
	uint8_t 	Power;		// 	[1]		����� 1 - ���� �������, 0 - ��� ������� 
	uint16_t 	LogicFlag;	//	[2]		����� ������ 
	uint8_t 	ErrorFlags;	//	[1]		����� ������
} SKLP_Command_RUS_Regul_Motor_Power_Managment_Answer_t;
#pragma pack()

#pragma pack(1)
typedef struct SKLP_Command_RUS_Regul_Overlap_Set_Query_struct
{
	int16_t OverlapPercent;			//	[1]		[%]	[0..+100]	������� ���������� ��������
} SKLP_Command_RUS_Regul_Overlap_Set_Query_t; // [1]
#pragma pack()

#pragma pack(1)
typedef struct SKLP_Command_RUS_Regul_Overlap_Set_Answer_struct
{
	uint8_t 	error;			//	[1]		[-]		[0/1]	0 - ��� ��, 1 - ������, �� ������������
	uint16_t	LogicFlag;		//	[2]		[-]		����� ������
	uint8_t		ErrorFlag;		//	[1]		[-]		����� ������
} SKLP_Command_RUS_Regul_Overlap_Set_Answer_t; // [1]
#pragma pack()


#pragma pack(1)
typedef struct SKLP_Command_HoldPress_Query_struct
{
	uint16_t tarPress;	// [4] [���] �������� ��� ��������� 
} SKLP_HoldPress_Query_t;
#pragma pack()

#pragma pack(1)
typedef struct EX_SKLP_ProcessCommand_RUS_Regul_Speed_Set_Query
{
	float speed_percent; //	[4]	[%]	[-100..100] ��������� �������� ������
} EX_SKLP_ProcessCommand_RUS_Regul_Speed_Set_Query_t;
#pragma pack()

#pragma pack(1)
typedef struct EX_SKLP_ProcessCommand_RUS_Regul_Angle_Set_Query
{
	int16_t angle;		//	[2]	[�]	���� � �������� - ����� �� -inf �� +inf (����������� ������������� ���� ����� ���� ������������)	
	float speed;		//	[4]	[%]	�������� � ��������� �� 0 �� 100
}EX_SKLP_Command_RUS_Regul_Angle_Set_Query_t;
#pragma pack()

#endif
