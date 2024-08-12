// SKLP_Interface_GGLP_SADC.h
//  омандный интерфейс спектрометрического ј÷ѕ для √√ -Ћѕ на шине — Ћ
//  онтроллер Ћ”„.638.00.02.00 в составе модуля Ћ”„.638.00.00.00
#ifndef	SKLP_INTERFACE_GGLP_SADC_H
#define	SKLP_INTERFACE_GGLP_SADC_H

#include <stdint.h>
#include "SKLP_Time.h"				// SKLP_Time_t
//#include "NVM.h"					// NVM_Result_t
#include "SKLP_MS_Transport.h"		// SKLP_Signature
#include "SKLP_Service.h"			// SKLP_FlagsModule_t, SKLP_CommandResult_t
#include "MathUtils.h"				// Float16_t
#include <stdint.h>

// –азмер энергетических спектров
#define	GGLP_SDAC_SPECTRUM_SIZE_SHORT			256
#define	GGLP_SDAC_SPECTRUM_SIZE_FULL			4096

// —пецифические команды протокола — Ћ для платы спектрометрического ј÷ѕ
typedef enum SKLP_Command_GGLP_SADC_enum
{
	SKLP_COMMAND_GGLP_SADC_MODE_SET				= 0x11,	// установка режима работыЄ
	SKLP_COMMAND_GGLP_SADC_DATA_FAST_SPEC_GET	= 0x12, // чтение основных данных со спектрами, накопление до 25 мс
	SKLP_COMMAND_GGLP_SADC_DATA_FAST_GET		= 0x13, // чтение основных данных, накопление до 25 мс
	SKLP_COMMAND_GGLP_SADC_DATA_TECH_GET		= 0x14, // чтение технологических параметров
	SKLP_COMMAND_GGLP_SADC_DATA_SLOW_SPEC_GET	= 0x15, // чтение основных данных со спектрами, накопление до 6000 мс
	SKLP_COMMAND_GGLP_SADC_DATA_SLOW_GET		= 0x16, // чтение основных данных, накопление до 6000 мс
	SKLP_COMMAND_GGLP_SADC_DATA_ACCUM_SPEC_GET	= 0x17, // чтение накопленного спектра за длительный период времени
} SKLP_Command_GGLP_SADC_t;

// ‘лаги платы спектрометрического ј÷ѕ
#pragma pack( 1 )
typedef union GGLP_SADC_FlagsModule_union
{
	uint8_t Byte;
	struct
	{
		uint8_t fEnabled			:1;		// принята команда разрешения работы
		uint8_t fReady				:1;		// завершение внутренней инициализации, готовность к работе
		uint8_t fReserved			:6;		// зарезервировано
	};
} GGLP_SADC_FlagsModule_t;
#pragma pack( )

// ‘лаги результата накопления и обработки спектров
#pragma pack( 1 )
typedef union GGLP_SADC_FlagsResult_union
{
	uint8_t Byte;
	struct
	{
		uint8_t fComplete			:1;		// спектр накоплен
		uint8_t fFailCntLoss		:1;		// переполнение счетов при помещении в буфер
		uint8_t fFailCntOvfLow		:1;		// переполнение счета в первой ячейке спектра (зашкалы вниз)
		uint8_t fFailCntOvfHigh		:1;		// переполнение счета в последней ячейке спектра (зашкалы вверх)
		uint8_t fFailCntOvfSpectrum	:1;		// переполнение счета в какой-либой ячейке спектра, кроме первой и последней
		uint8_t fFailCntOvfPhoto	:1;		// переполнение счетчика фотоэффекта
		uint8_t fFailCntOvfCompton	:1;		// переполнение счетчика комптон-эффекта
		uint8_t fFailCntOvfTotal	:1;		// переполнение счетчика всех импульсов
	};
} GGLP_SADC_FlagsResult_t;
#pragma pack( )

// *****************************************************************************
// [0x13] ќтвет на чтения основных данных, накопление до 25 мс
// *****************************************************************************
#pragma pack( 1 )
typedef struct SKLP_GGLP_SADC_DataFastGet_Answer_struct
{
	GGLP_SADC_FlagsModule_t	FlagsModule;	// [1]		флаги платы
	GGLP_SADC_FlagsResult_t	FlagsResult;	// [1]		флаги результата обработки гаммы
	uint16_t				TimeAccum_us;	// [2]		время накопления результата
	uint16_t				CountTotal;		// [2]		счет общий (полный)
	uint16_t				CountPhoto;		// [2]		счет по диапазону фотоэффекта
	uint16_t				CountCompton;	// [2]		счет по диапазону комптон-эффекта
} SKLP_GGLP_SADC_DataFastGet_Answer_t;	// [10]
#pragma pack( )

