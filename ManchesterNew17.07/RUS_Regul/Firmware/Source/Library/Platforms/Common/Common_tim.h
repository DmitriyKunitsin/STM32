// Платформо-зависимый инициализатор драйвера TIM
// Для инициализации канала TIM в заголовках проекта и/или платформы необходимо определить:
//	#define TIM#_USE						использовать TIM#
#ifndef	COMMON_TIM_H
#define	COMMON_TIM_H

#include "stm32xxxx_hal_tim.h"		// драйвер TIM
#include <stdbool.h>
#include <stdint.h>

// Таймеры STM32F4xx (* - кроме 401)
// TIM1	16 bit	APB2	Advanced-Control
// TIM2	32 bit	APB1	General Purpose
// TIM3	16 bit	APB1	General Purpose
// TIM4	16 bit	APB1	General Purpose
// TIM5	32 bit	APB1	General Purpose
// TIM6*	16 bit	APB1	Basic
// TIM7*	16 bit	APB1	Basic
// TIM8*	16 bit	APB2	Advanced-Control
// TIM9	16 bit	APB2	General Purpose
// TIM10	16 bit	APB2	General Purpose
// TIM11	16 bit	APB2	General Purpose
// TIM12*	16 bit	APB1	General Purpose
// TIM13*	16 bit	APB1	General Purpose
// TIM14*	16 bit	APB1	General Purpose

// Таймеры STM32L4x5, STM32L4x6
// TIM1	16 bit	APB2	Advanced-Control
// TIM2	32 bit	APB1	General Purpose
// TIM3	16 bit	APB1	General Purpose
// TIM4	16 bit	APB1	General Purpose
// TIM5	32 bit	APB1	General Purpose
// TIM6	16 bit	APB1	Basic
// TIM7	16 bit	APB1	Basic
// TIM8	16 bit	APB2	Advanced-Control
// TIM15	16 bit	APB2	General Purpose
// TIM16	16 bit	APB2	General Purpose
// TIM17	16 bit	APB2	General Purpose

// Таймеры STM32L433
// TIM1	16 bit	APB2	Advanced-Control
// TIM2	32 bit	APB1	General Purpose
// TIM6	16 bit	APB1	Basic
// TIM7	16 bit	APB1	Basic
// TIM15	16 bit	APB2	General Purpose
// TIM16	16 bit	APB2	General Purpose
// LPTIM1	16 bit	APB1	Low Power
// LPTIM2	16 bit	APB1	Low Power

// Таймеры STM32F373
// TIM2	32 bit	APB1	General Purpose
// TIM3	16 bit	APB1	General Purpose
// TIM4	16 bit	APB1	General Purpose
// TIM5	32 bit	APB1	General Purpose
// TIM6	16 bit	APB1	Basic
// TIM7	16 bit	APB1	Basic
// TIM12	16 bit	APB1	General Purpose
// TIM13	16 bit	APB1	General Purpose
// TIM14	16 bit	APB1	General Purpose
// TIM15	16 bit	APB2	General Purpose
// TIM16	16 bit	APB2	General Purpose
// TIM17	16 bit	APB2	General Purpose
// TIM18	16 bit	APB1	Basic
// TIM19	16 bit	APB2	General Purpose

// ***************** Описание используемых в проекте хендлеров TIM **************
// TIM#_hdl	- хендлер

#if	defined( TIM1_USE )
	extern TIM_HandleTypeDef TIM1_hdl;
#endif	// TIM1_USE

#if	defined( TIM2_USE )
	extern TIM_HandleTypeDef TIM2_hdl;
#endif	// TIM2_USE

#if	defined( TIM3_USE )
	extern TIM_HandleTypeDef TIM3_hdl;
#endif	// TIM3_USE

#if	defined( TIM4_USE )
	extern TIM_HandleTypeDef TIM4_hdl;
#endif	// TIM4_USE

#if	defined( TIM5_USE )
	extern TIM_HandleTypeDef TIM5_hdl;
#endif	// TIM5_USE

#if	defined( TIM6_USE )
	extern TIM_HandleTypeDef TIM6_hdl;
#endif	// TIM6_USE

#if	defined( TIM7_USE )
	extern TIM_HandleTypeDef TIM7_hdl;
#endif	// TIM7_USE

#if	defined( TIM8_USE )
	extern TIM_HandleTypeDef TIM8_hdl;
#endif	// TIM8_USE

#if	defined( TIM9_USE )
	extern TIM_HandleTypeDef TIM9_hdl;
#endif	// TIM9_USE

#if	defined( TIM10_USE )
	extern TIM_HandleTypeDef TIM10_hdl;
#endif	// TIM10_USE

#if	defined( TIM11_USE )
	extern TIM_HandleTypeDef TIM11_hdl;
#endif	// TIM11_USE

#if	defined( TIM12_USE )
	extern TIM_HandleTypeDef TIM12_hdl;
#endif	// TIM12_USE

#if	defined( TIM13_USE )
	extern TIM_HandleTypeDef TIM13_hdl;
#endif	// TIM13_USE

#if	defined( TIM14_USE )
	extern TIM_HandleTypeDef TIM14_hdl;
#endif	// TIM14_USE

#if	defined( TIM15_USE )
	extern TIM_HandleTypeDef TIM15_hdl;
#endif	// TIM15_USE

#if	defined( TIM16_USE )
	extern TIM_HandleTypeDef TIM16_hdl;
#endif	// TIM16_USE

#if	defined( TIM17_USE )
	extern TIM_HandleTypeDef TIM17_hdl;
#endif	// TIM17_USE

#if	defined( TIM18_USE )
	extern TIM_HandleTypeDef TIM18_hdl;
#endif	// TIM18_USE

#if	defined( TIM19_USE )
	extern TIM_HandleTypeDef TIM19_hdl;
#endif	// TIM19_USE


// Получить входную периферийную частоту таймера
uint32_t TIM_Common_GetFrq( TIM_TypeDef *pTIM_Instance );

// Расчет предделителей таймера
bool TIM_Common_SetupPrescalers( TIM_HandleTypeDef *pTIM, uint32_t Period_us, uint32_t ClockDivision );


#endif	// COMMON_TIM_H

