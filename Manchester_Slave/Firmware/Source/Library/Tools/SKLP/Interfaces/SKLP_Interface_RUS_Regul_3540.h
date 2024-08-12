// SKLP_Interface_GGLP_SADC.h
// Командный интерфейс спектрометрического АЦП длЯ ГГК-ЛП на шине СКЛ
// Контроллер ЛУЧ.638.00.02.00 в составе модулЯ ЛУЧ.638.00.00.00
#ifndef	SKLP_INTERFACE_RUS_REGUL_V2_H
#define	SKLP_INTERFACE_RUS_REGUL_V2_H

#include <stdint.h>
#include "SKLP_Time.h"				// SKLP_Time_t
#include "NVM.h"					// NVM_Result_t
#include "SKLP_MS_Transport.h"		// SKLP_Signature
#include "SKLP_Service.h"			// SKLP_FlagsModule_t, SKLP_CommandResult_t
#include "MathUtils.h"				// Float16_t
#include <stdint.h>

// Команды регулятора
typedef enum SKLP_Command_RUS_Regul_enum
{
	// Стандартные номера команд
	//SKLP_COMMAND_RUS_REGUL_ID_GET					= 0x01, // Стандартная команда: отдать ID модуля (адрес платы? (регулятор в составе РУС, у РУСа ID, у регулятора номер))
	SLKP_COMMAND_RUS_REGUL_NVM_GET					= 0x04, // Стандартная команда: прочитать данные из NVM (калибровка дачтика давления, серийный номер) 
	SLKP_COMMAND_RUS_REGUL_NVM_SET					= 0x05, // Стандартная команда: записать данные в NVM (калибровка датчика давления) (не факт, что реализованно)
	SKLP_COMMAND_RUS_REGUL_MAIN_DATA_GET			= 0x13, // Стандартная команда: отдать блок основных данных (!!!не реализованно!!!)
	SKLP_COMMAND_RUS_REGUL_EXTENDEED_DATA_GET		= 0x14, // Стандартная команда: отдать расширенный блок данных (для логирования)

	// Команды управления регулятором в процессе бурения (ПБ)
	SKLP_COMMAND_RUS_REGUL_MOTOR_POWER_MANAGE		= 0x60, // Подать питание на мотор
	SKLP_COMMAND_RUS_REGUL_MAX_CURRENT_SET			= 0x61, // Установить порог по току потребления мотора
	SKLP_COMMAND_RUS_REGUL_OVERLAP_SET				= 0x62, // Выставить процент перекрытия заслонки 
	SKLP_COMMAND_RUS_REGUL_CALIBR_PRESS_SET			= 0x63, // Запустить калибровку по давлению					
	SKLP_COMMAND_RUS_REGUL_CALIBR_CURRENT_SET		= 0x64, // Запустить калибровку по току потребления (если заслонка с упорами)
	SKLP_COMMAND_RUS_REGUL_PRESS_STABILIZATION		= 0x65, // Запустить процесс стабилизации давления (частичная реализация)
	SKLP_COMMAND_RUS_REGUL_ADVANCED_MANAGMENT		= 0x70, // Доступ к расширенному блоку данных (команды с префиксом EX_) (пока что команды доступны сразу)

	EX_SKLP_COMMAND_RUS_REGUL_ANGLE_SET				= 0x71, // Команда расширенного управления: высталение любого угла
	EX_SKLP_COMMAND_RUS_REGUL_SPEED_SET				= 0x72, // Выставление скорости  [%]	[0-100]
	
} SKLP_Command_RUS_Regul_t;

// [0x04] Команда чтениЯ блока из энергонезависимой памЯти	
// BlockID длЯ идентификации блока на чтение\запись блоков в EEPROM
#pragma pack(1)
typedef enum SKLP_NVM_ID_enum
{
	eSKLP_NVM_PressCoeff	= 0,	// Коэффициенты датчика давления
	eSKLP_NVM_MotorCalibr	= 1,	// Максимальная скорость вращения и ограничение по току
	eSKLP_NVM_NotDefined	= 0xFF,
} SKLP_NVM_ID_t;
#pragma pack()

// Структура тела команды чтениЯ блока из энергонезависимой памЯти
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
	uint8_t			xData;			// !! с этого адреса размещаетсЯ блок данных произвольного размера !!
} SKLP_Command_RUS_Regul_NVM_GetSet_Answer_t;
#pragma pack()

