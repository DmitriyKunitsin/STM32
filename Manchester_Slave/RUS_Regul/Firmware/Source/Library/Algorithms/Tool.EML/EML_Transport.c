// EML_Transport.c
// Транспортный уровень ЭМКС:
// - декодирование принятых интервалов в тетрады;
// - контроль начала и завершения приема пакета;
// - контроль целостности принятого пакета;
// - подстройка декодирования под актуальный сигнал;
// - формирование пакета импульсов на передачу по требуемому набору отправляемых тетрад;
#include "ProjectConfig.h"		// конфиг платформы
#include "stm32xxxx_hal.h"		// дрова периферии
#include "EML_DataLink.h"		// EML_SendPulses()
#include "EML_Transport.h"		// родной
#include "Platform_common.h"	// !!ADC_LIMIT_MIN()
#include "EML_Config.h"
#include "EML_Periphery.h"		// EML_PGA_Command()
#include <string.h>
#include "Utils.h"				// CalcCRC8SKLP_Load()

// Определение размера буфера импульсов на передачу, если не определено в конфиге проекта
#ifndef	EML_SEND_BUFFER
#define	EML_SEND_BUFFER		100
#endif	// EML_SEND_BUFFER

// Буфер импульсов на передачу
static __no_init EML_TxTick_t aEML_SendBuffer[EML_SEND_BUFFER];

// Перевод кода тетрады в интервал DPPM
EML_TxTick_t EML_Code2DPPMTxTicks( EML_CodeValue_t Code )
{
	float DPPM_Time = 0;
	if( EML_CODE_MARK_START == Code )
		DPPM_Time = EML_DPPM_TIME_START_s;
	else if( IS_EML_CODE_VALUE( Code ) )
		DPPM_Time = EML_DPPM_TIME_START_s + ( 1 + Code ) * EML_DPPM_TIME_INCREMENT_s;
	return EML_DPPM_TIME2TXTICK( DPPM_Time );
}

// Перевод интервала DPPM в код тетрады
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

/*/ Расчет контрольной суммы
// Пока тупо сумма тетрад
EML_CodeValue_t EML_CalcControlSumm( EML_CodeValue_t Summ, EML_CodeValue_t Code )
{
	return ( Summ + Code ) % ( EML_CODE_VALUE_MAX + 1 );
}*/

// Расчет контрольной суммы по CRC8
uint8_t EML_CalcControlSumm( uint8_t CRC8, EML_CodeValue_t Code )
{
	return CalcCRC8SKLP_Load( &Code, sizeof( Code ), CRC8 );
}


// Кодирование набора тетрад в пакет и передача в ЭМКС
// pCodes		массив тетрад
// CodesCount	количество тетрад в массиве
// return		успех/неуспех
// Кодирует тетрады длительностЯми между импульсами (DPPM),
// добавлЯет преамбулу и контрольную сумму.
bool EML_SendPacket( EML_CodeValue_t *pCodes, uint32_t CodesCount )
{
	bool Result = false;
	do
	{
		if( ( NULL == pCodes ) || ( 0 == CodesCount ) )
			break;
		// КонтрольнаЯ сумма
		uint8_t ControlSumm = 0;
		int i = 0;
		// Первый импульс - преамбула, от которого будет рассчитана длительность до первого значащего импульса
		aEML_SendBuffer[i++] = 0;
		// Второй импульс - 'S'
		aEML_SendBuffer[i++] = EML_Code2DPPMTxTicks( EML_CODE_MARK_START );
		int PreambulaImpulsesCount = i;
		do
		{	// Контроль размера пакета
			if( i >= SIZEOFARRAY( aEML_SendBuffer ) )
				break;
			// Расчет контрольной суммы 
			EML_CodeValue_t CodeValue = pCodes[ i - PreambulaImpulsesCount ];
			ControlSumm = EML_CalcControlSumm( ControlSumm, CodeValue );
			// Кодирование тетрады в тики таймера передатчика
			EML_TxTick_t TxTicks = EML_Code2DPPMTxTicks( CodeValue ); 
			if( 0 == TxTicks )
				break;		// кривой код, не удалось закодировать
			// Добавить в буфер длительность перед очередным импульсом	
			aEML_SendBuffer[i++] = TxTicks;
		} while( --CodesCount );
		if( 0 != CodesCount )
			break;		// не удалось раскодировать всю посылку
		if( i >= SIZEOFARRAY( aEML_SendBuffer ) )
			break;
		// Добавить в буфер дополнительный импульс - контрольную сумму
		aEML_SendBuffer[i++] = EML_Code2DPPMTxTicks( ControlSumm & EML_CODE_VALUE_MASK );	// !! отправить только младшую тетраду от CRC8
		// Передать сформированный буфер в передатчик
		uint32_t PulsesCount = i;
		if( PulsesCount != EML_SendFrame( aEML_SendBuffer, PulsesCount ) )
			break;		// в результате работы буфер передан не полностью
		Result = true;
	} while( 0 );

	return Result;
}

