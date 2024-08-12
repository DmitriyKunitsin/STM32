// stm32xxxx_hal_tim.h
// Выбор библиотеки STM32 HAL, в зависимости от целевой платформы
#ifndef	STM32XXXX_HAL_TIM_H
#define	STM32XXXX_HAL_TIM_H

#if		defined( STM32F3 )
#include "stm32f3xx_hal_tim.h"
#elif	defined( STM32F4 )
#include "stm32f4xx_hal_tim.h"
#elif	defined( STM32L4 )
#include "stm32l4xx_hal_tim.h"
#else
#error "Select Target STM32 Family!"
#endif

#endif	// STM32XXXX_HAL_TIM_H