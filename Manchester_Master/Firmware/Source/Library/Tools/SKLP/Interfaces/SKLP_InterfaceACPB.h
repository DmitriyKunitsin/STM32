// SKLP_InterfaceACPB.h
//  омандный интерфейс модулей ј ѕ(б) на шине — Ћ:
// - Ћ”„.619.00.04.00 - контроллер трехзондового каверномера в составе ј ѕ(б)	Ћ”„.619.00.00.00
// - Ћ”„.623.00.02.00 - контроллер однозондового каверномера в составе √√ѕ		Ћ”„.623.00.00.00
// - Ћ”„.624.00.04.00 - контроллер трехзондового каверномера в составе √√ѕ/ј ѕ	Ћ”„.624.00.00.00
// - Ћ”„.625.00.02.00 - контроллер однозондового каверномера в составе √√ѕ		Ћ”„.625.00.00.00
// - Ћ”„.513.00.02.00 - контроллер трехзондового каверномера в составе ј ѕ(а)	Ћ”„.513.00.00.00
// возможна унификация с:
// - Ћ”„.435.00.01.00 - контроллер восьмизондового каверномера в составе ј ѕ	Ћ”„.679.00.00.00
#ifndef	SKLP_INTERFACE_ACPB_H
#define	SKLP_INTERFACE_ACPB_H

#include <stdint.h>
#include "SKLP_Service.h"
#include "SKLP_MS_Transport.h"
#include "SKLP_Time.h"

// јдреса устройств на шине — Ћ, задано в SKLP_MS_Transport.h
// SKLP_ADDRESS_ACP_679		// [0xB0]
// SKLP_ADDRESS_ACPB_623	// [0xB1]	в т.ч. 625
// SKLP_ADDRESS_ACPB_619	// [0xB2]
// SKLP_ADDRESS_ACPB_624	// [0xB1]
// SKLP_ADDRESS_ACPA_513	// [0xB1]

// “ип устройства для команды [0x01].v2 (применяется у всех модулей	 на модемном интерфейсе)
#define SKLP_DEVICE_TYPE_ACPB_619		( ( LoochDeviceType_t ) 0x6190 )
#define SKLP_DEVICE_TYPE_ACPB_623		( ( LoochDeviceType_t ) 0x6230 )
#define SKLP_DEVICE_TYPE_ACPB_624		( ( LoochDeviceType_t ) 0x6240 )
//  онтроллер Ћ”„.435.00.01.00 работает в составе комплекса — Ћ-ј по шине RS-485, там используется команда [0x01].v1, без типа устройства
//  онтроллер Ћ”„.513.00.02.00 работает в составе комплекса — Ћ-ј по шине RS-485, там используется команда [0x01].v1, без типа устройства

//  онстанты с суффиксом _ACP - для поддержки старого проекта ј ѕ(а) 619
//  онстанты без суффикса - для всех проектов новой серии, как ј ѕ(б), так и ј ѕ(а)
//  оличество оцифровок в осциллограмме одного зонда
#define	ACPB_OSC_FRAME_SIZE				2048
//  оличество основных измерительных каналов
#define	ACPB_CHANNEL_COUNT_MAIN_679		8
#define	ACPB_CHANNEL_COUNT_MAIN_619		3
#define	ACPB_CHANNEL_COUNT_MAIN_623		1
#define	ACPB_CHANNEL_COUNT_MAIN_624		3
#define	ACPB_CHANNEL_COUNT_MAIN_513		3
#define	ACPB_CHANNEL_COUNT_MAIN_ACP		ACPB_CHANNEL_COUNT_MAIN_679		// 8
#define	ACPB_CHANNEL_COUNT_MAIN			ACPB_CHANNEL_COUNT_MAIN_619		// 3
//  оличество опорных каналов
#define	ACPB_CHANNEL_COUNT_REF_679		1
#define	ACPB_CHANNEL_COUNT_REF_619		0
#define	ACPB_CHANNEL_COUNT_REF_623		0
#define	ACPB_CHANNEL_COUNT_REF_624		0
#define	ACPB_CHANNEL_COUNT_REF_513		0
#define	ACPB_CHANNEL_COUNT_REF_ACP		ACPB_CHANNEL_COUNT_REF_679		// 1
#define	ACPB_CHANNEL_COUNT_REF			ACPB_CHANNEL_COUNT_REF_619		// 0
//  оличество служебных каналов (для проверки математической обработки)
#define	ACPB_CHANNEL_COUNT_AUX_679		1
#define	ACPB_CHANNEL_COUNT_AUX_619		1
#define	ACPB_CHANNEL_COUNT_AUX_623		1
#define	ACPB_CHANNEL_COUNT_AUX_624		1
#define	ACPB_CHANNEL_COUNT_AUX_513		1
#define	ACPB_CHANNEL_COUNT_AUX_ACP		ACPB_CHANNEL_COUNT_AUX_679		// 1
#define	ACPB_CHANNEL_COUNT_AUX			ACPB_CHANNEL_COUNT_AUX_619		// 1
// ќбщее число каналов в пакете
#define ACPB_CHANNEL_COUNT_ALL_ACP		( ACPB_CHANNEL_COUNT_MAIN_ACP + ACPB_CHANNEL_COUNT_REF_ACP + ACPB_CHANNEL_COUNT_AUX_ACP )	// 10
#define ACPB_CHANNEL_COUNT_ALL			( ACPB_CHANNEL_COUNT_MAIN + ACPB_CHANNEL_COUNT_REF + ACPB_CHANNEL_COUNT_AUX )				// 4
// »дентификатор формата в составе заголовка основных данных
#define	ACPB_DATAHEADER_ID_ACP			0x01
#define	ACPB_DATAHEADER_ID				0x20

