// EML_Transport.c
// ������������ ������� ����:
// - ������������� �������� ���������� � �������;
// - �������� ������ � ���������� ������ ������;
// - �������� ����������� ��������� ������;
// - ���������� ������������� ��� ���������� ������;
// - ������������ ������ ��������� �� �������� �� ���������� ������ ������������ ������;
#include "ProjectConfig.h"		// ������ ���������
#include "stm32xxxx_hal.h"		// ����� ���������
#include "EML_DataLink.h"		// EML_SendPulses()
#include "EML_Transport.h"		// ������
#include "Platform_common.h"	// !!ADC_LIMIT_MIN()
#include "EML_Config.h"
#include "EML_Periphery.h"		// EML_PGA_Command()
#include <string.h>
#include "Utils.h"				// CalcCRC8SKLP_Load()

// ����������� ������� ������ ��������� �� ��������, ���� �� ���������� � ������� �������
#ifndef	EML_SEND_BUFFER
#define	EML_SEND_BUFFER		100
#endif	// EML_SEND_BUFFER

// ����� ��������� �� ��������
static __no_init EML_TxTick_t aEML_SendBuffer[EML_SEND_BUFFER];

// ������� ���� ������� � �������� DPPM
EML_TxTick_t EML_Code2DPPMTxTicks( EML_CodeValue_t Code )
{
	float DPPM_Time = 0;
	if( EML_CODE_MARK_START == Code )
		DPPM_Time = EML_DPPM_TIME_START_s;
	else if( IS_EML_CODE_VALUE( Code ) )
		DPPM_Time = EML_DPPM_TIME_START_s + ( 1 + Code ) * EML_DPPM_TIME_INCREMENT_s;
	return EML_DPPM_TIME2TXTICK( DPPM_Time );
}

// ������� ��������� DPPM � ��� �������
EML_CodeValue_t EML_DPPMRxTicks2Code( EML_RxTick_t Ticks )
{
	float Time = EML_DPPM_RXTICK2TIME( Ticks );
	EML_CodeValue_t Result;
	if( ( Time > ( EML_DPPM_TIME_PULSE_s * 0.8f ) ) && ( Time < ( EML_DPPM_TIME_PULSE_s * 1.2f ) ) )
		Result = EML_CODE_MARK_SYNC;
	else if( Time < ( EML_DPPM_TIME_START_s * 0.8f ) )
		Result = EML_CODE_MARK_ERR_SHORT;
	else
	{
		int i = ( int ) ( ( Time - EML_DPPM_TIME_START_s ) / EML_DPPM_TIME_INCREMENT_s + 0.5f );
		if( i < 0 )
			Result = EML_CODE_MARK_ERR_SHORT;
		else if( 0 == i )
			Result = EML_CODE_MARK_START;
		else if( i <= ( EML_CODE_VALUE_MAX + 1 ) )
			Result = ( EML_CodeValue_t ) ( i - 1 );
		else if( i > EML_CODE_MARK_PAUSE )
			Result = EML_CODE_MARK_PAUSE;
		else
			Result = EML_CODE_MARK_ERR_LONG;
	}
	
	return Result;
}

/*/ ������ ����������� �����
// ���� ���� ����� ������
EML_CodeValue_t EML_CalcControlSumm( EML_CodeValue_t Summ, EML_CodeValue_t Code )
{
	return ( Summ + Code ) % ( EML_CODE_VALUE_MAX + 1 );
}*/

// ������ ����������� ����� �� CRC8
uint8_t EML_CalcControlSumm( uint8_t CRC8, EML_CodeValue_t Code )
{
	return CalcCRC8SKLP_Load( &Code, sizeof( Code ), CRC8 );
}


