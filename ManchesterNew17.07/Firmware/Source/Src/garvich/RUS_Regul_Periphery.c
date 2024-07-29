// SADC_Periphery.c
// ������������� � ������ ����������� �������, ���������� �� ��������� ����������������
#include "ProjectConfig.h"			// ������ ���������
#include "stm32xxxx_hal.h"			// ����� ���������
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
	// ������ SPI
	SPI_HandleTypeDef	SPI_ExtDAC_Comp_hdl;	// ���� ������ - ��� AD5320B, ��������� ���������� ����� ��������� �����
	SPI_HandleTypeDef	SPI_ExtDAC_Vref_hdl;	// ���� ������ - ��� AD5320B, ��������� ���������� VDDA/Vref uC
	SPI_HandleTypeDef	SPI_ExtTemp_hdl;		// ���� ������ - LM70, ������ ����������� �����������

	// �������-��������
	TIM_HandleTypeDef	*xTIM_GammaCnt_hdl;		// ������� ��� ����������� ������ ����� � ��������� �����
} SADC_Periph_t;


