// SKLP_Service.h
// Протокол последовательной шины СКЛ, обслуживание принятых пакетов.
#ifndef	SKLP_SERVICE_H
#define	SKLP_SERVICE_H

#include <stdint.h>
#include <stdbool.h>
#include "SKLP_Time.h"		// Дата и время

// ************** Объвление типов данных, используемых в протоколе СКЛ **********************
// Отработанное время
typedef uint16_t SKLP_WorkTimeCounter_t;	// счетчик десЯтиминуток

// Флаги памяти
// Предполагается, что флаги характеризуют работу файловой системы, а не SD-карты
#pragma pack( 1 )
typedef struct SKLP_MemoryFlags_struct
{
	// Флаг наличия сохраненных данных в памяти
	uint8_t	SD0_DataPresent		:1;			// относится только к региону основных данных (не относится к черному ящику и технологическим данным)
	// Результат последней файловой операции
	uint8_t	SD0_ErrorMemInit	:1;			// неготовность файловой системы
	uint8_t	SD0_ErrorMemWrite	:1;			// ошибки при записи файла или открывания файла на запись
	uint8_t	SD0_ErrorMemRead	:1;			// ошибки при чтении файла или открывания файла на чтение
	// Без второй карточки не используются
	uint8_t	SD1_DataPresent		:1;
	uint8_t	SD1_ErrorMemInit	:1;
	uint8_t	SD1_ErrorMemWrite	:1;
	uint8_t	SD1_ErrorMemRead	:1;
} SKLP_MemoryFlags_t;
#pragma pack( )

// Флаги инициализации структуры памяти
#pragma pack( 1 )
typedef struct SKLP_MemoryInit_struct
{
	uint8_t	SD0_ErrorRead		:1;
	uint8_t	SD0_ErrorCRC		:1;
	uint8_t	SD0_Reserved0		:1;
	uint8_t	SD0_Reserved1		:1;
	uint8_t	SD1_ErrorRead		:1;
	uint8_t	SD1_ErrorCRC		:1;
	uint8_t	SD1_Reserved0		:1;
	uint8_t	SD1_Reserved1		:1;
} SKLP_MemoryInit_t;
#pragma pack( )

// Флаги программы. Привязаны к конкретному приложению, но первый бит общий.
#pragma pack( 1 )
typedef struct SKLP_FlagsModule_struct
{
	uint8_t	DataSaving			:1;		// флаг автономного режима - по команде [0x13] сохранять данные в память
	uint8_t Reserved			:7;		// Реализация этих битов в приложении
} SKLP_FlagsModule_t;
#pragma pack( )

// Режим работы модуля.
// При запуске программы необходимо считать из EEPROM, чтобы понять, в каком режиме продолжить работу.
// В автономных режимах выставлен флаг SKLP_FlagsModule.DataSaving, в неавтономном режиме флаг сброшен.
// В автономных режимах резрешено сохранение собранных данных на SD.
// В автономных режимах модуль МПИ производит опрос модулей.
typedef enum SKLP_ModuleMode_enum
{
	SKLP_ModuleMode_NotAuto			= 0,	// неавтономный режим
	SKLP_ModuleMode_Auto			= 1,	// автономный режим
	SKLP_ModuleMode_AutoDrilling	= 6,	// автономный режим, бурение (МПИ)
	SKLP_ModuleMode_AutoLogging		= 7,	// автономный режим, каротаж (МПИ)
	SKLP_ModuleMode_Unknown			= 0xFF,	
} SKLP_ModuleMode_t;

// Идентификатор (серийник) изделия, формат BCD 0xNNCNCCYY
// YY	- год производства
// CCC	- децимальный номер изделиЯ
// NNN	- порЯдковый номер изделЯ в году
#pragma pack( 1 )
typedef	union LoochDeviceSerial_union
{
	char		aBytes[4];
	uint32_t	BCD;
	struct
	{
		uint32_t YY		:8;
		uint32_t CC_L	:8;
		uint32_t NN_L	:4;
		uint32_t CC_H	:4;
		uint32_t NN_H	:8;
	};
} LoochDeviceSerial_t;
#pragma pack( )

