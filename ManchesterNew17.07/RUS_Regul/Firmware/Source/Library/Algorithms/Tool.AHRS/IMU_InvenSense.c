// IMU_InvenSense.c
// ������ � IMU MPU-9250/ICM-20948 ��� ������ ����������� �����������
#include "ProjectConfig.h"			// ������ ���������
#include "stm32xxxx_hal.h"			// ����� ���������
#include "IMU_InvenSense.h"			// ������
#include "Driver_IMU_InvenSense.h"	// ������� ������ �� ���������������� ����
#include "Task.h"					// xTaskGetTickCount()
#include "common_gpio.h"
#include "MathUtils.h"				// ABS()

// ������������ ���������� ��������� ��������� � IMU ����� ��������� �� �����������������
#define	IMU_RECALL_COUNT_MAX					3
// ������������ ����� "���������" ������ �� ������������.
// ��� ��������� ���������� ������������ �� 100 sps, ���������� ���������� ������ 10 ��.
// ���������� ������ �� IMU ������ ������������� ������ 5 ��, ��� �������� � �������� ������������ �� 15 ��.
// ����� ����� ��������� �������� ������ � ������������� ����� ���������� I2C.
// ��� ������� ���������� �� ���������� ����������� ��������� �������� �� 20 ��,
// �� ������� ����������� - �� 50 ��.
#define	IMU_MAGNET_DATA_FREEZE_TIME_NORM_ms		20		// [ms]	14.5 �������� �� �������� 2 ��/�, ������ ������ ������� 22.5 ��������
#define	IMU_MAGNET_DATA_FREEZE_TIME_MAX_ms		50		// [ms]

// ��������� ��� ���������������� ������ MPU9250 � ����������� ����������� ����� ��������������� I2C
#pragma	pack( 1 )
typedef struct MPU9250_SlavePacketConfig_struct
{
	uint8_t	ConfigAddress;		// ����� ��������, ��� ��������� ��� ����� ������������
	uint8_t	SlaveAddress;		// ����� ������, ���� ������/������
	uint8_t	SlaveRegister;		// ����� ������� �������� ��� �����/�������� ������
	uint8_t	Control;			// ���������� �����������
} MPU9250_SlavePacketConfig_t;
#pragma	pack( )

// ������� ������������� � ������������� ���������
typedef struct IMU_CalibrTag_struct
{
	int16_t	Offset;			// [Bin]		�������� ��� � �������� ���
	float	Sensitivity;	// [Norm/Bin]	��������������� ���
} IMU_CalibrTag_t;	// [8]

// ������� �������� ���� IMU ��� ���������� � ���� �������
// �������� ������ �� 90 ��������
typedef struct IMU_Rotation90_struct
{
	int8_t aMatrix[IMU_AXIS_COUNT][IMU_AXIS_COUNT];	// [1*9]
} IMU_Rotation90_t;	// [12]

// ������������� � ������������� ��������� IMU, � ��� � ��������!
typedef struct IMU_Calibr_struct
{
	IMU_CalibrTag_t aAccel[IMU_AXIS_COUNT];			// [24]
	IMU_CalibrTag_t aGyro[IMU_AXIS_COUNT];			// [24]
	IMU_CalibrTag_t aMagnet[IMU_AXIS_COUNT];		// [24]
	IMU_Rotation90_t RotationMagnet;				// [12]	������� ������������ ������������ IMU
	IMU_Rotation90_t RotationIMU;					// [12]	������� IMU ������������ �������
	float			RotationRoll;					// [4]		�������������� ������� Roll/TF
	IMU_CalibrTag_t Temp;							// [24]
} IMU_Calibr_t;	// [120]

#ifdef	USE_MPU9250
// ****************************************************************************
// ���������� �������� � ������� ��� IMU MPU9250
// ****************************************************************************
#include "MPU9250_RegisterMap.h"		// �������� ��������� MPU-9250
// ������������� ������������ ��-��������� ������� MPU9250
#define	MPU9250_FS_GYRO_250_LSB			( 64.0f * 1024 / ( 2 * 250 ) )		// Gyro +-250 Deg/s
#define MPU9250_FS_GYRO_500_LSB			( 64.0f * 1024 / ( 2 * 500 ) )		// Gyro +-500 Deg/s
#define MPU9250_FS_GYRO_1000_LSB		( 64.0f * 1024 / ( 2 * 1000 ) )		// Gyro +-1000 Deg/s
#define MPU9250_FS_GYRO_2000_LSB		( 64.0f * 1024 / ( 2 * 2000 ) )		// Gyro +-2000 Deg/s
#define MPU9250_FS_ACCEL_2_LSB			( 64.0f * 1024 / ( 2 * 2 ) )		// Accel +-2 G
#define MPU9250_FS_ACCEL_4_LSB			( 64.0f * 1024 / ( 2 * 4 ) )		// Accel +-4 G
#define MPU9250_FS_ACCEL_8_LSB			( 64.0f * 1024 / ( 2 * 8 ) )		// Accel +-8 G
#define MPU9250_FS_ACCEL_16_LSB			( 64.0f * 1024 / ( 2 * 16 ) )		// Accel +-16 G
#define MPU9250_FS_MAGNET_5000_LSB		( ( 32768 / 4 ) / 4912.0f )			// Magnet +-4800 uT
#define MPU9250_FS_TEMP_LSB				( 338.87f )
// ������� ������������ AK8963 � ������� MPU9250
#define AK8963_ROTATION		\
	{	{	0,	1,	0 },	\
		{	1,	0,	0 },	\
		{	0,	0,	-1 } }

// ���������� �������
static bool MPU9250_Connect( void );
static bool MPU9250_Init( void );
static bool MPU9250_GetRaw( MPU9250_Frame_t *pRaw, TickType_t *pTimestamp, TickType_t *pTimestampMagnet );
static bool MPU9250_ConvertRaw2Bin( MPU9250_Frame_t * const pRaw, IMU_PacketBin_t *pBin );
// ������ �������������, ��������� � ������������
static const IMU_Address_t MPU9250_Address	= { .I2C = ( MPU9250_I2C_ADDRESS << 1 ) };
static const IMU_Address_t AK8963_Address	= { .I2C = ( MPU9250_MAG_ADDRESS << 1 ) };
// ������������� ��������� ���������������� ����� MPU9250_Init() ������, ���� ������� ��������
static const IMU_Calibr_t MPU9250_Calibr_Acc8_Gyro2000_Default =
{
	{	// �������������, +-8 G
		{ 0, 1.0f / MPU9250_FS_ACCEL_8_LSB },
		{ 0, 1.0f / MPU9250_FS_ACCEL_8_LSB },
		{ 0, 1.0f / MPU9250_FS_ACCEL_8_LSB },
	},
	{	// ���������, +-2000 ������/�
		{ 0, 1.0f / MPU9250_FS_GYRO_2000_LSB * M_PI / 180.0f },
		{ 0, 1.0f / MPU9250_FS_GYRO_2000_LSB * M_PI / 180.0f },
		{ 0, 1.0f / MPU9250_FS_GYRO_2000_LSB * M_PI / 180.0f },
	},
	{	// ������������, +-4800 ����	!!! ���� ����� ��������� ������, ������� ���������� !!!
		{ 0, 1.0f / MPU9250_FS_MAGNET_5000_LSB },
		{ 0, 1.0f / MPU9250_FS_MAGNET_5000_LSB },
		{ 0, 1.0f / MPU9250_FS_MAGNET_5000_LSB },
	},
	// ������� �������� ������������ ������������ ���� IMU
	{ AK8963_ROTATION },
	// ������� �������� ���������� IMU ������������ ���� ������� (��� ����� �������� ������ �������� ��� X)
	{ MPU9250_ROTATION },
	// �������������� ������� Roll/TF
	MPU9250_ROTATION_ROLL,
	// �����������
	{  ( int16_t ) ( -21.0f * MPU9250_FS_TEMP_LSB - 0.5f ), 1.0f / MPU9250_FS_TEMP_LSB },
};
// ��������� �� IMU ��������������� �������������
static float aAK8963_Sensitivity[IMU_AXIS_COUNT];

