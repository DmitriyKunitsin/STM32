// Платформо-зависимый инициализатор драйвера DAC
// Для инициализации канала DAC в заголовках проекта и/или платформы необходимо определить:
//	#define USE_DAC_CHANNEL_#						использовать DAC#
#ifndef	COMMON_DAC_H
#define	COMMON_DAC_H

#include "stm32xxxx_hal_dac.h"		// драйвер DAC
#include <stdbool.h>
#include <stdint.h>



// ***************** Описание используемых в проекте хендлеров DAC **************
// DAC#_hdl	- хендлер

#if	defined( USE_DAC_CHANNEL_1 )
	extern DAC_HandleTypeDef DAC1_hdl;
#endif	// USE_DAC_CHANNEL_1

#if	defined( USE_DAC_CHANNEL_2 )
	extern DAC_HandleTypeDef DAC2_hdl;
#endif	// USE_DAC_CHANNEL_2


#endif	// COMMON_DAC_H

