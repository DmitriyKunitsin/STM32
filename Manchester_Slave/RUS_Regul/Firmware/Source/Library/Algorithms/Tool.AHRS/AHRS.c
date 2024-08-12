// AHRS.c
// Расчет ориентации на основе данных с акселерометров, гироскопов и магнетометров
#include "ProjectConfig.h"			// конфиг платформы
#include "stm32xxxx_hal.h"			// дрова периферии
#include "AHRS.h"
#include <math.h>

// Параметры фильтрации по-умолчанию
#define	AHRS_SAMPLE_RATE_DEFAULT		50.0f		// [Гц]	частота обновлениЯ
#define	AHRS_BETA_DEFAULT				0.5f		// постоЯннаЯ времени

// РеализациЯ фильтра Мэджвика и переменные длЯ обмена данными с ним
static void MadgwickAHRSupdate( float gx, float gy, float gz, float ax, float ay, float az, float mx, float my, float mz );
static __no_init float q0, q1, q2, q3;
static float beta = AHRS_BETA_DEFAULT;
static float sampleFreq = AHRS_SAMPLE_RATE_DEFAULT;

// Установить параметры фильтра
void AHRS_Config( float Beta, float SampleRate )
{
	beta = Beta;
	sampleFreq = SampleRate;
}

// Расчет новой ориентации по новым данным датчика и предыдущей оринетации
// Вызов фильтра Мэджвика
void AHRS_Update( DirQuat_t *pDirQuat, IMU_DataNormal_t *pIMU )
{
	assert_param( ( NULL != pDirQuat ) && ( NULL != pIMU ) );
	q0 = pDirQuat->aQ[0];
	q1 = pDirQuat->aQ[1];
	q2 = pDirQuat->aQ[2];
	q3 = pDirQuat->aQ[3];
	MadgwickAHRSupdate(
		pIMU->aGyro[0],		pIMU->aGyro[1],		pIMU->aGyro[2],
		pIMU->aAccel[0],	pIMU->aAccel[1],	pIMU->aAccel[2],
		pIMU->aMagnet[0],	pIMU->aMagnet[1],	pIMU->aMagnet[2] );
	pDirQuat->aQ[0] = q0;
	pDirQuat->aQ[1] = q1;
	pDirQuat->aQ[2] = q2;
	pDirQuat->aQ[3] = q3;
}

// Инверсный квадратный корень длЯ фильтра Мэджвика.
// ПодразумеваетсЯ, что вычисление обеспечиваетсЯ достаточно "тЯжелой" процедурой,
// и здесь должен быть реализован алгорим быстрого приблизительного вычислениЯ.
// Однако Ядро Cortex-M4 поддерживает инструкцию длЯ вычислениЯ квадратного корнЯ,
// так что специальных алгоритмов не требуетсЯ.
static float invSqrt( float x )
{
	return 1.0f / sqrtf( x );
}