#endif	// USE_MPU9250
// ****************************************************************************

#ifdef	USE_ICM20948
// ****************************************************************************
// ���������� �������� � ������� ��� IMU ICM-20948
// ****************************************************************************
#include "ICM20948_RegisterMap.h"	// �������� ��������� ICM-20948
// ������������� ������������ ��-��������� ������� ICM-20948
#define	ICM20948_FS_GYRO_250_LSB		( 0x7FFF / 250.0f )		// Gyro +-250 Deg/s
#define ICM20948_FS_GYRO_500_LSB		( 0x7FFF / 500.0f )		// Gyro +-500 Deg/s
#define ICM20948_FS_GYRO_1000_LSB		( 0x7FFF / 1000.0f )	// Gyro +-1000 Deg/s
#define ICM20948_FS_GYRO_2000_LSB		( 0x7FFF / 2000.0f )	// Gyro +-2000 Deg/s
#define ICM20948_FS_ACCEL_2_LSB			( 0x7FFF / 2.0f )		// Accel +-2 G
#define ICM20948_FS_ACCEL_4_LSB			( 0x7FFF / 4.0f )		// Accel +-4 G
#define ICM20948_FS_ACCEL_8_LSB			( 0x7FFF / 8.0f )		// Accel +-8 G
#define ICM20948_FS_ACCEL_16_LSB		( 0x7FFF / 16.0f )		// Accel +-16 G
#define ICM20948_FS_MAGNET_5000_LSB		( 32760.0f / 4912 )		// Magnet +-4900 uT
#define ICM20948_FS_TEMP_LSB			( 338.87f )
// ������� ������������ AK09916 � ������� ICM-20948
#define AK09916_ROTATION	\
	{	{	1,	0,	0 },	\
		{	0,	-1,	0 },	\
		{	0,	0,	-1 } }
// ���������� �������
bool ICM20948_Connect( void );
bool ICM20948_Init( void );
bool ICM20948_GetRaw( ICM20948_Frame_t *pRaw, TickType_t *pTimestamp, TickType_t *pTimestampMagnet );
bool ICM20948_ConvertRaw2Bin( ICM20948_Frame_t * const pRaw, IMU_PacketBin_t *pBin );
// ������ �������������, ��������� � ������������
static const IMU_Address_t ICM20948_Address	= { .I2C = ( ICM20948_I2C_ADDRESS << 1 ) };
static const IMU_Address_t AK09916_Address	= { .I2C = ( AK09916_I2C_ADDR << 1 ) };
// ������������� ��������� ���������������� ����� ICM20948_Init() ������, ���� ������� ��������
// !!! ����� � MPU9250, ���������!
static const IMU_Calibr_t ICM20948_Calibr_Acc8_Gyro2000_Default =
{
	{	// �������������, +-8 G
		{ 0, 1.0f / ICM20948_FS_ACCEL_8_LSB },
		{ 0, 1.0f / ICM20948_FS_ACCEL_8_LSB },
		{ 0, 1.0f / ICM20948_FS_ACCEL_8_LSB },
	},
	{	// ���������, +-2000 ������/�
		{ 0, 1.0f / ICM20948_FS_GYRO_2000_LSB * M_PI / 180.0f },
		{ 0, 1.0f / ICM20948_FS_GYRO_2000_LSB * M_PI / 180.0f },
		{ 0, 1.0f / ICM20948_FS_GYRO_2000_LSB * M_PI / 180.0f },
	},
	{	// ������������, +-4800 ����	!!! ���� ����� ��������� ������, ������� ���������� !!!
		{ 0, 1.0f / ICM20948_FS_MAGNET_5000_LSB },
		{ 0, 1.0f / ICM20948_FS_MAGNET_5000_LSB },
		{ 0, 1.0f / ICM20948_FS_MAGNET_5000_LSB },
	},
	// ������� �������� ������������ ������������ ���� IMU
	{ AK8963_ROTATION },
	// ������� �������� ���������� IMU ������������ ���� ������� (��� ����� �������� ������ �������� ��� X)
	{ ICM20948_ROTATION },
	// �������������� ������� Roll/TF
	ICM20948_ROTATION_ROLL,
	// �����������
	{  ( int16_t ) ( -21.0f * MPU9250_FS_TEMP_LSB - 0.5f ), 1.0f / MPU9250_FS_TEMP_LSB },
};
#endif	// USE_ICM20948
// ****************************************************************************

// ���������� ���������� ������� � ������ IMU_InvenSense.c
static bool IMU_ConvertBin2Norm( IMU_PacketBin_t * const pBin, IMU_DataNormal_t *pNorm, float *pTemp );
static void IMU_CalibrMagnetInit( void );
// ���� ���������� ������������� ����������������� ����������
static bool bIMU_SerialInitComplete = false;
// ������������/������������ IMU
static IMU_Type_t IMU_Connected = IMU_Type_None;
// ������������� ��������� ������������� IMU
static __no_init IMU_Calibr_t IMU_Calibr;

// ����������� � IMU:
// - ���������������� ���������������� ���������
// - ��������� ����� � IMU (MPU9250 ��� ICM20948)
// - ��������� ������������������ ������������� � ��������� IMU
IMU_Type_t IMU_Connect( void )
{
	IMU_Connected = IMU_Type_None;
	bIMU_SerialInitComplete = false;
	do
	{
		if( HAL_OK != IMU_SerialInit( ) )
			break;
		bIMU_SerialInitComplete = true;
#ifdef	USE_MPU9250
		if( MPU9250_Connect( ) && MPU9250_Init( ) )
		{	
			IMU_Connected = IMU_Type_MPU9250;
			IMU_Calibr = MPU9250_Calibr_Acc8_Gyro2000_Default;
			for( int i = 0; i < IMU_AXIS_COUNT; i++ )
				IMU_Calibr.aMagnet[i].Sensitivity = aAK8963_Sensitivity[i];
			break;
		}
#endif	// USE_MPU9250
#ifdef	USE_ICM20948
		if( ICM20948_Connect( ) && ICM20948_Init( ) )
		{	
			IMU_Connected = IMU_Type_ICM20948;
			IMU_Calibr = ICM20948_Calibr_Acc8_Gyro2000_Default;
			break;
		}
#endif	// USE_ICM20948
	} while( 0 );
	IMU_CalibrMagnetInit( );
	return IMU_Connected;
}

// ���������� �� IMU
void IMU_Disconnect( void )
{
	IMU_Connected = IMU_Type_None;
}