// ДецимальнаЯ часть серийных номера модулей
typedef uint16_t LoochDeviceDecimalNumber_t;
//#define	LOOCH_DECIMALNUMBER_MP		265
//!! У МП могут быть разные децимальные номера, не проверЯть

/*typedef enum LoochDeviceDecimalNumber_enum
{
	LOOCH_DECIMALNUMBER_MP	= 265,
} LoochDeviceDecimalNumber_t;
*/

// Идентификатор (серийник) изделия, формат двоичный
typedef	struct LoochDeviceSerialBin_struct
{
	uint16_t					YearFrom2000;		// YY -> Bin
	LoochDeviceDecimalNumber_t	Decimal;			// CCC -> Bin
	uint16_t					Number;				// NNN -> Bin
} LoochDeviceSerialBin_t;

// Версия и дата прошивки, формат 0xYYMMDDVV
#pragma pack( 1 )
typedef struct LoochSoftVersion_struct
{
	uint8_t Version;		// [?]
	uint8_t Day;			// [01..31]
	uint8_t Month;			// [01..12]
	uint8_t YearFrom2000;	// [15....]
} LoochSoftVersion_t;
#pragma pack( )

// Идентификатор типа изделия, для использования в протокле МПИ и м.б., других модулях
typedef	uint16_t LoochDeviceType_t;	// 0x123V, 123 - децимальный номер изделия в BCD, V - версия железа

/*typedef union LoochDeviceType_union
{
	uint16_t Bin;
	struct
	{
		uint16_t Version	:4;
		uint16_t Decimal	:12;
	};
} LoochDeviceType_t;
*/

// ********************** Дополнение длЯ ЛУЧ.600 **********************
// ********************************************************************
// Флаги опрашиваемых модулей - работа, неответ, плохой ответ, нормальный ответ
typedef uint32_t Looch600_ModulesMask_t;
#pragma pack( 1 )
typedef union Looch600_Modules_union
{
	Looch600_ModulesMask_t Mask;
	struct
	{								//		'*' - внутренние компоненты МПИ, '=' модули на линии RS-485, '~' - модули на модемной линии
		// Первая тетрада для передачи по гидроканалу (основная)
		Looch600_ModulesMask_t Inclin 	:1; 	//			*	Инклинометр
		Looch600_ModulesMask_t Gamma	:1; 	// ГК		*	Гамма
		Looch600_ModulesMask_t VIKPB	:1; 	// ВИКПБ	~	Резак
		Looch600_ModulesMask_t GGKP		:1; 	// ГГКП		~	Гамма-гамма
		// Вторая тетрада для передачи по гидроканалу (вспомогательная)
		Looch600_ModulesMask_t NNKT		:1; 	// ННКТ		=	Нейтрон-нейтронный
		Looch600_ModulesMask_t BK 		:1; 	// БК		~	Боковой
		Looch600_ModulesMask_t AK 		:1; 	// АК		~	Акустика
		Looch600_ModulesMask_t INGK		:1; 	// ИНГК		~	Импульсно-нейтронный
		// ТретьЯ тетрада, модули питаниЯ
		Looch600_ModulesMask_t MP0		:1; 	// МП-0		=	Модуль питания
		Looch600_ModulesMask_t MP1		:1; 	// МП-1		=	Модуль питания
		Looch600_ModulesMask_t MP2		:1; 	// МП-2		=	Модуль питания
		Looch600_ModulesMask_t MP3		:1; 	// МП-3		=	Модуль питания
		// ЧетвертаЯ тетрада, прочие модули
		Looch600_ModulesMask_t AKP		:1; 	// АКП		~	Каверномер
		Looch600_ModulesMask_t NDM 		:1; 	// НДМ		~	Наддолотник
		Looch600_ModulesMask_t MPI		:1; 	// МПИ		*	Модуль памЯти и питаниЯ (использует только МУП, чтобы обозначить отсутствие МПИ при передаче по гидроканалу?)
		Looch600_ModulesMask_t MUP		:1; 	// МУП		=	Модуль управления пульсатором
		// ПЯтаЯ тетрада и т.д.
		Looch600_ModulesMask_t VIKPB_GK	:1; 	//			~	ВИКПБ Гамма, либо отдельный ГК на PLC
		Looch600_ModulesMask_t MP36_0	:1; 	// МП36-0	=	Модуль питания 36 В
		Looch600_ModulesMask_t MP36_1	:1; 	// МП36-1	=	Модуль питания 36 В
		Looch600_ModulesMask_t MP36_2	:1; 	// МП36-2	=	Модуль питания 36 В
		Looch600_ModulesMask_t MP36_3	:1; 	// МП36-3	=	Модуль питания 36 В
		// Резерв
//		Looch600_ModulesMask_t Reserved	:11;
		Looch600_ModulesMask_t NNKT2	:1; 	// ННКТ2	~	Нейтрон-нейтронный на модеме
		Looch600_ModulesMask_t Reserved	:10;
	};
} Looch600_Modules_t;		// [4]
#pragma pack( )