// *****************************************************************************
// [0x12] ќтвет на чтения основных данных со спектрами, накопление до 25 мс
// *****************************************************************************
#pragma pack( 1 )
typedef struct SKLP_GGLP_SADC_DataFastSpectrumGet_Answer_struct
{
	SKLP_GGLP_SADC_DataFastGet_Answer_t	Summary;		// [10]	копия ответа на аналогичный запрос, без спектров
	uint8_t	aSpectrum[GGLP_SDAC_SPECTRUM_SIZE_SHORT];	// [256]	спектр по энергиям
} SKLP_GGLP_SADC_DataFastSpectrumGet_Answer_t;	// [266]
#pragma pack( )

// *****************************************************************************
// [0x16] ќтвет на чтения основных данных, накопление до 6000 мс
// *****************************************************************************
#pragma pack( 1 )
typedef struct SKLP_GGLP_SADC_DataSlowGet_Answer_struct
{
	GGLP_SADC_FlagsModule_t	FlagsModule;	// [1]		флаги платы
	GGLP_SADC_FlagsResult_t	FlagsResult;	// [1]		флаги результата обработки гаммы
	uint16_t				TimeAccum_ms;	// [2]		время накопления результата
	uint16_t				CountTotal;		// [2]		счет общий (полный)
	uint16_t				CountPhoto;		// [2]		счет по диапазону фотоэффекта
	uint16_t				CountCompton;	// [2]		счет по диапазону комптон-эффекта
} SKLP_GGLP_SADC_DataSlowGet_Answer_t;	// [10]
#pragma pack( )
//typedef SKLP_GGLP_SADC_DataFastGet_Answer_t SKLP_GGLP_SADC_DataSlowGet_Answer_t;	// [10]

// *****************************************************************************
// [0x15] ќтвет на чтения основных данных со спектрами, накопление до 6000 мс
// *****************************************************************************
#pragma pack( 1 )
typedef struct SKLP_GGLP_SADC_DataSlowSpectrumGet_Answer_struct
{
	SKLP_GGLP_SADC_DataSlowGet_Answer_t	Summary;		// [10]	копия ответа на аналогичный запрос, без спектров
	uint16_t aSpectrum[GGLP_SDAC_SPECTRUM_SIZE_SHORT];	// [512]	спектр по энергиям
} SKLP_GGLP_SADC_DataSlowSpectrumGet_Answer_t;	// [522]
#pragma pack( )


// *****************************************************************************
// [0x11] «апрос на установку режима работы
// *****************************************************************************
#pragma pack( 1 )
typedef struct SKLP_GGLP_SADC_ModeSet_Query_struct
{
	uint8_t					Mode;			// [1]			режим (0x01 - разрешение работы)
	uint16_t				Vref_mV;		// [2]	[м¬]	требуемое напряжение VDDA/Vref, 0xFFFF - auto
	uint16_t				Vcomp_mV;		// [2]	[м¬]	требуемое напряжение порога компаратора, 0xFFFF - auto
	uint16_t				UHV;			// [2]	[¬]		установленное напряжение питания ‘Ё” (абс)
} SKLP_GGLP_SADC_ModeSet_Query_t;		// [7]
#pragma pack( )

#pragma pack( 1 )
typedef struct SKLP_GGLP_SADC_ModeSet_Answer_struct
{
	GGLP_SADC_FlagsModule_t	Flags;			// [1]			флаги
} SKLP_GGLP_SADC_ModeSet_Answer_t;		// [1]
#pragma pack( )

// *****************************************************************************
// [0x14] ќтвет на команду запроса блока технологических данных
// *****************************************************************************
#pragma pack( 1 )
typedef struct SKLP_GGLP_SADC_DataTechGet_Answer_struct
{
	GGLP_SADC_FlagsModule_t	Flags;			// [1]			флаги
	uint8_t					TempProbe;		// [1]	[?]		температура сцинтиблока
	uint8_t					TempMicro;		// [1]	[?]		температура микроконтроллера
	uint16_t				Vref_mV;		// [2]	[м¬]	установленое напряжение VDDA/Vref
	uint16_t				Vcomp_mV;		// [2]	[м¬]	установленое напряжение порога компаратора
	//  оэффициенты при обработке импульсов гаммы и формировании энегетического спектра
	float					KE;				// [4]	[кэ¬/ј÷ѕ]	коэффициент перевода из кода ј÷ѕ в энергию
	float					Emin;			// [4]	[кэ¬]	энергия в первой ячейке спектра
	float					dE;				// [4]	[кэ¬]	диапазон энергий в одной ячейке спектра
	float					aEp[2];			// [8]	[кэ¬]	диапазон энергий фотоэффекта
	float					aEc[2];			// [8]	[кэ¬]	диапазон энергий комптон-эффекта
} SKLP_GGLP_SADC_DataTechGet_Answer_t;	// [35]
#pragma pack( )

#endif	// SKLP_INTERFACE_GGLP_SADC_H
