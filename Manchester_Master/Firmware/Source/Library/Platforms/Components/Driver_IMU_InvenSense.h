// Driver_IMU_InvenSense.h
// ������� 9-������ �������� �������� (Inertial measurement unit) ������������ TDK-InvenSense:
// - ������ MPU-9250		(3xAccelerometer, 3xGyroscope and AK8963 3xMagnetometer)
// - ������ ICM-20948	(3xAccelerometer, 3xGyroscope and AK09916 3xMagnetometer)
// ����� ����� ���������������� ��������� I2C/SPI:
// - InitSerial/Read/Write
// - Poll/IT/DMA
// - IRQ
#ifndef	DRIVER_IMU_INVENSENSE_H
#define	DRIVER_IMU_INVENSENSE_H

#include <stdint.h>
#include <stdbool.h>
#include "Common_gpio.h"

// ���������� IMU �� ����
typedef union IMU_Address_union
{
	uint16_t			I2C;	// I2C Device Address
	GPIO_CommonIndex_t	SPI;	// SPI Chip Select
} IMU_Address_t;

// *****************************************
// ��������� ������� ������� ������ � IMU ����� ���������������� ��������� I2C/SPI
// *****************************************
// ������������� ����������������� ����������
HAL_StatusTypeDef IMU_SerialInit( void );
// ��������� ���� ���� �� IMU
HAL_StatusTypeDef IMU_ReadByte( IMU_Address_t DevAddress, uint8_t Address, uint8_t *pByte );
// ��������� ����� �� IMU
HAL_StatusTypeDef IMU_ReadBuffer( IMU_Address_t DevAddress, uint8_t Address, uint8_t *pBuffer, uint16_t BufferSize );
// �������� ���� � IMU
HAL_StatusTypeDef IMU_WriteByte( IMU_Address_t DevAddress, uint8_t Address, uint8_t Byte );
// �������� ����� � IMU
HAL_StatusTypeDef IMU_WriteBuffer( IMU_Address_t DevAddress, void *pBuffer, uint16_t BufferSize );

#endif	// DRIVER_IMU_INVENSENSE_H
