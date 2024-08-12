// stm32xxxx_hal.h
// Выбор библиотеки STM32 HAL, в зависимости от целевой платформы
#ifndef	STM32XXXX_HAL_H
#define	STM32XXXX_HAL_H

#if		defined( STM32F3 )
#include "stm32f3xx.h"
#elif	defined( STM32F4 )
#include "stm32f4xx.h"
#elif	defined( STM32L4 )
#include "stm32l4xx.h"
#else
#error "Select Target STM32 Family!"
#endif

#endif	// STM32XXXX_HAL_H
