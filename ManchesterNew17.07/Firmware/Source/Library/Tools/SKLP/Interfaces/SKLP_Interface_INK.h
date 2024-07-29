// SKLP_Interface_INK.h
// Командный интерфейс ИНК (ВНИИА, импульсно-нейтронный каротаж) на шине СКЛ
// Контроллер ЛУЧ.636.00.01.00
#ifndef	SKLP_INTERFACE_INK_H
#define	SKLP_INTERFACE_INK_H

#include <stdint.h>
#include "SKLP_Time.h"				// SKLP_Time_t
//#include "NVM.h"					// NVM_Result_t
#include "SKLP_MS_Transport.h"		// SKLP_Signature
#include "SKLP_Service.h"			// SKLP_FlagsModule_t
#include <stdint.h>

#define	INK_TIMING_ND_WINDOWS_COUNT_SKLP	60	// 58?			количество окон счетов нейтронов длЯ передачи по SKLP (число может быть уменьшено, чтобы влазить в размер пакета)

// Экспортируемые типы данных
typedef uint16_t INK_ND_CntSumm_t;	// [счета]	сумма счетов нейтронов в окнах
//typedef uint16_t INK_GK_ChargeSumm_t;	// [ADC]	сумма итеграторов гаммы

// Индексы зондов гаммы и нейтронов
typedef enum INK_Probes_enum
{
	iINK_ProbeNear = 0,		// ближний зонд
	iINK_ProbeFar,			// дальний зонд
	iINK_ProbesTotal,		// всего зондов
} INK_Probes_t;

// Специфические команды протокола СКЛ длЯ модулЯ ИНК 
typedef enum SKLP_Command_INK_enum
{
	SKLP_COMMAND_INK_DATA_TECH_SET			= 0x11,		// установка технологических параметров
	SKLP_COMMAND_INK_DATA_MAIN_GET			= 0x13,		// запрос блока основных данных, сброс блоков накопленных и основных данных
	SKLP_COMMAND_INK_DATA_TECH_GET			= 0x14,		// запрос блока технологических данных
	SKLP_COMMAND_INK_DATA_ACCUM_GET			= 0x15,		// запрос блока накопленных данных
	SKLP_COMMAND_INK_DATA_ASCII_GET 		= 0x16, 	// запрос блока накопленных данных в формате ASCII, не  соответствует SKLP!
	SKLP_COMMAND_INK_START_SINGLE			= 0xFF, 	// запуск однокоратного цикла измерениЯ
} SKLP_Command_INK_t;

// Флаги ИНК
#pragma pack( 1 )
typedef union SKLP_FlagsModuleINK_union
{
	uint8_t Byte;
	struct
	{
		uint8_t	fDataSaving			:1;		// автономный режим (не используетсЯ)
		uint8_t fWorkEnable			:1;		// разрешение работы по команде синхронизации времени
		uint8_t fTF_MnG				:1;		// рабочий отклонтель (0 - гравитационный, 1 - магнитный)
		uint8_t fFailPower			:1;		// отказ по питанию
		uint8_t fFailPulse			:1;		// отказ формирователя нейтронных импульсов
		uint8_t fFailGamma			:1;		// отказ измерителя гаммы
		uint8_t fFailNeutron		:1;		// отказ измерителя нейтронов
		uint8_t fFailACPTF			:1;		// отказ акустического каверномера или измерителя угла отклонителя 
	};
} SKLP_FlagsModuleINK_t;
#pragma pack( )

// *****************************************************************************
// [0x13] Ответ на команду запроса блока основных данных, сброс блоков накопленных и основных данных
// *****************************************************************************
#pragma pack( 1 )
typedef struct SKLP_INK_DataMainGet_Answer
{
	SKLP_FlagsModuleINK_t	Flags;			//	[1]			флаги
	uint16_t				RHOB;			// [2]	[??]	плотность
	uint16_t				TRNP;			// [2]	[??]	поритстость
	uint8_t					PulseCnt;		// [1]			счетчик импульсов, по которым накполены данные
	uint8_t					ACP_TF;			// [1]	[x2гр]	положение отклонителЯ
	uint8_t					ACP_R;			// [1]	[мм]	радиус
	uint8_t					ACP_A;			// [1]	[у.е.]	амплитуда акустического сигнала
	uint8_t					Temp;			// [1]	[грЦ+55]	температура
} SKLP_INK_DataMainGet_Answer_t;	// [10]
#pragma pack( )

// *****************************************************************************
// [0x11] Запрос на установку технологических параметров
// *****************************************************************************
#pragma pack( 1 )
typedef struct SKLP_INK_DataTechSet_Query
{
	uint16_t				UHV_Pulse;		// [2]	[В]		напряжение питания на высоковольтном источнике излучателя нейтронов
	uint16_t				UHV_GK;			// [2]	[В]		напряжение питания на высоковольтном источнике приемника гаммы
	uint16_t				UHV_ND;			// [2]	[В]		напряжение питания на высоковольтном источнике счетчика нейтронов
} SKLP_INK_DataTechSet_Query_t;	// [6]
#pragma pack( )

// *****************************************************************************
// [0x14] Ответ на команду запроса блока технологичсеких данных
// *****************************************************************************
#pragma pack( 1 )
typedef struct SKLP_INK_DataTechGet_Answer
{
	SKLP_FlagsModuleINK_t	Flags;			//	[1]			флаги
	uint16_t				UHV_Pulse;		// [2]	[В]		напряжение питания на высоковольтном источнике излучателя нейтронов
	uint16_t				UHV_GK;			// [2]	[В]		напряжение питания на высоковольтном источнике приемника гаммы
	uint16_t				UHV_ND;			// [2]	[В]		напряжение питания на высоковольтном источнике счетчика нейтронов
	uint8_t					TimeReady;		// [1]	[мс]	времЯ готовности излучателя нейтронов к работе
	uint16_t				PulseCntTotal;	// [2]			счетчик импульсов от момента включениЯ
	uint8_t					Temp;			// [1]	[грЦ+55]	температура
} SKLP_INK_DataTechGet_Answer_t;	// [9]
#pragma pack( )

// *****************************************************************************
// [0x15] Ответ на команду запроса блока накопленных данных
// *****************************************************************************
#pragma pack( 1 )
typedef struct SKLP_INK_DataAccumGet_Answer
{
	SKLP_FlagsModuleINK_t	Flags;								//	[1]			флаги
	uint8_t					PulseCnt;							// [1]			счетчик импульсов, по которым накполены данные
	uint16_t				aGammaIdle_mV[iINK_ProbesTotal];	// [4]	[мВ]	усредненные напрЯжениЯ на интеграторах гаммы перед импульсом
	uint16_t				aGammaPulse_mV[iINK_ProbesTotal];	// [4]	[мВ]	усредненные напрЯжениЯ на интеграторах гаммы после импульса
	INK_ND_CntSumm_t		aNeutronCnt[INK_TIMING_ND_WINDOWS_COUNT_SKLP][iINK_ProbesTotal];	// [240]	[у.е.]	суммы счетов нейтронов по-оконно
} SKLP_INK_DataAccumGet_Answer_t;	// [250]
#pragma pack( )

//STATIC_ASSERT( sizeof( SKLP_INK_DataAccumGet_Answer_t ) <= SKLP_SIZE_DATA_MAX_ANSWER );

#endif	// SKLP_INTERFACE_INK_H