//  оличество определяемых положений отклонителя на один оборот
#define	ACPB_TF_STEP_COUNT				16		// каждый сектор по 22.5 градуса

// —пециализированные команды
typedef enum SKLP_Command_ACPB_enum
{
	SKLP_COMMAND_ACPB_CONTROL				= 0x60,								// [0x60]	—пециализированая команда для для разного, в старом ј ѕ было SKLP_COMMAND_AKP_CUSTOM
	SKLP_COMMAND_ACPB_DATA_GET_OSC			= SKLP_COMMAND_DATA_ACQ_GET_LONG,	// [0x03]	„тение крупных блоков (осциллограмм)
	SKLP_COMMAND_ACPB_DATA_GET_OSC_FRAGMENT	= 0x67,								// [0x67]	„тение крупных блоков (осциллограмм), фрагментами
	SKLP_COMMAND_ACPB_DATA_GET_INSTANT		= SKLP_COMMAND_DATA_ACQ_GET,		// [0x13]	„тение мгновенных данных
	SKLP_COMMAND_ACPB_DATA_GET_INTEGRAL 	= 0x15,						 		// [0x15]	„тение таблицы интегральных данных, с разложением отклонителям
	SKLP_COMMAND_ACPB_DATA_GET_DIR_EXT		= 0x19,								// [0x19]	„тение расширенных данных с датчиков ориентации
} SKLP_Command_ACPB_t;

// ‘лаги модуля ј ѕ(б)
#pragma pack( 1 )
typedef union SKLP_FlagsModuleACPB_union
{
	SKLP_FlagsModule_t Common;
	struct
	{
		uint8_t	fReserved0			:1; 	// флаг автономного режима. контролируетс€ протоколом. модуль не должен трогать этот флаг, в т.ч. считывать (??!!)
		uint8_t	fDataNotReady		:1; 	// флаг отсутствия подготовленных данных для считывания по команде [0x13]. —брасывается при завершении подготовки очередного ответа. ¬ыставляется при выполении команд [0xFF] и [0x13].
		uint8_t	fDataAcqFail 		:1; 	// флаг нарушения последнего цикла измерений по команде [0xFF].
		uint8_t	fGenHV_Fail			:1; 	// флаг нарушения обмена с высоковольтным генератором в ходе последнего цикла
		uint8_t	fReboot				:1; 	// произошел перезапуск по assert() или HardFault
		uint8_t	fReserved			:1;
		uint8_t fTF_Ready			:1;		// флаг готовности рассчетного отклонителя
		uint8_t	fTFG_nTFM			:1;		// флаг активного отклонителя: 0 - TFM (магнитный), 1 - TFG (гравитационный)
	};
} SKLP_FlagsModuleACPB_t;
#pragma pack( )


// —трукртуры запросов и ответов на частные команды по протоколу — Ћ