// ��������� ������ �� IMU:
// - ������� �������� ����� ����� ���������������� ���������
// - �������������� � �������� ������
// - �������������� � ���������� (���������) ������
bool IMU_DataGet( IMU_Data_t *pData )
{
	bool bResult = false;
	do
	{
		if( NULL == pData )
			break;
		pData->bDataReady = false;
		// ���������, ��� ���������������� ��������� ���������������
		if( !bIMU_SerialInitComplete )
			break;
		switch( IMU_Connected )
		{
#ifdef USE_MPU9250
		case IMU_Type_MPU9250:
			// ������� �������� ������ �� MPU9250
			if( !MPU9250_GetRaw( &pData->DataRaw.MPU9250, &pData->Timestamp, &pData->TimestampMagnet ) )
				break;
			// �������������� �������� ������ MPU9250 � �������� ������
			pData->DeviceType = IMU_Connected;
			if( !MPU9250_ConvertRaw2Bin( &pData->DataRaw.MPU9250, &pData->DataBin ) )
				break;
			bResult = true;
			break;
#endif	// USE_MPU9250
#ifdef USE_ICM20948
		case IMU_Type_ICM20948:
			// ������� �������� ������ �� IMU ICM-20948
			if( !ICM20948_GetRaw( &pData->DataRaw.ICM20948, &pData->Timestamp, &pData->TimestampMagnet ) )
				break;
			// �������������� �������� ������ ICM-20948 � �������� ������
			pData->DeviceType = IMU_Connected;
			if( !ICM20948_ConvertRaw2Bin( &pData->DataRaw.ICM20948, &pData->DataBin ) )
				break;
			bResult = true;
			break;
#endif	// USE_ICM20948
		default:
			break;
		}
		if( !bResult )
			break;
		// �������������� �������� ������ IMU � ����������� ������
		if( !IMU_ConvertBin2Norm( &pData->DataBin, &pData->DataNorm, &pData->Temp ) )
			break;
		// ������ ������� �������� � ����������
		pData->bDataReady = true;
		bResult = true;
	} while( 0 );
	return bResult;
}

// �������������� ����� �������� ������ IMU � ���������� ��� -
// ������� �� ������ ��� � ���������� ��������
static bool IMU_ConvertBin2Norm( IMU_PacketBin_t * const pBin, IMU_DataNormal_t *pNorm, float *pTemp )
{
	bool bResult = false;
	do
	{
		// �������� ����������
		assert_param( ( NULL != pBin ) && ( NULL != pNorm ) && ( NULL != pTemp ) );
		// �������� ������������� IMU (� ���������� ����������� ���������� ������������� ���������)
		if( IMU_Type_None == IMU_Connected )
			break;
		IMU_Calibr_t * const pCalibr = &IMU_Calibr;
		// ������� �������� ������� � ���������� ��� ����� ������������� ������������
		for( int i = 0; i < IMU_AXIS_COUNT; i++ )
		{
			pNorm->aAccel[i]	= ( pBin->aAccel[i]	- pCalibr->aAccel[i].Offset )	* pCalibr->aAccel[i].Sensitivity;
			pNorm->aGyro[i]		= ( pBin->aGyro[i]	- pCalibr->aGyro[i].Offset )	* pCalibr->aGyro[i].Sensitivity;
			pNorm->aMagnet[i]	= ( pBin->aMagnet[i]- pCalibr->aMagnet[i].Offset )	* pCalibr->aMagnet[i].Sensitivity;
		}
		*pTemp					= ( pBin->Temp		- pCalibr->Temp.Offset	)		* pCalibr->Temp.Sensitivity;
		bResult = true;
	} while( 0 );
	return bResult;
}

// ****************************************************************************
// �������������� �������������
// ****************************************************************************
#define	IMU_MAGNET_CALIBR_TIME			8.0f	// [�]	�����, � ������� �������� ������������� ��������� � �������� ���������
#define	IMU_MAGNET_CALIBR_TIME_MIN		2.5f	// [�]	�����, � ������� �������� ������������� ��������� � �������� ��������� (����������� �������, ��� ������ � ����������� ��������)
#define	IMU_MAGNET_CALIBR_TIME_IDLE		1.5f	// [�]	���������� ����� ������� �� ����� ����������, ��� ������� ������� �� ������������
#define	IMU_MAGNET_CALIBR_RPS_TRESH		( 1.0f / IMU_MAGNET_CALIBR_TIME * 2 * M_PI )	// [rad/s]	��������� �������� ��������, ��� ������� ����������� � ������������ ����������
#define	IMU_MAGNET_CALIBR_ROUNDS_MIN	( 1.2f * 2 * M_PI )								// [rad]	����������� ���������� ��������, ����������� ��� ���������� ����������

// ��������� �������� ����������
typedef enum IMU_CalibrMagnetState_enum
{
	IMU_CalibrMagnetState_Stop = 0,
	IMU_CalibrMagnetState_Start,
	IMU_CalibrMagnetState_Work,
} IMU_CalibrMagnetState_t;

typedef struct IMU_CalibrMagnet_struct
{
	IMU_CalibrMagnetState_t State;				// ��������� �������� ����������
	TickType_t	TimestampStart;					// ������� ������� ������� ����������
	TickType_t	TimestampLastData;				// ������� ������� ��������� ��������� ������
	TickType_t	TimestampLastMotion;
	float		aRotations[IMU_AXIS_COUNT];		// [rad]	������� ���� ��� ����������� �������� �� ����� ����������
	int16_t		aMagnetsMin[IMU_AXIS_COUNT];	// ��������� ��������
	int16_t		aMagnetsMax[IMU_AXIS_COUNT];	// ��������� ���������
	int16_t		aOffsets[IMU_AXIS_COUNT];		// ��������� ����������, �������� �� ����
//	float		aSensitivity[MPU9250_AXIS_COUNT];	// ���������������, �� ���������� ���������� �� ������������
} IMU_CalibrMagnet_t;

// ��������� ��� �������������� ���������� �������������
static __no_init IMU_CalibrMagnet_t IMU_CalibrMagnet;
// ����������� � ����-����������������� ������ �������� �������������. ��������� ������� - ����������� �����.
static __no_init int16_t aIMU_CalibrMagnetOffsetsSaved[IMU_AXIS_COUNT+1] __PLACE_AT_SRAM2__;

// ������������� �������������� ������������:
// ��������� �� NVM � ������������� ���������� IMU ����� ����������� ��������.
static void IMU_CalibrMagnetInit( void )
{
	IMU_CalibrMagnet.State = IMU_CalibrMagnetState_Stop;

	// ��������� ����������� ���������� �������������
	int16_t OffsetSumm = 0;
	for( int i = 0; i < IMU_AXIS_COUNT; i++ )
	{
		int16_t OffsetSaved = aIMU_CalibrMagnetOffsetsSaved[i];
		IMU_CalibrMagnet.aOffsets[i] = OffsetSaved;
		OffsetSumm += OffsetSaved;
	}
	if( OffsetSumm != aIMU_CalibrMagnetOffsetsSaved[IMU_AXIS_COUNT] )
		for( int i = 0; i < IMU_AXIS_COUNT; i++ )
			IMU_CalibrMagnet.aOffsets[i] = 0;
	// ����������� ��������� ���������� � ������������� ��������� IMU
	for( int i = 0; i < IMU_AXIS_COUNT; i++ )
		IMU_Calibr.aMagnet[i].Offset = IMU_CalibrMagnet.aOffsets[i];
}

// ���������� � �������������� ���������� �������������
void IMU_CalibrMagnetStart( IMU_Data_t *pIMU_Data )
{
	assert_param( NULL != pIMU_Data );
	do
	{
		// ���������, ��� ����� ������ �� IMU ��������
		if( IMU_Type_None == pIMU_Data->DeviceType )
			break;
		if( !pIMU_Data->bDataReady )
			break;
		// ���������, ��� ���������� �� ��������
		if( IMU_CalibrMagnetState_Stop != IMU_CalibrMagnet.State )
			break;
		// ���������, ��� ������ ���������
		for( int i = 0; i < IMU_AXIS_COUNT; i++ )
			if( ABS( pIMU_Data->DataBin.aGyro[i] ) > IMU_MAGNET_CALIBR_RPS_TRESH )
			{	// �� ����� �� ���� ����������� ����������� ��� �������������� �������� ��������
				IMU_CalibrMagnet.State = IMU_CalibrMagnetState_Start;
				break;
			}
	} while( 0 );
}

// ��������� ������� ��������������
void IMU_CalibrMagnetStop( void )
{
	IMU_CalibrMagnet.State = IMU_CalibrMagnetState_Stop;
}

