#ifndef MANCHESTER_H
#define MANCHESTER_H

#include "./main.h"

#define AINT_RX_OK while (HAL_GPIO_ReadPin(GPIOA, AINTRX_Pin) != GPIO_PIN_RESET) {}

#define START_READ_MODE HAL_GPIO_WritePin(GPIOB, AMODE_Pin, GPIO_PIN_SET);

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
Byte* getCurrentStructByte();
/*Возвращает конкретное значение в uint8_t, что лежит в структуре*/
uint8_t getCurrentValueByte();
/*Записывает в структуру значения каждого порта в каждый бит*/
void readByteFromPort();
/*Записывает пришедший байт в структуру*/
void WriteByteFromStructure(uint8_t valueFromUart);
/*В зависимости от байта в структуре записывает это в порты*/
void WritePortsFromStructure();
/*Отправляет два байта на кабель*/
void pushDatesToPort(uint8_t oneByte, uint8_t secondDate);
/*Читает два байта информации*/
uint16_t readDatesToPorts();

void changeConfigureForRead(void);
void changeConfigureForWrite(void);
#endif