// *********************************************************************************
// [0x13]  оманда чтения мгновенных данных для ј ѕ(а), сильно отличается от команды для ј ѕ(б)
// *********************************************************************************
// SKLP.[0x13]	—труктура мгновенных данных для команды [0x13] для проекта ј ѕ(а)
#pragma pack( 1 )
typedef struct ACPA_ResultInstant13_struct
{
	uint16_t	aGaps[ACPB_CHANNEL_COUNT_ALL_ACP];	// [мм*0.1]	–ассчитанные рассто€ни€ до стенок скважины
	uint16_t	SpeedOfSound;						// [м/с]	—корость звука в среде, вычисленная по опорному каналу
	uint16_t	Diameter;							// [мм*0.1]	ƒиаметр скважины
} ACPA_ResultInstant13_t;	// [24]
#pragma pack( )

// —труктура ответа модул€ ј ѕ(а) на — Ћ-команду [0x13] "«апрос измеренных данных"
#pragma pack( 1 )
typedef struct SKLP_ACPA_DataInstantAnswer_struct
{
	SKLP_FlagsModuleACPB_t		FlagsModule;
	SKLP_MemoryFlags_t			MemoryFlags;
	ACPA_ResultInstant13_t		Data;
 	int16_t						Temp;			// [гр÷]		“емпература модул€
} SKLP_ACPA_DataInstantAnswer_t;
#pragma pack( )

// *********************************************************************************
// [0x13]  оманда чтения мгновенных данных (сильно отличается от ACP_FastResult_t проекта ј ѕ 679)
// *********************************************************************************
// SKLP.[0x13]	—труктура мгновенных данных для команды [0x13]
#pragma pack( 1 )
typedef struct ACPB_ResultInstant13_struct
{
	uint8_t		TF;		// [2гр/бит]	”гол отклонителя
	uint8_t		R0;		// [1мм/бит]	–адиус по первому зонду
	uint8_t		A0;		// [у.е.]		јмплитуда сигнала по первому зонду
	uint8_t		R1;		// [1мм/бит]	–адиус по второму зонду
	uint8_t		A1;		// [у.е.]		јмплитуда сигнала по второму зонду
	uint8_t		R2;		// [1мм/бит]	–адиус по третьему зонду
	uint8_t		A2;		// [у.е.]		јмплитуда сигнала по первому зонду
	uint8_t		Time;	// [20мс/бит]	¬ремя нахождения в этом секторе отклонителя (22.5гр) с последнего запроса
} ACPB_ResultInstant13_t;	// [8]
#pragma pack( )

// SKLP.[0x13]	 —труктура ответа модул€ ј ѕ(б) на команду "«апрос мгновенных данных"
#pragma pack( 1 )
typedef struct SKLP_ACPB_DataInstantAnswer_struct
{
	SKLP_FlagsModuleACPB_t		FlagsModule;	// [1]
	SKLP_MemoryFlags_t			MemoryFlags;	// [1]
	ACPB_ResultInstant13_t		Data;			// [8]
} SKLP_ACPB_DataInstantAnswer_t;	// [10]
#pragma pack( )

// *********************************************************************************
// [0x15]  оманда чтения таблицы интегральных данных, с разложением отклонителям
// *********************************************************************************
// SKLP.[0x15]	“аблица интегральных данных, с привязкой к углу отклонителя
#pragma pack( 1 )
typedef struct ACPB_ResultIntergral_struct
{
	struct
	{
		uint16_t	RadiusAvg;	// [2]	[0.1мм/бит]	”средненный радиус, накопленный за время нахождения зондов на этом отклонителе
		uint16_t	TimeSumm;	// [2]	[1мс/бит]	¬ремя нахождения зондов на этом отклонителе
	} aTableByTF[ACPB_TF_STEP_COUNT];
} ACPB_ResultIntergral_t;	// [64]
#pragma pack( )

// SKLP.[0x15]	—труктура мгновенных данных для команды [0x15]
#pragma pack( 1 )
typedef struct ACPB_ResultInstant15_struct
{
	uint16_t	TF;				// [2]	[1гр/бит]	”гол отклонителя
	uint16_t	Diam;			// [2]	[0.1мм/бит] ƒиаметр скважины
	uint16_t	R0;				// [2]	[0.1мм/бит] ѕервый радиус
	uint16_t	R1;				// [2]	[0.1мм/бит] ¬торой радиус
	uint16_t	R2;				// [2]	[0.1мм/бит] “ретий радиус
	uint16_t	SndSpd;			// [2]	[м/с]		—корость звука в среде
	SKLP_Temp_t	Temp;			// [2]	[гр÷]		“емпература модуля
} ACPB_ResultInstant15_t;	// [14]
#pragma pack( )