// ������� �� DirectionUpdate() ��� ���������� �������������� ���������� �������������
void IMU_CalibrMagnetUpdate( IMU_Data_t *pIMU_Data )
{
	assert_param( NULL != pIMU_Data );
	
	// ���������, ��� ����� ������ �� IMU ��������
	if( ( IMU_Type_None == pIMU_Data->DeviceType ) ||
		( !pIMU_Data->bDataReady ) )
		return;

	// ���������� ���������� ������������
	TickType_t TimestampCurrent = pIMU_Data->Timestamp;
	switch( IMU_CalibrMagnet.State )
	{
	case IMU_CalibrMagnetState_Stop:
		break;

	case IMU_CalibrMagnetState_Start:
		// �������� "���������" ������������
		if( ( pIMU_Data->Timestamp - pIMU_Data->TimestampMagnet ) > IMU_MAGNET_DATA_FREEZE_TIME_NORM_ms )
			break;
		// ���������� � ����������
		IMU_CalibrMagnet.TimestampStart 		= TimestampCurrent;
		IMU_CalibrMagnet.TimestampLastData		= TimestampCurrent;
		IMU_CalibrMagnet.TimestampLastMotion	= TimestampCurrent;
		for( int i = 0; i < IMU_AXIS_COUNT; i++ )
		{
			IMU_CalibrMagnet.aMagnetsMin[i] = INT16_MAX;
			IMU_CalibrMagnet.aMagnetsMax[i] = INT16_MIN;
			IMU_CalibrMagnet.aRotations[i]	= 0.0f;
		}
		IMU_CalibrMagnet.State = IMU_CalibrMagnetState_Work;
		break;
	
	case IMU_CalibrMagnetState_Work:
		// ���������� ���������� ������������
/*		if( pDirData->MPU9250_DataRaw.Magnet.ST1 & 0x01 )
		{
			// �� ������������ �������, � ������ ������������� ������ ������������ ���� �� +-256 � �������� �������.
			// ��� ��� ���� ��������� � ��������� ���������� ������ ������������, ����� ��������� ���� AK8963.ST1.DRDY
			// (��� ���� ������� ����� AK8963.ST1.DRDY ��� �� ��������, ��� ��� ���������� ����� �����) -
			// ��������, ��� ����� ���� ������� � �������������� ���������� ������ ������?
			// �� ������ AHRS ��� ���� ����� ��� �� ������, �� ��� ������������� ����������� �������������� ����������
			// �� ��������� � ���������� ��� ��������� ������������ �������.
			// ��-�����, ��� ����������� � ������ ������������ ����� AK8963.ST1.DRDY,
			// ���������� ��� �������� ����������.
			break;
		}*/
		// �������� "���������" ������������
		if( ( pIMU_Data->Timestamp - pIMU_Data->TimestampMagnet ) > IMU_MAGNET_DATA_FREEZE_TIME_NORM_ms )
		{	// ��������� ���������� ������������ � ����� � ���������� ����������� ������ �� ������������
			IMU_CalibrMagnet.State = IMU_CalibrMagnetState_Stop;
			break;
		}
		// �������� ������ ��������, ��������� �������� � ������� �������
		for( int i = 0; i < IMU_AXIS_COUNT; i++ )
		{
			// ����������� �������� � ��������� ������������
			int16_t MagnetBin = pIMU_Data->DataBin.aMagnet[i];
			if( MagnetBin < IMU_CalibrMagnet.aMagnetsMin[i] )
				IMU_CalibrMagnet.aMagnetsMin[i] = MagnetBin;
			if( MagnetBin > IMU_CalibrMagnet.aMagnetsMax[i] )
				IMU_CalibrMagnet.aMagnetsMax[i] = MagnetBin;
			// ���������� ������� ���� � ������ ��������
			float Gyro = pIMU_Data->DataNorm.aGyro[i];
			IMU_CalibrMagnet.aRotations[i] += Gyro * ( ( TimestampCurrent - IMU_CalibrMagnet.TimestampLastData ) / ( float ) configTICK_RATE_HZ );
			if( ABS( Gyro ) > IMU_MAGNET_CALIBR_RPS_TRESH )
				IMU_CalibrMagnet.TimestampLastMotion = TimestampCurrent;
		}
		IMU_CalibrMagnet.TimestampLastData = TimestampCurrent;

		// ��������� ������� ���������� ����������
		if( ( TimestampCurrent - IMU_CalibrMagnet.TimestampLastMotion ) > pdMS_TO_TICKS( IMU_MAGNET_CALIBR_TIME_IDLE * 1000 ) )
		{	// ��������� ���������� ������������ � ����� � ������������ ��������
			IMU_CalibrMagnet.State = IMU_CalibrMagnetState_Stop;
			break;
		}
		if( ( TimestampCurrent - IMU_CalibrMagnet.TimestampStart ) > pdMS_TO_TICKS( IMU_MAGNET_CALIBR_TIME * 1000 ) )
		{	// ��������� ���������� ������������ � ����� � ���������� ����������� �������
			IMU_CalibrMagnet.State = IMU_CalibrMagnetState_Stop;
			break;
		}
		if( ( TimestampCurrent - IMU_CalibrMagnet.TimestampStart ) > pdMS_TO_TICKS( IMU_MAGNET_CALIBR_TIME_MIN * 1000 ) )
		{	// ���������, ��� �� ���� ��������� ����������� ��� �������������� �������
			bool bRoundComplete = false;
			for( int i = 0; i < IMU_AXIS_COUNT; i++ )
				if( ( ABS( IMU_CalibrMagnet.aRotations[ ( i + 1 ) % IMU_AXIS_COUNT ] ) > IMU_MAGNET_CALIBR_ROUNDS_MIN ) ||
					( ABS( IMU_CalibrMagnet.aRotations[ ( i + 2 ) % IMU_AXIS_COUNT ] ) > IMU_MAGNET_CALIBR_ROUNDS_MIN ) )
				{	// �� ����, ���������������� ����������� ���, ��������� ����������� �������
					IMU_CalibrMagnet.aOffsets[i] = ( IMU_CalibrMagnet.aMagnetsMax[i] + IMU_CalibrMagnet.aMagnetsMin[i] ) / 2;
					bRoundComplete = true;
				}
			if( bRoundComplete )
			{	// ��������� ��������� ���������� � ���������� ������ � � ������������� ��������� IMU
				taskENTER_CRITICAL( );
				int16_t OffsetsSumm = 0;
				for( int i = 0; i < IMU_AXIS_COUNT; i++ )
				{
					IMU_Calibr.aMagnet[i].Offset		= IMU_CalibrMagnet.aOffsets[i];
					aIMU_CalibrMagnetOffsetsSaved[i]	= IMU_CalibrMagnet.aOffsets[i];
					OffsetsSumm += IMU_CalibrMagnet.aOffsets[i];
				}
				aIMU_CalibrMagnetOffsetsSaved[IMU_AXIS_COUNT] = OffsetsSumm;
				taskEXIT_CRITICAL( );
				// ��������� ���������� ������������ � ����� � ����������� ������������ ������
				IMU_CalibrMagnet.State = IMU_CalibrMagnetState_Stop;
			}
		}
		break;
	
	default: assert_param( 0 );
	}
}


#ifdef	USE_MPU9250
// ****************************************************************************
// ����������� ������� ��� IMU MPU9250
// ****************************************************************************

// �������� ����� � MPU-9250
static bool MPU9250_Connect( void )
{
	if( !bIMU_SerialInitComplete )
		return false;
	
	bool bResult = false;

	GPIO_Common_Write( iGPIO_TestPinDir, GPIO_PIN_SET );
	for( int i = 0; i < IMU_RECALL_COUNT_MAX; i++ )
	{
		// MPU-9250 check connection
		uint8_t Byte;
		if( HAL_OK != IMU_ReadByte( MPU9250_Address, MPU9250_WHO_AM_I, &Byte ) )
			continue;
		if( ( MPU9250_WHOAMI_RESET_VAL != Byte ) && ( MPU9250_WHOAMI_DEFAULT_VAL != Byte ) )
			continue;
		bResult = true;
		break;
	}
	GPIO_Common_Write( iGPIO_TestPinDir, GPIO_PIN_RESET );

	return bResult;
}

