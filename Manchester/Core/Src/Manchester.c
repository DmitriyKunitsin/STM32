#include  "../Inc/Manchester.h"

uint8_t getCurrentValueByte() {
	Byte *byte = getCurrentStructByte();
	uint8_t currentByte = *((char*) &byte);
	return currentByte;
}

Byte* getCurrentStructByte() {
	static Byte currentByte;
	return &currentByte;
}

void readByteFromPort() {
	Byte *byte = getCurrentStructByte();
	byte->bit0_DA00 = HAL_GPIO_ReadPin(GPIOA, DA0_Pin);
	byte->bit1_DA01 = HAL_GPIO_ReadPin(GPIOA, DA1_Pin);
	byte->bit2_DA02 = HAL_GPIO_ReadPin(GPIOA, DA2_Pin);
	byte->bit3_DA03 = HAL_GPIO_ReadPin(GPIOA, DA3_Pin);
	byte->bit4_DA04 = HAL_GPIO_ReadPin(GPIOA, DA4_Pin);
	byte->bit5_DA05 = HAL_GPIO_ReadPin(GPIOA, DA5_Pin);
	byte->bit6_DA06 = HAL_GPIO_ReadPin(GPIOA, DA6_Pin);
	byte->bit7_DA07 = HAL_GPIO_ReadPin(GPIOA, DA7_Pin);
}

void WriteByteFromStructure(uint8_t valueFromUart) {
	Byte *byte = getCurrentStructByte();
	byte->bit0_DA00 = (valueFromUart & 0x01) ? 1 : 0;
	byte->bit1_DA01 = (valueFromUart & 0x02) ? 1 : 0;
	byte->bit2_DA02 = (valueFromUart & 0x04) ? 1 : 0;
	byte->bit3_DA03 = (valueFromUart & 0x08) ? 1 : 0;
	byte->bit4_DA04 = (valueFromUart & 0x10) ? 1 : 0;
	byte->bit5_DA05 = (valueFromUart & 0x20) ? 1 : 0;
	byte->bit6_DA06 = (valueFromUart & 0x40) ? 1 : 0;
	byte->bit7_DA07 = (valueFromUart & 0x80) ? 1 : 0;

}

void WritePortsFromStructure() {
	Byte *byte = getCurrentStructByte();
	HAL_GPIO_WritePin(GPIOA, DA0_Pin, byte->bit0_DA00);
	HAL_GPIO_WritePin(GPIOA, DA1_Pin, byte->bit1_DA01);
	HAL_GPIO_WritePin(GPIOA, DA2_Pin, byte->bit2_DA02);
	HAL_GPIO_WritePin(GPIOA, DA3_Pin, byte->bit3_DA03);
	HAL_GPIO_WritePin(GPIOA, DA4_Pin, byte->bit4_DA04);
	HAL_GPIO_WritePin(GPIOA, DA5_Pin, byte->bit5_DA05);
	HAL_GPIO_WritePin(GPIOA, DA6_Pin, byte->bit6_DA06);
	HAL_GPIO_WritePin(GPIOA, DA7_Pin, byte->bit7_DA07);
}

