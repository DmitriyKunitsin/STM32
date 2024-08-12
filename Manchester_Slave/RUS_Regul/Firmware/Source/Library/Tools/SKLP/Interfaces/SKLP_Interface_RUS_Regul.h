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
	SKLP_COMMAND_RUS_REGUL_OVERLAP_SET				= 0x62, // Выставить процент перекрытия заслонки (!!!не реализованно!!! (в процессе))
	SKLP_COMMAND_RUS_REGUL_CALIBR_PRESS_SET			= 0x63, // Запустить калибровку по давлению					
	SKLP_COMMAND_RUS_REGUL_CALIBR_CURRENT_SET		= 0x64, // Запустить калибровку по току потребления (если заслонка с упорами)
	SKLP_COMMAND_RUS_REGUL_PRESS_STABILIZATION		= 0x65, // Запустить процесс стабилизации давления (частичная реализация)
	SKLP_COMMAND_RUS_REGUL_ADVANCED_MANAGMENT		= 0x70, // Доступ к расширенному блоку данных (команды с префиксом EX_) (пока что команды доступны сразу)

	EX_SKLP_COMMAND_RUS_REGUL_ANGLE_SET				= 0x71, // Команда расширенного управления: высталение любого угла
	EX_SKLP_COMMAND_RUS_REGUL_SPEED_SET				= 0x72, // Выставление любой скорости (Регулятор схавает любую команду, но фактически скорость ограничена 0 - мин, 100 - макс)
	
} SKLP_Command_RUS_Regul_t;


// [0x04] Команда чтениЯ блока из энергонезависимой памЯти	
// BlockID длЯ идентификации блока на чтение\запись блоков в EEPROM
#pragma pack(1)
typedef enum SKLP_NVM_ID_enum
{
	eSKLP_NVM_PressCoeff				= 0,	// Коэффициенты датчика давления
	eSKLP_NVM_NotDefined				= 0xFF,
} SKLP_NVM_ID_t;
#pragma pack()

// Структура тела команды чтениЯ блока из энергонезависимой памЯти
#pragma pack(1)
typedef struct SKLP_RUS_Regul_NVM_Get_Query
{
	SKLP_NVM_ID_t SKLP_NVM_ID;
} SKLP_RUS_Regul_NVM_Get_Query_t;
#pragma pack() 

#pragma pack(1)
typedef struct SKLP_RUS_Regul_NVM_GetSet_Answer
{
	NVM_Result_t	Result;
	uint8_t			xData;			// !! с этого адреса размещаетсЯ блок данных произвольного размера!!
} SKLP_RUS_Regul_NVM_GetSet_Answer_t;
#pragma pack()

// eSKLP_NVM_PressCoeff = 0 Коэффициенты датчика давления
#pragma pack(1)
typedef struct RUS_Regul_Calibr_Press_struct
{
	SKLP_Time6_t date;
	float Press_Calibr_Coef_C2;
	float Press_Calibr_Coef_C1;
	float Press_Calibr_Coef_C0;	
}RUS_Regul_Calibr_Press_t;
#pragma pack()

#pragma pack(1)
typedef struct RUS_Regul_Calibr_Motor_struct
{
	SKLP_Time6_t date;
	float MaxCurrent; // [мА] Макисмальный ток потребления
	float MaxRotSpeed; // [0-100%] Макс скорость вращения мотора
}RUS_Regul_Calibr_Motor_t;
#pragma pack()


#pragma pack(1)
typedef struct SKLP_Command_RUS_Regul_NVM_Get_Answer
{
	// Калибровочные коэффициенты ДД. По полиному C2 * ADC^2 + C1 * ADC + C0
	SKLP_Time6_t date;
	float Press_Calibr_Coef_C2;
	float Press_Calibr_Coef_C1;
	float Press_Calibr_Coef_C0;
} SKLP_Command_RUS_Regul_NVM_Get_Answer_t;
#pragma pack()

#pragma pack(1)
typedef struct SKLP_Command_RUS_Regul_NVM_Set_Query
{
	RUS_Regul_Calibr_Press_t CalibrPressData;
	RUS_Regul_Calibr_Motor_t MotorPressData;
	// Добавить предельный ток
	// Добавить скорость вращения
} SKLP_Command_RUS_Regul_NVM_Query_t;
#pragma pack()

//[0x13]
#pragma pack(1)
typedef struct SKLP_Command_RUS_Regul_Main_Data_Answer_struct
{
	uint8_t test;
} SKLP_Command_RUS_Regul_Main_Data_get_struct_t;
#pragma pack()