// ���������������� MPU9250 � ����� ��������������� ������ ���� ��������
static bool MPU9250_Init( void )
{
	if( !bIMU_SerialInitComplete  )
		return false;
	// �������� ���� ����������� � IMU - ���� ������������� �������, �� ����� ������������
	IMU_Connected = IMU_Type_None;
	bool bResult = false;
	uint8_t Byte;

	GPIO_Common_Write( iGPIO_TestPinDir, GPIO_PIN_SET );
	// ��������������� MPU9250
	for( int i = 0; i < IMU_RECALL_COUNT_MAX; i++ )
	{
		// Reset
		if( HAL_OK != IMU_WriteByte( MPU9250_Address, MPU9250_PWR_MGMT_1, MPU9250_H_RESET_MASK ) )
			continue;
		vTaskDelay( pdMS_TO_TICKS( 2 ) );
		// Set Sample Rate divider
		if( HAL_OK != IMU_WriteByte( MPU9250_Address, MPU9250_SMPLRT_DIV, 0 ) )
			continue;
		// Set clock source - auto
		if( HAL_OK != IMU_WriteByte( MPU9250_Address, MPU9250_PWR_MGMT_1, MPU9250_CLKSEL_MASK & 0x01 ) )
			continue;

		// Accelerometer and Gyroscope Config
		// **********************************
		// Enable Accel & Gyro
		if( HAL_OK != IMU_WriteByte( MPU9250_Address, MPU9250_PWR_MGMT_2, 0 ) )
			continue;
		// Gyro Filter to 3600 Hz, Sample Rate to 8 kHz
		if( HAL_OK != IMU_WriteByte( MPU9250_Address, MPU9250_CONFIG, ( MPU9250_FIFO_MODE_MASK & 0 ) | ( MPU9250_EXT_SYNC_SET_MASK & 0 ) | ( MPU9250_DLPF_CFG_MASK & 7 ) ) )
			continue;
		// Accelerometer Range to +-8G
		if( HAL_OK != IMU_WriteByte( MPU9250_Address, MPU9250_ACCEL_CONFIG, MPU9250_FULL_SCALE_8G << MPU9250_FULL_SCALE_bp ) )
			continue;
		// Accelerometer Filter to 1046 Hz, Sample Rate to 4 kHz
		if( HAL_OK != IMU_WriteByte( MPU9250_Address, MPU9250_ACCEL_CONFIG2, MPU9250_ACCEL_FCHOICE_B_MASK | ( 0 & MPU9250_A_DLPF_CFG_MASK ) ) )							
			continue;
		// Gyroscope Range to +-2000 DpS
		if( HAL_OK != IMU_WriteByte( MPU9250_Address, MPU9250_GYRO_CONFIG, MPU9250_GYRO_FULL_SCALE_2000DPS << MPU9250_GYRO_FULL_SCALE_bp ) )
			continue;

		// Magnetometer Config
		// *******************
		// Disable MPU-9250 I2C Master Mode
		if( HAL_OK != IMU_WriteByte( MPU9250_Address, MPU9250_USER_CTRL, ( 0 & MPU9250_I2C_MST_EN_MASK ) ) )
			continue;
		// Enable I2C bypass to Magnetometer
		if( HAL_OK != IMU_WriteByte( MPU9250_Address, MPU9250_INT_PIN_CFG, MPU9250_BYPASS_EN_MASK ) )
			continue;
		vTaskDelay( pdMS_TO_TICKS( 3 ) );
		// Read Who Am I from Magnetometer
		if( HAL_OK != IMU_ReadByte( AK8963_Address, MPU9250_MAG_WIA, &Byte ) )
			continue;
		if( MPU9250_MAG_WIA_MASK != Byte )
			continue;
		// Reset Magnetometer
		if( HAL_OK != IMU_WriteByte( AK8963_Address, MPU9250_MAG_CNTL2, 0x01 ) )
			continue;
		vTaskDelay( pdMS_TO_TICKS( 3 ) );

		// Read Sensitivity
		uint8_t aMagnetSensitivity[IMU_AXIS_COUNT];
		if( HAL_OK != IMU_ReadBuffer( AK8963_Address, MPU9250_MAG_ASAX, ( uint8_t * ) &aMagnetSensitivity, sizeof( aMagnetSensitivity ) ) )
			continue;
		for( int i = 0; i < IMU_AXIS_COUNT; i++ )
			aAK8963_Sensitivity[i] = ( aMagnetSensitivity[i] - 128 ) / 256.0f + 1;
			
		// Set Magnetometer to 16 bit resolution, 100 Hz update rate (Mode 2)
		if( HAL_OK != IMU_WriteByte( AK8963_Address, MPU9250_MAG_CNTL1, 0x16 ) )
			continue;

		// Setup Continues Read Sequence from Magnetometer
		// ***********************************************
		// Enable MPU-9250 I2C Master Mode
		if( HAL_OK != IMU_WriteByte( MPU9250_Address, MPU9250_USER_CTRL, MPU9250_I2C_MST_EN_MASK ) )
			continue;
		// Set I2C Master Speed to 500 kHz (max)
		if( HAL_OK != IMU_WriteByte( MPU9250_Address, MPU9250_I2C_MST_CTRL, MPU9250_I2C_MST_CLK_MASK & MPU9250_I2C_MST_CLK_500kHz ) )
			continue;
		vTaskDelay( pdMS_TO_TICKS( 3 ) );
		// Set I2C Master delay (update slaves every 32 internal sample at 4 kHz -> 125 sps ) - low update rate greatly increase high temperature stability!
		if( HAL_OK != IMU_WriteByte( MPU9250_Address, MPU9250_I2C_SLV4_CTRL, MPU9250_I2C_MST_DLY_MASK & ( 32 - 1 ) ) )
			continue;
		vTaskDelay( pdMS_TO_TICKS( 3 ) );
		// Enable I2C Master delay for Slave0 (Magnetometer AK8963)
		if( HAL_OK != IMU_WriteByte( MPU9250_Address, MPU9250_I2C_MST_DELAY_CTRL, MPU9250_DELAY_ES_SHADOW_MASK | MPU9250_I2C_SLV0_DLY_EN_MASK ) )
			continue;
		vTaskDelay( pdMS_TO_TICKS( 3 ) );
		// Setup Continues Read Sequence from Magnetometer (about 25..50 ms to Magnetometer data ready!)
		MPU9250_Frame_t MPU9250_Frame = { 0 };
		uint8_t aSlaveSequenceConfig[] =
		{
			MPU9250_I2C_SLV0_ADDR,									// Config transaction for Slave0
			MPU9250_MAG_ADDRESS | MPU9250_I2C_SLV0_RNW_MASK,		// Set slave 0 to the AK8963 and set for read
			MPU9250_MAG_ST1, 										// Set the register to the desired AK8963 sub address
			MPU9250_I2C_SLV0_EN_MASK | ( sizeof( MPU9250_Frame.Magnet ) & MPU9250_I2C_SLV0_LENG_MASK )	// Enable I2C and request the bytes
		};
		if( HAL_OK != IMU_WriteBuffer( MPU9250_Address, aSlaveSequenceConfig, sizeof( aSlaveSequenceConfig ) ) )
			continue;
		// Wait for Magnetometer setup
		vTaskDelay( pdMS_TO_TICKS( 25 ) );
		// Read full frame
		if( HAL_OK != IMU_ReadBuffer( MPU9250_Address, MPU9250_INT_STATUS, ( uint8_t * ) &MPU9250_Frame, sizeof( MPU9250_Frame ) ) )
			continue;
		// Check for Magnetometer data ready
		if( ( 0 == MPU9250_Frame.Magnet.aData[0] ) && ( 0 != MPU9250_Frame.Magnet.aData[1] ) && ( 0 != MPU9250_Frame.Magnet.aData[2] ) )
			continue;

		// Init Complete!
		bResult = true;
		IMU_Connected = IMU_Type_MPU9250;
		break;
	}
	GPIO_Common_Write( iGPIO_TestPinDir, GPIO_PIN_RESET );
	return bResult;
}

