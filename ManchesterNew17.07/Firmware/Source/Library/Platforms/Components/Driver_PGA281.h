// Driver_PGA281.h
// ƒрайвер PGA281, инструментального усилителя с программируемым усилением 0.125-128
#ifndef	DRIVER_PGA281_H
#define	DRIVER_PGA281_H

#include "Driver_PGA.h"
#include "ProjectConfig.h"		// конфиг платформы
#include <stdbool.h>

typedef struct PGA281_Hdl_struct
{
	int					iGain;			// [0..21]	
	GPIO_CommonIndex_t	aiGPIO_G[5];	// !!массив в порядке возрастания веса пина, т.е. начинается с G4!
} PGA281_Hdl_t;

// »нициализация - привязка пинов параллельного управления. iG в соответсвии с обозначением на PGA281
bool PGA281_Init( PGA281_Hdl_t *pPGA_Hdl, GPIO_CommonIndex_t iG0, GPIO_CommonIndex_t iG1, GPIO_CommonIndex_t iG2, GPIO_CommonIndex_t iG3, GPIO_CommonIndex_t iG4 );
// ¬ыполнение типовых команд
PGA_Gain_t PGA281_Command( PGA281_Hdl_t *pPGA_Hdl, PGA_Command_t Command );

#endif	// DRIVER_PGA281_H