#pragma pack(1)
typedef struct SKLP_Command_RUS_Regul_Extendeed_Data_Get_Answer
{
	int8_t			SpeedSet;				// [1]	[0-100%]	[0]		Выставленная скорость
	uint16_t		SpeedMeas;				// [2]	[об/мин]	[,00]	Измеренная скорость об/мин
	uint16_t		AngleSet;				// [2]	[град]		[,00]	Выставленный угол поворота
	uint16_t		CurPosition;			// [2]	[град]		[,00]	Текущая позиция
	uint16_t		TimerCNTValue;			// [2]	[-]			[0]		Значение счетчика таймера
	int16_t			Press;					// [2]	[атм]		[,00]	Измеренное давление
	int32_t			ADC_code;				// [4]  [-]			[0]		Давление - код АЦП
	int16_t			CurPress;				// [2]	[атм]		[,00]	Мгновенное давление
	uint16_t		TarPress;				// [2]	[атм]		[0]		Выставленное давление для удержания
	uint16_t		MotorPowerSupply;		// [2]	[В]			[,00]	Измеренное напряжение + 12В
	uint16_t		MotorCurrent;			// [2]	[А]			[,00]	Измеренный ток потребления
	uint16_t		CalibrNull;				// [2]	[град]		[,00]	Полученное смещение текущего положения по результатам калибровки (0 смещается в точку минимума давления)
	uint16_t		MinAngle;				// [2]	[град]		[,00]	Минимальный угол за калибровку
	uint16_t		MaxAngle;				// [2]	[град]		[,00]	Максимальный угол за калибровку
	int16_t			MinPress;				// [2]	[атм]		[,00]	Минимальное давление за калибровку
	int16_t			MaxPress;				// [2]	[атм]		[,00]	Максимальное давление за калибровку
	uint16_t		WorkZone;				// [2]	[град]		[,00]	Рабочая зона (maxAngle - minAngle)
	uint8_t			WorkState;				// [1]  [-]			[0]		Примитивное состояние (вращение или остановлен)
	uint8_t			WorkMode;				// [1]	[-]			[0]		Режим работы
	uint8_t 		ErrorFlag;				// [1]	[-]			[0]		Флаг ошибки
	uint16_t		LogicalFlag;			// [2]	[-]			[0]		Логические флаги, корректирующие логику работы программы 
} SKLP_Command_RUS_Regul_Extendeed_Data_Get_Answer_t;	// [31] - Общий размер посылки
#pragma pack()

#pragma pack(1)
typedef struct SKLP_Command_RUS_Regul_Motor_Power_Managment_Query
{
	uint8_t pwrOn; // [1]	Параметр команды. Если  1 - подать питание, если 0 - снять питание
} SKLP_Command_RUS_Regul_Motor_Power_Managment_Query_t;
#pragma pack()

#pragma pack(1)
typedef struct SKLP_Command_RUS_Regul_Motor_Power_Managment_Answer
{
	uint8_t Power; // [1]	Ответ 1 - есть питание, 0 - нет питания 
} SKLP_Command_RUS_Regul_Motor_Power_Managment_Answer_t;
#pragma pack()

#pragma pack(1)
typedef struct SKLP_SetAngle_Query
{
	float speed_percent;		// [4]	[%]	[0..+100] 
	uint16_t AngleSet;			// [2]	[°]	[0..WorkArea]
} SKLP_SetAngle_Query_t;	// [6]
#pragma pack()

#pragma pack(1)
typedef struct SKLP_ProcessCommand_RUS_Regul_Overlap_Set_Query
{
	int16_t OverlapPercent;			//	[1]		[%]	[0..+100]	Процент перекрытия заслонки
} SKLP_ProcessCommand_RUS_Regul_Overlap_Set_Query_t; // [1]
#pragma pack()

#pragma pack(1)
typedef struct SKLP_ProcessCommand_RUS_Regul_Overlap_Set_Answer
{
	uint8_t error;			//	[1]		[-]	[0/1]	0 - Все ОК, 1 - ошибка, не откалиброван
} SKLP_ProcessCommand_RUS_Regul_Overlap_Set_Answer_t; // [1]
#pragma pack()


#pragma pack(1)
typedef struct SKLP_HoldPress_Query
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
}EX_SKLP_ProcessCommand_RUS_Regul_Angle_Set_Query_t;
#pragma pack()

#endif
