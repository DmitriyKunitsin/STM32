#ifndef __GGKP_ALGORITHM_H__
#define __GGKP_ALGORITHM_H__

#include "Tool.Algorithm.h"

////////////////////// ������������� (�����������) ����������� /////////////////

//������������� ������������ ��� ������������ Q-A*log10(x*C)
typedef struct {
	float Q, A, C;
} GgkpCalibrStd;

//! ��������� ����������
typedef struct {
	uint16_t c_source_calibr; //�������� ��������� ��� ���������� 10^7, 0.95 -> 950
	GgkpCalibrStd rhob;       //������������� ������������ ��� ������� ��������� �� ��������� ���������/�������� �����
	GgkpCalibrStd rhob_short; //������������� ������������ ��� ������� ��������� �� ��������� �����
	GgkpCalibrStd rhob_long;  //������������� ������������ ��� ������� ��������� �� �������� �����
} GgkpCalibr;

//! ��������� ��� ������ 
typedef struct {
	uint16_t rhob_mud;			//! ��������� �������� �������� ��/�3
	uint16_t c_source_work;		//! �������� ��������� ��� ���������  
} GgkpWorkCoeff;

//! ��������� ����� �� �������, ��� ���������� �������� ��������� ����� �����������
typedef struct {
	uint16_t c_short_etalon;    //! ���� �� ��������� ������� ��������� �����
	uint16_t c_long_etalon;     //! ���� �� ��������� ������� �������� �����
} GgkpEtalonCount;

////////////////////// ��������� ������ ////////////////////////////////////////

//! ������������������ ���������� �� ������� ������� ����
typedef struct {
	uint16_t c_short; //!���� � ��������� �� �������� ����� (�������)
	uint16_t c_long;  //!���� � ��������� �� ������� �����  (�������)
	uint16_t time;    //!����� ��������� � ������� � ������������
} GgkpInitSector;

//! ��������� ��������� ������� �� ������� �������
typedef struct {
	uint16_t density;		  //! ��������� ����������� �� ���������, 3 ����� ����� �������
	uint16_t density_short;   //! ��������� �� ��������� �����
	uint16_t density_long;    //! ��������� �� �������� �����
	uint16_t density_true;    //! ��������� ����������������� �� �� ��������
	uint8_t  StandOff;        //! �� �������� � 0.1 mm;
} GgkpCalcSector;

//! ������������� ����� A*(sin(x+W))+B (�� ������������)
//! ����� �� 8 ��������, ��������������� ��� 16 ��������, ��������� �������
/*typedef struct {
	uint8_t A, B, W, mask;
}SinApproximation;

//! ����������� ���������� (�� ������������)
typedef struct {
	uint8_t interval; //������ �������, ������, ������
	uint8_t density[8];
}IntervalApproximation;*/


//! ���������� ����� ���������������� �� �������� ��������� � ����� ���������� ���/��� 
extern "C" EXPORT_EMF_CORE uint16_t NormalizationCount(const uint16_t counts, const uint16_t time, const uint16_t c_source_calibr, const uint16_t c_source_work);

//! ������ ������������� ������� ����
/// @ingroup ggkp
/// @param[in] val				- ������������������ ��������
/// @param[in] calibr			- ������������� ������������ 
extern "C" EXPORT_EMF_CORE uint16_t CalcCalibrFunction(const float val, const GgkpCalibrStd &calibr);

//! ������ ������������ ��������� �� ����������
/// @ingroup ggkp
/// @param[in] mud_rhob			- ��������� �������� ��������, ��/�3
/// @param[in] rhob_short		- ��������� �� ��������� �����, ��/�3
/// @param[in] rhob_long		- ��������� �� �������� �����, ��/�3
/// @param[out] rhob_true		- ��������� �� ��������� �����, ��/�3
/// @param[out] offset			- ��������� �� �������� �����, 0.1 ��
extern "C" EXPORT_EMF_CORE void CalcCorrectGgkp(const uint16_t mud_rhob,const uint16_t rhob_short,const uint16_t rhob_long, uint16_t* rhob_true, uint8_t* offset);

//! ������ �������� ������, ������ ������������
/// @ingroup ggkp
/// @param[in] n				- ���������� ������������ �������� 
/// @param[in] init_sectors[n]  - ������������������ �������
/// @param[out] init_total      - �������� �������
extern "C" EXPORT_EMF_CORE void CalcTotalCountSimple(const uint8_t n, const GgkpInitSector init_sectors[], GgkpInitSector* init_total);

//! ������ ��������� � �� �������� ��� ��������
/// @ingroup ggkp
/// @param[in] n				- ���������� ������������ �������� 
/// @param[in] init_sectors[n]  - ������������������ �������
/// @param[in] calibr			- ������������� ������������
/// @param[in] calibr_work		- ��������� �������
/// @param[out] calc_sectors[n]	- ��������� ���������
extern "C" EXPORT_EMF_CORE void CalcDensityAndOffset(const uint8_t n, const GgkpInitSector init_sectors[],
	const GgkpCalibr &calibr, const GgkpWorkCoeff &calibr_work, GgkpCalcSector* calc_sectors);

/*extern "C" EXPORT_EMF_CORE void CalcDensityAndOffsetInterval(const uint8_t n, const GgkpInitSector init_sectors[],
	const GgkpCalibr &calibr, const GgkpWorkCoeff &calibr_work, GgkpCalcSector* min_calc_sectors, GgkpCalcSector* calc_sectors, GgkpCalcSector* max_calc_sectors);*/

//!������ ��������� �������� ��������
//bool CalcSinApproximation(uint8_t n, GgkpCalcSector* calc_sectors, SinApproximation *approximation);


#endif