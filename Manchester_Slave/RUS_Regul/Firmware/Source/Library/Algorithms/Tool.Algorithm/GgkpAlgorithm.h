#ifndef __GGKP_ALGORITHM_H__
#define __GGKP_ALGORITHM_H__

#include "Tool.Algorithm.h"

////////////////////// Калибровочные (настроечные) коэффицинты /////////////////

//Калибровочные коэффициенты для зависимсотей Q-A*log10(x*C)
typedef struct {
	float Q, A, C;
} GgkpCalibrStd;

//! Параметры калибровки
typedef struct {
	uint16_t c_source_calibr; //Мощность источника при калибровке 10^7, 0.95 -> 950
	GgkpCalibrStd rhob;       //Калибровочные коэффициенты для расчета плотности по отношению короткого/длинного зонда
	GgkpCalibrStd rhob_short; //Калибровочные коэффициенты для расчета плотности по короткому зонда
	GgkpCalibrStd rhob_long;  //Калибровочные коэффициенты для расчета плотности по длинному зонду
} GgkpCalibr;

//! Параметры для работы 
typedef struct {
	uint16_t rhob_mud;			//! Плотность бурового раствора кг/м3
	uint16_t c_source_work;		//! Мощность источника при измерении  
} GgkpWorkCoeff;

//! Эталонные счета на образце, для вычисления мощности источника через эксперемент
typedef struct {
	uint16_t c_short_etalon;    //! Счет на эталонном образце короткого зонда
	uint16_t c_long_etalon;     //! Счет на эталонном образце длинного зонда
} GgkpEtalonCount;

////////////////////// Структуры данных ////////////////////////////////////////

//! Зарегестрированная информация по каждому сектору ГГКП
typedef struct {
	uint16_t c_short; //!Счет в импульсах на коротком зонде (ближнем)
	uint16_t c_long;  //!Счет в импульсах на длинном зонде  (дальнем)
	uint16_t time;    //!Время измерения в секторе в милисекундах
} GgkpInitSector;

//! Структура расчетных велечин по каждому сектору
typedef struct {
	uint16_t density;		  //! Плотность посчитанная по отношению, 3 знака после запятой
	uint16_t density_short;   //! Плотность по короткому зонду
	uint16_t density_long;    //! Плотность по дальнему зонду
	uint16_t density_true;    //! Плотность скорректированная на не прижатие
	uint8_t  StandOff;        //! Не прижатие в 0.1 mm;
} GgkpCalcSector;

//! Аппроксимация через A*(sin(x+W))+B (не использовать)
//! Маска на 8 секторов, восстанавливаем все 16 секторов, применяем папарно
/*typedef struct {
	uint8_t A, B, W, mask;
}SinApproximation;

//! Кодирование интервалом (не использовать)
typedef struct {
	uint8_t interval; //первая тетрада, начало, вторая
	uint8_t density[8];
}IntervalApproximation;*/


//! Вычисления счета нормализованного на мощность источника и время измеренеия имп/сек 
extern "C" EXPORT_EMF_CORE uint16_t NormalizationCount(const uint16_t counts, const uint16_t time, const uint16_t c_source_calibr, const uint16_t c_source_work);

//! Расчет калибровочной функции ГГКП
/// @ingroup ggkp
/// @param[in] val				- зарегестрированное значение
/// @param[in] calibr			- калибровочные коэффициенты 
extern "C" EXPORT_EMF_CORE uint16_t CalcCalibrFunction(const float val, const GgkpCalibrStd &calibr);

//! Расчет поправленной плотности на неприжатие
/// @ingroup ggkp
/// @param[in] mud_rhob			- плотность бурового раствора, кг/м3
/// @param[in] rhob_short		- плотность по короткому зонду, кг/м3
/// @param[in] rhob_long		- плотность по длинному зонду, кг/м3
/// @param[out] rhob_true		- плотность по короткому зонду, кг/м3
/// @param[out] offset			- плотность по длинному зонду, 0.1 мм
extern "C" EXPORT_EMF_CORE void CalcCorrectGgkp(const uint16_t mud_rhob,const uint16_t rhob_short,const uint16_t rhob_long, uint16_t* rhob_true, uint8_t* offset);

//! Расчет итоговых счетов, просто суммирование
/// @ingroup ggkp
/// @param[in] n				- количество азимутальных секторов 
/// @param[in] init_sectors[n]  - зарегестрированные сигналы
/// @param[out] init_total      - сумарные сигналы
extern "C" EXPORT_EMF_CORE void CalcTotalCountSimple(const uint8_t n, const GgkpInitSector init_sectors[], GgkpInitSector* init_total);

//! Расчет плотности и не прижатия для секторов
/// @ingroup ggkp
/// @param[in] n				- количество азимутальных секторов 
/// @param[in] init_sectors[n]  - зарегестрированные сигналы
/// @param[in] calibr			- калибровочные коэффициенты
/// @param[in] calibr_work		- параметры бурения
/// @param[out] calc_sectors[n]	- Расчетные параметры
extern "C" EXPORT_EMF_CORE void CalcDensityAndOffset(const uint8_t n, const GgkpInitSector init_sectors[],
	const GgkpCalibr &calibr, const GgkpWorkCoeff &calibr_work, GgkpCalcSector* calc_sectors);

/*extern "C" EXPORT_EMF_CORE void CalcDensityAndOffsetInterval(const uint8_t n, const GgkpInitSector init_sectors[],
	const GgkpCalibr &calibr, const GgkpWorkCoeff &calibr_work, GgkpCalcSector* min_calc_sectors, GgkpCalcSector* calc_sectors, GgkpCalcSector* max_calc_sectors);*/

//!Расчет плотности бурового раствора
//bool CalcSinApproximation(uint8_t n, GgkpCalcSector* calc_sectors, SinApproximation *approximation);


#endif