// РеализациЯ фильтра Мэджвика, расчет кватерниона ориентации по предыдущей ориентации и новым данным с датчиков.
// ОригинальнаЯ статьЯ http://www.x-io.co.uk/res/doc/madgwick_internal_report.pdf
// Перевод статьи https://habr.com/post/255661/
// Код цельнотЯнут с http://www.x-io.co.uk/res/sw/madgwick_algorithm_c.zip
// Апдейт с http://diydrones.com/forum/topics/madgwick-imu-ahrs-and-fast-inverse-square-root?id=705844%3ATopic%3A1018435&page=4#comments
// Аргументы:
// - g[3]		[rad/s]	скорость оборотов
// - a[3]		[g]		ускорение
// - m[3]		[uT]	индукциЯ магнитного полЯ
// Используемые данные:
// - q[4]		[+-1]	кватернион ориентации
// - sampleFreq [Hz]	частота вызова этой функции
// - beta		[?]		?постоЯннаЯ времени фильтра
// ПринЯтое направление по-умолчанию (нулевое):
// - все углы Эйлера (Roll, Pitch, Yaw) нулевые - "горизонтально, на север, килем вверх";
// - кватернион { 1, 0, 0, 0 };
// - ориентациЯ осей IMU - X на север, Y на запад, Z вверх.
// !!! У типового "самолетного" AHRS оси направлены иначе - X на север, Y на восток, Z вниз.
// !!! ДлЯ того, чтобы углы Эйлера соответсвовали типовым (Roll положительный при крене направо,
// !!! Pitch положительный при тангаже вверх, Yaw положительный при рысканье вправо),
// !!! в алгоритме расчета углов AHRS_Quat2Angles() углы Pitch и Yaw инвертируютсЯ.
static void MadgwickAHRSupdate( float gx, float gy, float gz, float ax, float ay, float az, float mx, float my, float mz )
{
	float recipNorm;
	float s0, s1, s2, s3;
	float qDot1, qDot2, qDot3, qDot4;
	float hx, hy;
	float _2q0mx, _2q0my, _2q0mz, _2q1mx, _2bx, _2bz, _4bx, _4bz, _2q0, _2q1, _2q2, _2q3, _2q0q2, _2q2q3, q0q0, q0q1, q0q2, q0q3, q1q1, q1q2, q1q3, q2q2, q2q3, q3q3;

	// Use IMU algorithm if magnetometer measurement invalid (avoids NaN in magnetometer normalisation)
	if((mx == 0.0f) && (my == 0.0f) && (mz == 0.0f)) {
//		MadgwickAHRSupdateIMU(gx, gy, gz, ax, ay, az);
		// !!! Магнитометр не должен возвращать нули, но такое случаетсЯ.
		// !!! Необходимо разбиратьсЯ с работой магнитометра, чтобы при нулЯх до AHRSupdate() вообще не доходило,
		// !!! или действительно использовать MadgwickAHRSupdateIMU().
		// !!! Пока что assert() убираю.
//!!!		assert_param( 0 );
		return;
	}

	// Rate of change of quaternion from gyroscope
	qDot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
	qDot2 = 0.5f * (q0 * gx + q2 * gz - q3 * gy);
	qDot3 = 0.5f * (q0 * gy - q1 * gz + q3 * gx);
	qDot4 = 0.5f * (q0 * gz + q1 * gy - q2 * gx);

	// Compute feedback only if accelerometer measurement valid (avoids NaN in accelerometer normalisation)
	if(!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {

		// Normalise accelerometer measurement
		recipNorm = invSqrt(ax * ax + ay * ay + az * az);
		ax *= recipNorm;
		ay *= recipNorm;
		az *= recipNorm;

		// Normalise magnetometer measurement
		recipNorm = invSqrt(mx * mx + my * my + mz * mz);
		mx *= recipNorm;
		my *= recipNorm;
		mz *= recipNorm;

		// Auxiliary variables to avoid repeated arithmetic
		_2q0mx = 2.0f * q0 * mx;
		_2q0my = 2.0f * q0 * my;
		_2q0mz = 2.0f * q0 * mz;
		_2q1mx = 2.0f * q1 * mx;
		_2q0 = 2.0f * q0;
		_2q1 = 2.0f * q1;
		_2q2 = 2.0f * q2;
		_2q3 = 2.0f * q3;
		_2q0q2 = 2.0f * q0 * q2;
		_2q2q3 = 2.0f * q2 * q3;
		q0q0 = q0 * q0;
		q0q1 = q0 * q1;
		q0q2 = q0 * q2;
		q0q3 = q0 * q3;
		q1q1 = q1 * q1;
		q1q2 = q1 * q2;
		q1q3 = q1 * q3;
		q2q2 = q2 * q2;
		q2q3 = q2 * q3;
		q3q3 = q3 * q3;

		// Reference direction of Earth's magnetic field
		hx = mx * q0q0 - _2q0my * q3 + _2q0mz * q2 + mx * q1q1 + _2q1 * my * q2 + _2q1 * mz * q3 - mx * q2q2 - mx * q3q3;
		hy = _2q0mx * q3 + my * q0q0 - _2q0mz * q1 + _2q1mx * q2 - my * q1q1 + my * q2q2 + _2q2 * mz * q3 - my * q3q3;
		_2bx = sqrtf(hx * hx + hy * hy);
		_2bz = -_2q0mx * q2 + _2q0my * q1 + mz * q0q0 + _2q1mx * q3 - mz * q1q1 + _2q2 * my * q3 - mz * q2q2 + mz * q3q3;
		_4bx = 2.0f * _2bx;
		_4bz = 2.0f * _2bz;

/*		// Gradient decent algorithm corrective step
		s0 = -_2q2 * (2.0f * q1q3 - _2q0q2 - ax) + _2q1 * (2.0f * q0q1 + _2q2q3 - ay) - _2bz * q2 * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (-_2bx * q3 + _2bz * q1) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + _2bx * q2 * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
		s1 = _2q3 * (2.0f * q1q3 - _2q0q2 - ax) + _2q0 * (2.0f * q0q1 + _2q2q3 - ay) - 4.0f * q1 * (1 - 2.0f * q1q1 - 2.0f * q2q2 - az) + _2bz * q3 * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (_2bx * q2 + _2bz * q0) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + (_2bx * q3 - _4bz * q1) * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
		s2 = -_2q0 * (2.0f * q1q3 - _2q0q2 - ax) + _2q3 * (2.0f * q0q1 + _2q2q3 - ay) - 4.0f * q2 * (1 - 2.0f * q1q1 - 2.0f * q2q2 - az) + (-_4bx * q2 - _2bz * q0) * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (_2bx * q1 + _2bz * q3) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + (_2bx * q0 - _4bz * q2) * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
		s3 = _2q1 * (2.0f * q1q3 - _2q0q2 - ax) + _2q2 * (2.0f * q0q1 + _2q2q3 - ay) + (-_4bx * q3 + _2bz * q1) * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (-_2bx * q0 + _2bz * q2) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + _2bx * q1 * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
*/
		// Gradient decent algorithm corrective step
		// Original algoritm fix from http://diydrones.com/forum/topics/madgwick-imu-ahrs-and-fast-inverse-square-root?id=705844%3ATopic%3A1018435&page=4#comments
		float _8bz = 4.0f * _2bz;
		float _8bx = 4.0f * _2bx;
		( void ) _2q0q2;
		( void ) _2q2q3;
		s0 = -_2q2 * ( 2.0f * ( q1q3 - q0q2 ) - ax ) + _2q1 * ( 2.0f * ( q0q1 + q2q3 ) - ay ) + -_4bz * q2 * ( _4bx * ( 0.5f - q2q2 - q3q3 ) + _4bz * ( q1q3 - q0q2 ) - mx ) + ( -_4bx * q3 + _4bz * q1 ) * ( _4bx * ( q1q2 - q0q3 ) + _4bz * ( q0q1 + q2q3 ) - my ) + _4bx * q2 * ( _4bx * ( q0q2 + q1q3 ) + _4bz * ( 0.5f - q1q1 - q2q2 ) - mz );
		s1 = _2q3 * ( 2.0f * ( q1q3 - q0q2 ) - ax ) + _2q0 *( 2.0f * ( q0q1 + q2q3 ) - ay ) + -4.0f * q1 * ( 2.0f * ( 0.5f - q1q1 - q2q2 ) - az ) + _4bz * q3 * ( _4bx * ( 0.5f - q2q2 - q3q3 ) + _4bz * ( q1q3 - q0q2 ) - mx ) + ( _4bx * q2 + _4bz * q0 ) * ( _4bx * ( q1q2 - q0q3 ) + _4bz * ( q0q1 + q2q3 ) - my ) + ( _4bx * q3 - _8bz * q1 ) * ( _4bx * ( q0q2 + q1q3 ) + _4bz * ( 0.5f - q1q1 - q2q2 ) - mz ); 
		s2 = -_2q0 * ( 2.0f * ( q1q3 - q0q2 ) - ax ) + _2q3 *( 2.0f * ( q0q1 + q2q3 ) - ay ) + ( -4.0f * q2 ) * ( 2.0f * ( 0.5f - q1q1 - q2q2 ) - az ) + ( -_8bx * q2 - _4bz * q0 ) * ( _4bx * ( 0.5f - q2q2 - q3q3 ) + _4bz * ( q1q3 - q0q2 ) - mx ) + ( _4bx * q1 + _4bz * q3 ) * ( _4bx * ( q1q2 - q0q3 ) + _4bz * ( q0q1 + q2q3 ) - my ) + ( _4bx * q0 - _8bz * q2 ) * ( _4bx * ( q0q2 + q1q3 ) + _4bz *( 0.5f - q1q1 - q2q2 ) - mz );
		s3 = _2q1 * ( 2.0f * ( q1q3 - q0q2 ) - ax ) + _2q2 * ( 2.0f * ( q0q1 + q2q3 ) - ay ) + ( -_8bx * q3 + _4bz * q1 ) * ( _4bx * ( 0.5f - q2q2 - q3q3 ) + _4bz * ( q1q3 - q0q2 ) - mx ) + ( -_4bx * q0 + _4bz * q2 ) * ( _4bx * ( q1q2 - q0q3 ) + _4bz * ( q0q1 + q2q3 ) - my ) + ( _4bx * q1 ) * ( _4bx * ( q0q2 + q1q3 ) + _4bz * ( 0.5f - q1q1 - q2q2 ) - mz );

		recipNorm = invSqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3); // normalise step magnitude
		s0 *= recipNorm;
		s1 *= recipNorm;
		s2 *= recipNorm;
		s3 *= recipNorm;

		// Apply feedback step
		qDot1 -= beta * s0;
		qDot2 -= beta * s1;
		qDot3 -= beta * s2;
		qDot4 -= beta * s3;
	}

	// Integrate rate of change of quaternion to yield quaternion
/*	q0 += qDot1 * (1.0f / sampleFreq);
	q1 += qDot2 * (1.0f / sampleFreq);
	q2 += qDot3 * (1.0f / sampleFreq);
	q3 += qDot4 * (1.0f / sampleFreq);
*/
	float SamplePeriod = 1.0f / sampleFreq;
	q0 += qDot1 * SamplePeriod;
	q1 += qDot2 * SamplePeriod;
	q2 += qDot3 * SamplePeriod;
	q3 += qDot4 * SamplePeriod;

	// Normalise quaternion
	recipNorm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
	q0 *= recipNorm;
	q1 *= recipNorm;
	q2 *= recipNorm;
	q3 *= recipNorm;
}

