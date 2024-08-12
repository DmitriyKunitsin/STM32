// IMU_InvenSense.h
// –абота с IMU MPU-9250/ICM-20948 для задачи определения направления
#ifndef	IMU_INVENSENSE_H
#define	IMU_INVENSENSE_H

#include <stdint.h>
#include <stdbool.h>
#include "AHRS.h"			// IMU_DataNormal_t
#include "FreeRtos.h"		// TickType_t

//  оличество осей акселерометров, магнтометров, гироскопов
#define	IMU_AXIS_COUNT		3

// ‘ормат регистров данных IMU - акселерометр, гироскоп, температура
typedef int16_t IMU_DataRaw_t;				// BIG Endian
// ‘ормат регистров данных IMU - магнитометр
typedef int16_t IMU_DataRawMagnet_t;		// LITTLE Endian

// “ипы поддерживаемых датчиков IMU
typedef enum IMU_Type_enum
{
	IMU_Type_None = 0,
	IMU_Type_MPU9250,
	IMU_Type_ICM20948,
} IMU_Type_t;

// **************************************************************************
// —борка MPU-9250 (3xAccelerometer, 3xGyroscope and AK8963 3xMagnetometer)
// **************************************************************************
// ‘ормат пакета данных магнитометра AK8963 в исходном формате
#pragma pack( 1 )
typedef struct AK8963_Frame_struct
{												// Size	Register address
	uint8_t				ST1;					// [1]		0x02	Status1
	IMU_DataRawMagnet_t	aData[IMU_AXIS_COUNT];	// [6]		0x03	Magnetic Sensor data
	uint8_t				ST2;					// [1]		0x09	Status2
	// Read Mode (test)
	uint8_t				CNTL1;					// [1]		0x0A	Control1
} AK8963_Frame_t;		// [8]
#pragma pack( )

// ‘ормат полного пакета данных MPU-9250 в исходном формате
#pragma pack( 1 )
typedef struct MPU9250_Frame_struct
{												// Size	Register address
	uint8_t				IntStatus;				// [1]		0x3A	58
	IMU_DataRaw_t		aAccel[IMU_AXIS_COUNT]; // [6]		0x3B	59..64
	IMU_DataRaw_t		Temp;					// [2]		0x41	65..66
	IMU_DataRaw_t		aGyro[IMU_AXIS_COUNT];	// [6]		0x43	67..72
	AK8963_Frame_t		Magnet;					// [8]		0x49	73..81	(Slave0)
} MPU9250_Frame_t;	// [23]
#pragma pack( )

// **************************************************************************
// —борка ICM-20948 (3xAccelerometer, 3xGyroscope and AK09916 3xMagnetometer)
// **************************************************************************
#pragma pack( 1 )
typedef struct AK09916_Frame_struct
{												// Size	Register address
	uint8_t				ST1;					// [1]		0x10	Status1
	IMU_DataRawMagnet_t	aData[IMU_AXIS_COUNT];	// [6]		0x11	Magnetic Sensor data
	uint8_t				Dummy;					// [1]		0x17	TMPS?
	uint8_t				ST2;					// [1]		0x18	Status2
} AK09916_Frame_t;	// [9]
#pragma pack( )

// ‘ормат полного пакета данных ICM-20948 в исходном формате
#pragma pack( 1 )
typedef struct ICM20948_Frame_struct
{												// Size	Register address (Bank0)
	IMU_DataRaw_t		aAccel[IMU_AXIS_COUNT]; // [6]		0x2D	45..50
	IMU_DataRaw_t		aGyro[IMU_AXIS_COUNT];	// [6]		0x33	51..56
	IMU_DataRaw_t		Temp;					// [2]		0x39	57..58
	AK09916_Frame_t		Magnet;					// [9]		0x3B	59..67	(Slave0)
} ICM20948_Frame_t;	// [23]
#pragma pack( )


// ‘ормат полного пакета данных IMU в привычном (Little Endian) формате, без упаковки.
// ≈диницы кодирования соответсвуют запрограммированному режиму.
typedef struct IMU_PacketBin_struct
{
	int16_t aAccel[IMU_AXIS_COUNT];		// [6]
	int16_t aGyro[IMU_AXIS_COUNT];		// [6]
	int16_t aMagnet[IMU_AXIS_COUNT];	// [6]
	int16_t Temp;						// [2]
} IMU_PacketBin_t;	// [20]

//  адр считанных данных с IMU без какой-либо обработки
typedef union IMU_Frame_union
{
	MPU9250_Frame_t		MPU9250;	// [23]
	ICM20948_Frame_t	ICM20948;	// [23]
} IMU_Frame_t;		// [23]

// —труктура данных датчика положения
typedef struct IMU_Data_struct
{
	IMU_Type_t			DeviceType;			// [1] 	тип IMU
	bool				bDataReady;			// [1]		флаг готовности данных
	TickType_t			Timestamp;			// [4]		отсечка времени формирования пакета
	TickType_t			TimestampMagnet;	// [4]		отсечка времени получения данных магнитометра
	IMU_Frame_t			DataRaw;			// [23+1]	кадр исходных данных от IMU MPU-9250/ICM-20948
	IMU_PacketBin_t		DataBin;			// [20]	перевод исходных данных в бинарный формат
	IMU_DataNormal_t	DataNorm;			// [36]	перевод исходных данных в натуральные единицы (без температуры)
	float				Temp;				// [4]		[гр—]	температура
} IMU_Data_t;			// [96]

// *****************************************
// ѕрототипы функций для взаимодействия с IMU MPU-9250/ICM-20948
// *****************************************
// »нициализировать последовательный канал, определить и проинициализировать IMU
IMU_Type_t IMU_Connect( void );
// —читать пакет данных ориентации из IMU, конвертировать в физические единицы
bool IMU_DataGet( IMU_Data_t *pData );
// ‘ункции для автокалибровки магнитометра
void IMU_CalibrMagnetStart( IMU_Data_t *pIMU_Data );
void IMU_CalibrMagnetStop( void );
void IMU_CalibrMagnetUpdate( IMU_Data_t *pIMU_Data );

#endif	// IMU_INVENSENSE_H
