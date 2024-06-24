#ifndef MANCHESTER_H
#define MANCHESTER_H

#include "./main.h"

typedef struct {
	uint8_t bit0_DA00 :1;
	uint8_t bit1_DA01 :1;
	uint8_t bit2_DA02 :1;
	uint8_t bit3_DA03 :1;
	uint8_t bit4_DA04 :1;
	uint8_t bit5_DA05 :1;
	uint8_t bit6_DA06 :1;
	uint8_t bit7_DA07 :1;
} Byte;

/*Возвращает указатель на текущую структуру с текущими значениями*/
Byte *getCurrentStructByte();
/*Возвращает конкретное значение в uint8_t, что лежит в структуре*/
uint8_t getCurrentValueByte();
/*Записывает в структуру значения каждого порта в каждый бит*/
void readByteFromPort();
/*Записывает пришедший байт в структуру*/
void WriteByteFromStructure(uint8_t valueFromUart);
/*В зависимости от байта в структуре записывает это в порты*/
void WritePortsFromStructure();




#endif