// eSKLP_NVM_PressCoeff = 0 Коэффициенты датчика давления
#pragma pack(1)
typedef struct RUS_Regul_Calibr_Press_struct
{
	// Калибровочные коэффициенты ДД. По полиному C2 * ADC^2 + C1 * ADC + C0
	SKLP_Time6_t date;				//	[6]
	// Все коэффициенты пока что в атмосферах
	float Press_Calibr_Coef_C2;		//	[4]		
	float Press_Calibr_Coef_C1;		//	[4]
	float Press_Calibr_Coef_C0;		//	[4]
} RUS_Regul_Calibr_Press_t;			//	[18]
#pragma pack()

#pragma pack(1)
typedef struct RUS_Regul_Calibr_Motor_struct
{
	SKLP_Time6_t date;	//	[6]	
	float MaxCurrent; 	// 	[4]	[А]		 Макисмальный ток потребления
	float MaxRotSpeed; 	//	[4]	[0 - 100%] 	 Максимальная скорость вращения мотора
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
	int16_t	Press;	// [2]	[атм]		[,00]	Измеренное давление	
} SKLP_Command_RUS_Regul_Main_Data_get_t; // [пока что 2]
#pragma pack()

#pragma pack(1)
typedef struct SKLP_RUS_Regul_NVM_Set_Query_struct
{
	SKLP_NVM_ID_t 	SKLP_NVM_ID;
	uint8_t 		xData;				// !! с этого адреса размещаетсЯ блок данных произвольного размера!!
	uint16_t 		CRC16;				// !! размещение полЯ CRC16 зависит от размера полЯ данных!!
} SKLP_RUS_Regul_NVM_Set_Query_t;
#pragma pack()


// отдельный калибровочный блок
#pragma pack(1)
typedef struct RUS_Regul_Positioning_Data_struct
{
	uint16_t		MinAngle;				// [2]	[град]		[,00]	Минимальный угол за калибровку
	uint16_t		MaxAngle;				// [2]	[град]		[,00]	Максимальный угол за калибровку
	int16_t			MinPress;				// [2]	[атм]		[,00]	Минимальное давление за калибровку
	int16_t			MaxPress;				// [2]	[атм]		[,00]	Максимальное давление за калибровку
	uint16_t		WorkZone;				// [2]	[град]		[,00]	Рабочая зона (maxAngle - minAngle)
} RUS_Regul_Positioning_Data_t;
#pragma pack()

#pragma pack(1)
typedef struct SKLP_Command_RUS_Regul_Positioning_Data_Set_Answer
{
	uint8_t 	error;			//	[1]		0 = OK, 1 = ERROR
	uint16_t	LogicFlags;		//	[2]		Флаги логики
	uint8_t		ErrorFlags;		//	[1]		Флаги ошибок
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
	Standing = 0,				// 	[0]		Стою
	Rotating					// 	[1]		Вращаюсь бесконечно
} RUS_Regul_Motor_Workstate_t;
#pragma pack()


#pragma pack(1)
typedef enum RUS_Regul_Motor_Workmode_enum
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
} RUS_Regul_Motor_Workmode_t;
#pragma pack()