// Информационные флаги комплекса ЛУЧ.600
// Часть флагов управлЯетсЯ МУП, часть флагов МПИ (*)
#pragma pack( 1 )
typedef union Looch600_FlagsInfo_union
{
	uint16_t Mask;
	struct
	{
		uint16_t Reserved0		:1;		// 0	
		uint16_t Static			:1;		// 1		Стат-замер
		uint16_t Rotor			:1;		// 2*	Вращение ротором
		uint16_t TF_MnG			:1;		// 3*	Действующий отклонитель (0 - гравитационный, 1 - магнитный)
		uint16_t Reserved4_7	:4;		// 4..7
		uint16_t Flow1			:1;		// 8		Наличие потока (МПИ работает по этому флагу)
		uint16_t Flow2			:1;		// 9		Наличие потока
		uint16_t Reserved10_15	:6;		// 10..15
	};
} Looch600_FlagsInfo_t;		// [2]
#pragma pack( )


// ************** Объявление запросов и ответов на запросы по протоколу СКЛ ****************

// [0x01]	Ответ на запрос идентификатора модуля
#pragma pack( 1 )
typedef struct SKLP_GetLoochID_Answer_struct
{
	SKLP_FlagsModule_t	FlagsModule;
	LoochDeviceSerial_t Serial;
	LoochSoftVersion_t	SoftVersion;
} SKLP_GetLoochID_Answer_t;
#pragma pack( )

#pragma pack( 1 )
typedef struct SKLP_GetLoochID_v2_Answer_struct
{
	SKLP_FlagsModule_t	FlagsModule;
	LoochDeviceSerial_t Serial;
	LoochDeviceType_t	Type;
	LoochSoftVersion_t	SoftVersion;
} SKLP_GetLoochID_v2_Answer_t;
#pragma pack( )

// [0x02]	Запрос на изменение идентификатора модуля
#pragma pack( 1 )
typedef struct SKLP_SetLoochID_Query_struct
{
	LoochDeviceSerial_t Serial;			// новый серийник
	LoochSoftVersion_t	SoftVersion;	// версиЯ ПО обычно игнорируетсЯ
} SKLP_SetLoochID_Query_t;
#pragma pack( )

// [0x02]	Ответ на запрос на изменение идентификатора модуля
typedef SKLP_GetLoochID_Answer_t SKLP_SetLoochID_Answer_t;
typedef SKLP_GetLoochID_v2_Answer_t SKLP_SetLoochID_v2_Answer_t;

