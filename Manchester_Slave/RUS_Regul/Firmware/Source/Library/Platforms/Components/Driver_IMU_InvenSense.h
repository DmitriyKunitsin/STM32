// Driver_IMU_InvenSense.h
// Драйвер 9-осевых датчиков движениЯ (Inertial measurement unit) производства TDK-InvenSense:
// - сборка MPU-9250		(3xAccelerometer, 3xGyroscope and AK8963 3xMagnetometer)
// - сборка ICM-20948	(3xAccelerometer, 3xGyroscope and AK09916 3xMagnetometer)
// Обмен через последовательный интерфейс I2C/SPI:
// - InitSerial/Read/Write
// - Poll/IT/DMA
// - IRQ
#ifndef	DRIVER_IMU_INVENSENSE_H
#define	DRIVER_IMU_INVENSENSE_H

#include <stdint.h>
#include <stdbool.h>
#include "Common_gpio.h"

// АдрессациЯ IMU на шине
typedef union IMU_Address_union
{
	uint16_t			I2C;	// I2C Device Address
	GPIO_CommonIndex_t	SPI;	// SPI Chip Select
} IMU_Address_t;

// *****************************************
// Прототипы базовых функций обмена с IMU через последовательный интерфейс I2C/SPI
// *****************************************
// ИнициализациЯ последовательного интерфейса
HAL_StatusTypeDef IMU_SerialInit( void );
// Прочитать один байт из IMU
HAL_StatusTypeDef IMU_ReadByte( IMU_Address_t DevAddress, uint8_t Address, uint8_t *pByte );
// Прочитать буфер из IMU
HAL_StatusTypeDef IMU_ReadBuffer( IMU_Address_t DevAddress, uint8_t Address, uint8_t *pBuffer, uint16_t BufferSize );
// Записать байт в IMU
HAL_StatusTypeDef IMU_WriteByte( IMU_Address_t DevAddress, uint8_t Address, uint8_t Byte );
// Записать буфер в IMU
HAL_StatusTypeDef IMU_WriteBuffer( IMU_Address_t DevAddress, void *pBuffer, uint16_t BufferSize );

#endif	// DRIVER_IMU_INVENSENSE_H
