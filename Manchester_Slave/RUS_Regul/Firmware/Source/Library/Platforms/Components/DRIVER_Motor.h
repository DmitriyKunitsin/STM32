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
	Standing = 0,				// 	[0]		Стою
	Rotating					// 	[1]		Вращаюсь бесконечно
} WorkState_t;
#pragma pack()
#endif

void Motor_Rotation_Set(DirRotation_t Rotation);

#if 0
#pragma pack(1)
typedef enum Motor_WorMode_enum
{
	Ready = 0,					//	[0]		Готов к выполнению дальнейших команд
	CalibratingPress,			//	[1]		Калибруюсь по давлению
	CalibratingConsumption,		//	[2]		Калибруюсь по потреблению
	RotatingByAngle,			// 	[3]		Вращаюсь на определенный угол	
	HoldingPressure,			//	[4]		Удерживаю давление
	SettingPressure,			//	[5]		Выставляю давление
	SettingOverlap,				//	[6]		Выставляю перекрытие
	SettingCalibrNullPos,		//	[7]		Иду в минмальное давление после калибровки
	DiscoverOverCurrent			//	[8]		Обнаружено превышение тока потребления
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
	DirRotation_t 	Rotation;				// Направление вращения мотора
	RTOS_Timers_t	Timers;					// Таймеры RTOS (иногда нужно оставновить какой-то таймер, забрать из этой структуры)
	float			SpeedSet;				// Выставленная скорость в %
	float			SpeedMeas;				// Измеренная скорость об/мин
	float			AngleSet;				// Выставленный угол поворота
	float			CurPosition;			// Текущая позиция
	float			longCurPosition;		// Та же текущая позиция, только угол от 0 до ARR_COUNT
	uint16_t		TimerCNTValue;			// Значение счетчика таймера
	float			Press;					// Измеренное давление
	float			PressSet;				// Выставленное давление для удержания
	RUS_Regul_Calibr_Press_t	Settings;	// куда надо туда и поставишь!!!
	float			MotorPowerSupply;		// Измеренное напряжение + 12В
	float			MotorCurrent;			// Измеренный ток потребления
	float			CalibrNull;				// Полученное смещение текущего положения по результатам калибровки (0 смещается в точку минимума давления)
	float			MinAngle;				// Минимальный угол за калибровку
	float			MaxAngle;				// Максимальный угол за калибровку
	float			MinPress;				// Минимальное давление за калибровку
	float			MaxPress;				// Максимальное давление за калибровку
	float			WorkZone;				// Рабочая зона (maxAngle - minAngle)
	float			OPAMPThreshold;			// Порог тока потребления, при превышении которого мотор должен быть остановлен
	RUS_Regul_Motor_Workstate_t		WorkState;				// Состояние работы
	RUS_Regul_Motor_Workmode_t		WorkMode;				// Режим работы

	union{
		uint8_t 		ErrorFlag;				// Флаги ошибок
		struct
		{
			uint8_t	fNotCalibr			:1;		// Не могу выполнить команду, потому что не откалиброван
			uint8_t	fBadCalibr			:1;		// Плохая калибровка по тем или иным причинам (мин и макс давления одинаковые или одинаковые углы при мин и макс давлении)	
			uint8_t	fNoCNT				:1;		// Нет значений счетчика обратной связи при вращении. Фатальный флаг ошибки. Мотор крутится только в ручном режиме выставления скорости
			uint8_t	fNoMotorPower		:1;		// Нет напряжения на моторе по причне опущенного выхода контроллера
			uint8_t	fNoPressure			:1;		// Нет измерений датчика давления
			uint8_t fStuck				:1;		// Понял, что застрял по токоограничителю
			uint8_t fERReserved			:2;		// Зарезервированно	
		};
	};

	union{
		uint16_t		LogicalFlag;			// Логические флаги, корректирующие логику работы программы
		struct
		{
			uint16_t fReverseLogic		:1;		// Флаг реверсивной логики. 1 если при калибровке минимальный угол ближе к 360, чем максимальный, и 0 если наоборот
			uint16_t fWorkOverCurrent	:1;		// Флаг первышения рабочего потребления
			uint16_t fExOverCurrent		:1;		// Расширенный флаг превышения потребления, поднимается только по команде когда надо повысить предел по потреблению
			uint16_t fCalibrStart		:1;		// Флаг начала калибровки
			uint16_t fCalibrated		:1;		// Флаг того, что калибровка была выполнена успешно
			uint16_t fHoldPress			:1;		// Флаг выставления режима удержания давления
			uint16_t fCheckOPAMPValue	:1;		// Флаг готовности измерить значение потребления
			uint16_t fFoundLimit		:1;		// Флаг нахождения границы при калибровке по потреблению
			uint16_t fClearCNTValue		:1;		// Флаг сброса значения счетчика CNT
			uint16_t fSetCalNullPos		:1;		// Флаг выставления позиции в "нулевое"(= в положение с минимальным давлением) после завершения калибровки
			uint16_t fLFReserved		:6;		// Зарезервированно	
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