/*/ !! Тест!!
// [0x04], [0x05] Результат выполнениЯ команд чтениЯ и записи NVM
typedef enum NVM_Result_enum
{
	NVM_Result_Ok			= 0,	// операциЯ выполнена успешно
	NVM_Result_FailBlockID	= 1,	// недопустимый идентификатор блока
	NVM_Result_FailData		= 2,	// недопустимый формат данных блока (размер, CRC, содержимое)
	NVM_Result_FailEEPROM	= 3,	// ошибка при доступе к EEPROM
} NVM_Result_t;
*/
// [0x07]	Запрос отработанного времени
#pragma pack( 1 )
typedef struct SKLP_WorkTimeSetQuery_struct
{
	char					Signature;
	SKLP_WorkTimeCounter_t	Counter;
} SKLP_WorkTimeSetQuery_t;
#pragma pack( )

// [0x07]	Ответ на запрос отработанного времени
#pragma pack( 1 )
typedef struct SKLP_WorkTimeAnswer_struct
{
	SKLP_FlagsModule_t		FlagsModule;
	char					Signature;
	SKLP_WorkTimeCounter_t	WorkTimeCounter;
} SKLP_WorkTimeAnswer_t;
#pragma pack( )

#pragma pack( 1 )
typedef struct SKLP_WorkTimeAnswer_v2_struct
{
	SKLP_FlagsModule_t		FlagsModule;
	char					Signature;
	SKLP_WorkTimeCounter_t	WorkTimeCounter;
	uint16_t				CRC16;
} SKLP_WorkTimeAnswer_v2_t;
#pragma pack( )

// [0x21]	Ответ на запрос проверки состояния памяти
#pragma pack( 1 )
typedef struct SKLP_MemoryGetStateAnswer_struct
{
	SKLP_FlagsModule_t	FlagsModule;
	SKLP_MemoryFlags_t	MemoryFlags;
} SKLP_MemoryGetStateAnswer_t;
#pragma pack( )

// [0x37, 0x3F]	Запрос на синхронизацию времени
#pragma pack( 1 )
typedef struct SKLP_TimeSinchronization1_Query_struct
{
	SKLP_Time_t	Time;
} SKLP_TimeSinchronization1_Query_t;
#pragma pack( )

#pragma pack( 1 )
typedef struct SKLP_TimeSinchronization2_Query_struct
{
	SKLP_Time_t	Time;
	uint8_t		aDummy[5];
} SKLP_TimeSinchronization2_Query_t;
#pragma pack( )

// [0x3F]	Запрос на синхронизацию времени, выбора работающих модулей и передачу общих данных
// длЯ комплекса ЛУЧ.600
#pragma pack( 1 )
typedef struct SKLP_TimeSinchronization600_Query_struct
{
	SKLP_Time_t				Time;			// [7]	Время комплекса
	Looch600_Modules_t		ModulesWork;	// [4]	Модули, которые должны быть включены
	// Дополнительно
	Looch600_FlagsInfo_t	FlagsInfo;		// [2]	Флаги информационные
	int16_t					FrqRot;			// [2]	[0.1 об/мин]	Частота вращения ротором
	uint16_t				Toolface;		// [2]	[0.01 deg]	Активный отклонитель
} SKLP_TimeSinchronization600_Query_t;	//	[17]
#pragma pack( )

// [0x46]	Запрос на установку скорости обмена
#pragma pack( 1 )
typedef struct SKLP_BaudSetQuery_struct
{
	uint8_t				Speed;
} SKLP_BaudSetQuery_t;
#pragma pack( )

// [0x46]	Ответ на запрос на установки скорости обмена
#pragma pack( 1 )
typedef struct SKLP_BaudSetAnswer_struct
{
	SKLP_FlagsModule_t	FlagsModule;
	SKLP_MemoryFlags_t	MemoryFlags;
} SKLP_BaudSetAnswer_t;
#pragma pack( )

// [0x32, 0x36]	Запрос на установку времени запуска/остановки логгирования
#pragma pack( 1 )
typedef struct SKLP_SetTimeLogStartStop_Query_struct
{
//	char			aSignature[ sizeof( SKLP_SIGNATURE_COMMAND_NVM_SET ) - 1 ];
	SKLP_Time6_t	Time;
} SKLP_SetTimeLogStartStop_Query_t;
#pragma pack( )

