// EML_Transport.h
// Транспортный уровень ЭМКС:
// - декодирование принятых интервалов в тетрады;
// - контроль начала и завершения приема пакета;
// - контроль целостности принятого пакета;
// - подстройка декодирования под актуальный сигнал;
// - формирование пакета импульсов на передачу по требуемому набору отправляемых тетрад;
#ifndef	EML_TRANSPORT_H
#define	EML_TRANSPORT_H

#include "EML_Config.h"		// EML_TxTick_t
#include "EML_DataLink.h"	// EML_RecFrameResult_t

// Кодирование произволных чисел шестнадцатиричными тетрадами
typedef uint8_t EML_CodeValue_t;
#define	EML_CODE_VALUE_MIN		( ( EML_CodeValue_t ) 0x00 )		// код 0
#define	EML_CODE_VALUE_MAX		( ( EML_CodeValue_t ) 0x0F )		// код 15
#define	EML_CODE_VALUE_MASK		( ( EML_CodeValue_t ) 0x0F )		// маска
#define	EML_CODE_MARK_START		( ( EML_CodeValue_t ) 0xFF )		// код 'S', "Start"
#define	EML_CODE_MARK_PAUSE		( ( EML_CodeValue_t ) 0x1F )		// индикатор паузы после последнего импульса
#define	EML_CODE_MARK_SYNC		( ( EML_CodeValue_t ) 0x20 )		// индикатор интервала, соответствующего последовательным импульсам в составе синхропосылки
#define	EML_CODE_MARK_ERR_SHORT	( ( EML_CodeValue_t ) 0xFD )		// код слишком короткого интервала
#define	EML_CODE_MARK_ERR_LONG	( ( EML_CodeValue_t ) 0xFE )		// код слишком длинного интервала
//#define	IS_EML_CODE_VALUE( __CODE__ )	( ( ( __CODE__ ) >= EML_CODE_VALUE_MIN ) && ( ( __CODE__ ) <= EML_CODE_VALUE_MAX ) )
inline bool IS_EML_CODE_VALUE( EML_CodeValue_t __CODE__ )
{
#pragma	diag_suppress=Pe186		// Warning[Pe186]: pointless comparison of unsigned integer with zero
	return ( ( __CODE__ ) >= EML_CODE_VALUE_MIN ) && ( ( __CODE__ ) <= EML_CODE_VALUE_MAX );
#pragma	diag_default=Pe186
}

// Перевод кода тетрады в интервал DPPM
EML_TxTick_t EML_Code2DPPMTxTicks( EML_CodeValue_t Code );
// Перевод интервала DPPM в код тетрады
EML_CodeValue_t EML_DPPMRxTicks2Code( EML_RxTick_t Ticks );

// Расчет контрольной суммы
//EML_CodeValue_t EML_CalcControlSumm( EML_CodeValue_t Summ, EML_CodeValue_t Code );
uint8_t EML_CalcControlSumm( uint8_t CRC8, EML_CodeValue_t Code );

// Кодирование набора тетрад в пакет и передача в ЭМКС
bool EML_SendPacket( EML_CodeValue_t *pCodes, uint32_t CodesCount );	
// Принимать импульсы в течении заданного времени, попытатьсЯ выделить из них пакет
void EML_ReceivePacket( EML_CodeValue_t *pCodes, uint32_t *pCodesCount, float Time_s, EML_RecFrameResult_t *pRecFramesSummary );

#endif	// EML_TRANSPORT_H

