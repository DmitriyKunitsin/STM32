#include "RUS_Regul_Events.h"
#include "SKLP_Interface_RUS_Regul_3540.h"

#if defined(USE_MOTOR_CM36_3650)
#include "DRIVER_CM36-3650.h"
#elif defined(USE_SMALL_MOTOR)
#include "DRIVER_Small_Motor.h"
#endif

bool Motor_Speed_Set(float speed_percent);
void Motor_CreateSpeedTimerTask(void);
void Motor_Angle_Set(int16_t angle, float speed_percent);
void Motor_Stop(void);

void Motor_Calibration_Current_Create(void);
void Motor_Calibration_Current_Start(void);

void Motor_Calibration_Press_Create(void);
void Motor_Calibration_Press_Start(void);

void Motor_Stabilization_Press_Create(void);
void Motor_Stabilization_Press_Start(void);

#pragma pack(1)
typedef enum DirRotation_enum
{
	Clockwise = 0,
	CounterClockWise
} DirRotation_t;
#pragma pack()

#if 0
#pragma pack(1)
typedef enum Motor_WorState_enum
{
	Standing = 0,				// 	[0]		����
	Rotating					// 	[1]		�������� ����������
} WorkState_t;
#pragma pack()
#endif

void Motor_Rotation_Set(DirRotation_t Rotation);

#if 0
#pragma pack(1)
typedef enum Motor_WorMode_enum
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
} WorkMode_t;
#pragma pack()
#endif

#pragma pack(1)
typedef struct RTOS_Timers
{
	TimerHandle_t CalibrTimer, CalibrPressTimer, SpeedCounterTimer, StabPressTimer, OPAMPTimer;
} RTOS_Timers_t;
#pragma pack()


typedef struct Motor_Main_Struct
{
	DirRotation_t 	Rotation;				// ����������� �������� ������
	RTOS_Timers_t	Timers;					// ������� RTOS (������ ����� ����������� �����-�� ������, ������� �� ���� ���������)
	float			SpeedSet;				// ������������ �������� � %
	float			SpeedMeas;				// ���������� �������� ��/���
	float			AngleSet;				// ������������ ���� ��������
	float			CurPosition;			// ������� �������
	float			longCurPosition;		// �� �� ������� �������, ������ ���� �� 0 �� ARR_COUNT
	uint16_t		TimerCNTValue;			// �������� �������� �������
	float			Press;					// ���������� ��������
	float			PressSet;				// ������������ �������� ��� ���������
	RUS_Regul_Calibr_Press_t	Settings;	// ���� ���� ���� � ���������!!!
	float			MotorPowerSupply;		// ���������� ���������� + 12�
	float			MotorCurrent;			// ���������� ��� �����������
	float			CalibrNull;				// ���������� �������� �������� ��������� �� ����������� ���������� (0 ��������� � ����� �������� ��������)
	float			MinAngle;				// ����������� ���� �� ����������
	float			MaxAngle;				// ������������ ���� �� ����������
	float			MinPress;				// ����������� �������� �� ����������
	float			MaxPress;				// ������������ �������� �� ����������
	float			WorkZone;				// ������� ���� (maxAngle - minAngle)
	float			OPAMPThreshold;			// ����� ���� �����������, ��� ���������� �������� ����� ������ ���� ����������
	RUS_Regul_Motor_Workstate_t		WorkState;				// ��������� ������
	RUS_Regul_Motor_Workmode_t		WorkMode;				// ����� ������

	union{
		uint8_t 		ErrorFlag;				// ����� ������
		struct
		{
			uint8_t	fNotCalibr			:1;		// �� ���� ��������� �������, ������ ��� �� ������������
			uint8_t	fBadCalibr			:1;		// ������ ���������� �� ��� ��� ���� �������� (��� � ���� �������� ���������� ��� ���������� ���� ��� ��� � ���� ��������)	
			uint8_t	fNoCNT				:1;		// ��� �������� �������� �������� ����� ��� ��������. ��������� ���� ������. ����� �������� ������ � ������ ������ ����������� ��������
			uint8_t	fNoMotorPower		:1;		// ��� ���������� �� ������ �� ������ ���������� ������ �����������
			uint8_t	fNoPressure			:1;		// ��� ��������� ������� ��������
			uint8_t fStuck				:1;		// �����, ��� ������� �� ����������������
			uint8_t fERReserved			:2;		// ����������������	
		};
	};

	union{
		uint16_t		LogicalFlag;			// ���������� �����, �������������� ������ ������ ���������
		struct
		{
			uint16_t fReverseLogic		:1;		// ���� ����������� ������. 1 ���� ��� ���������� ����������� ���� ����� � 360, ��� ������������, � 0 ���� ��������
			uint16_t fWorkOverCurrent	:1;		// ���� ���������� �������� �����������
			uint16_t fExOverCurrent		:1;		// ����������� ���� ���������� �����������, ����������� ������ �� ������� ����� ���� �������� ������ �� �����������
			uint16_t fCalibrStart		:1;		// ���� ������ ����������
			uint16_t fCalibrated		:1;		// ���� ����, ��� ���������� ���� ��������� �������
			uint16_t fHoldPress			:1;		// ���� ����������� ������ ��������� ��������
			uint16_t fCheckOPAMPValue	:1;		// ���� ���������� �������� �������� �����������
			uint16_t fFoundLimit		:1;		// ���� ���������� ������� ��� ���������� �� �����������
			uint16_t fClearCNTValue		:1;		// ���� ������ �������� �������� CNT
			uint16_t fSetCalNullPos		:1;		// ���� ����������� ������� � "�������"(= � ��������� � ����������� ���������) ����� ���������� ����������
			uint16_t fLFReserved		:6;		// ����������������	
		};
	};
} Motor_Main_t;



#if defined(USE_MOTOR_CM36_3650)
#define MIN_SPEED 						(MIN_SPEED_CM36_3650)
#define MAX_SPEED						(MAX_SPEED_CM36_3650)
#define PULSE_PER_ONE_TURN 				(PULSE_PER_ONE_TURN_CM36_3650)
#define PULSE_PER_ONE_ROUND_INTERNAL	(PULSE_PER_ROUND_MOTOR_CM36_3650)
#define CALIBR_TURN						(CALIBR_TURN_CM36_3650)
#define ARR_COUNT						(ARR_VALUE_CM36_3650)
#define GEAR_RATIO						(GEAR_RATIO_CM36_3650)
#define OPAMP_TIMER_MS					(OPAMP_TIMER_MS_MOTOR_CM36_3650)


#elif defined(USE_SMALL_MOTOR)
#define MIN_SPEED 						(MIN_SPEED_SMALL_MOTOR)
#define MAX_SPEED						(MAX_SPEED_SMALL_MOTOR)
#define PULSE_PER_ONE_TURN 				(PULSE_PER_ONE_TURN_SMALL_MOTOR)
#define PULSE_PER_ONE_TURN_INTERNAL		(PULSE_PER_ROUND_MOTOR_SMALL_MOTOR)
#define CALIBR_TURN						(CALIBR_TURN_SMALL_MOTOR)
#define ARR_COUNT						(ARR_VALUE_SMALL_MOTOR)
#define GEAR_RATIO						(GEAR_RATIO_SMALL_MOTOR)


#endif