// [0x32, 0x36]	Ответ на запрос на установку времени запуска/остановки логгирования
#pragma pack( 1 )
typedef struct SKLP_SetTimeLogStartStop_Answer_struct
{
	SKLP_Time6_t	Time;
} SKLP_SetTimeLogStartStop_Answer_t;
#pragma pack( )

// [0x35, 0x37]	Ответ на запрос запуска/остановки логгирования
typedef SKLP_SetTimeLogStartStop_Answer_t SKLP_GetTimeLogStartStop_Answer_t;

// [0x34]	Ответ на запрос чтения заморожденного времени
#pragma pack( 1 )
typedef struct SKLP_TimeFrozenGet_Answer_struct
{
	SKLP_Time_t	Time;
} SKLP_TimeFrozenGet_Answer_t;
#pragma pack( )

// ######################### Память #########################
#define	SKLP_MEMSECTORSIZE		512			// размер элементарного сектора при обращении к памяти
#ifndef	SKLP_MEMBLOCKSIZEMAX				// предполагается, что считывание памяти происходит блоками по SKLP_MEMSECTORSIZE * N, где N <= SKLP_MEMBLOCKSIZEMAX
#define	SKLP_MEMBLOCKSIZEMAX	64			// необходимо выравнивать начало регионов памяти по адресам, кратным SKLP_MEMSECTORSIZE * SKLP_MEMBLOCKSIZEMAX
#endif	// SKLP_MEMBLOCKSIZEMAX
											
typedef struct SKLP_Sector_struct
{
	uint8_t aSector[SKLP_MEMSECTORSIZE];
} SKLP_Sector_t;

typedef uint32_t SKLP_SectorIndex_t;		// Индекс сектора памяти
typedef uint16_t SKLP_ByteIndex_t;		// Индекс байта в секторе

// Структура описания блока памяти "черный ящик"
#pragma pack( 1 )
typedef struct SKLP_MemInfo_StructLog_struct
{
	SKLP_SectorIndex_t	iSectorFirst;		// [4]	Индекс первого сектора
	SKLP_SectorIndex_t	iSectorLast;		// [4]	Индекс последнего сектора
	SKLP_ByteIndex_t	iLastByte;			// [2]	Индекс последнего байта в последнем секторе
} SKLP_MemInfo_StructLog_t;	// [10]
#pragma pack( )

// Структура описания блока памяти "данные"
#pragma pack( 1 )
typedef struct SKLP_MemInfo_StructData_struct
{
	SKLP_SectorIndex_t	iSectorFirst;		// [4]	Индекс первого сектора
	SKLP_SectorIndex_t	iSectorLast;		// [4]	Индекс последнего сектора
	union
	{
		uint32_t		Mark;				// [4]	?? адрес метки времени? размер реальной области записанной памЯти?
		uint32_t		v1_Alias;			// [4]	!!! Тест! Экспорт метки длЯ идентификации файла, длЯ MemInfo_v1
	};
} SKLP_MemInfo_StructData_t;	// [12]
#pragma pack( )