// ����������� ������ ������ � ����� � �������� � ����
// pCodes		������ ������
// CodesCount	���������� ������ � �������
// return		�����/�������
// �������� ������� �������������� ����� ���������� (DPPM),
// ��������� ��������� � ����������� �����.
bool EML_SendPacket( EML_CodeValue_t *pCodes, uint32_t CodesCount )
{
	bool Result = false;
	do
	{
		if( ( NULL == pCodes ) || ( 0 == CodesCount ) )
			break;
		// ����������� �����
		uint8_t ControlSumm = 0;
		int i = 0;
		// ������ ������� - ���������, �� �������� ����� ���������� ������������ �� ������� ��������� ��������
		aEML_SendBuffer[i++] = 0;
		// ������ ������� - 'S'
		aEML_SendBuffer[i++] = EML_Code2DPPMTxTicks( EML_CODE_MARK_START );
		int PreambulaImpulsesCount = i;
		do
		{	// �������� ������� ������
			if( i >= SIZEOFARRAY( aEML_SendBuffer ) )
				break;
			// ������ ����������� ����� 
			EML_CodeValue_t CodeValue = pCodes[ i - PreambulaImpulsesCount ];
			ControlSumm = EML_CalcControlSumm( ControlSumm, CodeValue );
			// ����������� ������� � ���� ������� �����������
			EML_TxTick_t TxTicks = EML_Code2DPPMTxTicks( CodeValue ); 
			if( 0 == TxTicks )
				break;		// ������ ���, �� ������� ������������
			// �������� � ����� ������������ ����� ��������� ���������	
			aEML_SendBuffer[i++] = TxTicks;
		} while( --CodesCount );
		if( 0 != CodesCount )
			break;		// �� ������� ������������� ��� �������
		if( i >= SIZEOFARRAY( aEML_SendBuffer ) )
			break;
		// �������� � ����� �������������� ������� - ����������� �����
		aEML_SendBuffer[i++] = EML_Code2DPPMTxTicks( ControlSumm & EML_CODE_VALUE_MASK );	// !! ��������� ������ ������� ������� �� CRC8
		// �������� �������������� ����� � ����������
		uint32_t PulsesCount = i;
		if( PulsesCount != EML_SendFrame( aEML_SendBuffer, PulsesCount ) )
			break;		// � ���������� ������ ����� ������� �� ���������
		Result = true;
	} while( 0 );

	return Result;
}

// ���������� ������������ ��������� ��������
static bool EML_PulseValidate( EML_PulseRx_t *pPulse )
{
	assert_param( NULL != pPulse );
	bool Result = false;
	do
	{
		if( pPulse->Duration > EML_DPPM_TIME2RXTICK( EML_DPPM_TIME_PULSE_s * 1.5f ) )
			break;
		if( pPulse->Duration < EML_DPPM_TIME2RXTICK( EML_DPPM_TIME_PULSE_s * 0.5f ) )
			break;
		if( pPulse->Amplitude > 2047 )
			break;
		if( pPulse->Amplitude < 25 )
			break;
		Result = true;
	} while( 0 );
	return Result;
}

// ��������� �������� ��������� ����� �� ���������� ����� ����������
typedef enum EML_ReceivePacketState_enum
{
	EML_ReceivePacketState_Wait0,
	EML_ReceivePacketState_WaitS,
	EML_ReceivePacketState_WaitNext,
	EML_ReceivePacketState_WaitPause,
	EML_ReceivePacketState_Received,
} EML_ReceivePacketState_t;

