// EML_DataLink.c
// ��������� ������� ����:
// - ������������ ������ �����/��������
// - ��������� ����������� ��������� �� ������ ���������� ������������ �������;
// - �������������� ���������� PGA;
// - ������������ ��������� �������� ��������� ����������;
#include "ProjectConfig.h"		// ������ ���������
#include "stm32xxxx_hal.h"		// ����� ���������
#include "EML_Periphery.h"
#include "EML_DataLink.h"		// ������
#include <string.h>				// memcpy()
#include "Platform_common.h"	// ADC_SPAN()
#include "MathUtils.h"			// ABS()
#include "EML_Transport.h"		// EML_DPPMRxTicks2Code()
#include <math.h>				// sqrtf()

// �������� ������ ���������
// pPulsesTicks	������ ����������, ����� ������� ������ ���� �������� ��������.
//					������ �������� ������ ���� �������, ����� ������� ����� ������������ ����������.
// PulsesCount	���������� ��������� �� ��������
// return			���������� ���������� ���������
uint32_t EML_SendFrame( EML_TxTick_t *pPulsesTicks, uint32_t PulsesCount )
{
	uint32_t Result = 0;
	do
	{
		if( ( NULL == pPulsesTicks ) || ( 0 == PulsesCount ) )
			break;
		if( 0 != pPulsesTicks[0] )
			break;

		EML_TxOn_RxOff( );
		vTaskDelay( pdMS_TO_TICKS( 20 ) );
		#warning "23.08.2019 ������� ��� ��������, �� ����� ����� ����� �� ����� �� 45 ��"
		vTaskDelay( pdMS_TO_TICKS( 30 ) );

		uint32_t i;
		for( i = 0; i < PulsesCount; i++ )
		{
			if( !EML_TxPulseDelayed( pPulsesTicks[i] ) )
				break;
		}

#warning "10.09.2019 ������� ��� ��������, ����� ������� ��� ���������� ����������� 752_07-02 �� ����� ����������� ��������� ���������� �������"
		vTaskDelay( pdMS_TO_TICKS( EML_RX_FRAME_TIME_s * 1.1 * 1000  ) );

		EML_TxOff_RxOn( );
		Result = i;
	} while( 0 );

	return Result;
}

bool EML_SendSync( int PulsesCount )
{
	EML_TxOn_RxOff( );
	#warning "����� �������� ��� ����� �����, ���� vTaskDelay( 2 )"
	vTaskDelay( 100 ); //����� �������� ��� ����� �����, ���� vTaskDelay( 2 );

	assert_param( NULL != xEML_EventGroup_hdl );
	( void ) xEventGroupClearBits( xEML_EventGroup_hdl, EVENT_EML_DAC_DMA );
	EML_DAC_Pulses( PulsesCount );
	xEventGroupWaitBits( xEML_EventGroup_hdl, EVENT_EML_DAC_DMA, pdTRUE, pdFALSE, 50 );

	EML_TxOff_RxOn( );
	return true;
}



typedef struct EML_Receiver_struct
{
	ADC_EML_Frame_t	Frame;
	ADC_EML_Data_t	Maximum;
	ADC_EML_Data_t	Minimum;
	ADC_EML_Data_t	Average;
	ADC_EML_Data_t	TreshPeakPos;
	ADC_EML_Data_t	TreshPeakNeg;
	ADC_EML_Data_t	TreshIdlePos;
	ADC_EML_Data_t	TreshIdleNeg;
	EML_RxTick_t	TimeFrame;				// ������� ������� ������ �����
	EML_RxTick_t	TimeIdle;
	EML_RxTick_t	TimePeakMin;
	EML_RxTick_t	TimePeakMax;
	EML_RxTick_t	TimeStartCalcAmpl;		// ������� ������� ������ ������� ��������� ����������� �������
	uint8_t			SyncPulseCount;			// ���������� ��������� ������ ��������� �������������
	EML_RxTick_t	SyncPulseTicksCount;	// ���������� ���������, �� ������� ���������� ������ ���������
	uint32_t		PulseAmplSumm;			// ����� �������� ��� ������ �������������
} EML_Receiver_t;