// Структура описания памяти MemInfo_v0
// !!! Тест!!! На основе этой структуры строитсЯ MemInfo_v1,
// !!! путем добавлениЯ произвольного количество полей v1_aAuxStructData[] перед полем CRC8
#pragma pack( 1 )
typedef struct SKLP_MemInfo_struct
{
	uint8_t						Format;				// [1]	Формат структуры (0 - v0, 1 - v1, ...)
	uint8_t						TypeBlackBox;		// [1]	Формат "Черного ящика"
	uint8_t						TypeTechData;		// [1]	Формат технических данных
	uint8_t						TypeData;			// [1]	Формат основных данных
	uint16_t					MemSize;			// [2]	[МБ]	Рабочий объем памяти
	uint16_t					SectorSize;			// [2]	[Б]	Размер сектора
	uint8_t						DiskCount;			// [1]	Количество карт памяти
	SKLP_MemoryFlags_t			Flags[2];			// [2]	Флаги состояния карт памяти
	SKLP_MemInfo_StructLog_t	StructBlackBox;		// [10]	структура черного ящика
	SKLP_MemInfo_StructLog_t	StructTechData;		// [10]	структура технических данных
	SKLP_MemInfo_StructData_t	StructData;			// [12]	структура основных данных
//	SKLP_MemInfo_StructData_t	StructDataZip;		// структура сжатых данных
	SKLP_MemInfo_StructData_t	StructDataAsync;	// [12]	структура асинхронных данных (заместо сжатых)
	union
	{
		uint32_t	SegmentSize;					// [4]	?? Размер непрерывного блока данных (соответствует максимальному размеру файла)
		struct
		{
			uint8_t	AuxDataStructCount;				// 		Количество дополнительных структур данных, только длЯ MemInfo_v1
			uint8_t	AuxReadBufferSize;				//		Размер буфера длЯ чтениЯ памЯти, (в секторах)
			uint8_t	Reserved2;
			uint8_t	Reserved3;
		} v1_Aux;									// [4]
	};
//	SKLP_MemInfo_StructData_t	v1_aAuxStructData[N];	// [12*N]	Набор дополнительных структур данных, только длЯ MemInfo_v1
	uint8_t			CRC8;							// [1]	???
} SKLP_MemInfo_t;		// [60] длЯ v0, [60 + 12*N] длЯ v1
#pragma pack( )

// Ответ на запрос структуры памяти
#pragma pack( 1 )
typedef struct SKLP_MemInfoAnswer_struct
{
	SKLP_MemoryInit_t	InitFlags;
	SKLP_MemoryFlags_t	MemoryFlags;
	SKLP_MemInfo_t		Info;
} SKLP_MemInfoAnswer_t;
#pragma pack( )
	
// Запрос на чтение памяти
#pragma pack( 1 )
typedef struct SKLP_MemReadQuery_struct
{
	uint8_t				DiskNum;				// Номер диска (SD-карты, Flash и т.п.)
	uint16_t			SectorsCount;			// Количество запрашиваемых секторов
	SKLP_SectorIndex_t	iSectorStart;			// Индекс первого запрашиваемого сектора
} SKLP_MemReadQuery_t;
#pragma pack( )
	
// Структура ответа на запрос чтения памяти
#pragma pack( 1 )
typedef struct SKLP_MemReadAnswerFrame_struct
{
	uint8_t 			Start;
	uint16_t			SectorsCount;
	SKLP_MemoryFlags_t	MemoryFlags;
	SKLP_Sector_t		aBuffer[SKLP_MEMBLOCKSIZEMAX];			// Буфер максимального поддерживаемого размера, запрашиваемый объем может быть меньше
	uint8_t 			CRC8;
} SKLP_MemReadAnswerFrame_t;
#pragma pack( )

// Сигнатура, которой помечаютсЯ блоки, сохранЯемые в памЯть
#pragma pack( 1 )
typedef struct SKLP_MemSaveSign_struct
{
//	char aBuff[ sizeof( SKLP_SIGNATURE_SAVING_START ) - 1 ];	// [5]	"START"
	char aBuff[5];			// [5]	"START"
} SKLP_MemSaveSign_t;		// [5 байт]
#pragma pack( )

#define	SKLP_SIGNATURE_MEMSAVING_START		( ( SKLP_MemSaveSign_t ) { SKLP_SIGNATURE_SAVING_START } )

// Заголовок блока основных данных (для записи на SD-карту или ответа по команде 0x03), принятый в протоколе сохранения данных СКЛ
#pragma pack( 1 )
typedef struct SKLP_MemStruct_DataHeader_struct
{
	char					aSignature[5];		// [5]	"START"
//	SKLP_MemSaveSign_t		Signature;			// [5]	"START"
	uint8_t					ID;					// [1]	??
	SKLP_Time_t				Time;				// [7]
	SKLP_FlagsModule_t		FlagsModule;		// [1]
	SKLP_MemoryFlags_t		MemoryFlags;		// [1]
} SKLP_MemStruct_DataHeader_t;				// [15 байт]
#pragma pack( )

