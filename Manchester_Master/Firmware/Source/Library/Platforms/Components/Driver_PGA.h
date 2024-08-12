// Driver_PGA.h
// ќбщий заголовок драйвера ряда управляемых усилителей
#ifndef	DRIVER_PGA_H
#define	DRIVER_PGA_H

//  оманды управления PGA
typedef enum PGA_Command_enum
{
	PGA_Command_Set,			// установить текущее усиление
	PGA_Command_SetMin,			// установить минимальное усиление
	PGA_Command_SetMax,			// установить максимальное усиление
	PGA_Command_Inc,			// увеличить усиление на один пункт
	PGA_Command_Dec,			// уменьшить усиление на один пункт
	PGA_Command_GetCurrent, 	// вернуть текущее усиление
	PGA_Command_GetMin, 		// вернуть минимальное усиление
	PGA_Command_GetMax, 		// вернуть максимальное усиление
} PGA_Command_t;

// “ип данных для определения коэффициента усиления PGA
typedef float PGA_Gain_t;

// “ип данных для хендлера PGA
//typedef void * xPGA_Hdl_t;

#endif	// DRIVER_PGA_H