static __no_init EML_Receiver_t EML_REC, EML_REC_tmp;
static bool bEML_REC_InitComplete = false;

static __no_init ADC_EML_Data_t aEML_REC_Archive[ADC_EML_FRAME_SIZE*60];
static int iEML_REC_Archive = 0;

typedef enum RecWait_enum
{	// ���� �������� ���������� �������:
	RecWait_Idle,			// �����
	RecWait_PosStart,		// ������ �������������� ����
	RecWait_PosFinish,		// ���������� �������������� ����
	RecWait_Gap1,			// ����� ����� ������������� � ������������� �����
	RecWait_NegStart,		// ������ �������������� ����
	RecWait_NegFinish,		// ���������� �������������� ����
	RecWait_Gap2,			// ����� ����� �������������� ����
//	RecWait_Idle2,			// ���������� ������ � ������� ���-�������� ����� ������ �������� 8-)
} RecWait_t;

// ����� � ��������� ����� ���������
// - ��������� ���������� ��������� �����;
// - ����������� ������������ �����;
// - ���������� ��������� ��������� (�������, ��������, �������);
// - ���������� ��������� ���������;
// - �������� ���������� ������� � �������.
// pResult			- ��������� ��������� �������� �����
// pPulses			- ������ ��� �������� ���������
// PulsesCountMax	- ������ ������� ��� �������� ���������
void EML_ReceiveFrame( EML_RecFrameResult_t *pResult, EML_PulseRx_t *pPulses, uint32_t PulsesCountMax )
{
	if( !bEML_REC_InitComplete )
	{
		EML_REC = ( EML_Receiver_t ) { 0 };
//		EML_REC.TreshPeak	= 100;
//		EML_REC.TreshIdle	= 10;
		EML_REC.TreshPeakPos	= 300;
		EML_REC.TreshIdlePos	= 50;
		EML_REC.TreshPeakNeg	= 500;
		EML_REC.TreshIdleNeg	= 70;
		EML_REC.TimeFrame	= 0;
		EML_REC.TimeIdle	= EML_DPPM_TIME2RXTICK( EML_DPPM_TIME_PULSE_s );			// ����� ��������� ������ ����� ������������� �� ����� ������ ��������
		EML_REC.TimePeakMin	= 3;														// �� ������ ���� ����� �� ���, ����� �� ������ ������
//		EML_REC.TimePeakMax	= EML_DPPM_TIME2RXTICK( EML_DPPM_TIME_PULSE_s * 0.45f );	// ������������ ����� ��������� �� ����� �������� ���������� ��������
		EML_REC.TimePeakMax	= EML_DPPM_TIME2RXTICK( EML_DPPM_TIME_PULSE_s * 0.60f );	// ������������ ����� ��������� �� ����� �������� ���������� ��������
		EML_REC.TimeStartCalcAmpl	= 0;
		EML_REC.SyncPulseCount		= 0;
		EML_REC.SyncPulseTicksCount	= 0;
		EML_REC.PulseAmplSumm		= 0;
		EML_REC.Average				= 2048;
		EML_REC.Maximum				= 2048;
		EML_REC.Minimum				= 2048;
		bEML_REC_InitComplete = true;
	}
	
	EML_RecFrameResult_t RecFrame;
	RecFrame.bResult = false;
	RecFrame.PGA_Gain = EML_PGA_Command( PGA_Command_GetCurrent );
	RecFrame.PulseAmpl = 0;
	do
	{
		// ��������� ���������� ��������� �����
		assert_param( NULL != xEML_EventGroup_hdl );
		EventBits_t EventBitsResult = xEventGroupWaitBits( xEML_EventGroup_hdl, EVENT_EML_ADC_DMA, pdTRUE, pdFALSE, pdMS_TO_TICKS( 2 * 1000 * EML_RX_FRAME_TIME_s ) );
		if( 0 == ( EVENT_EML_ADC_DMA & EventBitsResult ) )
			break;		// �� ��������� ������� �� EML.ADC.DMA!

		// �������� ������������ �����
		ADC_EML_Frame_t *pFrame = EML_ADC_GetFrameCurrent( );
		EML_REC.TimeFrame += SIZEOFARRAY( pFrame->aBuffer );
		if( NULL == pFrame )
			break;		// ������� ����, � ����� ���?!
		RecFrame.EventsCount = 0;
		RecFrame.Timestamp = EML_REC.TimeFrame;

		// ����������� ������������ �����
		EML_REC.Frame = *pFrame;
		if( pFrame != EML_ADC_GetFrameCurrent( ) )
			break;		// �������� �� ��������� ����, �������� ��� ����������� ���� ��� �������
		// �������������� ����� ��� ���������, � ������� �� ����������� �������� ��� ������������
		EML_REC_tmp = EML_REC;
			
		// ���������� ��������� ��������� (�������, ��������, �������)
		RecFrame.Maximum = ADC_EML_MIN;
		RecFrame.Minimum = ADC_EML_MAX;
		uint32_t AvgSumm = 0;
		for( EML_RxTick_t i = 0; i < SIZEOFARRAY( EML_REC.Frame.aBuffer ); i++ )
		{
			ADC_EML_Data_t Data = EML_REC.Frame.aBuffer[i];
			if( Data > RecFrame.Maximum )
				RecFrame.Maximum = Data;
			if( Data < RecFrame.Minimum )
				RecFrame.Minimum = Data;
			AvgSumm += Data;
		}
		if( RecFrame.Maximum > ADC_EML_MAX )
			break;
		RecFrame.Average = AvgSumm / SIZEOFARRAY( EML_REC.Frame.aBuffer );
		if( ( 0 == RecFrame.Average ) || ( RecFrame.Average > 4095 ) )
			RecFrame.Average &= 0x0FFF;

		// ���������� ����������, ���������� ������� � �������� ����� ����������
		RecFrame.MaximumFilt = ADC_EML_MIN;
		RecFrame.MinimumFilt = ADC_EML_MAX;
		for( EML_RxTick_t i = 0; i < SIZEOFARRAY( EML_REC.Frame.aBuffer ); i++ )
		{
/*			// ��������� ������� ������� �����
			EML_RxTick_t i_0, i_1, i_2, i_3;
			if( i > ( EML_RxTick_t ) ( EML_DPPM_TIME_PULSE_s / EML_RX_TICK_s + 0.5f ) )
			{
				i_0 = i;
				i_1 = i - ( EML_RxTick_t ) ( 1 * EML_DPPM_TIME_PULSE_s / EML_RX_TICK_s / 4 + 0.5f );
				i_2 = i - ( EML_RxTick_t ) ( 2 * EML_DPPM_TIME_PULSE_s / EML_RX_TICK_s / 4 + 0.5f );
				i_3 = i - ( EML_RxTick_t ) ( 3 * EML_DPPM_TIME_PULSE_s / EML_RX_TICK_s / 4 + 0.5f );
			}
			else
			{
				i_0 = i;
				i_1 = i + ( EML_RxTick_t ) ( 1 * EML_DPPM_TIME_PULSE_s / EML_RX_TICK_s / 4 + 0.5f );
				i_2 = i + ( EML_RxTick_t ) ( 2 * EML_DPPM_TIME_PULSE_s / EML_RX_TICK_s / 4 + 0.5f );
				i_3 = i + ( EML_RxTick_t ) ( 3 * EML_DPPM_TIME_PULSE_s / EML_RX_TICK_s / 4 + 0.5f );
			}
			// ��������� ������� �� ������� ������
			ADC_EML_Data_t SignalAvg = ( EML_REC_tmp.Frame.aBuffer[i_0] + EML_REC_tmp.Frame.aBuffer[i_1] + EML_REC_tmp.Frame.aBuffer[i_2] + EML_REC_tmp.Frame.aBuffer[i_3] + 2 ) / 4;
*/

/*			int j_inc;
			if( i > ( EML_RxTick_t ) ( EML_DPPM_TIME_PULSE_s / EML_RX_TICK_s + 0.5f ) )
				j_inc = -1;
			else
				j_inc = 1;
			uint32_t TmpSumm = 0;
			uint32_t TmpSummCnt = 0;
			for( int j = i; TmpSummCnt < ( ( EML_RxTick_t ) ( EML_DPPM_TIME_PULSE_s / EML_RX_TICK_s + 0.5f ) ); j += j_inc, TmpSummCnt++ )
				TmpSumm += EML_REC_tmp.Frame.aBuffer[j];
			ADC_EML_Data_t SignalAvg = ( ADC_EML_Data_t ) ( TmpSumm / TmpSummCnt );
			// ������� ��������� �������, ������������ ����� �������
			EML_REC.Frame.aBuffer[i] = ( EML_REC_tmp.Frame.aBuffer[i] + EML_REC.Average ) - SignalAvg;
*/

			if( EML_REC.Frame.aBuffer[i] > 4095 )
				EML_REC.Frame.aBuffer[i] &= 0x0FFF;

			// ���������� ������� � �������� ����� ����������
			ADC_EML_Data_t Data = EML_REC.Frame.aBuffer[i];
			if( Data > RecFrame.MaximumFilt )
				RecFrame.MaximumFilt = Data;
			if( Data < RecFrame.MinimumFilt )
				RecFrame.MinimumFilt = Data;
		}
		if( ( RecFrame.MaximumFilt > 3500 ) || ( RecFrame.MaximumFilt < 1500 ) || ( RecFrame.MinimumFilt > 2500 ) || ( RecFrame.MinimumFilt < 200 ))
			RecFrame.MaximumFilt &= 0x0FFF;

		static RecWait_t RecWait = RecWait_Idle, RecWaitPrev = RecWait_Idle;
		// ����������� ������ (����� �������, ����� ���������� ������� �������� �� ������� �����)
		ADC_EML_Data_t DeltaMaxPos = MAX( ( RecFrame.MaximumFilt - RecFrame.Average ), ( EML_REC.Maximum - EML_REC.Average ) );
		ADC_EML_Data_t DeltaMaxNeg = MAX( ( RecFrame.Average - RecFrame.MinimumFilt ), ( EML_REC.Average - EML_REC.Minimum ) );
		if( DeltaMaxNeg < DeltaMaxPos )
			DeltaMaxNeg = DeltaMaxPos;
		EML_REC.Maximum = RecFrame.MaximumFilt;
		EML_REC.Minimum = RecFrame.MinimumFilt;
		EML_REC.Average = RecFrame.Average;
		if( ( RecWait_Idle == RecWait ) || ( RecWait_PosStart == RecWait ) )
		{
			EML_REC.TreshPeakPos = ( ADC_EML_Data_t ) ( DeltaMaxPos * EML_ADC_THRESH_PEAK_POS_REL );
			EML_REC.TreshIdlePos = ( ADC_EML_Data_t ) ( DeltaMaxPos * EML_ADC_THRESH_IDLE_POS_REL );
			EML_REC.TreshPeakNeg = ( ADC_EML_Data_t ) ( DeltaMaxNeg * EML_ADC_THRESH_PEAK_NEG_REL );
			EML_REC.TreshIdleNeg = ( ADC_EML_Data_t ) ( DeltaMaxNeg * EML_ADC_THRESH_IDLE_NEG_REL );
			if( ( EML_REC.TreshPeakPos > 1000 ) || ( EML_REC.TreshPeakNeg > 1000 ) )
				EML_REC.TreshPeakPos &= 0x0FFF;
			if( ( EML_REC.TreshPeakPos < EML_ADC_THRESH_PEAK_POS_ABS ) || ( EML_REC.TreshPeakNeg < EML_ADC_THRESH_PEAK_NEG_ABS ) )
			{	// � ����� �� ���� �������� ���������� ������� ���������, ���������� ��������� �����
				RecFrame.bResult = true;
				break;
			}
		}
		else
		{	// ���� ������� � ���������� ����� ����� �� ������� ������, ��������� ������ ����������� �����
		}

		// ���������� ��������� ���������
		static EML_RxTick_t PhaseCounter = 0;	// ������� ���������� � ������� ����
		static int16_t iPosStart, iPosFinish, iNegStart, iNegFinish;
		static ADC_EML_Data_t PulseMinimum, PulseMaximum;
		for( EML_RxTick_t i = 0; i < SIZEOFARRAY( EML_REC.Frame.aBuffer ); i++ )
		{
			ADC_EML_Data_t Signal = EML_REC.Frame.aBuffer[i];
			if( ( EML_REC.SyncPulseCount > 3 ) && ( EML_REC.SyncPulseCount < ( EML_SYNC_PERIODS_COUNT - 2 ) ) )
			{	// ������� ���������� ��������������� ����� ����������� ������ ���������
				// ��������� ������� ������� �����
				EML_RxTick_t i_0 = i;
				EML_RxTick_t i_1 = i - ( EML_RxTick_t ) ( 1 * EML_DPPM_TIME_PULSE_s / EML_RX_TICK_s / 4 + 0.5f );
				EML_RxTick_t i_2 = i - ( EML_RxTick_t ) ( 2 * EML_DPPM_TIME_PULSE_s / EML_RX_TICK_s / 4 + 0.5f );
				EML_RxTick_t i_3 = i - ( EML_RxTick_t ) ( 3 * EML_DPPM_TIME_PULSE_s / EML_RX_TICK_s / 4 + 0.5f );
				if( i_3 < i_0 )
				{
					// ��������� ������� �� ������� ������
					float SignalAvg = ( EML_REC_tmp.Frame.aBuffer[i_0] + EML_REC_tmp.Frame.aBuffer[i_1] + EML_REC_tmp.Frame.aBuffer[i_2] + EML_REC_tmp.Frame.aBuffer[i_3] ) / 4.0f;
					// ���������� ��������� �� ������� ������
					ADC_EML_Data_t SignalAmpl = ( ADC_EML_Data_t ) ( 0.5f + sqrtf( -( EML_REC_tmp.Frame.aBuffer[i_0] - SignalAvg ) * ( EML_REC_tmp.Frame.aBuffer[i_2] - SignalAvg ) - ( EML_REC_tmp.Frame.aBuffer[i_1] - SignalAvg ) * ( EML_REC_tmp.Frame.aBuffer[i_3] - SignalAvg ) ) );
					EML_REC.PulseAmplSumm += SignalAmpl;
					EML_REC.SyncPulseTicksCount++;
				}
			}
			int32_t SignalDelta = ( int32_t ) ( Signal - RecFrame.Average );
			RecWaitPrev = RecWait;
			bool bPulseCaptured = false;
			switch( RecWait )
			{
			case RecWait_Idle:		// �������� ����� �� �����
				if( ABS( SignalDelta ) < EML_REC.TreshIdlePos )
				{
					PhaseCounter++;
					if( PhaseCounter > EML_REC.TimeIdle )
					{	// ���������� � �������� ������ ������ ���������
						RecWait = RecWait_PosStart;
						PulseMinimum = ADC_EML_MAX;
						PulseMaximum = ADC_EML_MIN;
					}
				}
				else
					PhaseCounter = 0;
				break;
				
			case RecWait_PosStart:	// �������� ������ �������������� ����
				if( -SignalDelta > EML_REC.TreshPeakNeg )
				{	// ������ ��������, ��������� � �������� �����
//					RecWait = RecWait_Idle;
					RecWait = RecWait_PosStart;
					break;
				}
				if( SignalDelta > EML_REC.TreshPeakPos )
				{	// ������ ������ ������������ ������ ��� ��������� ���� ������ ���������
					if( Signal > PulseMaximum )
						PulseMaximum = Signal;
					PhaseCounter++;
					if( PhaseCounter < EML_REC.TimePeakMin )
						break;
					// ������������ ������ ������������ ������ � ������������.
					// ������� � �������� ���������� ������ ���������
					RecWait = RecWait_PosFinish;
					iPosStart = i - EML_REC.TimePeakMin;
				}
				else
					PhaseCounter = 0;
				break;
				
			case RecWait_PosFinish:	// �������� ���������� �������������� ����
				if( SignalDelta > EML_REC.TreshPeakPos )
				{
					PhaseCounter++;
					if( Signal > PulseMaximum )
						PulseMaximum = Signal;
				}
				if( SignalDelta < EML_REC.TreshPeakPos )
				{	// ������ ��������, ������� � �������� �������� ����� 0
					RecWait = RecWait_Gap1;
					iPosFinish = i;
				}
				else if( PhaseCounter > EML_REC.TimePeakMax )
				{	// ������ ������� ����� ��������� �� ������� ������, ����� ����� �����
//					RecWait = RecWait_Idle;
					RecWait = RecWait_PosStart;
				}
				break;

			case RecWait_Gap1:		// �������� �������� ����� ����
				PhaseCounter++;
				if( SignalDelta < EML_REC.TreshIdlePos )
				{	// ������ ����������� � ����, ������� � �������� ������������� ���������
					RecWait = RecWait_NegStart;
				}
				if( PhaseCounter > EML_REC.TimePeakMax )
				{	// ������� ������ �������� ������� ������, ����� ����� �����
//					RecWait = RecWait_Idle;
					RecWait = RecWait_PosStart;
				}
				break;

			case RecWait_NegStart:	// �������� ������ �������������� ����
				if( SignalDelta > EML_REC.TreshPeakPos )
				{	// ������ ������� ������, ��������� � �������� �����
//					RecWait = RecWait_Idle;
					RecWait = RecWait_PosStart;
					break;
				}
				if( -SignalDelta > EML_REC.TreshPeakNeg )
				{	// ������ ������ ������������ ������ ��� ��������� ���� ������ ���������
					if( Signal < PulseMinimum )
						PulseMinimum = Signal;
					PhaseCounter++;
					if( PhaseCounter < EML_REC.TimePeakMin )
						break;
					// ������������ ������ ������������ ������ � ������������.
					// ������� � �������� ���������� ������ ���������
					RecWait = RecWait_NegFinish;
					iNegStart = i - EML_REC.TimePeakMin;
				}
				else
				{
					PhaseCounter = 0;
					if( ( i - iPosFinish ) > ( EML_REC.TimeIdle / 2 ) )
					{	// ������� ������ �������� ������ ������ ���������
//						RecWait = RecWait_Idle;
						RecWait = RecWait_PosStart;
						break;
					}
				}
				break;

			case RecWait_NegFinish:	// �������� ���������� �������������� ����
				if( -SignalDelta > EML_REC.TreshPeakNeg )
				{	// ������ �� ���������� ������ ������
					PhaseCounter++;
					if( Signal < PulseMinimum )
						PulseMinimum = Signal;
					if( PhaseCounter > EML_REC.TimePeakMax )
					{	// ������ ������� ����� ��������� �� ������ ������, ����� ����� �����
//						RecWait = RecWait_Idle;
						RecWait = RecWait_PosStart;
					}
				}
				else
				{	// ������ ��������, ������������� ������ ����� � ����� ������� � �������� �����
					RecWait = RecWait_Gap2;
					iNegFinish = i;
				}
				break;

			case RecWait_Gap2:		// �������� �������� ����� ����
				PhaseCounter++;
				if( -SignalDelta < EML_REC.TreshIdleNeg )
				{	// ������ ����������� � ����, ��������� ����� �������� �������� � ������� � �������� ����������
					bPulseCaptured = true;
					RecWait = RecWait_PosStart;
//					RecWait = RecWait_Idle2;
				}
				if( PhaseCounter > EML_REC.TimePeakMax )
				{	// ������� ������ �������� ������� ������, ����� ����� �����
//					RecWait = RecWait_Idle;
					RecWait = RecWait_PosStart;
				}
				break;
/*
			case RecWait_Idle2:		// ����� ����� ������ ���������� ��������
				PhaseCounter++;
				if( PhaseCounter > ( EML_REC.TimeIdle / 2 ) )
					RecWait = RecWait_PosStart;
				break;
*/
			default:
				assert_param( 0 );
			}
			if( RecWait != RecWaitPrev )
			{	// � ���������� ������� ���������� ����, �������� ������� ������� ����
				PhaseCounter = 0;
				EML_REC.Frame.aBuffer[i] = ( RecWaitPrev + 1 ) * 50;
			}
			if( bPulseCaptured )
			{	// � ���������� ������� ������������ �������, ������������� ��� ��������������
				static EML_PulseRx_t PulsePrev = { 0, 0, 0 };
				EML_PulseRx_t Pulse;
				Pulse.Amplitude	= ( PulseMaximum - PulseMinimum ) / 2;
				Pulse.Duration	= iNegFinish - iPosStart;
				Pulse.Timestamp	= EML_REC.TimeFrame + ( iNegFinish + iPosStart ) / 2;
				if( ( NULL != pPulses ) && ( RecFrame.EventsCount < PulsesCountMax ) )
					pPulses[RecFrame.EventsCount++] = Pulse;
				// ����������, ��� ��������� �������������
				if( EML_CODE_MARK_SYNC == EML_DPPMRxTicks2Code( Pulse.Timestamp - PulsePrev.Timestamp ) )
				{	// ������ ������� � ������� �������������
					if( 0 == EML_REC.TimeStartCalcAmpl )
						EML_REC.TimeStartCalcAmpl = Pulse.Timestamp;
					EML_REC.SyncPulseCount++;
					if( EML_REC.SyncPulseCount > ( EML_SYNC_PERIODS_COUNT - 2 ) )
					{	// ������� ���������� ��������������� ����� ����������� ������ ���������
						RecFrame.PulseAmpl = ( ADC_EML_Data_t ) ( EML_REC.PulseAmplSumm / EML_REC.SyncPulseTicksCount );
					}
				}
				else
				{	// ��� �� ������� � ������� �������������
					EML_REC.TimeStartCalcAmpl	= 0;
					EML_REC.SyncPulseCount		= 0;
					EML_REC.SyncPulseTicksCount = 0;
					EML_REC.PulseAmplSumm		= 0;
				}
				PulsePrev = Pulse;
				PulseMinimum = ADC_EML_MAX;
				PulseMaximum = ADC_EML_MIN;
				EML_REC.Frame.aBuffer[i] = ADC_EML_MAX;
			}
		}

		if( ( RecWait_Idle != RecWait ) && ( RecWait_PosStart != RecWait ) )
		{	// ���� ������� ����� �� ����� �����, �� ��������� ���������� ��� ��������� ���������� �����
			iPosStart	-= SIZEOFARRAY( EML_REC.Frame.aBuffer );
			iPosFinish	-= SIZEOFARRAY( EML_REC.Frame.aBuffer );
			iNegStart	-= SIZEOFARRAY( EML_REC.Frame.aBuffer );
			iNegFinish	-= SIZEOFARRAY( EML_REC.Frame.aBuffer );
		}

		RecFrame.bResult = true;
	} while( 0 );
	EML_REC.Frame.aBuffer[0] = EML_REC.TreshPeakNeg;
	EML_REC.Frame.aBuffer[1] = EML_REC.TreshPeakPos;

	// ��������� �������� ���� (� ��������� ������������� ���������) � �����
	( ( ADC_EML_Frame_t * ) aEML_REC_Archive )[iEML_REC_Archive] = EML_REC.Frame;
	iEML_REC_Archive = ( iEML_REC_Archive + 1 ) % ( SIZEOFARRAY( aEML_REC_Archive ) / SIZEOFARRAY( EML_REC.Frame.aBuffer ) );

	if( NULL != pResult )
		*pResult = RecFrame;
	return;
}

