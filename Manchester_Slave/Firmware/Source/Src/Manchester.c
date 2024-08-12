#include "ProjectConfig.h"		// êîíôèã ïëàòôîðìû
#include "stm32xxxx_hal.h"		// äðîâà ïåðèôåðèè
#include "Manchester.h"



/*Функция возвращает байт информации, что лежит в структуре*/
uint8_t getCurrentValueByte() {
	Byte *byte = getCurrentStructByte();
	uint8_t currentByte = (byte->bit0_DA00 << 0) | (byte->bit1_DA01 << 1)
			| (byte->bit2_DA02 << 2) | (byte->bit3_DA03 << 3)
			| (byte->bit4_DA04 << 4) | (byte->bit5_DA05 << 5)
			| (byte->bit6_DA06 << 6) | (byte->bit7_DA07 << 7);
	return currentByte;
}
/*Функция возвращает указатель на статическую структуру,
 *  ее вызов позволяет в любом месте кода узнать значения в структуре,
 *  что в ней лежат в конкретный момент времени*/
Byte* getCurrentStructByte() {
	static Byte currentByte;
	return &currentByte;
}
/*Функция читает порты 588ВГ6 и записывает значения в структуру*/
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

/*Функция получает один байт и записывает его в структуру по битам*/
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
/*Фнкция записывает значения структуры в порт 588ВГ6*/
void WritePortsFromStructure() {
	Byte *byte = getCurrentStructByte();
	HAL_GPIO_WritePin(GPIOA, DA0_Pin, (byte->bit0_DA00 == 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, DA1_Pin, (byte->bit1_DA01 == 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, DA2_Pin, (byte->bit2_DA02 == 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, DA3_Pin, (byte->bit3_DA03 == 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, DA4_Pin, (byte->bit4_DA04 == 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, DA5_Pin, (byte->bit5_DA05 == 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, DA6_Pin, (byte->bit6_DA06 == 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, DA7_Pin, (byte->bit7_DA07 == 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
/*Функция получается два байта информации и записывает их в паралельный порт 588ВГ6*/
void pushDatesToPort(uint8_t oneByte, uint8_t secondDate) {
	while (HAL_GPIO_ReadPin(GPIOA, AINTTX_Pin) != GPIO_PIN_RESET) {
	}
	WriteByteFromStructure(oneByte);
	WritePortsFromStructure();
	HAL_GPIO_WritePin(GPIOA, AWRL_1byte_Pin, GPIO_PIN_SET);
	while (HAL_GPIO_ReadPin(GPIOA, AINTTX_Pin) != 0) {
	}
	HAL_GPIO_WritePin(GPIOB, AWRL_1byte_Pin, GPIO_PIN_RESET);
	WriteByteFromStructure(secondDate);
	WritePortsFromStructure();
	HAL_GPIO_WritePin(GPIOB, AWRH_2byte_Pin, GPIO_PIN_SET);
	while (HAL_GPIO_ReadPin(GPIOA, AINTTX_Pin) != GPIO_PIN_RESET) {
	}
	HAL_GPIO_WritePin(GPIOB, AWRH_2byte_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, ASD_Pin, GPIO_PIN_SET);
	while (HAL_GPIO_ReadPin(GPIOA, AINTTX_Pin) == GPIO_PIN_SET) {
	}
	HAL_GPIO_WritePin(GPIOB, ASD_Pin, GPIO_PIN_RESET);
	HAL_Delay(1);
}
/*Функция читает два байта информации с 588ВГ6 и возвращает 16 байтное число*/
uint16_t readDatesToPorts() {
	AINT_RX_OK;
	HAL_GPIO_WritePin(GPIOB, ARDL_1byte_Pin, GPIO_PIN_SET);
	readByteFromPort();
	uint8_t firstByte = getCurrentValueByte();
	HAL_GPIO_WritePin(GPIOB, ARDL_1byte_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, ARDH_2byte_Pin, GPIO_PIN_SET);
	readByteFromPort();
	uint8_t secondByte = getCurrentValueByte();
	HAL_GPIO_WritePin(GPIOB, ARDH_2byte_Pin, GPIO_PIN_RESET);

	uint16_t result = 0x0000;
	result = (firstByte << 8) | secondByte;
	return result;
}
/*Функция меняет конфигурацию портов на чтение(INPUT) данных с них, чтобы считывать данные с 588ВГ6*/
void changeConfigureForRead(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	GPIO_InitStruct.Pin = DA0_Pin | DA1_Pin | DA2_Pin | DA3_Pin | DA4_Pin
			| DA5_Pin | DA6_Pin | DA7_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}
/*Функция меняет конфигурацию портов на запись(OUTPUT) данных в них, чтобы передать 588ВГ6 данные*/
void changeConfigureForWrite(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	GPIO_InitStruct.Pin = DA0_Pin | DA1_Pin | DA2_Pin | DA3_Pin | DA4_Pin
			| DA5_Pin | DA6_Pin | DA7_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}
