// ���������-��������� ������������� �������� DAC
// ��� ������������� ������ DAC � ���������� ������� �/��� ��������� ���������� ����������:
//	#define USE_DAC_CHANNEL_#						������������ DAC#
#ifndef	COMMON_DAC_H
#define	COMMON_DAC_H

#include "stm32xxxx_hal_dac.h"		// ������� DAC
#include <stdbool.h>
#include <stdint.h>



// ***************** �������� ������������ � ������� ��������� DAC **************
// DAC#_hdl	- �������

#if	defined( USE_DAC_CHANNEL_1 )
	extern DAC_HandleTypeDef DAC1_hdl;
#endif	// USE_DAC_CHANNEL_1

#if	defined( USE_DAC_CHANNEL_2 )
	extern DAC_HandleTypeDef DAC2_hdl;
#endif	// USE_DAC_CHANNEL_2


#endif	// COMMON_DAC_H

