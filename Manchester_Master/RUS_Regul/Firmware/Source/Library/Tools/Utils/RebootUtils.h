// RebootUtils.h
// Утилиты, связанные с запуском и перезагрузкой
#ifndef	REBOOT_UTILS_H
#define	REBOOT_UTILS_H

#include <stdint.h>
#include <stdbool.h>
#include "SKLP_Time.h"		// TimeSubs_t

// Интерфейс
void WatchdogReset( void );									// Перезапуск сторожевого таймера
void SystemHardwareInit( void );								// Инициализация системного осциллятора и сторожевого таймера, обработка источника сброса
bool RebootDelayed( const float Delay_s, const char *pNote );	// Вызвать Reboot() через таймаут
__noreturn void Reboot( const char *pNote );						// Произвести перезагрузку, пояснение будет сохранено в лог при последующем запуске
__noreturn void ResetToApplication( uint32_t xApplication );		// Произвести перезагрузку и перейти на указанное приложение
__noreturn void ResetToFunction( FunctionVoid_t xFunction );	// Произвести перезагрузку и перейти на указаную функцию
extern void FaultCallback( void );								// Коллбек из обработчика assert() и Hard Fault - можно произвести необходимые действия (дернуть пинами) в случае отказа
///*static*/ /*__noreturn*/ void JumpToApplication( uint32_t ApplicationAddress );

// Тэг, определяющий причину ручной перезагрузки
typedef enum ResetInfoTag_enum
{
	ResetInfoTag_Hardware	= 0x00000000ul,			// признак аппаратной загрузки
	ResetInfoTag_Clear		= 0x01234567ul,			// после обработки сбросить тэг в это состояние. источник сброса не ясен.
	ResetInfoTag_Assertion	= 0x12345678ul,			// перезагрузка после Assert, вывести информацию Assertion Info
	ResetInfoTag_HardFault	= 0x23456789ul,			// перезагрузка после Hard Fault, вывести информацию Hard Fault Info
	ResetInfoTag_Reboot		= 0x34567890ul,			// ручная перезагрузка, вывести информацию о причине
	ResetInfoTag_JumpToApp	= 0x45678901ul,			// ручная перезагрузка для сброса всей периферии перед переходом на другое приложение
	ResetInfoTag_JumpToFunc	= 0x56789012ul, 		// ручная перезагрузка для сброса всей периферии перед переходом на требуемую функцию
	// всякое иное значение свидетельствует об аппаратной перезагрузке
} ResetInfoTag_t;

// Определение источника перезагрузки - аппаратной или программной
typedef enum ResetSource_enum
{
	ResetSource_Undefined = 0,		// не удалось определить (не должно появляться)
	ResetSource_Power,				// сброс по пропаданию питания или пониженному питанию
	ResetSource_Watchdog,			// сброс по сторожевому таймеру (независимому или оконному)
	ResetSource_Pin,				// сброс по ножке Reset при нормальном питании
	ResetSource_SoftUndefined,		// программная перезагрузка, не удалось определить причину
	ResetSource_SoftReboot,			// перезагрузка по решению софта (например, чтобы полностью переинициализировать файловую систему после форматирования)
	ResetSource_SoftAssertion,		// перезагрузка по Assert
	ResetSource_SoftFault,			// перезагрузка по Hard Fault
} ResetSource_t;

// СтадиЯ загрузки
typedef enum ResetStage_enum
{
	ResetStage_PowerUp = 0,					// после сброса питаниЯ ОЗУ обнулено
	ResetStage_SystemInitStart,				// вход в SystemInit()->SystemInitCallback()
	ResetStage_SystemInitFinish,			// выход из SystemInit()->SystemInitCallback()
	// где-то здесь инициализируютсЯ статические переменные
	ResetStage_SystemHardwareInitStart,		// вход в main()->SystemHardwareInit()
	ResetStage_SystemHardwareInitFinish,	// выход из main()->SystemHardwareInit()
	ResetStage_InitComplete,				// инициализациЯ завершена
	ResetStage_SoftReset,					// выполнение программного сброса
} ResetStage_t;

// Структура, в которую сохраняется информация перед выполнением ручной перезагрузкой
typedef struct ResetInfo_struct
{
	ResetInfoTag_t	Tag;							// тэг, по которому можно определить источник ручного сброса (устанавливается перед ручной перезагрузкой, сбрасывается после обработки при старте)
	ResetInfoTag_t	TagSaved;						// сохраненный тег, видимый в программе во время работы
	ResetStage_t	ResetStage;
	ResetSource_t	ResetSource;					// источник последней перезагрузки - аппаратный или программный
	uint32_t		ResetCounter;					// всегда инкремент после перезагрузки
	uint32_t		xResetToApplication;			// адрес, на который необходимо перейти при запуске, если тэг равен ResetInfoTag_JumpToApp
	char			aRebootMessage[256];			// текст, формируемый перед принудительной перезагрузкой. должен быть записан в лог после перезапуска программы
	char			aLogMessage[256+128];			// текст, формируемый в обработчике Reset (начало main) для последующего вывода в лог после запуска файловой системы и интерфейса логгирования
	TimeSubs_t		RTC_SubsecondCorrection;		// поправка при установке времени RTC, т.к. миллисекунды тот устанавливать не дает, а коррекциЯ должна быть сохранена при перезагрузке. Нужно не терЯть при перезагрузке
} ResetInfo_t;

extern volatile ResetInfo_t ResetInfo;


// Объявление полей из автогенеренного файла BuildInfo.c - перенести?
extern const char aBuildInfo_Date[];
extern const char aBuildInfo_Time[];
extern const char aBuildInfo_ProjName[];
extern const char aBuildInfo_ConfigName[];
extern const char aBuildInfo_ComputerName[];
extern const char aBuildInfo_SVN_Revision[];

#endif	// REBOOT_UTILS_H