// ��������� �������� � ������� ��������� �������, ���������� �������� �� ��� �����.
// - � ������� ����������� ������� ����������� ��������� � �������� �����;
// - �� ������ �������� �������� �� ����������� �������� ��� ���������� ����������� ������;
// - �� ���������� ��������� �������� 'S', � ����� � ���� �����;
// - ��������� �� �������� ��� ��� ���������� ������ ������.
// pCodes				- ������ ��� �������� ����������� ������;
// pCodesCount		- ������ �������, �� ������� ��������� pCodes.
//						��� ���������� ���� ������������ ���������� ���������� �����;
// Time_s				- �������� �������, � ������� �������� ����������� ��������� � ��������� ������.
// pRecFramesSummary	- ��������� ��������� ���� �������� ������
void EML_ReceivePacket( EML_CodeValue_t *pCodes, uint32_t *pCodesCount, float Time_s, EML_RecFrameResult_t *pRecFramesSummary )
{
	uint32_t FramesCount = ( uint32_t ) ( Time_s / EML_RX_FRAME_TIME_s );	// ������������ ���������� ����������� �������
	static __no_init EML_RecFrameResult_t RecFramesSummary;
	RecFramesSummary = ( EML_RecFrameResult_t ) { 0 };
	RecFramesSummary.PGA_Gain	= EML_PGA_CommandExt( PGA_Command_GetCurrent, &RecFramesSummary.iADC_PGA );
	RecFramesSummary.Minimum	= ADC_EML_MAX;
	RecFramesSummary.Maximum	= ADC_EML_MIN;
	RecFramesSummary.MinimumFilt= ADC_EML_MAX;
	RecFramesSummary.MaximumFilt= ADC_EML_MIN;
	uint32_t AvgSumm = 0, AvgCnt = 0;											// ���������� ��� ������� �������� �������� �� ���� ������
	EML_ReceivePacketState_t ReceivePacketState = EML_ReceivePacketState_Wait0;	// ��������� �������� ��������� ����� �� ���������
	EML_PulseRx_t PulseLast = ( EML_PulseRx_t ) { 0 };							// ��������� ������������ �������
	assert_param( NULL != pCodesCount );
	uint32_t CodesCountMax = *pCodesCount;
	*pCodesCount = 0;
	uint8_t CheckSumm;															// ����������� �����, ������������ �� ��������������� �����
	for( uint32_t i = 0; i < FramesCount; i++ )
	{
		// ������� ��������� �����, � �.�. ���������, � �������� �� ���� ��������
		static __no_init EML_RecFrameResult_t RecFrameResult;
		RecFrameResult = ( EML_RecFrameResult_t ) { 0 };
		EML_PulseRx_t aRecPulsesTmp[EML_PULSES_PER_RXFRAME_MAX];
		EML_ReceiveFrame( &RecFrameResult, aRecPulsesTmp, SIZEOFARRAY( aRecPulsesTmp ) );
		if( !RecFrameResult.bResult )
		{	// ��� ��������� ������ �������� ������, ������ ��������� ������ ������
			ReceivePacketState = EML_ReceivePacketState_Wait0;
			continue;
		}
		// ������� ������� ������� �� ������� ������������� �����
		if( 0 == RecFramesSummary.Timestamp )
			RecFramesSummary.Timestamp = RecFrameResult.Timestamp;
		// ���������, ��� ��� ���������� �������� ���������� � ��������������� �����
		assert_param( RecFrameResult.EventsCount <= SIZEOFARRAY( aRecPulsesTmp ) );
		// ���������, ��� � ���������� ��������� ��������� ����� ���� �������� ��������� ���������������
		if( 0 != RecFrameResult.PulseAmpl )
			RecFramesSummary.PulseAmpl = RecFrameResult.PulseAmpl;		// ��������� �������� ���������
		// �������� ���������� �� �������� �������
		RecFramesSummary.EventsCount += RecFrameResult.EventsCount;
		if( RecFrameResult.Maximum > RecFramesSummary.Maximum )
			RecFramesSummary.Maximum = RecFrameResult.Maximum;
		if( RecFrameResult.Minimum < RecFramesSummary.Minimum )
			RecFramesSummary.Minimum = RecFrameResult.Minimum;
		if( RecFrameResult.MaximumFilt > RecFramesSummary.MaximumFilt )
			RecFramesSummary.MaximumFilt = RecFrameResult.MaximumFilt;
		if( RecFrameResult.MinimumFilt < RecFramesSummary.MinimumFilt )
			RecFramesSummary.MinimumFilt = RecFrameResult.MinimumFilt;
		AvgSumm += RecFrameResult.Average;
		AvgCnt++;
/*
{
if( RecFrameResult.Maximum > 3000 )
	AvgCnt++;
if( RecFrameResult.Minimum < 1000 )
	AvgCnt-=2;
if( RecFrameResult.EventsCount > 28 )
	AvgCnt--;
}*/
		// ��������� �� ����� ����� �������� ����������� ����� (��������� ����� ������)
		if( EML_ReceivePacketState_WaitPause == ReceivePacketState )
			if( 0 == RecFrameResult.EventsCount )
				if( EML_CODE_MARK_PAUSE == EML_DPPMRxTicks2Code( RecFrameResult.Timestamp - PulseLast.Timestamp ) )
				{	// �������� ����� ����� �������� �������� ����������� �����
					assert_param( *pCodesCount > 1 );
					// ������ �� ����� �������� ����� ����������� �����
					( *pCodesCount )--;
					// ��������� ����� ������
					ReceivePacketState = EML_ReceivePacketState_Received;
					RecFrameResult.bResult = true;
					break;
				}
		// ��������� ���������� ��������
		for( uint32_t j = 0; j < RecFrameResult.EventsCount; j++ )
		{
			EML_PulseRx_t PulseCurrent = aRecPulsesTmp[j];
			EML_CodeValue_t CodeValue;
			if( !EML_PulseValidate( &PulseCurrent ) )
				continue;	// ����� �������� ����������, �� �������� ����� ������� �������
			// ��������� ��������� �������� �����������
			switch( ReceivePacketState )
			{
			case EML_ReceivePacketState_Wait0:		// ��������� ����� ������� ��������
				// ������� � ������� � �������� 'S'
				ReceivePacketState = EML_ReceivePacketState_WaitS;
				*pCodesCount = 0;
				CheckSumm = 0;
				break;

			case EML_ReceivePacketState_WaitS:		// ��������� ����� ������������� 'S'
				// ��������� �� ������������
				CodeValue = EML_DPPMRxTicks2Code( PulseCurrent.Timestamp - PulseLast.Timestamp );
				if( EML_CODE_MARK_START == CodeValue )
				{	// ������ ������������ 'S'. ���������� ��������� �������
					ReceivePacketState = EML_ReceivePacketState_WaitNext;
				}
				else
				{	// ������ �����-�� ������ ���. ���������� �������� 'S'
					ReceivePacketState = EML_ReceivePacketState_WaitS;
				}
				*pCodesCount = 0;
				CheckSumm = 0;
				break;
				
			case EML_ReceivePacketState_WaitNext:	// ��������� ����� ���������� ���� ����� 'S'
			case EML_ReceivePacketState_WaitPause:	// ��������� ����� ����� ����� ����, ��� ������� ����������� �����
				CodeValue = EML_DPPMRxTicks2Code( PulseCurrent.Timestamp - PulseLast.Timestamp );
				if( IS_EML_CODE_VALUE( CodeValue ) )
				{	// ������ ���������� ���, �������� ��� � �������� �����
					if( *pCodesCount < CodesCountMax )
						pCodes[*pCodesCount] = CodeValue;
					// ��������� ���������� ����������� �����, ��������� ������ ������� ������� CRC8
					if( ( CodeValue == ( CheckSumm & EML_CODE_VALUE_MASK ) ) && ( *pCodesCount > 0 ) )
						ReceivePacketState = EML_ReceivePacketState_WaitPause;		// ������� ����������� �����, �������� ��� ����� ������, ������� ����� ��� ��� ���� �������
					else
						ReceivePacketState = EML_ReceivePacketState_WaitNext;		// ����������� ����� �� �������, ���������� �������� ��������
					CheckSumm = EML_CalcControlSumm( CheckSumm, CodeValue );
					( *pCodesCount )++;
//if( *pCodesCount > 26 )
//	AvgCnt++;
				}
				else if( EML_CODE_MARK_START == CodeValue )
				{	// ����� ������ 'S', ������� ����� ������� �������
					ReceivePacketState = EML_ReceivePacketState_WaitNext;
					*pCodesCount = 0;
					CheckSumm = 0;
				}
				else
				{	// ������ ������� ���, ������ �������� ������� �������
					ReceivePacketState = EML_ReceivePacketState_WaitS;
				}
				break;
			
			default:
				assert_param( 0 );
			}
			PulseLast = PulseCurrent;
		}
	}

	if( 0 != AvgCnt )
		RecFramesSummary.Average = AvgSumm / AvgCnt;
	
	// �������������� ���������
	ADC_EML_Data_t SignalGaugeMax = RecFramesSummary.Maximum - RecFramesSummary.Minimum;
	if( SignalGaugeMax < 1600 )
		EML_PGA_Command( PGA_Command_Inc );
	else if( SignalGaugeMax > 3600 )
		EML_PGA_Command( PGA_Command_Dec );
	// !!! �����! � ������ ���� ������� EML_ReceivePacket()
	// !!! ���������� ���������� ����� RecFramesSummary.PGA_Gain � RecFramesSummary.iADC_PGA
	// !!! ��������� � �������� PGA, ������� ����� �������� ��� ������� ��������� ����� ������.
	// !!! � ���������� ���������, ��� ������� ������ ��� ������� ������� ��������
	// !!! ������������ �������������� PGA. ������������ PGA ������������ ����� ��.
	// !!! � ������� ����������������� ���, ����� ������ PGA ������ ���� ������� � ���,
	// !!! ����� ����� ������ ������� ������� ��� �� ��� ������� � ����� �� ���������.
	// !!! ������� ����� ���������� ������ RecFramesSummary.iADC_PGA �� ��������� �����, ������� ����� ������� � ���,
	// !!! ������ �������� ����������� �������� � ���� ����� RecFramesSummary.PGA_Gain �����������.
	EML_PGA_CommandExt( PGA_Command_GetCurrent, &RecFramesSummary.iADC_PGA );

	// ������� ���������
	if( NULL != pRecFramesSummary )
		*pRecFramesSummary = RecFramesSummary;
}

