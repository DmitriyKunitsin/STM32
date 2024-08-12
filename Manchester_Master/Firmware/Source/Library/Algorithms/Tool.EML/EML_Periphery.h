// EML_Periphery.h
// ������ (����������) ������� ����:
// - ��� ���������;
// - PGA ���������;
// - ����� ���������� �����-���������;
// - ������������ ��������� ����� GPIO ��� DAC;
#ifndef	EML_PERIPHERY_H
#define	EML_PERIPHERY_H

#include <stdint.h>
#include <stdbool.h>
#include "Driver_PGA.h"		// ������� PGA
#include "EML_Config.h"		// EML_TxTick_t
#include "FreeRTOS.h"
#include "Task.h"
#include "Event_groups.h"

// ������������� ���� ����������� ��������� (ADC, DAC, Timer, PGA)
void EML_PeripheryInit( void );
// ������������ �������� PGA
PGA_Gain_t EML_PGA_Command( PGA_Command_t Command );
PGA_Gain_t EML_PGA_CommandExt( PGA_Command_t Command, int *piADC_PGA );
// ������� ����� �� ������ ���������� �������� � ������������ ����� ������� � ���������� �������
bool EML_TxPulseDelayed( EML_TxTick_t Delay_ticks );
// ������������ ������ ������ ��������� � ����������� ����
void EML_TxOn_RxOff( void );
void EML_TxOff_RxOn( void );
// ������� ��������� ������������ ����
ADC_EML_Frame_t *EML_ADC_GetFrameCurrent( void );


bool EML_DAC_Pulse( void );
bool EML_DAC_Pulses( uint32_t PulsesCount );


// ������� ��� ���������� ����
#define	EVENT_EML_TIMERTX				( 1 << 0 )		// ���������� ��������� ���� �������, ������������ ������������ ������
#define	EVENT_EML_ADC_DMA				( 1 << 1 )		// ���������� ��������� �������� ��� ������ ������ ������������ �������
#define	EVENT_EML_DAC_DMA				( 1 << 2 )
#define	EVENT_EML_SKLP_SETUP			( 1 << 3 )		// �������� ������� �� SKLP �� ��� ��������� ����������� ����������
#define	EVENT_EML_ALL					( EVENT_EML_TIMERTX | EVENT_EML_ADC_DMA | EVENT_EML_DAC_DMA | EVENT_EML_SKLP_SETUP)
extern EventGroupHandle_t xEML_EventGroup_hdl;
//#define	xEML_EventGroup_hdl		EventGroup_System
extern bool bEML_TxEn;

#endif	// EML_PERIPHERY_H

