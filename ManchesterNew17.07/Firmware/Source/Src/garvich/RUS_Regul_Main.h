#ifndef	RUS_REGUL_MAIN_H
#define	RUS_REGUL_MAIN_H

#include "TaskConfig.h"
#include "NVM.h"			// NVM_Tag_t

#include "RUS_Regul_ADC.h"
#include "SKLP_Interface_RUS_Regul_3540.h"

//#define DEBUG_OUTPUT

// Величина промежутка времени таймера RTOS в RUS_Regul main task
#define		RUS_REGUL_WATCHDOGTIMER_S			1.0f 		// 1[с] период вызова коллбека таймера длЯ сброса WatchDog
#define 	RUS_REEGUL_DELAY_TIMEDELTA_MS		( 200 )		// [мс]  200 мс 
#define 	VIKPB_SERVICEUPDATE_TIMEDELTA_MS	( 100 )		// [мс]  100 мс 
#define 	RUS_REGUL_STARTDELAY1_MS			( 650 )		// [мс]	 Возможные задержки
#define 	RUS_REGUL_STARTDELAY2_MS			( 1500 )	// [мс]  Возможные задержки

#define		ADS1231_GAIN						(128.0f)	// Усиление внешнего АЦП
#define		ADS1231_VDDA						(5.2f)		// Измерено на плате мультиметром
#define		ADS1231_MAX_CODE					(8388607.0f)// Максимальный код 24-битного внешнего АЦП (±MAX_CODE)
#define		ADS1231_V_TO_MV						(1000.0f)	// Перевод из В в мВ
#define		ADS1231_CALIBR_COEF_A				(40.27f)	// Коэффициент калибровки A датчика далвения (press * A + B)
#define		ADS1231_CALIBR_COEF_B				(6.2f)		// Коэффициент калибровки B датчика давления (press * A + B)
#define		CONVERT_TO_BAR						(((((0.5f * ADS1231_VDDA) / ADS1231_GAIN) / ADS1231_MAX_CODE) * ADS1231_V_TO_MV) / ADS1231_VDDA)

#if defined(TEST_BOARD)
#define		TEST_COEF_A							(1000.0f)
#define		TEST_COEF_B							(-13.48f)
#endif

#define		PRESS_MEDIAN_SIZE					(40)		// Размер массива медианного фильтра (раз в секунду массив полностью запонен, потому что скорость АЦП 80 значений в секунду)
#define		PRESS_MEDIAN_HALF_SIZE				((PRESS_MEDIAN_SIZE) / (2)) // Для индекса, по которому надо взять значение
#define		PRESS_WINDOW						(30)		// Размер окна для скользящего среднего	

#define		BETA_COEF_FILTER					(1.0f / 2.0f) // Коэффициент бета для бета фильтра

#define 	CURRENT_LIMITER_VALUE				(1.0f)	// [A] Значение токоограничителя. (Добавить возможность записать его сверху командой)

typedef struct RUS_Regul_Main_Struct
{
	TimerHandle_t	xStartDelayTimer;		// Таймер задержки старта на 650 мс
	float			SpeedSet;				// Выставленная скорость в %
	float			SpeedMeas;				// Измеренная скорость об/мин
	float			AngleSet;				// Выставленный угол поворота
	float			CurPosition;			// Текущая позиция
	float			longCurPosition;		// Та же текущая позиция, только от 0 до ARR_COUNT
	uint16_t		TimerCNTValue;			// Значение счетчика таймера
	float			Press;					// Измеренное давление (отфильтровано)
	uint16_t		PressSet;				// Выставленное давление для удержания
	float			MotorPowerSupply;		// Измеренное напряжение + 12В
	float			MotorCurrent;			// Измеренный ток потребления
	float			CalibrNull;				// Полученное смещение текущего положения по результатам калибровки (0 смещается в точку минимума давления)
	float			MinAngle;				// Минимальный угол за калибровку
	float			MaxAngle;				// Максимальный угол за калибровку
	float			MinPress;				// Минимальное давление за калибровку
	float			MaxPress;				// Максимальное давление за калибровку
	float			WorkZone;				// Рабочая зона (maxAngle - minAngle)
	int32_t			ADC_Code;				// Давление в кодах АЦП
	float			CurPress;				// Мгновенное давление
	uint8_t			WorkMode;				// Режим работы
	uint8_t			WorkState;				// Примитивное состояние мотора (остановлен/ вращение)
	uint8_t			BoardAdress;			// Адрес платы (определяется автоматически в TransportLocal_354_10)
	
	union
	{
		uint8_t 		ErrorFlag;				// Флаги ошибок
		struct
		{
			uint8_t fNotCalibr			:1; 	// Не могу выполнить команду, потому что не откалиброван
			uint8_t fBadCalibr			:1; 	// Плохая калибровка по тем или иным причинам (мин и макс давления одинаковые или одинаковые углы при мин и макс давлении)	
			uint8_t fNoCNT				:1; 	// Нет значений счетчика обратной связи при вращении. Фатальный флаг ошибки. Мотор крутится только в ручном режиме выставления скорости
			uint8_t fNoMotorPower		:1; 	// Нет напряжения на моторе
			uint8_t fNoPressure 		:1; 	// Нет измерений датчика давления
			uint8_t fERReserved 		:3; 	// Зарезервированно 
		};
	};
	
	
	union
	{
		uint16_t		LogicalFlag;			// Логические флаги, корректирующие логику работы программы
		struct
		{
			uint16_t fReverseLogic		:1; 	// Флаг реверсивной логики. 1 если при калибровке минимальный угол ближе к 360, чем максимальный, и 0 если наоборот
			uint16_t fWorkOverCurrent	:1; 	// Флаг первышения рабочего потребления
			uint16_t fExOverCurrent 	:1; 	// Расширенный флаг превышения потребления, поднимается только по команде когда надо повысить предел по потреблению
			uint16_t fCalibrStart		:1; 	// Флаг начала калибровки
			uint16_t fCalibrated		:1; 	// Флаг того, что калибровка была выполнена успешно
			uint16_t fHoldPress 		:1; 	// Флаг выставления режима удержания давления
			uint16_t fCheckOPAMPValue	:1; 	// Флаг готовности измерить значение потребления
			uint16_t fFoundLimit		:1; 	// Флаг нахождения границы при калибровке по потреблению
			uint16_t fClearCNTValue 	:1; 	// Флаг сброса значения счетчика CNT
			uint16_t fSetCalNullPos 	:1; 	// Флаг выставления позиции в "нулевое"(= в положение с минимальным давлением) после завершения калибровки
			uint16_t fPowerManage		:1;		// Флаг подачи питания на мотор
			uint16_t fLFReserved		:5; 	// Зарезервированно 
		};
	};
} RUS_Regul_t;

// Прототипы интерфейсных функций
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