//[0x14]
#pragma pack(1)
typedef struct SKLP_Command_RUS_Regul_Extendeed_Data_Get_Answer
{
	union
	{
		uint8_t 		ErrorFlag;				//	[1] Флаги ошибок
		struct
		{
			uint8_t fNotCalibr			:1; 	// Не могу выполнить команду, потому что не откалиброван
			uint8_t fBadCalibr			:1; 	// Плохая калибровка по тем или иным причинам (мин и макс давления одинаковые или одинаковые углы при мин и макс давлении)	
			uint8_t fNoCNT				:1; 	// Нет значений счетчика обратной связи при вращении. Фатальный флаг ошибки. Мотор крутится только в ручном режиме выставления скорости
			uint8_t fNoMotorPower		:1; 	// Нет напряжения на моторе
			uint8_t fNoPressure 		:1; 	// Нет измерений датчика давления
			uint8_t fERReserved 		:3; 	// Зарезервированно 
		};
	};		// [1]
	
	
	union
	{
		uint16_t		LogicalFlag;			// 	[2]	Логические флаги, корректирующие алгоритмы работы программы
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
		};	// [2]
	};

	int8_t			SpeedSet;				// [1]	[0-100%]	[0]		Выставленная скорость
	int16_t			SpeedMeas;				// [2]	[об/мин]	[,00]	Измеренная скорость об/мин
	uint16_t		AngleSet;				// [2]	[град]		[,00]	Выставленный угол поворота
	uint16_t		CurPosition;			// [2]	[град]		[,00]	Текущая позиция
	uint16_t		TimerCNTValue;			// [2]	[-]			[0]		Значение счетчика таймера
	int16_t			Press;					// [2]	[атм]		[,00]	Измеренное давление
	int32_t			ADC_code;				// [4]  [-]			[0]		Давление - код АЦП
	int16_t			CurPress;				// [2]	[атм]		[,00]	Мгновенное давление
	uint16_t		TarPress;				// [2]	[атм]		[0]		Выставленное давление для удержания
	uint8_t			MotorPowerSupply;		// [1]	[В]			[,0]	Измеренное напряжение + 12В
	uint8_t			MotorCurrent;			// [1]	[мА]		[0]		Измеренный ток потребления
	uint16_t		CalibrNull;				// [2]	[град]		[,00]	Полученное смещение текущего положения по результатам калибровки (0 смещается в точку минимума давления)
	uint16_t		MinAngle;				// [2]	[град]		[,00]	Минимальный угол за калибровку
	uint16_t		MaxAngle;				// [2]	[град]		[,00]	Максимальный угол за калибровку
	int16_t			MinPress;				// [2]	[атм]		[,00]	Минимальное давление за калибровку
	int16_t			MaxPress;				// [2]	[атм]		[,00]	Максимальное давление за калибровку
	uint16_t		WorkZone;				// [2]	[град]		[,00]	Рабочая зона (maxAngle - minAngle)
	
	RUS_Regul_Motor_Workmode_t			WorkState;				// [1]  [-]			[0]		Примитивное состояние (вращение или остановлен)
	RUS_Regul_Motor_Workstate_t			WorkMode;				// [1]	[-]			[0]		Режим работы
} SKLP_Command_RUS_Regul_Extendeed_Data_Get_Answer_t;	// [38] - Общий размер посылки
#pragma pack()

#pragma pack(1)
typedef struct SKLP_Command_RUS_Regul_Motor_Power_Managment_Query
{
	uint8_t 	pwrOn; 		// 	[1]		Параметр команды. Если  1 - подать питание, если 0 - снять питание
} SKLP_Command_RUS_Regul_Motor_Power_Managment_Query_t;
#pragma pack()

#pragma pack(1)
typedef struct SKLP_Command_RUS_Regul_Motor_Power_Managment_Answer
{
	uint8_t 	Power;		// 	[1]		Ответ 1 - есть питание, 0 - нет питания 
	uint16_t 	LogicFlag;	//	[2]		Флаги логики 
	uint8_t 	ErrorFlags;	//	[1]		Флаги ошибок
} SKLP_Command_RUS_Regul_Motor_Power_Managment_Answer_t;
#pragma pack()

#pragma pack(1)
typedef struct SKLP_Command_RUS_Regul_Overlap_Set_Query_struct
{
	int16_t OverlapPercent;			//	[1]		[%]	[0..+100]	Процент перекрытия заслонки
} SKLP_Command_RUS_Regul_Overlap_Set_Query_t; // [1]
#pragma pack()

#pragma pack(1)
typedef struct SKLP_Command_RUS_Regul_Overlap_Set_Answer_struct
{
	uint8_t 	error;			//	[1]		[-]		[0/1]	0 - Все ОК, 1 - ошибка, не откалиброван
	uint16_t	LogicFlag;		//	[2]		[-]		Флаги логики
	uint8_t		ErrorFlag;		//	[1]		[-]		Флаги ошибок
} SKLP_Command_RUS_Regul_Overlap_Set_Answer_t; // [1]
#pragma pack()


#pragma pack(1)
typedef struct SKLP_Command_HoldPress_Query_struct
{
	uint16_t tarPress;	// [4] [атм] Давление для удержания 
} SKLP_HoldPress_Query_t;
#pragma pack()

#pragma pack(1)
typedef struct EX_SKLP_ProcessCommand_RUS_Regul_Speed_Set_Query
{
	float speed_percent; //	[4]	[%]	[-100..100] Установка скорости мотора
} EX_SKLP_ProcessCommand_RUS_Regul_Speed_Set_Query_t;
#pragma pack()

#pragma pack(1)
typedef struct EX_SKLP_ProcessCommand_RUS_Regul_Angle_Set_Query
{
	int16_t angle;		//	[2]	[°]	Угол в градусах - любой от -inf до +inf (отображение выставленного угла может быть некорректным)	
	float speed;		//	[4]	[%]	Скорость в процентах от 0 до 100
}EX_SKLP_Command_RUS_Regul_Angle_Set_Query_t;
#pragma pack()

#endif
