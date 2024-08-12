// AHRS.h
// ������ ���������� �� ������ ������ � IMU
// AHRS	- Attitude and heading reference system	(��������������)
// IMU	- Inertial measurement unit				(����������������)

#ifndef	AHRS_H
#define	AHRS_H

#include <stdbool.h>

// ���������
#define	IMU_AXES					3			// X, Y, Z	���������� ���� �������� IMU, ��������� ��� �������� ���������
#define M_PI						3.14159f	// ����� ��
#ifndef	AHRS_PICH_ASIN_SINGULAR
#define	AHRS_PICH_ASIN_SINGULAR		0.997f		// asin(85.6) - ������, ��� ������� ���� �������� �������������� �� ����������� �� �����
#endif	// AHRS_PICH_ASIN_SINGULAR
#define	AHRS_DIR_QUAT_DEFAULT		{ 1.0f, 0.0f, 0.0f, 0.0f }	// ���������� ����������� ��-��������� (�������������, �� �����)

// ��������� ������
// *******************************************
// !!! ��������� �������������� ��� ����������!
// !!! ��� ������������� ���������� ������ � ������ ����������,
// !!! ���������� �������������� � ������������ ���������!
// (����, ��� ��������� ������� �� float, � ������ ���� ������������� �������������...)

// ��������� ��������������� ������ �������� IMU, ���������� �� Float � ����������� ��������
typedef struct IMU_DataNormal_struct
{
	float aAccel[IMU_AXES];		// [12]	[g]			���������
	float aGyro[IMU_AXES];		// [12]	[rad/s]	�������� ��������
	float aMagnet[IMU_AXES];	// [12]	[uT]		�������� ���������� ����
} IMU_DataNormal_t;			// [36]

// ���������� ��� ������������� ����������, ���������� �� Float
typedef union Quaternion_union
{
	float aQ[4];
	struct
	{
		float Q0;
		float Q1;
		float Q2;
		float Q3;
	};
	struct
	{
		float QW;
		float QX;
		float QY;
		float QZ;
	};
} Quaternion_t;		// [16]

typedef Quaternion_t DirQuat_t;

// ������������ ���� ������, ���������� �� Float, � ��������
typedef union DirAngEuler_union
{
	float aEuler[3];
	struct
	{
		float Roll;		// -180..+180	����						0 - �� ����		(Inclin.TF)
		float Pitch;	//  -90.. +90	������						0 - �� ��������	(Inclin.ZENI)	+90 - �����
		float Yaw;		// -180..+180	��������					0 - �� �����	(Inclin.AZIM)
		// �� ������������ �������������� (|Pitch| > 85), Roll ������������� ��� ������ (0 ������������ ������)
	};
} DirAngEuler_t;		// [12]

// ������������ ���� ������������������, ���������� �� Float, � ��������
typedef struct DirAngInclin_struct
{
	union
	{
		float aInclin[4];
		struct
		{
			float ZENI; // 0..+180		�������� ����				0 - ����		(Euler.Pitch)	+180 - �����
			float AZIM; // 0..+360		������������ ����			0 - �� �����	(Euler.Yaw)
			float TFG;	// 0..+360		����������� ��������������	0 - ������		(Euler.Roll)
			float TFM;	// 0..+360		����������� ���������		0 - �� �����	(Euler.Roll @ |Pitch| > 85)
		};
	};
	bool bTFG_nTFM;		// �������� ����������: true - TFG, false - TFM
} DirAngInclin_t;		//[20]

// �������� �������
void AHRS_Config( float Beta, float SampleRate );							// ���������� ��������� �������
void AHRS_Update( DirQuat_t *pDirQuat, IMU_DataNormal_t * const pIMU );		// ������ ����� ���������� �� ����� ������ ������� � ���������� ����������
void AHRS_Quat2Angles( DirQuat_t *const pQuat, DirAngEuler_t *pAngEuler, DirAngInclin_t *pAngInclin, float RotateRoll );	// �������������� ����������� ����������� � ������������ ����

#endif	// AHRS_H

