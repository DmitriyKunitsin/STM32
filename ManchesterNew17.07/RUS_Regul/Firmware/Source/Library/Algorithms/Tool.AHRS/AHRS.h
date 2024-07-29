// AHRS.h
// –асчет ориентации на основе данных с IMU
// AHRS	- Attitude and heading reference system	( урсовертикаль)
// IMU	- Inertial measurement unit				(√иростабилизатор)

#ifndef	AHRS_H
#define	AHRS_H

#include <stdbool.h>

//  онстанты
#define	IMU_AXES					3			// X, Y, Z	количество осей датчиков IMU, требуемых для расчетов орентации
#define M_PI						3.14159f	// число ѕи
#ifndef	AHRS_PICH_ASIN_SINGULAR
#define	AHRS_PICH_ASIN_SINGULAR		0.997f		// asin(85.6) - тангаж, при котором крен начинает рассчитываться по направлению на север
#endif	// AHRS_PICH_ASIN_SINGULAR
#define	AHRS_DIR_QUAT_DEFAULT		{ 1.0f, 0.0f, 0.0f, 0.0f }	// кватернион направления по-умолчанию (горизонтально, на север)

// —труктуры данных
// *******************************************
// !!! —труктуры оптимизированы для вычислений!
// !!! ѕри необходимости передавать данные в другие устройства,
// !!! необходимо конвертировать в запакованные структуры!
// (хотя, все структуры состоят из float, и должны быть автоматически запакованными...)

// —труктура нормализованных данных датчиков IMU, реализация на Float в натуральных единицах
typedef struct IMU_DataNormal_struct
{
	float aAccel[IMU_AXES];		// [12]	[g]			ускорение
	float aGyro[IMU_AXES];		// [12]	[rad/s]	скорость оборотов
	float aMagnet[IMU_AXES];	// [12]	[uT]		индукция магнитного поля
} IMU_DataNormal_t;			// [36]

//  ватернион для представления ориентации, реализация на Float
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

// ƒирекционные углы Ёйлера, реализация на Float, в градусах
typedef union DirAngEuler_union
{
	float aEuler[3];
	struct
	{
		float Roll;		// -180..+180	крен						0 - на киле		(Inclin.TF)
		float Pitch;	//  -90.. +90	тангаж						0 - на горизонт	(Inclin.ZENI)	+90 - вверх
		float Yaw;		// -180..+180	рысканье					0 - на север	(Inclin.AZIM)
		// Ќа вертикальных сингулярностях (|Pitch| > 85), Roll рассчитывется как азимут (0 соответсвует северу)
	};
} DirAngEuler_t;		// [12]

// ƒирекционные углы инклинометрические, реализация на Float, в градусах
typedef struct DirAngInclin_struct
{
	union
	{
		float aInclin[4];
		struct
		{
			float ZENI; // 0..+180		зенитный угол				0 - вниз		(Euler.Pitch)	+180 - вверх
			float AZIM; // 0..+360		азимутальный угол			0 - на север	(Euler.Yaw)
			float TFG;	// 0..+360		отклонитель гравитационный	0 - наверх		(Euler.Roll)
			float TFM;	// 0..+360		отклонитель магнитный		0 - на север	(Euler.Roll @ |Pitch| > 85)
		};
	};
	bool bTFG_nTFM;		// јктивный отлонитель: true - TFG, false - TFM
} DirAngInclin_t;		//[20]

// ќписание функций
void AHRS_Config( float Beta, float SampleRate );							// ”становить параметры фильтра
void AHRS_Update( DirQuat_t *pDirQuat, IMU_DataNormal_t * const pIMU );		// –асчет новой ориентации по новым данным датчика и предыдущей оринетации
void AHRS_Quat2Angles( DirQuat_t *const pQuat, DirAngEuler_t *pAngEuler, DirAngInclin_t *pAngInclin, float RotateRoll );	// ѕреобразование кватерниона направления в дирекционные углы

#endif	// AHRS_H