// Результат выполнения обработчиков - количество байт в подготовленном ответе
typedef int32_t SKLP_CommandResult_t;		// для 8-битного ядра может быть int16_t
#define	SKLP_COMMAND_RESULT_RETURN			( ( SKLP_CommandResult_t ) 1 )		// (и более) - команда нормально обработана, поле данных ответа заполнено указанным количеством байтов
#define	SKLP_COMMAND_RESULT_NO_REPLY		( ( SKLP_CommandResult_t ) 0 )		// команда нормально обработана, отвечать не требуется
#define	SKLP_COMMAND_RESULT_ERROR			( ( SKLP_CommandResult_t ) -1 )		// возникла ошибка при обработке команды (без уточнения)
#define	SKLP_COMMAND_RESULT_ERROR_FORMAT	( ( SKLP_CommandResult_t ) -2 )		// - ошибка в формате пакета (Start, Size, CRC или внутерннЯЯ структура данных)
#define	SKLP_COMMAND_RESULT_ERROR_NOCB		( ( SKLP_CommandResult_t ) -3 )		// - не найден подходЯщий обработчик команды
#define	SKLP_COMMAND_RESULT_ERROR_CB		( ( SKLP_CommandResult_t ) -4 )		// - ошибка внутри обработчика
#define	SKLP_COMMAND_RESULT_ERROR_TM		( ( SKLP_CommandResult_t ) -5 )		// - формирование ответа не уложилось в отведенный таймаут

// Тип коллбека длЯ обработки команды СКЛ
// pQuery	- Адрес буфера с пришедшим запросом, начинаЯ с со старта. Буфер размером не менее 256 байт.
// ppAnswer	- Адрес, куда коллбек положил ответ: по-умолчанию (длЯ небольших ответов), *ppAnswer == pQuery,
//			но длинный ответ лучше сложить в другой буфер.
//			Также через ppAnswer в коллбек передаетсЯ адрес интерфейса, откуда прилетел запрос, см. SKLP_ProcessPacket() и SKLP_ProcessCommand_BaudSet()
// Return	- количество байт в ответе или код ошибки
typedef SKLP_CommandResult_t ( *SKLP_Callback_t )( uint8_t *pQuery, uint8_t **ppAnswer );

typedef int16_t SKLP_Temp_t;	// в разных модулЯх встречаетсЯ кодирование с дискретой 1 грЦ и 0.01 грЦ
#define	SKLP_TEMP_UNKNOWN		( ( SKLP_Temp_t ) 0xFFFF )
#define	SKLP_TEMP_MIN			( ( SKLP_Temp_t ) -56 )		// !!важно!! при кодировани температуры одним байтом (напр., в МПИ) - дискрета 1 грЦ и смещение +56 грЦ (итого, диапазон от -56 до 199 грЦ)
#define	SKLP_TEMP_MAX			( ( SKLP_Temp_t ) +155 )
#define	SKLP_TEMP_MAX_EEPROM1	( ( SKLP_Temp_t ) +85 )
#define	SKLP_TEMP_MAX_EEPROM2	( ( SKLP_Temp_t ) +90 )
#define	SKLP_TEMP_MAX_SD1		( ( SKLP_Temp_t ) +120 )
#define	SKLP_TEMP_MAX_SD2		( ( SKLP_Temp_t ) +125 )
#define	SKLP_TEMP_VALIDATE( __TEMP__ )		( ( ( __TEMP__ ) >= SKLP_TEMP_MIN ) && ( ( __TEMP__ ) <= SKLP_TEMP_MAX ) )

