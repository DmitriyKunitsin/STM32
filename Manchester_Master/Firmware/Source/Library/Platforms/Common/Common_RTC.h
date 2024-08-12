// Common_RTC.h
// ”тилиты для работы с часами реального времени
#ifndef	COMMON_RTC_H
#define	COMMON_RTC_H

#include "SKLP_Time.h"		// SKLP_Time_t

// ¬нешняя переменная, в которую сохраняется компенсация долей секунд при установке нового времени,
// т.к. RTC можно устанавливать только с точностью до 1 сек.
// ѕеременная должна быть размещена в проектно-зависимой части так, чтобы не повреждаться при перезагрузке.
extern TimeSubs_t * const pCommonRTC_SubsCorrection;

// ѕрототипы функций
bool CommonRTC_Init( bool *pbCalendarInitialized );			// »нициализация иапуск RTC
bool CommonRTC_SetSKLP( SKLP_Time_t NewTime );				// ”становить часы по формату времени — Ћ
SKLP_Time_t CommonRTC_GetSKLP( void );						// ¬ернуть часы по формату времени — Ћ
TimeExt_t CommonRTC_GetTimeExt( void );					// ¬ернуть часы по формату времени TimeExt_t
int32_t CommonRTC_GetCalibration( int16_t *pCalibr_ppm );	// ¬ернуть текущую калибровку
int32_t CommonRTC_SetCalibration( int16_t Calibr_ppm );		// ”становить новую калибровку
void CommonRTC_AutoCalibr( bool bReset );

#endif	// COMMON_RTC_H