// ������� �� MPU9250 �������� ������
static bool MPU9250_GetRaw( MPU9250_Frame_t *pRaw, TickType_t *pTimestamp, TickType_t *pTimestampMagnet )
{

	assert_param( NULL != pRaw );
	bool Result = false;
	do
	{
		// ��������� ������������� ����������������� ������
		if( !bIMU_SerialInitComplete )
			break;
		// ��������� ������������� MPU9250
		if( IMU_Type_MPU9250 != IMU_Connected )
			break;
		// ������� ����� ����� ������
		GPIO_Common_Write( iGPIO_TestPinIMU, GPIO_PIN_SET );
		if( HAL_OK != IMU_ReadBuffer( MPU9250_Address, MPU9250_INT_STATUS, ( uint8_t * ) pRaw, sizeof( *pRaw ) ) )
			break;
		// ������ ����� ��������� ������
		TickType_t TimeStamp = xTaskGetTickCount( );

		// ������ ����������� ������ � ������ ��������� �������� (�������� � ������ MPU9250 � ����������� I2C).
		// ��������� ���� �� �������������������� ����� � ����������� I2C,
		// �� ����� ������� ������, ��� � ����� �������� � ������ ����������������� ������ � MPU9250.
		static __no_init AK8963_Frame_t MagFrameSaved;
		static TickType_t MagLastTimeStamp = 0;
		pRaw->Magnet.ST1 &= ~MPU9250_MAG_ST1_DNEW;		// �������� ����������� ���� ���������� ������
		if( (	( pRaw->Magnet.aData[0] == MagFrameSaved.aData[0] ) &&
				( pRaw->Magnet.aData[1] == MagFrameSaved.aData[1] ) &&
				( pRaw->Magnet.aData[2] == MagFrameSaved.aData[2] ) ) ||
			( ( 0 == pRaw->Magnet.aData[0] ) && ( 0 == pRaw->Magnet.aData[1] ) && ( 0 == pRaw->Magnet.aData[2] ) ) )
		{	// ���������� ������ ������������ � ���������� ������, ��� ������� ������
			if( ( TimeStamp - MagLastTimeStamp ) > IMU_MAGNET_DATA_FREEZE_TIME_MAX_ms )
			{	// ������� ����� �� ������������ �� �������� ����� ������. ������� ������, �� ������� ����� ����������������� ������ � IMU
				break;
			}
/*			{	// � ������ ���� ������������ - �������������������� ������ ���
				// ���� ����������������� �� �������, ������� false, ����� �������������������� ���� MPU-9250
				if( HAL_OK != AK8963_WriteByte( MPU9250_MAG_CNTL2, 0x01 ) )		// ����������� ����� ������������
					break;
				if( HAL_OK != AK8963_WriteByte( MPU9250_MAG_CNTL1, 0x16 ) )		// ���������� ���������, 100 ��
					break;
				if( HAL_OK != AK8963_ReadBuffer( MPU9250_MAG_ST1, ( uint8_t * ) &pRaw->Magnet, sizeof( pRaw->Magnet ) ) )
					break;
				AK8963_DataPacketRawRepeatCount = 0;
			}
*/
		}
		else
		{	// ������� ����� � ������ ������� �� ������������
			pRaw->Magnet.ST1 |= MPU9250_MAG_ST1_DNEW;
			// �������� ����� ������� ��������� ��������� ������ �� ������������
			MagLastTimeStamp = TimeStamp;
			GPIO_Common_Write( iGPIO_TestPinIMU, GPIO_PIN_RESET );
		}
		// ��������� ��������� �������� ����� �� ������������ ��� �������� ��������� ������
		MagFrameSaved = pRaw->Magnet;
		// ��������� ����� ������������ ������ � ��������� ������ �� ������������
		if( NULL != pTimestamp )
			*pTimestamp = TimeStamp;
		if( NULL != pTimestampMagnet )
			*pTimestampMagnet = MagLastTimeStamp;
		Result = true;
	} while( 0 );
	return Result;
}

// �������� �������� ������ MPU9250 � ������������ ��������� ����.
// ��������� ��� � ������������ � ����������� �������.
// ��������� ������� ��� MPU9250, ��� ��� � ������ IMU ������ ������� ������� ����� ������
static bool MPU9250_ConvertRaw2Bin( MPU9250_Frame_t * const pRaw, IMU_PacketBin_t *pBin )
{
	bool bResult = false;
	do
	{
		// �������� ����������
		if( ( NULL == pRaw ) && ( NULL == pBin ) )
			break;
		if( SIZEOFARRAY( pRaw->aAccel ) != SIZEOFARRAY( pBin->aAccel ) )
			break;
		// �������� ������������� IMU (� ���������� ����������� ���������� ������� ��������)
		if( IMU_Type_MPU9250 != IMU_Connected )
			break;
		IMU_Calibr_t * const pCalibr = &IMU_Calibr;

		// ��������� 3 ��� ������������ AK8963, ����������� � MPU9250, ��� ���������� � �������� ���� IMU
		IMU_Rotation90_t *pRot = &pCalibr->RotationMagnet;
		IMU_DataRawMagnet_t aMagnet[IMU_AXIS_COUNT];
		for( int i = 0; i < IMU_AXIS_COUNT; i++ )
			aMagnet[i]		= pRot->aMatrix[i][0] * pRaw->Magnet.aData[0]		+ pRot->aMatrix[i][1] *	pRaw->Magnet.aData[1]		+ pRot->aMatrix[i][2] *	pRaw->Magnet.aData[2];

		// ��������� ��� 9 ���� �������� ���������� IMU � ������� �������, ������������� ������ �����������
		pRot = &pCalibr->RotationIMU;
		for( int i = 0; i < IMU_AXIS_COUNT; i++ )
		{
			pBin->aAccel[i]	= pRot->aMatrix[i][0] * __REV16( pRaw->aAccel[0] )	+ pRot->aMatrix[i][1] * __REV16( pRaw->aAccel[1] )	+ pRot->aMatrix[i][2] * __REV16( pRaw->aAccel[2] );
			pBin->aGyro[i]	= pRot->aMatrix[i][0] * __REV16( pRaw->aGyro[0] )	+ pRot->aMatrix[i][1] * __REV16( pRaw->aGyro[1] )	+ pRot->aMatrix[i][2] * __REV16( pRaw->aGyro[2] );
			pBin->aMagnet[i]= pRot->aMatrix[i][0] * 			aMagnet[0]		+ pRot->aMatrix[i][1] * 			aMagnet[1]		+ pRot->aMatrix[i][2] * 			aMagnet[2];
		}

		// �������������� �����������
		pBin->Temp = __REV16( pRaw->Temp );
		// ��������� ������
		bResult = true;
	} while( 0 );
	return bResult;
}
#endif	// USE_MPU9250
// ****************************************************************************

#ifdef	USE_ICM20948
// ****************************************************************************
// ����������� ������� ��� IMU ICM-20948
// ****************************************************************************

