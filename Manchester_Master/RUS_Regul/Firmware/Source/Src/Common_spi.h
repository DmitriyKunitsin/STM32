// Платформо-зависимый инициализатор драйвера SPI
// Для инициализации канала SPI в заголовках проекта и/или платформы необходимо определить:
//	#define USE_SPI#						использовать SPI#
#ifndef	COMMON_SPI_H
#define	COMMON_SPI_H

#include "ProjectConfig.h"			//
#include "stm32xxxx_hal.h"			//
#include "stm32xxxx_hal_spi.h"                  // драйвер TIM

//#include <stdbool.h>
//#include <stdint.h>


// ***************** Описание используемых в проекте хендлеров SPI **************
#if 1
#if	defined( USE_SPI1 )
	extern SPI_HandleTypeDef SPI1_hdl;
	extern void SPI1_MspInit(void);
	extern void SPI1_MspDeInit(void);
	extern SPI_HandleTypeDef* getSPI1Handle(void);
	extern void SPI1_TxRxCpltCallback(SPI_HandleTypeDef* phspi);
#endif	// USE_SPI1

#if	defined( USE_SPI2 )
	extern SPI_HandleTypeDef SPI2_hdl;
	extern void SPI2_MspInit(void);
	extern void SPI2_MspDeInit(void);
	extern SPI_HandleTypeDef* getSPI2Handle(void);
	extern void SPI2_TxRxCpltCallback(SPI_HandleTypeDef* phspi);
#endif	// USE_SPI2

#if	defined( USE_SPI3 )
	extern SPI_HandleTypeDef SPI3_hdl;
	extern void SPI3_MspInit(void);
	extern void SPI3_MspDeInit(void);
	extern SPI_HandleTypeDef* getSPI3Handle(void);
	extern void SPI3_TxRxCpltCallback(SPI_HandleTypeDef* phspi);
#endif	// USE_SPI3
#endif

#endif	// COMMON_SPI_H

