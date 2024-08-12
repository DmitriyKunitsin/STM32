// EML_DataLink.h
// ��������� ������� ����:
// - ������������ ������ �����/��������
// - ��������� ����������� ��������� �� ������ ���������� ������������ �������;
// - �������������� ���������� PGA;
// - ������������ ��������� �������� ��������� ����������;
#ifndef	EML_DATALINK_H
#define	EML_DATALINK_H
#include "EML_Config.h"		// EML_TxTick_t
#include "Driver_PGA.h"		// PGA_Gain_t

// ��������� ��������� ����� ���� ��������
typedef struct EML_PulseRx_struct
{
	EML_RxTick_t	Timestamp;		// ����� ������� (�� �������� ��������)
	EML_RxTick_t	Duration;		// ������������ �������� (�� ��������� ����� �����)
	ADC_EML_Data_t	Amplitude;		// ���������
} EML_PulseRx_t;

// ��������� ��������� ������������� �����
typedef struct EML_RecFrameResult_struct
{
	bool			bResult;
	EML_RxTick_t	Timestamp;		// ������� ������� ������ �����
	ADC_EML_Data_t	Minimum;		// ����������� �������� ��������� � �����
	ADC_EML_Data_t	Maximum;		// ������������ �������� ��������� � �����
	ADC_EML_Data_t	Average;		// ����������� �������� ��������� � �����
	ADC_EML_Data_t	MinimumFilt;	// ����������� �������� ��������� � ����� (����� ����������)
	ADC_EML_Data_t	MaximumFilt;	// ������������ �������� ��������� � ����� (����� ����������)
//	ADC_EML_Data_t	AverageFilt;	// ����������� �������� ��������� � ����� (����� ����������)
	uint32_t		EventsCount;	// ���������� ���������� �������, ���������������� ���������
	PGA_Gain_t		PGA_Gain;		// [1/8..176]	�������� PGA ���������
	int 			iADC_PGA;		// [0..21]		������ PGA ���������
	ADC_EML_Data_t	PulseAmpl;		// [0..2047]	����������� ��������� �������� ���������������
} EML_RecFrameResult_t;

bool EML_DataLinkInit( void );														// �������������
uint32_t EML_SendFrame( EML_TxTick_t *pPulsesTicks, uint32_t PulsesCount );		// �������� ������ ���������
bool EML_SendSync( int PulsesCount );											// ��������� ����������� ����� ���������
void EML_ReceiveFrame( EML_RecFrameResult_t *pResult, EML_PulseRx_t *pPulses, uint32_t PulsesCountMax );	// ����� � ��������� ����� ��������� 

#endif	// EML_DATALINK_H
