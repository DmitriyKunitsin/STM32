// TaskConfig.h
// ������������ ����� FreeRTOS
// ������ ������������������� ��� ��� ���-��
#ifndef	TASK_CONFIG_H
#define	TASK_CONFIG_H

#include "FreeRTOS.h"
#include "Task.h"
#include "Event_groups.h"
#include <stdbool.h>

// �������� ����� �� �����������
// 0*	Idle					FreeRTOS internal thread
// 1	Main/SADC				�������� ���������
// 2	Spectrum				������������ ��������
// 3	SKLP					�������� ������ �� UART
// 4*	SysTimer/Daemon		FreeRTOS internal thread

// [00]
// FreeRTOS_IdleTask

// !!?? �������� �� ������ "MainTask" ������������ "�������" ��������� MemoryThread,
// !!?? � ����������� ���� ������� �� ������� � ������� �����������?

// ������ ��� ���������� ��������/��������� ���������� 
// [01]
#define TASK_REGUL_MAIN_NAME			"REGUL_MAIN"
#define TASK_REGUL_MAIN_STACK_SIZE	configMINIMAL_STACK_SIZE
#define TASK_REGUL_MAIN_PRIORITY		( tskIDLE_PRIORITY + 1UL )
extern bool RUS_Regul_TaskInit( void );

// ������ ������������ �������� �� ������������ ���������
// [02]
#define TASK_SPECTRUM_NAME			"SPECTRUM"
#define TASK_SPECTRUM_STACK_SIZE	configMINIMAL_STACK_SIZE
#define TASK_SPECTRUM_PRIORITY		( tskIDLE_PRIORITY + 2UL )
extern bool Spectrum_TaskInit( void );

// �������� ��� - ������������� ��������� (UART � �������), ������ �������� ������ ������� �� UART.
// ������ ������������� ��������� ��������� ������ �� UART ����� �������.
// ����� � ������� ����������� � ���������� UARTx.Idle
// ��� ��������� ������, �������� ��������������� �������.
// ������ ��������� ������������ �����������.
// �������� ������ ���� ������������ �������, ����� �� ��������� ��� ������� �� �������.
// [03]
#define TASK_SKLP_NAME				"SKLP"
#define TASK_SKLP_STACK_SIZE		configMINIMAL_STACK_SIZE
#define TASK_SKLP_PRIORITY			( tskIDLE_PRIORITY + 3UL )
extern bool SKLP_TaskInit( void );

// [04]
// FreeRTOS_TimerTask


// ������ ������ �������, ������������ ����������������� ��������� ����������� �������
extern EventGroupHandle_t EventGroup_System;
#define	EVENTSYSTEM_SPRINTF_READY		( 1 << 0 )

extern void SysState_AppendTaskHandler( TaskHandle_t xTaskHandle );
extern void DWT_AppendTimestampTag( char *pTag );

#endif	// TASK_CONFIG_H

