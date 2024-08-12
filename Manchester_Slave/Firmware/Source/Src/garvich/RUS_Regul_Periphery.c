// SADC_Periphery.c
// »нициализация и работа программных модулей, завязанных на периферию микроконтроллера
#include "ProjectConfig.h"			// конфиг платформы
#include "stm32xxxx_hal.h"			// дрова периферии
#include "platform_common.h"
#include "common_tim.h"
#include "common_gpio.h"
#include "common_rcc.h"
#include "RUS_Regul_Common.h"
#include "RebootUtils.h"
//#include "stm32l4xx_ll_tim.h"		// LL_TIM_OC_EnablePreload()
#include "stm32l4xx_hal_spi.h"
#include "Utils.h"					// Round_UI16()

#ifndef	STM32L433xx
#error "Periphery utilization for STM32L433xx only!"
#endif

typedef struct SADC_Periph_struct
{
	//  аналы SPI
	SPI_HandleTypeDef	SPI_ExtDAC_Comp_hdl;	// один девайс - ÷јѕ AD5320B, установка напряжения опоры детектора гаммы
	SPI_HandleTypeDef	SPI_ExtDAC_Vref_hdl;	// один девайс - ÷јѕ AD5320B, установка напряжения VDDA/Vref uC
	SPI_HandleTypeDef	SPI_ExtTemp_hdl;		// один девайс - LM70, датчик температуры сцинтиблока

	// “аймеры-счетчики
	TIM_HandleTypeDef	*xTIM_GammaCnt_hdl;		// счетчик для регистрации общего счета с детектора гаммы
} SADC_Periph_t;


