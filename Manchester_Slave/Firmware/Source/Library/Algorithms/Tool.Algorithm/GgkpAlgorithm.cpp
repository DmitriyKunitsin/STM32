#include <math.h>
#include "GgkpAlgorithm.h"

#define EPS 0.000001

//Плотность пласта
const float CoeffRhob[] = {
	270.4231221321	,
	0.0170836003	,
	-0.0351918822	,
	0.7677826095	,
	-0.0018415549	,
	0.0001439078	,
	-0.0001817324	,
	0.0008476625	,
	0.0010498699	,
	0.0000166898	,
};

/*//Плотность мнимая
const float CoeffRhob[] = {
-4.53E-07,
8.40E-10,
-0.278472806,
1.278472806,
-2.53E-12,
9.32E-13,
-6.75E-13,
1.58E-12,
8.23E-13,
-6.49E-13,
};*/

const float CoeffOffset[] = {
	31.086594166754431,
	0.00025820005301913562,
	-0.026528723248770064,
	0.0026139393321147489,
	-2.8736033748109817E-05,
	1.2803148521029794E-05,
	-2.0319594841327057E-05,
	1.9187937495130969E-05,
	1.5739750388925713E-05,
	9.7383910066732612E-06,
};

//! Функция для вычисления аппроксимации неприжатия ГГКП
float CalcApproximationGgkp(const uint16_t _mud_rhob, const uint16_t x, const uint16_t y,const float param[]) {
	float mud_rhob = _mud_rhob * 0.001;
	float res= param[0] + param[1] * mud_rhob + param[2] * x + param[3] * y
		+ param[4] * x* (float)y + param[5] * mud_rhob * (float)y + param[6] * x * (float)mud_rhob
		+ param[7] * x *(float)x + param[8] * y * (float)y + param[9] * mud_rhob * (float)mud_rhob;
	return res;
}

uint16_t RoundingGgkp(const float val,const uint16_t mlt,const uint16_t min,const uint16_t max){
	int32_t var = (int32_t)((val* mlt) + 0.5);
	if (var < min) {
		return min;
	}
	if (var > max) {
		return max;
	}
	return (uint16_t)var;
}

void CalcCorrectGgkp(const uint16_t mud_rhob,const uint16_t rhob_short,const uint16_t rhob_long, uint16_t* rhob_true, uint8_t* offset) {
	// Плотность измеряется от 0 до 4000 гк/см3 с шагои 1 кг/см3
	*rhob_true = RoundingGgkp(CalcApproximationGgkp(mud_rhob, rhob_short, rhob_long, CoeffRhob),1,0,4000);
	// StandOff  от 0 до 25.5 мм с шагои 0.1 мм
	*offset= RoundingGgkp(CalcApproximationGgkp(mud_rhob, rhob_short, rhob_long, CoeffOffset), 10, 0, 255);
}

void CalcTotalCountSimple(const uint8_t n, const GgkpInitSector init_sectors[], GgkpInitSector* init_total) {
	init_total->c_long = 0;
	init_total->c_short = 0;
	init_total->time = 0;
	for (uint8_t i = 0; i < n; i++) {
		init_total->c_long += init_sectors[i].c_long;
		init_total->c_short += init_sectors[i].c_short;
		init_total->time += init_sectors[i].time;
	}
}

void CalcCountInterval(const uint16_t count, const uint16_t c_source, const uint16_t time,	uint16_t *min, uint16_t *max) {
	//Надо рассписать на листочке
}

uint16_t NormalizationCount(const uint16_t counts,const uint16_t time, const uint16_t c_source_calibr, const uint16_t c_source_work) {
	if (c_source_calibr == 0 || c_source_work == 0)
		return 0;
	return (((counts*(uint32_t)c_source_calibr)*(uint64_t)1000) / (c_source_work*(uint32_t)time)) ;
}

//Вычисление отношения
float GgkpCountDivision(const uint16_t c_short, const uint16_t c_long) {
	if (c_short == 0)
		return 0;
	if (c_short < c_long)
		return 1;
	return c_long / (float) c_short;
}

uint16_t CalcCalibrFunction(const float val, const GgkpCalibrStd &calibr) {
	if (val < EPS)
		return (uint16_t)0;
	return (uint16_t)((calibr.Q - calibr.A * log10f(val*calibr.C))+0.5);
}

void CalcDensityAndOffset(const uint8_t n, const GgkpInitSector init_sectors[],
	const GgkpCalibr &calibr, const GgkpWorkCoeff &calibr_work, GgkpCalcSector* calc_sectors) {
	for (uint8_t i = 0; i < n; i++) {
		calc_sectors[i].density = 
			CalcCalibrFunction(GgkpCountDivision(init_sectors[i].c_short, init_sectors[i].c_long), calibr.rhob);
		calc_sectors[i].density_short = 
			CalcCalibrFunction(NormalizationCount(init_sectors[i].c_short, init_sectors[i].time, calibr.c_source_calibr,calibr_work.c_source_work),calibr.rhob_short);
		calc_sectors[i].density_long =
			CalcCalibrFunction(NormalizationCount(init_sectors[i].c_long, init_sectors[i].time, calibr.c_source_calibr, calibr_work.c_source_work), calibr.rhob_long);
		CalcCorrectGgkp(calibr_work.rhob_mud, calc_sectors[i].density_short, calc_sectors[i].density_long, &(calc_sectors[i].density_true), &(calc_sectors[i].StandOff));
	}
}

/*void CalcDensityAndOffsetInterval(const uint8_t n, const GgkpInitSector init_sectors[],
	const GgkpCalibr &calibr, const GgkpWorkCoeff &calibr_work, GgkpCalcSector* min_calc_sectors, GgkpCalcSector* calc_sectors, GgkpCalcSector* max_calc_sectors) {
	const uint8_t sigma_rule = 2;
	for (int i = 0; i < n; i++) {
		uint16_t min_c_short, max_c_short, min_c_long, max_c_long;
		{
			uint8_t eps_short = (uint8_t)(sqrtf(init_sectors[i].c_short)+0.5);
		}
	}
}*/