// �������� ����� � ICM-20948
bool ICM20948_Connect( void )
{
	bool bResult = false;
	uint8_t Byte;

	GPIO_Common_Write( iGPIO_TestPinDir, GPIO_PIN_SET );
	for( int i = 0; i < IMU_RECALL_COUNT_MAX; i++ )
	{
		// ICM-20948 check connection
		if( HAL_OK != IMU_WriteByte( ICM20948_Address, ICMREG_20948_BANK_SEL, REG_BANK( ICMREG_20948_WHOAMI ) ) )
			continue;
		if( HAL_OK != IMU_ReadByte( ICM20948_Address, REG_ADDRESS( ICMREG_20948_WHOAMI ), &Byte ) )
			continue;
		if( ICM_WHOAMI_20948 != Byte )
			continue;
		bResult = true;
		break;
	}
	GPIO_Common_Write( iGPIO_TestPinDir, GPIO_PIN_RESET );

	return bResult;
}

// ���������������� ICM-20948 � ����� ��� ������ � ������� ������� ����������
bool ICM20948_Init( void )
{
	if( !bIMU_SerialInitComplete  )
		return false;
	// �������� ���� ����������� � IMU - ���� ������������� �������, �� ����� ������������
	IMU_Connected = IMU_Type_None;
	bool bResult = false;
	uint8_t Byte;

	GPIO_Common_Write( iGPIO_TestPinDir, GPIO_PIN_SET );
	// ��������������� ICM-20948
	for( int i = 0; i < IMU_RECALL_COUNT_MAX; i++ )
	{
		// ICM-20948 check connection
		// ## Set Bank #0
		if( HAL_OK != IMU_WriteByte( ICM20948_Address, ICMREG_20948_BANK_SEL, REG_BANK( ICMREG_20948_WHOAMI ) ) )
			continue;
		if( HAL_OK != IMU_ReadByte( ICM20948_Address, REG_ADDRESS( ICMREG_20948_WHOAMI ), &Byte ) )
			continue;
		if( ICM_WHOAMI_20948 != Byte )
			continue;
		// Reset
		if( HAL_OK != IMU_WriteByte( ICM20948_Address, REG_ADDRESS( ICMREG_20948_PWR_MGMT_1 ), ICM_BIT_PWR_MGMT_1_RESET ) )
			continue;
		HAL_Delay( 2 );
		// Enable Duty Cycle Mode for I2C Master, Accel and Gyro
		// Set clock source - auto
		// Enable Accel & Gyro
		uint8_t aPowerConfigConfig[] =
		{
			REG_ADDRESS( ICMREG_20948_LP_CONFIG ),
			0x70,																// ICMREG_20948_LP_CONFIG
			( ICM_BIT_PWR_MGMT_1_CLKSEL_MASK & 0x01 ),							// PWR_MGMT_1
			0,																	// PWR_MGMT_2
		};
		if( HAL_OK != IMU_WriteBuffer( ICM20948_Address, aPowerConfigConfig, sizeof( aPowerConfigConfig ) ) )
			continue;

		// ## Set Bank #2
		if( HAL_OK != IMU_WriteByte( ICM20948_Address, ICMREG_20948_BANK_SEL, REG_BANK( ICMREG_20948_GYRO_SMPLRT_DIV ) ) )
			continue;
		// Config Gyro Filter 200 Hz, Sample Rate 1.125 kHz (1.125 / ( 1 + 0 ) )
		// Config Gyro Range +-2000 DpS
		uint8_t aGyroConfig[] =
		{
			REG_ADDRESS( ICMREG_20948_GYRO_SMPLRT_DIV ),
			0,																	// GYRO_SMPLRT_DIV
			( ICM_BITS_GYRO_DLPF_CFG_197HZ | ICM_BITS_GYRO_FS_SEL_2000DPS ),	// GYRO_CONFIG_1
			0,																	// GYRO_CONFIG_2
		};
		if( HAL_OK != IMU_WriteBuffer( ICM20948_Address, aGyroConfig, sizeof( aGyroConfig ) ) )
			continue;
		// Config Accel Sample Rate 1.125 kHz (1.125 / ( 1 + 0 ) )
		uint8_t aAccelSampleRate[] =
		{
			REG_ADDRESS( ICMREG_20948_ACCEL_SMPLRT_DIV_1 ),
			0,																	// ACCEL_SMPLRT_DIV_1
			0,																	// ACCEL_SMPLRT_DIV_2
		};
		if( HAL_OK != IMU_WriteBuffer( ICM20948_Address, aAccelSampleRate, sizeof( aAccelSampleRate ) ) )
			continue;
		// Config Accel Filter 250 Hz, Sample Rate 1.125 kHz (1.125 / ( 1 + 0 ) )
		// Config Accel Range +-8 G, no decimation
		uint8_t aAccelConfig[] =
		{
			REG_ADDRESS( ICMREG_20948_ACCEL_CONFIG ),
			( ICM_BITS_ACCEL_DLPF_CFG_246HZ_1125 | ICM_BITS_ACCEL_FS_SEL_8G ),	// ACCEL_CONFIG
			0,																	// ACCEL_CONFIG_2
		};
		if( HAL_OK != IMU_WriteBuffer( ICM20948_Address, aAccelConfig, sizeof( aAccelConfig ) ) )
			continue;

		// Magnetometer Config
		// *******************
		// ## Set Bank #0
		if( HAL_OK != IMU_WriteByte( ICM20948_Address, ICMREG_20948_BANK_SEL, REG_BANK( ICMREG_20948_INT_PIN_CFG ) ) )
			continue;
		// Disable ICM-20948 I2C Master Mode, enable bypass to Magnetometer
		if( HAL_OK != IMU_WriteByte( ICM20948_Address, REG_ADDRESS( ICMREG_20948_INT_PIN_CFG ), ICM_BIT_INT_PIN_CFG_BYPASS_EN ) )
			continue;
		vTaskDelay( 3 );
		// Read Who Am I from Magnetometer
		uint8_t aMagnetWIA[2] = { 0 };
		if( HAL_OK != IMU_ReadBuffer( AK09916_Address, AK09916REG_WIA1, aMagnetWIA, sizeof( aMagnetWIA ) ) )
			continue;
		if( ( AK09916_DEVICE_ID_A != aMagnetWIA[0] ) || ( AK09916_DEVICE_ID_B != aMagnetWIA[1] ) )
			continue;
		// Set Magnetometer to 16 bit resolution, 100 Hz update rate (Mode 4)
		if( HAL_OK != IMU_WriteByte( AK09916_Address, AK09916REG_CNTL2, AK09916_CNTL2_CONTINOUS_MODE_100HZ ) )
			continue;

		// Setup Continues Read Sequence from Magnetometer
		// *******************
		// Enable ICM-20948 I2C Master Mode, disable INTs
		if( HAL_OK != IMU_WriteByte( ICM20948_Address, REG_ADDRESS( ICMREG_20948_INT_PIN_CFG ), ( 0 & ICM_BIT_INT_PIN_CFG_BYPASS_EN ) ) )
			continue;
		if( HAL_OK != IMU_WriteByte( ICM20948_Address, REG_ADDRESS( ICMREG_20948_USER_CTRL ), ( ICM_BIT_USER_CTRL_I2C_MST_ENABLE ) ) )
			continue;
		// ## Set Bank #3
		if( HAL_OK != IMU_WriteByte( ICM20948_Address, ICMREG_20948_BANK_SEL, REG_BANK( ICMREG_20948_I2C_MST_CTRL ) ) )
			continue;
		ICM20948_Frame_t IMU_Frame = { 0 };
		// Set I2C Master Speed to default 345 Hz (400 ??), disable delay
		uint8_t aI2CMasterConfig[] =
		{
			REG_ADDRESS( ICMREG_20948_I2C_MST_CTRL ),
			ICM_BITS_I2C_MST_CLOCK_400HZ,										// I2C_MST_CTRL
			0x80,																// I2C_MST_DELAY_CTRL
			0x80 | AK09916_I2C_ADDR,											// I2C_SLV0_ADDR
			AK09916REG_ST1,														// I2C_SLV0_REG
			0x80 | sizeof( IMU_Frame.Magnet ),									// I2C_SLV0_CTRL
		};
		if( HAL_OK != IMU_WriteBuffer( ICM20948_Address, aI2CMasterConfig, sizeof( aI2CMasterConfig ) ) )
			continue;
		// Wait for autoread from Magnetometer setup
		vTaskDelay( pdMS_TO_TICKS( 25 ) );
		// ## Set Bank #0
		if( HAL_OK != IMU_WriteByte( ICM20948_Address, ICMREG_20948_BANK_SEL, REG_BANK( ICMREG_20948_ACCEL_XOUT_H ) ) )
			continue;
		// Read full frame from ICM-20948
		if( HAL_OK != IMU_ReadBuffer( ICM20948_Address, REG_ADDRESS( ICMREG_20948_ACCEL_XOUT_H ), ( uint8_t * ) &IMU_Frame, sizeof( IMU_Frame ) ) )
			continue;
		// Check for Magnetometer data ready
		if( ( 0 == IMU_Frame.Magnet.aData[0] ) && ( 0 != IMU_Frame.Magnet.aData[1] ) && ( 0 != IMU_Frame.Magnet.aData[2] ) )
			continue;

		// Init Complete!
		bResult = true;
		IMU_Connected = IMU_Type_ICM20948;
		break;
	}
	GPIO_Common_Write( iGPIO_TestPinDir, GPIO_PIN_RESET );
	return bResult;
}