// Определить допустимость принЯтого импульса
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

// СостоЯние автомата выделениЯ кодов из интервалов между импульсами
typedef enum EML_ReceivePacketState_enum
{
	EML_ReceivePacketState_Wait0,
	EML_ReceivePacketState_WaitS,
	EML_ReceivePacketState_WaitNext,
	EML_ReceivePacketState_WaitPause,
	EML_ReceivePacketState_Received,
} EML_ReceivePacketState_t;

// Принимать импульсы в течении заданного времени, попытатьсЯ выделить из них пакет.
// - в течении отведенного времени производить оцифровку и получать кадры;
// - из кадров выделЯть импульсы до наступлениЯ таймаута или завершениЯ отведенного времни;
// - из выделенных импульсов выделЯть 'S', а затем и весь пакет;
// - завершить по таймауту или при завершении приема пакета.
// pCodes				- массив длЯ возврата выделенного пакета;
// pCodesCount		- размер массива, на который указывает pCodes.
//						при завершении сюда возвращаетсЯ количество выделенных кодов;
// Time_s				- интервал времени, в течении которого производить оцифровку и выделение пакета.
// pRecFramesSummary	- результат обработки всех принЯтых кадров
void EML_ReceivePacket( EML_CodeValue_t *pCodes, uint32_t *pCodesCount, float Time_s, EML_RecFrameResult_t *pRecFramesSummary )
{
	uint32_t FramesCount = ( uint32_t ) ( Time_s / EML_RX_FRAME_TIME_s );	// максимальное количество принимаемых фреймов
	static __no_init EML_RecFrameResult_t RecFramesSummary;
	RecFramesSummary = ( EML_RecFrameResult_t ) { 0 };
	RecFramesSummary.PGA_Gain	= EML_PGA_CommandExt( PGA_Command_GetCurrent, &RecFramesSummary.iADC_PGA );
	RecFramesSummary.Minimum	= ADC_EML_MAX;
	RecFramesSummary.Maximum	= ADC_EML_MIN;
	RecFramesSummary.MinimumFilt= ADC_EML_MAX;
	RecFramesSummary.MaximumFilt= ADC_EML_MIN;
	uint32_t AvgSumm = 0, AvgCnt = 0;											// переменные длЯ расчета среднего значениЯ по всем кадрам
	EML_ReceivePacketState_t ReceivePacketState = EML_ReceivePacketState_Wait0;	// состоЯние автомата выделениЯ кодов из импульсов
	EML_PulseRx_t PulseLast = ( EML_PulseRx_t ) { 0 };							// последний обработанный импульс
	assert_param( NULL != pCodesCount );
	uint32_t CodesCountMax = *pCodesCount;
	*pCodesCount = 0;
	uint8_t CheckSumm;															// контрольнаЯ сумма, рассчитаннаЯ по дешифрированным кодам
	for( uint32_t i = 0; i < FramesCount; i++ )
	{
		// ПринЯть очередной фрейм, в т.ч. неудачный, и выделить из него импульсы
		static __no_init EML_RecFrameResult_t RecFrameResult;
		RecFrameResult = ( EML_RecFrameResult_t ) { 0 };
		EML_PulseRx_t aRecPulsesTmp[EML_PULSES_PER_RXFRAME_MAX];
		EML_ReceiveFrame( &RecFrameResult, aRecPulsesTmp, SIZEOFARRAY( aRecPulsesTmp ) );
		if( !RecFrameResult.bResult )
		{	// При получении фрейма возникли ошибки, начать выделение пакета заново
			ReceivePacketState = EML_ReceivePacketState_Wait0;
			continue;
		}
		// ПринЯть отметку времени из первого обработанного кадра
		if( 0 == RecFramesSummary.Timestamp )
			RecFramesSummary.Timestamp = RecFrameResult.Timestamp;
		// Проверить, что все выделенные импульсы вместились в предоставленный буфер
		assert_param( RecFrameResult.EventsCount <= SIZEOFARRAY( aRecPulsesTmp ) );
		// Проверить, что в результате обработки принЯтого кадра была выделена амплитуда синхроимпульсов
		if( 0 != RecFrameResult.PulseAmpl )
			RecFramesSummary.PulseAmpl = RecFrameResult.PulseAmpl;		// сохранить принЯтую амплитуду
		// Обновить статистику по принЯтым фреймам
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
		// Проверить на паузу после принЯтиЯ контрольной суммы (индикатор конца пакета)
		if( EML_ReceivePacketState_WaitPause == ReceivePacketState )
			if( 0 == RecFrameResult.EventsCount )
				if( EML_CODE_MARK_PAUSE == EML_DPPMRxTicks2Code( RecFrameResult.Timestamp - PulseLast.Timestamp ) )
				{	// Получена пауза после принЯтиЯ валидной контрольной суммы
					assert_param( *pCodesCount > 1 );
					// Убрать из числа принЯтых кодов контрольную сумму
					( *pCodesCount )--;
					// Завершить прием пакета
					ReceivePacketState = EML_ReceivePacketState_Received;
					RecFrameResult.bResult = true;
					break;
				}
		// Разобрать выделенные импульсы
		for( uint32_t j = 0; j < RecFrameResult.EventsCount; j++ )
		{
			EML_PulseRx_t PulseCurrent = aRecPulsesTmp[j];
			EML_CodeValue_t CodeValue;
			if( !EML_PulseValidate( &PulseCurrent ) )
				continue;	// битые импульсы пропускать, не начинать прием посылки сначала
			// Разобрать состоЯние автомата расшифровки
			switch( ReceivePacketState )
			{
			case EML_ReceivePacketState_Wait0:		// ожидаетсЯ прием первого импульса
				// ПринЯть и перейти к ожиданию 'S'
				ReceivePacketState = EML_ReceivePacketState_WaitS;
				*pCodesCount = 0;
				CheckSumm = 0;
				break;

			case EML_ReceivePacketState_WaitS:		// ожидаетсЯ прием синхросигнала 'S'
				// Проверить на соответствие
				CodeValue = EML_DPPMRxTicks2Code( PulseCurrent.Timestamp - PulseLast.Timestamp );
				if( EML_CODE_MARK_START == CodeValue )
				{	// ПринЯт синхросигнал 'S'. Продолжить выделение посылки
					ReceivePacketState = EML_ReceivePacketState_WaitNext;
				}
				else
				{	// ПринЯт какой-то другой код. Продолжить ожидание 'S'
					ReceivePacketState = EML_ReceivePacketState_WaitS;
				}
				*pCodesCount = 0;
				CheckSumm = 0;
				break;
				
			case EML_ReceivePacketState_WaitNext:	// ожидаетсЯ прием очередного кода после 'S'
			case EML_ReceivePacketState_WaitPause:	// ожидаетсЯ прием паузы после того, как сошлась контрольнаЯ сумма
				CodeValue = EML_DPPMRxTicks2Code( PulseCurrent.Timestamp - PulseLast.Timestamp );
				if( IS_EML_CODE_VALUE( CodeValue ) )
				{	// ПринЯт нормальный код, добавить его в приемный буфер
					if( *pCodesCount < CodesCountMax )
						pCodes[*pCodesCount] = CodeValue;
					// Проверить совпадение контрольной суммы, проверЯть только младшую тетраду CRC8
					if( ( CodeValue == ( CheckSumm & EML_CODE_VALUE_MASK ) ) && ( *pCodesCount > 0 ) )
						ReceivePacketState = EML_ReceivePacketState_WaitPause;		// совпала контрольнаЯ сумма, возможно это конец пакета, ожидать паузу или еще один импульс
					else
						ReceivePacketState = EML_ReceivePacketState_WaitNext;		// контрольнаЯ сумма не совпала, продолжать выделЯть импульсы
					CheckSumm = EML_CalcControlSumm( CheckSumm, CodeValue );
					( *pCodesCount )++;
//if( *pCodesCount > 26 )
//	AvgCnt++;
				}
				else if( EML_CODE_MARK_START == CodeValue )
				{	// Снова принЯт 'S', ожидать прием посылки сначала
					ReceivePacketState = EML_ReceivePacketState_WaitNext;
					*pCodesCount = 0;
					CheckSumm = 0;
				}
				else
				{	// ПринЯт невеный код, начать выделЯть посылку сначала
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
	
	// Автоподстройка усилителЯ
	ADC_EML_Data_t SignalGaugeMax = RecFramesSummary.Maximum - RecFramesSummary.Minimum;
	if( SignalGaugeMax < 1600 )
		EML_PGA_Command( PGA_Command_Inc );
	else if( SignalGaugeMax > 3600 )
		EML_PGA_Command( PGA_Command_Dec );
	// !!! Важно! В начале этой функции EML_ReceivePacket()
	// !!! происходит заполнение полей RecFramesSummary.PGA_Gain и RecFramesSummary.iADC_PGA
	// !!! величиной и индексом PGA, который будет применен при попытке выделениЯ этого пакета.
	// !!! В результате выделениЯ, при слишком слабом или слишком сильном сигналах
	// !!! производитсЯ автоподстройка PGA. Переключение PGA производитсЯ сразу же.
	// !!! В проекте приемопередатчика НДМ, новый индекс PGA должен быть передан в МПИ,
	// !!! чтобы после сброса питаниЯ принЯть его от МПИ обратно и сразу же применить.
	// !!! Поэтому далее происходит замена RecFramesSummary.iADC_PGA на требуемый новый, который будет передан в МПИ,
	// !!! причем реальный коэффициент усилениЯ в этом цикле RecFramesSummary.PGA_Gain сохранЯетсЯ.
	EML_PGA_CommandExt( PGA_Command_GetCurrent, &RecFramesSummary.iADC_PGA );

	// Вернуть результат
	if( NULL != pRecFramesSummary )
		*pRecFramesSummary = RecFramesSummary;
}

