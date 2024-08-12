// Common_RTC.h
// ������� ��� ������ � ������ ��������� �������
#ifndef	COMMON_RTC_H
#define	COMMON_RTC_H

#include "SKLP_Time.h"		// SKLP_Time_t

// ������� ����������, � ������� ����������� ����������� ����� ������ ��� ��������� ������ �������,
// �.�. RTC ����� ������������� ������ � ��������� �� 1 ���.
// ���������� ������ ���� ��������� � ��������-��������� ����� ���, ����� �� ������������ ��� ������������.
extern TimeSubs_t * const pCommonRTC_SubsCorrection;

// ��������� �������
bool CommonRTC_Init( bool *pbCalendarInitialized );			// ������������� ������ RTC
bool CommonRTC_SetSKLP( SKLP_Time_t NewTime );				// ���������� ���� �� ������� ������� ���
SKLP_Time_t CommonRTC_GetSKLP( void );						// ������� ���� �� ������� ������� ���
TimeExt_t CommonRTC_GetTimeExt( void );					// ������� ���� �� ������� ������� TimeExt_t
int32_t CommonRTC_GetCalibration( int16_t *pCalibr_ppm );	// ������� ������� ����������
int32_t CommonRTC_SetCalibration( int16_t Calibr_ppm );		// ���������� ����� ����������
void CommonRTC_AutoCalibr( bool bReset );

#endif	// COMMON_RTC_H