// Обрезка длинных углов, чтобы попадали в отведенный диапазон
static float AngleCutoff_Deg180180( float Angle )
{
	return ( ( Angle > 180.0f ) ? ( Angle - 360.0f ) : ( ( Angle < -180.0f ) ? ( Angle + 360.0f ) : Angle ) );
}

static float AngleCutoff_Deg0360( float Angle )
{
	return ( ( Angle >= 0.0f ) ? ( Angle ) : ( Angle + 360.0f ) );
}

// Преобразование кватерниона направлениЯ в дирекционные углы
// *pQuat		- кватернион направлениЯ
// *pAngEuler	- дирекционные углы Эйлера (может быть NULL, если не нужны)
// *pAngInclin	- дирекционные углы инклинометрические (может быть NULL, если не нужны)
void AHRS_Quat2Angles( DirQuat_t * const pQuat, DirAngEuler_t *pAngEuler, DirAngInclin_t *pAngInclin, float RotateRoll )
{
	assert_param( NULL != pQuat );

	// ****************************************
	// Произвести расчет углов Эйлера (даже если требуютсЯ только инклинометрические углы)
	// ****************************************
	float Roll, Pitch, Yaw, RollSingular;
	bool bRollSingularity;

	// Roll (TFG), -180..+180
	float sinr = 		+2.0f * ( pQuat->QW * pQuat->QX + pQuat->QY * pQuat->QZ );
	float cosr = +1.0f - 2.0f * ( pQuat->QX * pQuat->QX + pQuat->QY * pQuat->QY );
	Roll = atan2f( sinr, cosr ) * ( 180.0f / M_PI );
	// КоррекциЯ крена длЯ компенсации поворота платы с датчиком относительно нулЯ прибора
	Roll = AngleCutoff_Deg180180( Roll + RotateRoll );

	// Pitch (ZENI), -90..+90
	float sinp = 		+2.0f * ( pQuat->QW * pQuat->QY - pQuat->QZ * pQuat->QX );
	float sinp_abs = fabsf( sinp );
	bRollSingularity = ( ( sinp_abs > AHRS_PICH_ASIN_SINGULAR ) ? true : false );
	if( sinp_abs >= 1.0f )
    {
        if( sinp > 0.0f )
        	Pitch = M_PI / 2.0f;
        else
        	Pitch = - M_PI / 2.0f;
    }
    else
		Pitch = asinf( sinp ) * ( 180.0f / M_PI );

	// Yaw (AZIM), -180..+180
	float siny =		+2.0f * ( pQuat->QW * pQuat->QZ + pQuat->QX * pQuat->QY );
	float cosy = +1.0f - 2.0f * ( pQuat->QY * pQuat->QY + pQuat->QZ * pQuat->QZ );
	Yaw = atan2f( siny, cosy ) * ( 180.0f / M_PI );

	// Roll on Singulariry (TFM), -180..+180
	RollSingular = 2.0f * atan2f( pQuat->QX, pQuat->QW ) * ( 180.0f / M_PI );
	// КоррекциЯ крена длЯ компенсации поворота платы с датчиком относительно нулЯ прибора
	RollSingular = AngleCutoff_Deg180180( RollSingular + RotateRoll );

	// Углы инвертируютсЯ из-за несоответствиЯ типовой "самолетной" ориентации осей
	// и принЯтой ориентации длЯ алгортма, реализованного в MadgwickAHRSupdate() -
	// см. коммент перед MadgwickAHRSupdate().
	Yaw		= -Yaw;
	Pitch	= -Pitch;
	// (типа, отмазалсЯ ))

	if( NULL != pAngEuler )
	{
		// ****************************************
		// Вернуть углы Эйлера
		// ****************************************
		pAngEuler->Roll		= ( bRollSingularity ? RollSingular : Roll );
		pAngEuler->Pitch	= Pitch;
		pAngEuler->Yaw		= Yaw;
	}

	if( NULL != pAngInclin )
	{
		// ****************************************
		// Преобразовать в инклинометрические углы и вернуть
		// ****************************************
		pAngInclin->ZENI	= Pitch + 90.0f;
		pAngInclin->AZIM	= AngleCutoff_Deg0360( Yaw );
		pAngInclin->TFG		= AngleCutoff_Deg0360( Roll );
		pAngInclin->TFM 	= AngleCutoff_Deg0360( ( Pitch < 0 ) ? RollSingular : RollSingular + 180.0f );
		pAngInclin->bTFG_nTFM = !bRollSingularity;
	}
}

