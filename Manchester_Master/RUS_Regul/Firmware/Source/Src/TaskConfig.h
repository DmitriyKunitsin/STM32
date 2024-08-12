// TaskConfig.h
// Конфигуратор задач FreeRTOS
// Проект спектрометрического АЦП длЯ ГГК-ЛП
#ifndef	TASK_CONFIG_H
#define	TASK_CONFIG_H

#include "FreeRTOS.h"
#include "Task.h"
#include "Event_groups.h"
#include <stdbool.h>

// Разбивка задач по приоритетам
// 0*	Idle					FreeRTOS internal thread
// 1	Main/SADC				Основные алгоритмы
// 2	Spectrum				Формирование спектров
// 3	SKLP					Протокол обмена по UART
// 4*	SysTimer/Daemon		FreeRTOS internal thread

// [00]
// FreeRTOS_IdleTask

// !!?? Возможно ли вместо "MainTask" использовать "типовую" песочницу MemoryThread,
// !!?? и подкидывать туда задачки по таймеру в порЯдке поступлениЯ?

// Задача для реализации основных/медленных алгоритмов 
// [01]
#define TASK_REGUL_MAIN_NAME			"REGUL_MAIN"
#define TASK_REGUL_MAIN_STACK_SIZE	configMINIMAL_STACK_SIZE
#define TASK_REGUL_MAIN_PRIORITY		( tskIDLE_PRIORITY + 1UL )
extern bool RUS_Regul_TaskInit( void );

// Задача формированиЯ спектров из оцифрованных импульсов
// [02]
#define TASK_SPECTRUM_NAME			"SPECTRUM"
#define TASK_SPECTRUM_STACK_SIZE	configMINIMAL_STACK_SIZE
#define TASK_SPECTRUM_PRIORITY		( tskIDLE_PRIORITY + 2UL )
extern bool Spectrum_TaskInit( void );

// Протокол СКЛ - инициализация периферии (UART и таймеры), запуск ожидания приема пакетов по UART.
// Обычно заблокирована ожиданием получениЯ пакета от UART через очередь.
// Пакет в очередь формируетсЯ в прерывании UARTx.Idle
// При получении пакета, вызывает соответствующий коллбек.
// Список коллбеков определЯетсЯ приложением.
// Приортет должен быть сравнительно высоким, чтобы не тормозить при ответах на запросы.
// [03]
#define TASK_SKLP_NAME				"SKLP"
#define TASK_SKLP_STACK_SIZE		configMINIMAL_STACK_SIZE
#define TASK_SKLP_PRIORITY			( tskIDLE_PRIORITY + 3UL )
extern bool SKLP_TaskInit( void );

// [04]
// FreeRTOS_TimerTask


// Единая группа событий, отображающая работоспособность различных компонентов системы
extern EventGroupHandle_t EventGroup_System;
#define	EVENTSYSTEM_SPRINTF_READY		( 1 << 0 )

extern void SysState_AppendTaskHandler( TaskHandle_t xTaskHandle );
extern void DWT_AppendTimestampTag( char *pTag );

#endif	// TASK_CONFIG_H