// Объявление переменных, доступных для внешнего обращения
extern volatile SKLP_FlagsModule_t		SKLP_FlagsModule;			// Флаги модуля
extern volatile SKLP_MemoryFlags_t		SKLP_MemoryFlags;			// Флаги памяти
extern volatile SKLP_Time_t				SKLP_Time;					// Системное время СКЛ
extern volatile SKLP_WorkTimeCounter_t	SKLP_WorkTimeCounter;		// Счетчик отработанного времени (дестиминутки)
extern volatile SKLP_Temp_t				TemperatureBoard;			// Температура платы
extern LoochDeviceSerial_t				LoochDeviceSerial;			// Серийный номер модулЯ
extern LoochSoftVersion_t				LoochSoftVersion;			// Дата и версиЯ микропрограммы
//extern ModuleMode_t						ModuleMode;					// Режим работы модулЯ

// Объявление функций
bool SKLP_ServiceInit( void );							// Инициализация
SKLP_Callback_t SKLP_ServiceSetCallback( uint8_t CommandNumber, SKLP_Callback_t xNewCallback );		// Ассоциировать команду и обработчик
SKLP_CommandResult_t SKLP_ProcessCommand_Common( uint8_t *pQuery, uint8_t **ppAnswer );		// Поиск подходящего обработчика команды в списке и вызов его

// Наиболее употребимые обработчики.
// Объявлены в SKLP_ServiceDefault.c как __weak,
// могут быть переопределены в приложении, по необходимости.
// Описание интерфеса см. выше, при объЯвлении SKLP_Callback_t.
SKLP_CommandResult_t SKLP_ProcessCommand_ID_Get( uint8_t *pQuery, uint8_t **ppAnswer ); 							// [0x01]	Вернуть идентификатор прибора
SKLP_CommandResult_t SKLP_ProcessCommand_ID_Set( uint8_t *pQuery, uint8_t **ppAnswer ); 							// [0x02]	Установить идентификатор прибора
SKLP_CommandResult_t SKLP_ProcessCommand_TimeSinchronization( uint8_t *pQuery, uint8_t **ppAnswer );			// [0x37]	Синхронизация времени комплекса
SKLP_CommandResult_t SKLP_ProcessCommand_WorkTime_Get( uint8_t *pQuery, uint8_t **ppAnswer );					// [0x07]	Вернуть отработанное время
SKLP_CommandResult_t SKLP_ProcessCommand_WorkTime_Inc( uint8_t *pQuery, uint8_t **ppAnswer );					// [0x08]	Инкрементировать отработанное время
SKLP_CommandResult_t SKLP_ProcessCommand_WorkTime_Set( uint8_t *pQuery, uint8_t **ppAnswer );					// [0x09]	Установить отработанное время
SKLP_CommandResult_t SKLP_ProcessCommand_MemoryGetState( uint8_t *pQuery, uint8_t **ppAnswer ); 				// [0x21]	Вернуть состояние памяти
SKLP_CommandResult_t SKLP_ProcessCommand_BaudSet( uint8_t *pQuery, uint8_t **ppAnswer );							// [0x46]	Установить скорость обмена
SKLP_CommandResult_t SKLP_ProcessCommand_GoTo_BootLoader( uint8_t *pQuery, uint8_t **ppAnswer );			// [0xB1]	Перейти на BootLoader

SKLP_CommandResult_t SKLP_ProcessCommand_BaudSetLocal( void *pTransportInterface, uint32_t NewBaudRate );		// [0x46]	ЛокальнаЯ процедура установки скорости
//SKLP_Time_t Time_LoochSoftVersion2SKLP( LoochSoftVersion_t LoochSoftVersion );			// Перевод времени созданиЯ прошивки во времЯ СКЛ
SKLP_Time6_t Time_LoochSoftVersion2SKLP( LoochSoftVersion_t LoochSoftVersion );			// Перевод времени созданиЯ прошивки во времЯ СКЛ
LoochDeviceSerialBin_t SKLP_LoochDeviceSerial2Bin( LoochDeviceSerial_t SerialBCD );		// Перевод серийника из BCD в Bin
LoochDeviceSerial_t SKLP_LoochDeviceSerial2BCD( LoochDeviceSerialBin_t SerialBin );		// Перевод серийника из Bin в BCD

#endif	// SKLP_SERVICE_H
