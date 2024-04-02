#ifndef LCD_STATIC_H
#define LCD_STATIC_H

#include <stm32f4xx_hal.h>
#include <main.h>

#define SH GPIO_PIN_7
#define SD GPIO_PIN_8
#define SC GPIO_PIN_9
#define SE GPIO_PIN_10
#define SB GPIO_PIN_11
#define SF GPIO_PIN_12
#define SA GPIO_PIN_13
#define SG GPIO_PIN_14

#define SA_RESET HAL_GPIO_WritePin(GPIOE, SA, GPIO_PIN_RESET);
#define SA_SET HAL_GPIO_WritePin(GPIOE, SA, GPIO_PIN_SET);
#define SB_RESET HAL_GPIO_WritePin(GPIOE, SB, GPIO_PIN_RESET);
#define SB_SET HAL_GPIO_WritePin(GPIOE, SB, GPIO_PIN_SET);
#define SC_RESET HAL_GPIO_WritePin(GPIOE, SC, GPIO_PIN_RESET);
#define SC_SET HAL_GPIO_WritePin(GPIOE, SC, GPIO_PIN_SET);
#define SD_RESET HAL_GPIO_WritePin(GPIOE, SD, GPIO_PIN_RESET);
#define SD_SET HAL_GPIO_WritePin(GPIOE, SD, GPIO_PIN_SET);
#define SE_RESET HAL_GPIO_WritePin(GPIOE, SE, GPIO_PIN_RESET);
#define SE_SET HAL_GPIO_WritePin(GPIOE, SE, GPIO_PIN_SET);
#define SF_RESET HAL_GPIO_WritePin(GPIOE, SF, GPIO_PIN_RESET);
#define SF_SET HAL_GPIO_WritePin(GPIOE, SF, GPIO_PIN_SET);
#define SG_RESET HAL_GPIO_WritePin(GPIOE, SG, GPIO_PIN_RESET);
#define SG_SET HAL_GPIO_WritePin(GPIOE, SG, GPIO_PIN_SET);
#define SH_RESET HAL_GPIO_WritePin(GPIOE, SH, GPIO_PIN_RESET);
#define SH_SET HAL_GPIO_WritePin(GPIOE, SH, GPIO_PIN_SET);

#define RESET_ALL() do { \
	SA_RESET; \
	SB_RESET; \
	SC_RESET; \
	SD_RESET; \
	SE_RESET; \
	SF_RESET; \
	SG_RESET; \
	SH_RESET; \
} while(0)

void segchar(uint8_t seg);

#endif /*LCD_STATIC_H*/
