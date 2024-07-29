// EML_Periphery.h
// Ќижний (физический) уровень Ёћ —:
// - ј÷ѕ приемника;
// - PGA приемника;
// -  лючи управлени€ премо-передачей;
// - ‘ормирование импульсов через GPIO или DAC;
#ifndef	EML_PERIPHERY_H
#define	EML_PERIPHERY_H

#include <stdint.h>
#include <stdbool.h>
#include "Driver_PGA.h"		// драйвер PGA
#include "EML_Config.h"		// EML_TxTick_t
#include "FreeRTOS.h"
#include "Task.h"
#include "Event_groups.h"

// »нициализация всей необходимой периферии (ADC, DAC, Timer, PGA)
void EML_PeripheryInit( void );
// ѕереключение усиления PGA
PGA_Gain_t EML_PGA_Command( PGA_Command_t Command );
PGA_Gain_t EML_PGA_CommandExt( PGA_Command_t Command, int *piADC_PGA );
// ¬ыждать время от начала последнего импульса и сформировать новый импульс в передающую катушку
bool EML_TxPulseDelayed( EML_TxTick_t Delay_ticks );
// ѕереключение режима работы приемника и передатчика Ёћ —
void EML_TxOn_RxOff( void );
void EML_TxOff_RxOn( void );
// ¬ернуть последний оцифрованный кадр
ADC_EML_Frame_t *EML_ADC_GetFrameCurrent( void );


bool EML_DAC_Pulse( void );
bool EML_DAC_Pulses( uint32_t PulsesCount );


// —обытия для реализации Ёћ —
#define	EVENT_EML_TIMERTX				( 1 << 0 )		// завершение очередной фазы таймера, формирующего передаваемый сигнал
#define	EVENT_EML_ADC_DMA				( 1 << 1 )		// заверешена оцифровка половины или целого буфера принимаемого сигнала
#define	EVENT_EML_DAC_DMA				( 1 << 2 )
#define	EVENT_EML_SKLP_SETUP			( 1 << 3 )		// получена команда по SKLP от ћѕ» установки сохраненных параметров
#define	EVENT_EML_ALL					( EVENT_EML_TIMERTX | EVENT_EML_ADC_DMA | EVENT_EML_DAC_DMA | EVENT_EML_SKLP_SETUP)
extern EventGroupHandle_t xEML_EventGroup_hdl;
//#define	xEML_EventGroup_hdl		EventGroup_System
extern bool bEML_TxEn;

#endif	// EML_PERIPHERY_H