// SKLP.[0x15]	 —труктура ответа модул€ ј ѕ(б) на — Ћ-команду "«апрос таблицы интегральных данных"
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
// [0x19]  оманда чтения расширенных данных с датчиков ориентации
// *********************************************************************************
// SKLP.[0x19]	—труктура ответа модул€ ј ѕ(б) на — Ћ-команду "«апрос расширенных данных с датчиков ориентации"
#pragma pack( 1 )
typedef struct ACPB_DataGetDirExt_Answer_struct
{
	SKLP_FlagsModuleACPB_t	FlagsModule;	// [1]
	SKLP_MemoryFlags_t		MemoryFlags;	// [1]
	float	aAccel[3];		// [12]	[G]		ускорения
	float	aGyro[3];		// [12]	[rps]	скорость вращения
	float	aMagnet[3];		// [12]	[uT]	индукция магнитного поля
	float	Temp;			// [4]	[degC]	температура MPU9250
	float	aDirQuat[4];	// [16]	[+-1]	кватернион направления
	float	ZENI;			// [4]	[0-180]	угол зенитный
	float	AZIM;			// [4]	[0-360]	угол азимутальный
	float	TFG;			// [4]	[0-360]	угол отклонителя гравитационного
	float	TFM;			// [4]	[0-360]	угол отклонителя магнитного
} ACPB_DataGetDirExt_Answer_t;	// [74]
#pragma pack( )
// *********************************************************************************


// *********************************************************************************
// Ѕлок основных данных
// *********************************************************************************
// Ѕуфер дл€ оцифрованных акустических данных всех каналов в составе блока основных данных,
// для передачи по командам [0x03] и [0x67], и для сохранения на SD.
// ѕолностью соответсвует внутренней структуре ACPB_OscBuffer_t,
// но здесь он декларативно запакован для гарантированной поддержки протокола.
// ѕри копировании ACPB_OscBuffer_t -> ACPB_OscBufferPacket_t необходимо контролировать, что их размеры совпадают.
#pragma pack( 1 )
typedef struct ACPB_OscBufferPacket_struct
{
	uint16_t	aBuffer[ACPB_CHANNEL_COUNT_ALL][ACPB_OSC_FRAME_SIZE];	// [2*4*2048]
} ACPB_OscBufferPacket_t;	// [16K]
#pragma pack( )

// «аголовок блока основных данных
#pragma pack( 1 )
typedef struct ACPB_DataSaveHeader_struct
{
	// ѕоля, полностью соответствующие заголовку ACP_679
	uint8_t					ID;					// [1]	0x20	‘ормат заголовка
	uint16_t				HeaderDataSize;		// [2]			–азмер заголовка и блока данных - без заголовка — Ћ и CRC16
	uint32_t				CycleCounter;		// [4]	[0++]	—четчик циклов измерени€ - обнул€етс€ при включении модул€
	OADate_t				DateTime;			// [8]	[дн.]	¬рем€ начала оцифровки осциллограм этого блока
	// ѕоля для проектов ACPB_6xx
	uint32_t				TickCounter;		// [2]	[мс]	“ики операционки от запуска
	// - радиусы и уровни сигнала
	// - таблица по отклонителям
	// - дирекционные углы, в т.ч. исходные данные?
	// - радиусы и уровни сигнала
	// - работа высоковольтника
	// - напряжения
	// - ???
} ACPB_DataSaveHeader_t;	// [??]
#pragma pack( )

// Ѕлок основных данных для сохранения на SD и ответа на команды [0x03] и [0x67]
#pragma pack( 1 )
typedef struct ACPB_DataSave_struct
{
	SKLP_MemStruct_DataHeader_t	SKLP_Header;	// [15]		заголовок по протоколу — Ћ
	ACPB_DataSaveHeader_t		ACPB_Header;	// [??]		заголовок ј ѕ(б)
	uint8_t						aPadding[ SKLP_MEMSECTORSIZE - sizeof( SKLP_Header ) - sizeof( ACPB_Header ) - sizeof( uint16_t ) ];
	ACPB_OscBufferPacket_t		ACPB_Osc;		// [16K]	осциллограммы
	uint16_t					CRC16;			// [2]
} ACPB_DataSave_t;	// [16.5K]
#pragma pack( )