// ������� �� ICM-20948 �������� ������
static bool ICM20948_GetRaw( ICM20948_Frame_t *pRaw, TickType_t *pTimestamp, TickType_t *pTimestampMagnet )
{

	assert_param( NULL != pRaw );
	bool Result = false;
	do
	{
		// ��������� ������������� ����������������� ������
		if( !bIMU_SerialInitComplete )
			break;
		// ��������� ������������� ICM-20948
		if( IMU_Type_ICM20948 != IMU_Connected )
			break;
		// ������� ����� ����� ������
		// !! � ���������� ������������� ������ ���� ���������� ���������� ���� (BANK0), ����� ��� �� ��������������!
		if( HAL_OK != IMU_ReadBuffer( ICM20948_Address, REG_ADDRESS( ICMREG_20948_ACCEL_XOUT_H ), ( uint8_t * ) pRaw, sizeof( *pRaw ) ) )
			break;
		// ������ ����� ��������� ������
		TickType_t TimeStamp = xTaskGetTickCount( );

		// !!! ������ � AK8963!
		// ������ ����������� ������ � ������ ��������� �������� (�������� � ������ MPU9250 � ����������� I2C).
		// ��������� ���� �� �������������������� ����� � ����������� I2C,
		// �� ����� ������� ������, ��� � ����� �������� � ������ ����������������� ������ � MPU9250.
		static __no_init AK09916_Frame_t MagFrameSaved;
		static TickType_t MagLastTimeStamp = 0;
		GPIO_Common_Write( iGPIO_TestPinIMU, GPIO_PIN_SET );
		pRaw->Magnet.ST1 &= ~AK09916_ST1_DRDY;		// �������� ����������� ���� ���������� ������
		if( (	( pRaw->Magnet.aData[0] == MagFrameSaved.aData[0] ) &&
				( pRaw->Magnet.aData[1] == MagFrameSaved.aData[1] ) &&
				( pRaw->Magnet.aData[2] == MagFrameSaved.aData[2] ) ) ||
			( ( 0 == pRaw->Magnet.aData[0] ) && ( 0 == pRaw->Magnet.aData[1] ) && ( 0 == pRaw->Magnet.aData[2] ) ) )
		{	// ���������� ������ ������������ � ���������� ������, ��� ������� ������
			if( ( TimeStamp - MagLastTimeStamp ) > IMU_MAGNET_DATA_FREEZE_TIME_MAX_ms )
			{	// ������� ����� �� ������������ �� �������� ����� ������. ������� ������, �� ������� ����� ����������������� ������ � IMU
				break;
			}
		}
		else
		{	// ������� ����� � ������ ������� �� ������������
			pRaw->Magnet.ST1 |= AK09916_ST1_DRDY;
			// �������� ����� ������� ��������� ��������� ������ �� ������������
			MagLastTimeStamp = TimeStamp;
			GPIO_Common_Write( iGPIO_TestPinIMU, GPIO_PIN_RESET );
		}
		// ��������� ��������� �������� ����� �� ������������ ��� �������� ��������� ������
		MagFrameSaved = pRaw->Magnet;
		// ��������� ����� ������������ ������ � ��������� ������ �� ������������
		if( NULL != pTimestamp )
			*pTimestamp = TimeStamp;
		if( NULL != pTimestampMagnet )
			*pTimestampMagnet = MagLastTimeStamp;
		Result = true;
	} while( 0 );
	return Result;
}

// �������� �������� ������ ICM-20948 � ������������ ��������� ����.
// ��������� ��� � ������������ � ����������� �������.
// ��������� ������� ��� ICM-20948, ��� ��� � ������ IMU ������ ������� ������� ����� ������
static bool ICM20948_ConvertRaw2Bin( ICM20948_Frame_t * const pRaw, IMU_PacketBin_t *pBin )
{
	bool bResult = false;
	do
	{
		// �������� ����������
		if( ( NULL == pRaw ) && ( NULL == pBin ) )
			break;
		if( SIZEOFARRAY( pRaw->aAccel ) != SIZEOFARRAY( pBin->aAccel ) )
			break;
		// �������� ������������� IMU (� ���������� ����������� ���������� ������� ��������)
		if( IMU_Type_ICM20948 != IMU_Connected )
			break;
		IMU_Calibr_t * const pCalibr = &IMU_Calibr;

		// ��������� 3 ��� ������������ AK09916, ����������� � ICM-20948, ��� ���������� � �������� ���� IMU
		IMU_Rotation90_t *pRot = &pCalibr->RotationMagnet;
		IMU_DataRawMagnet_t aMagnet[IMU_AXIS_COUNT];
		for( int i = 0; i < IMU_AXIS_COUNT; i++ )
			aMagnet[i]		= pRot->aMatrix[i][0] * pRaw->Magnet.aData[0]		+ pRot->aMatrix[i][1] *	pRaw->Magnet.aData[1]		+ pRot->aMatrix[i][2] *	pRaw->Magnet.aData[2];

		// ��������� ��� 9 ���� �������� ���������� IMU � ������� �������, ������������� ������ �����������
		pRot = &pCalibr->RotationIMU;
		for( int i = 0; i < IMU_AXIS_COUNT; i++ )
		{
			pBin->aAccel[i]	= pRot->aMatrix[i][0] * __REV16( pRaw->aAccel[0] )	+ pRot->aMatrix[i][1] * __REV16( pRaw->aAccel[1] )	+ pRot->aMatrix[i][2] * __REV16( pRaw->aAccel[2] );
			pBin->aGyro[i]	= pRot->aMatrix[i][0] * __REV16( pRaw->aGyro[0] )	+ pRot->aMatrix[i][1] * __REV16( pRaw->aGyro[1] )	+ pRot->aMatrix[i][2] * __REV16( pRaw->aGyro[2] );
			pBin->aMagnet[i]= pRot->aMatrix[i][0] * 			aMagnet[0]		+ pRot->aMatrix[i][1] * 			aMagnet[1]		+ pRot->aMatrix[i][2] * 			aMagnet[2];
		}

		// �������������� �����������
		pBin->Temp = __REV16( pRaw->Temp );
		// ��������� ������
		bResult = true;
	} while( 0 );
	return bResult;
}

#endif	// USE_ICM20948

