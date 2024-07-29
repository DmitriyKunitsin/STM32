// EML_Transport.h
// ������������ ������� ����:
// - ������������� �������� ���������� � �������;
// - �������� ������ � ���������� ������ ������;
// - �������� ����������� ��������� ������;
// - ���������� ������������� ��� ���������� ������;
// - ������������ ������ ��������� �� �������� �� ���������� ������ ������������ ������;
#ifndef	EML_TRANSPORT_H
#define	EML_TRANSPORT_H

#include "EML_Config.h"		// EML_TxTick_t
#include "EML_DataLink.h"	// EML_RecFrameResult_t

// ����������� ����������� ����� ������������������ ���������
typedef uint8_t EML_CodeValue_t;
#define	EML_CODE_VALUE_MIN		( ( EML_CodeValue_t ) 0x00 )		// ��� 0
#define	EML_CODE_VALUE_MAX		( ( EML_CodeValue_t ) 0x0F )		// ��� 15
#define	EML_CODE_VALUE_MASK		( ( EML_CodeValue_t ) 0x0F )		// �����
#define	EML_CODE_MARK_START		( ( EML_CodeValue_t ) 0xFF )		// ��� 'S', "Start"
#define	EML_CODE_MARK_PAUSE		( ( EML_CodeValue_t ) 0x1F )		// ��������� ����� ����� ���������� ��������
#define	EML_CODE_MARK_SYNC		( ( EML_CodeValue_t ) 0x20 )		// ��������� ���������, ���������������� ���������������� ��������� � ������� �������������
#define	EML_CODE_MARK_ERR_SHORT	( ( EML_CodeValue_t ) 0xFD )		// ��� ������� ��������� ���������
#define	EML_CODE_MARK_ERR_LONG	( ( EML_CodeValue_t ) 0xFE )		// ��� ������� �������� ���������
//#define	IS_EML_CODE_VALUE( __CODE__ )	( ( ( __CODE__ ) >= EML_CODE_VALUE_MIN ) && ( ( __CODE__ ) <= EML_CODE_VALUE_MAX ) )
inline bool IS_EML_CODE_VALUE( EML_CodeValue_t __CODE__ )
{
#pragma	diag_suppress=Pe186		// Warning[Pe186]: pointless comparison of unsigned integer with zero
	return ( ( __CODE__ ) >= EML_CODE_VALUE_MIN ) && ( ( __CODE__ ) <= EML_CODE_VALUE_MAX );
#pragma	diag_default=Pe186
}

// ������� ���� ������� � �������� DPPM
EML_TxTick_t EML_Code2DPPMTxTicks( EML_CodeValue_t Code );
// ������� ��������� DPPM � ��� �������
EML_CodeValue_t EML_DPPMRxTicks2Code( EML_RxTick_t Ticks );

// ������ ����������� �����
//EML_CodeValue_t EML_CalcControlSumm( EML_CodeValue_t Summ, EML_CodeValue_t Code );
uint8_t EML_CalcControlSumm( uint8_t CRC8, EML_CodeValue_t Code );

// ����������� ������ ������ � ����� � �������� � ����
bool EML_SendPacket( EML_CodeValue_t *pCodes, uint32_t CodesCount );	
// ��������� �������� � ������� ��������� �������, ���������� �������� �� ��� �����
void EML_ReceivePacket( EML_CodeValue_t *pCodes, uint32_t *pCodesCount, float Time_s, EML_RecFrameResult_t *pRecFramesSummary );

#endif	// EML_TRANSPORT_H