// *********************************************************************************
// [0x03]  оманда чтения крупных блоков (осциллограмм)
// *********************************************************************************
// SKLP.[0x03]	ѕолный ответ на команду, включая обрамление
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
// [0x67]  оманда чтения крупных блоков (осциллограмм) фрагментами.
//  оманда позволяет вычитывать большой пакет осциллограм при помощи множества малых кадров,
// что необходимо при ненадежном канале связи.
// *********************************************************************************
#define	SKLP_LONGDATAANSWER_FRAGMENT_SIZE		512		// размер фрагмента

// SKLP.[0x67]	—труктура запроса команды "¬ернуть пакет команды [0x03] по кадрам"
#pragma pack( 1 )
typedef struct SKLP_LongDataFragmentQuery_struct
{
	uint8_t	iFragment;
} SKLP_LongDataFragmentQuery_t;
#pragma pack( )

// SKLP.[0x67]	—труктура ответа на команду "¬ернуть пакет команды [0x03] по кадрам", включая обрамление
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
// [0x04]	 оманда чтения калибровочной структуры
// [0x05]	 оманда записи калибровочной структуры
// *********************************************************************************
// —труктура констант, связанных с процессом оцифровки.
// Ќе хранится в EEPROM.
#pragma pack( 1 )
typedef struct ACPB_Parameters_struct
{
	// ѕараметры из проекта ACP.679, перекочевавшие в проекты ACPB.6xx
	uint8_t		ID;						// [1]			0x20	‘ормат
	uint8_t		Size;					// [1]	[байт]	?28?	–азмер структуры
	uint16_t	SampleRate_kSPS;		// [2]	[kSPS]	4xxx	„астота сэмплировани€ ј÷ѕ  (зависит от скорости работы контроллера)
	uint16_t 	SamplesCount;			// [2]			2048	 оличество оцифровок на каждый канал
	uint8_t 	ChannelCount;			// [1]			4		 оличество каналов оцифровки (3+1)
	uint8_t		SampleDataSize;			// [1]	[бит]	12		–азмер элементарной оцифровки
	// ѕараметры из проекта ACP.679, сохраненные для поддержки ѕќ
	int16_t		StartSamplingDelay_ns[ACPB_CHANNEL_COUNT_ALL_ACP];		// [2x10]	[нс]	«адержки между зондирующим импульсом и запуском оцифровки (в старом проекте были положительными и разными для разных каналов, в этом проекте отрицательные и одинаковые для всех каналов
} ACPB_Parameters_t;			// [28]
#pragma pack( )

// —труктура калибровочных коэффициентов для вычисления радиусов
#pragma pack( 1 )
typedef union ACPB_Calibration_union
{
	float aVector[2*ACPB_CHANNEL_COUNT_MAIN];
	struct
	{
		float aK[2];		// полином 1-й степени ( K0 + K1*x ) 
	} aChannels[ACPB_CHANNEL_COUNT_MAIN];	//  алибровка на три канала, в ACP.679 было на 8 каналов
} ACPB_Calibration_t;			// [24]
#pragma pack( )

// SKLP.[0x04]	—труктура ответа модул€ ј ѕ(б) на команду "«апрос коэффициентов"
#pragma pack( 1 )
typedef struct SKLP_ACPB_NVMGetAnswer_struct
{
	SKLP_FlagsModuleACPB_t	FlagsModule;	// [1]
	ACPB_Parameters_t		Parameters;		// [28]
	ACPB_Calibration_t		Calibration;	// [24]
} SKLP_ACPB_NVMGetAnswer_t;	// [53]
#pragma pack( )

// SKLP.[0x05]	—труктура запроса команды "«апись коэффициентов"
#pragma pack( 1 )
typedef struct SKLP_ACPB_NVMSetQuery_struct
{
	char					aSignature[ sizeof( SKLP_SIGNATURE_COMMAND_NVM_SET ) - 1 ];
	ACPB_Parameters_t		Parameters;		// »гнорировать
	ACPB_Calibration_t		Calibration;	// «аписать в EEPROM
} SKLP_ACPB_NVMSetQuery_t;
#pragma pack( )

// SKLP.[0x05]	—труктура ответа ј ѕ(б) на команду "«апись коэффициентов"
typedef SKLP_ACPB_NVMGetAnswer_t SKLP_ACPB_NVMSetAnswer_t;

#endif	// SKLP_INTERFACE_ACPB_H
