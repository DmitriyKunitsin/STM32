#ifndef	RUS_REGUL_MAIN_H
#define	RUS_REGUL_MAIN_H

#include "TaskConfig.h"
#include "NVM.h"			// NVM_Tag_t

#include "RUS_Regul_ADC.h"
#include "SKLP_Interface_RUS_Regul_3540.h"

//#define DEBUG_OUTPUT

// �������� ���������� ������� ������� RTOS � RUS_Regul main task
#define		RUS_REGUL_WATCHDOGTIMER_S			1.0f 		// 1[�] ������ ������ �������� ������� ��� ������ WatchDog
#define 	RUS_REEGUL_DELAY_TIMEDELTA_MS		( 200 )		// [��]  200 �� 
#define 	VIKPB_SERVICEUPDATE_TIMEDELTA_MS	( 100 )		// [��]  100 �� 
#define 	RUS_REGUL_STARTDELAY1_MS			( 650 )		// [��]	 ��������� ��������
#define 	RUS_REGUL_STARTDELAY2_MS			( 1500 )	// [��]  ��������� ��������

#define		ADS1231_GAIN						(128.0f)	// �������� �������� ���
#define		ADS1231_VDDA						(5.2f)		// �������� �� ����� ������������
#define		ADS1231_MAX_CODE					(8388607.0f)// ������������ ��� 24-������� �������� ��� (�MAX_CODE)
#define		ADS1231_V_TO_MV						(1000.0f)	// ������� �� � � ��
#define		ADS1231_CALIBR_COEF_A				(40.27f)	// ����������� ���������� A ������� �������� (press * A + B)
#define		ADS1231_CALIBR_COEF_B				(6.2f)		// ����������� ���������� B ������� �������� (press * A + B)
#define		CONVERT_TO_BAR						(((((0.5f * ADS1231_VDDA) / ADS1231_GAIN) / ADS1231_MAX_CODE) * ADS1231_V_TO_MV) / ADS1231_VDDA)

#if defined(TEST_BOARD)
#define		TEST_COEF_A							(1000.0f)
#define		TEST_COEF_B							(-13.48f)
#endif

#define		PRESS_MEDIAN_SIZE					(40)		// ������ ������� ���������� ������� (��� � ������� ������ ��������� �������, ������ ��� �������� ��� 80 �������� � �������)
#define		PRESS_MEDIAN_HALF_SIZE				((PRESS_MEDIAN_SIZE) / (2)) // ��� �������, �� �������� ���� ����� ��������
#define		PRESS_WINDOW						(30)		// ������ ���� ��� ����������� ��������	

#define		BETA_COEF_FILTER					(1.0f / 2.0f) // ����������� ���� ��� ���� �������

#define 	CURRENT_LIMITER_VALUE				(1.0f)	// [A] �������� ����������������. (�������� ����������� �������� ��� ������ ��������)

typedef struct RUS_Regul_Main_Struct
{
	TimerHandle_t	xStartDelayTimer;		// ������ �������� ������ �� 650 ��
	float			SpeedSet;				// ������������ �������� � %
	float			SpeedMeas;				// ���������� �������� ��/���
	float			AngleSet;				// ������������ ���� ��������
	float			CurPosition;			// ������� �������
	float			longCurPosition;		// �� �� ������� �������, ������ �� 0 �� ARR_COUNT
	uint16_t		TimerCNTValue;			// �������� �������� �������
	float			Press;					// ���������� �������� (�������������)
	uint16_t		PressSet;				// ������������ �������� ��� ���������
	float			MotorPowerSupply;		// ���������� ���������� + 12�
	float			MotorCurrent;			// ���������� ��� �����������
	float			CalibrNull;				// ���������� �������� �������� ��������� �� ����������� ���������� (0 ��������� � ����� �������� ��������)
	float			MinAngle;				// ����������� ���� �� ����������
	float			MaxAngle;				// ������������ ���� �� ����������
	float			MinPress;				// ����������� �������� �� ����������
	float			MaxPress;				// ������������ �������� �� ����������
	float			WorkZone;				// ������� ���� (maxAngle - minAngle)
	int32_t			ADC_Code;				// �������� � ����� ���
	float			CurPress;				// ���������� ��������
	uint8_t			WorkMode;				// ����� ������
	uint8_t			WorkState;				// ����������� ��������� ������ (����������/ ��������)
	uint8_t			BoardAdress;			// ����� ����� (������������ ������������� � TransportLocal_354_10)
	
	union
	{
		uint8_t 		ErrorFlag;				// ����� ������
		struct
		{
			uint8_t fNotCalibr			:1; 	// �� ���� ��������� �������, ������ ��� �� ������������
			uint8_t fBadCalibr			:1; 	// ������ ���������� �� ��� ��� ���� �������� (��� � ���� �������� ���������� ��� ���������� ���� ��� ��� � ���� ��������)	
			uint8_t fNoCNT				:1; 	// ��� �������� �������� �������� ����� ��� ��������. ��������� ���� ������. ����� �������� ������ � ������ ������ ����������� ��������
			uint8_t fNoMotorPower		:1; 	// ��� ���������� �� ������
			uint8_t fNoPressure 		:1; 	// ��� ��������� ������� ��������
			uint8_t fERReserved 		:3; 	// ���������������� 
		};
	};
	
	
	union
	{
		uint16_t		LogicalFlag;			// ���������� �����, �������������� ������ ������ ���������
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
		};
	};
} RUS_Regul_t;

// ��������� ������������ �������
int RUS_Regul_Motor_Speed_Set(float speed_percent);
void MotorCurrentSpeed_Get(void);
static void RUS_Regul_ServiceTimerCallback( TimerHandle_t xTimer );
void RUS_Regul_Motor_Stop(void);
void RUS_Regul_Motor_State_Get(void);
void RUS_Regul_Main_Struct_Update(void);
void RUS_Regul_Press_Calibration_Start(void);
void RUS_Regul_Absolute_Angle_Set(int16_t angle, float speed_percent);
float RUS_Regul_Motor_Angle_Get(void);
void EX_RUS_Regul_Motor_Speed_Set(float speed_percent);
void EX_RUS_Regul_Motor_Angle_Set(int16_t angle, float speed_percent);
void RUS_Regul_Overlap_Set(int16_t percent);
float RUS_Regul_Motor_Long_Angle_Get(void);
void RUS_Regul_OPAMP_Timer_Create(void);
void Motor_Press_Init(void);

#if defined(DEBUG_OUTPUT)
void Debug_Timer_Start(void);

typedef struct Debug_Output_struct
{
	char marker_1;
	char marker_2;	
	uint8_t ReadyCount;
} Debug_Output_t;

#endif



//HeapSort
void heapsort_2(float* arr, uint16_t N);



#endif //VIKPB_MAIN_H
