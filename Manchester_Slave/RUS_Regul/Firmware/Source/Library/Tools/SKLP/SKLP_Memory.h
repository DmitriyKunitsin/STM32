// SKLP_Memory.h
// Доступ к регионам памяти модуля по протоколу СКЛ
#ifndef	SKLP_MEMORY_H
#define	SKLP_MEMORY_H

#include "SKLP_Service.h"
#include "LogFile.h"

// Регион памЯти - область виртуальной памЯти модулЯ, в которую отображаетсЯ фрагмент
// физической памЯти. Карта виртуальной памЯти может быть считана по команде [0x41],
// содержимое памЯти - по команде [0x22].

// Типы организации памяти региона
typedef enum SKLP_MemoryRegionType_enum
{
	SKLP_MemoryRegionType_EMPTY = 0,		// Регион заявлен, но не реализован
	SKLP_MemoryRegionType_File,				// Файл в составе файловой системы (обычно на uSD)
	SKLP_MemoryRegionType_FileSet, 			// Набор файлов в составе файловой системы
	SKLP_MemoryRegionType_EEPROM,			// Эмулятор EEPROM на базе Flash
	SKLP_MemoryRegionType_Flash,			// ПрЯмой доступ ко Flash
	SKLP_MemoryRegionType_DiskImage,		// ПрЯмой доступ к образу диска
	SKLP_MemoryRegionType_TOTAL,			// Количество возможных типов
} SKLP_MemoryRegionType_t;

// Наименование региона, служит длЯ идентификации его содержимого
typedef union SKLP_MemoryRegionAlias_union
{
	char		aTxt[4];					// Текстовое представление длЯ наглЯдной инициализации
	uint32_t	ID;							// Цифровое представление длЯ сравнениЯ и поиска
} SKLP_MemoryRegionAlias_t;
 
 // Идентификаторы (алиасы) регионов по-умолчанию
 // В MemInfo.v0 отсутствовали алиасы регионов, но в регионах MAIN и ZIP
 // было предусмотрено поле Mark, которое можно попробовать использовать длЯ экспорта алиасов.
 // [N] - порЯдковый номер региона в MemInfo.v0
 // [*] - алиас региона не экспортируетсЯ в MemInfo.v0 из-за особенности формата MemInfo
 // [+] - регион отсутствует в MemInfo.v0.
#define	SKLP_MEMEMORY_REGION_ALIAS_BLACKBOX		( ( SKLP_MemoryRegionAlias_t ) { "BBOX" } )		// [0*]	черный Ящик
#define	SKLP_MEMEMORY_REGION_ALIAS_TEXTLOG		( ( SKLP_MemoryRegionAlias_t ) { "LOG" } )		// [1*]	текстовый лог, замена TECH во всех проектах
#define	SKLP_MEMEMORY_REGION_ALIAS_DATATECH		( ( SKLP_MemoryRegionAlias_t ) { "TECH" } )		// [1*]	технологические данные (раз в  мин?)
#define	SKLP_MEMEMORY_REGION_ALIAS_DATAMAIN		( ( SKLP_MemoryRegionAlias_t ) { "MAIN" } )		// [2]	основные данные
#define	SKLP_MEMEMORY_REGION_ALIAS_DATAZIP		( ( SKLP_MemoryRegionAlias_t ) { "ZIP" } )		// [3]	запакованные данные (ускорение загрузки больших файлов?)
#define	SKLP_MEMEMORY_REGION_ALIAS_DATAASYNC	( ( SKLP_MemoryRegionAlias_t ) { "ASYN" } )		// [3]	асинхронные данные, МПИ, замена ZIP
#define	SKLP_MEMEMORY_REGION_ALIAS_DATAINTEGRAL	( ( SKLP_MemoryRegionAlias_t ) { "INTG" } )		// [4+]	интегральные (азимутальные) данные модулей, МПИ
#define	SKLP_MEMEMORY_REGION_ALIAS_ACCELOSC		( ( SKLP_MemoryRegionAlias_t ) { "ACCL" } )		// [5+]	осциллограммы акселерометров, МПИ
#define	SKLP_MEMEMORY_REGION_ALIAS_SD_IMAGE		( ( SKLP_MemoryRegionAlias_t ) { "SDIM" } )		// [??+]	имидж SD-карты

typedef struct LogFileExt_struct
{
	LogFileHdl_t	*pLogFileHdl;
	char			*pDescription;
	uint32_t		FileMaxSize;
	FIL_Position_t	*pPositionSaved;
} LogFileExt_t;

bool SKLP_Memory_Init( void );
// Добавление нового региона
bool SKLP_MemoryRegionAppend( SKLP_MemoryRegionAlias_t Alias, SKLP_MemoryRegionType_t Type, void const *pHdl );
// Инициализатор регионов памЯти по-умолчанию
void SKLP_MemoryRegionsInitDefault( void );
// Опциональный (проекто-зависимый) инициализатор оегионов памЯти
extern void SKLP_MemoryRegionsInit( void );

// Инициализировать доступ к лог-файлу (открыть на запись, перемотать), и отправить отчет в лог
void SKLP_Memory_OpenAndReport( LogFileExt_t *pLogFileExt );
void SKLP_Memory_ReportFreeSpace( void );

// Описание переменных и функций, привязанных к конкретному проекту

extern SKLP_ModuleMode_t *pSKLP_ModuleMode;										// Указатель на режим работы модулЯ, должен быть реализован в проекте
extern void *SKLP_Memory_TmpBufferGet( uint32_t BufferMinSize );	// Получение доступа на запись к временному буферу
extern uint32_t SKLP_Memory_TmpBufferGetSize( void );			// Узнать размер временного буфера
extern void SKLP_Memory_TmpBufferRelease( bool FromISR );		// Освобождение доступа к буферу

#endif	// SKLP_MEMORY_H


