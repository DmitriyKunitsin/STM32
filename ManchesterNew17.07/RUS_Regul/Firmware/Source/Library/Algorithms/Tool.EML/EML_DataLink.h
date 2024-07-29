// EML_DataLink.h
//  анальный уровень Ёћ —:
// - переключение режима прием/передача
// - выделение принимаемых импульсов из буфера оцифровкии формирование очереди;
// - автоматическа€ подстройка PGA;
// - формирование импульсов согласно требуемым интервалам;
#ifndef	EML_DATALINK_H
#define	EML_DATALINK_H
#include "EML_Config.h"		// EML_TxTick_t
#include "Driver_PGA.h"		// PGA_Gain_t

// —труктура принятого через Ёћ — импульса
typedef struct EML_PulseRx_struct
{
	EML_RxTick_t	Timestamp;		// время прихода (по середине импульса)
	EML_RxTick_t	Duration;		// длительность импульса (по переходам через порог)
	ADC_EML_Data_t	Amplitude;		// амплитуда
} EML_PulseRx_t;

// –езультат обработки оцифрованного кадра
typedef struct EML_RecFrameResult_struct
{
	bool			bResult;
	EML_RxTick_t	Timestamp;		// отсечка времени начала кадра
	ADC_EML_Data_t	Minimum;		// минимальная величина оцифровки в кадре
	ADC_EML_Data_t	Maximum;		// максимальная величина оцифровки в кадре
	ADC_EML_Data_t	Average;		// усредненная величина оцифровок в кадре
	ADC_EML_Data_t	MinimumFilt;	// минимальная величина оцифровки в кадре (после фильтрации)
	ADC_EML_Data_t	MaximumFilt;	// максимальная величина оцифровки в кадре (после фильтрации)
//	ADC_EML_Data_t	AverageFilt;	// усредненная величина оцифровок в кадре (после фильтрации)
	uint32_t		EventsCount;	// количество выделенных событий, предположительно импульсов
	PGA_Gain_t		PGA_Gain;		// [1/8..176]	величина PGA приемника
	int 			iADC_PGA;		// [0..21]		индекс PGA приемника
	ADC_EML_Data_t	PulseAmpl;		// [0..2047]	усредненная амплитуда принятых синхроимпульсов
} EML_RecFrameResult_t;

bool EML_DataLinkInit( void );														// »нициализация
uint32_t EML_SendFrame( EML_TxTick_t *pPulsesTicks, uint32_t PulsesCount );		// ѕередача пакета импульсов
bool EML_SendSync( int PulsesCount );											// ќтправить непрерывную серию импульсов
void EML_ReceiveFrame( EML_RecFrameResult_t *pResult, EML_PulseRx_t *pPulses, uint32_t PulsesCountMax );	// ѕрием и обработка кадра оцифровки 

#endif	// EML_DATALINK_H
