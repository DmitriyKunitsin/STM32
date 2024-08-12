#ifndef __VLASOV_INCL_CALC_H__
#define __VLASOV_INCL_CALC_H__

#define _USE_MATH_DEFINES

#include <cmath>
#include <cstdint>

#define M_PI       3.14159265358979323846
#define M_PI_2     1.57079632679489661923
#define M_PI_4     0.785398163397448309616

		//Корректное округление float
		inline float Round(float number)
		{
			return number < 0.0 ? ceil(number - 0.5) : floor(number + 0.5);
		}

		//! Перевести градусы в радианы
		inline float Deg2RadF(const float angle){
			return (M_PI / 180) * angle;
		}

		//! Перевести радианы в градусы
		inline float Rad2DegF(const float rad){
			return rad * (180 / M_PI);
		}

		// Расчет грав отклонителя  от 0 до 2*PI, по двум компонентам акселерометров
		inline float CalcTFG(const int16_t AY, const int16_t AZ){
			// 4 -sin,5 -cos,
			if (AZ == 0){
				if (AY > 0){
					return M_PI_2;
				}
				else{
					return M_PI_2 + M_PI;
				}
			}

			float tan = AY / (float)AZ;
			float angel = atan(tan);
			if (AZ < 0){
				angel = M_PI + angel;
			}
			if (angel < 0){
				angel = 2 * M_PI + angel;
			}
			return angel;
		}

		// Расчет магнитного отклонителя по углам
		inline float CalcTFM(const int16_t HY, const  int16_t HZ){
			return CalcTFG(-HY, -HZ);
		}

		// Калибровочные коэффициенты
		typedef struct {
			float H_A[3][3];
			float H_B[3];
			float A_A[3][3];
			float A_B[3];
		}CalibrIncl;

		// Измеренные исходные данные магнитометров и акселерометров
		typedef struct {
			int16_t H[3]; //HX,HY,HZ
			int16_t A[3]; //AX,AY,AZ
		}MeasureValue;

		// Вычисление модуля трехмерного вектора ускорения
		inline uint16_t CalcG(const int16_t AX, const int16_t AY, const int16_t AZ){
			return (uint16_t)Round(sqrt((float)(AX*(int32_t)AX + AY*(int32_t)AY + AZ*(int32_t)AZ)));
		}

		// Вычисление модуля трехмерного вектора магнитного поля
		inline uint16_t CalcH(const int16_t HX, const int16_t HY, const int16_t HZ){
			return CalcG(HX, HY, HZ);
		}

		// Вычисление зенита
		inline float CalcZeni(const int16_t AX, const uint16_t G){
			if (G > 0)
			{
				float val = AX / (float)G;
				if (val < -1)
				{
					return M_PI;
				}
				if (val > 1)
				{
					return 0;
				}
				return acos(val);
			}
			else{
				return AX >= 0 ? 0 : M_PI;
			}
		}

		//Вычисление азимута
		inline float CalcAzim(const int16_t AX, const int16_t AY, const int16_t AZ, const int16_t HX, const int16_t HY, const int16_t HZ, const uint16_t G){
			float numerator = (HZ*(int32_t)AY - HY*(int32_t)AZ)*(float)G;
			long long denominator = (int64_t)HX*(AY*(int32_t)AY + AZ*(int32_t)AZ) - (int64_t)HY*(AY*(int32_t)AX) - (int64_t)HZ*(AX*(int32_t)AZ);

			if (denominator == 0){
				if (numerator > 0){
					return M_PI_2;
				}
				else{
					return M_PI_2 + M_PI;
				}
			}

			float angel = atan(numerator / denominator);

			if (denominator < 0){
				angel = M_PI + angel;
			}
			if (angel < 0){
				angel = 2 * M_PI + angel;
			}
			return angel;
		}

		//! Расчёт магнитного наклонения (угол входа магнитных линий в землю) 
		inline float CalcDip(const int16_t AX, const int16_t AY, const int16_t AZ, const int16_t HX, const int16_t HY, const int16_t HZ, const uint16_t G, const uint16_t B){
			if (G == 0 || B == 0){
				//Надоподумать что возвращать в этом случае
				return 0;
			}
			return asin((HX * (int32_t)AX + HY * (int32_t)AY + HZ * (int32_t)AZ) / (float)(G*(uint32_t)B));
		}

		// Значение калибровочных коэф. умолчательное
		/*inline CalibrIncl GetInitCalibrIncl(){
			const CalibrIncl ci{
				{ { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } },
				{ 0, 0, 0 },
				{ { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } },
				{ 0, 0, 0 }
			};
			return ci;
		}*/

		// Применение поправочных калибровочных коэффициентов к измеренным значениям
		inline void ApplyCalibrIncl(const CalibrIncl & ci, MeasureValue *val){
			MeasureValue tmp_val = *val;
			for (int i = 0; i < 3; i++){
				val->H[i] = (int16_t)Round(tmp_val.H[0] * ci.H_A[i][0] + tmp_val.H[1] * ci.H_A[i][1] + tmp_val.H[2] * ci.H_A[i][2] + ci.H_B[i]);
				val->A[i] = (int16_t)Round(tmp_val.A[0] * ci.A_A[i][0] + tmp_val.A[1] * ci.A_A[i][1] + tmp_val.A[2] * ci.A_A[i][2] + ci.A_B[i]);
			}
		}

		inline void CalcOnlyTFDeg(const MeasureValue& mv, uint16_t *tfg, uint16_t *tfm){
			*tfg = (uint16_t)Round(Rad2DegF(CalcTFG(mv.A[1], mv.A[2])) * 100);
			*tfm = (uint16_t)Round(Rad2DegF(CalcTFM(mv.H[1], mv.H[2])) * 100);
		}

		// расчет всех показателей инклинометрии за один вызов, углы хранятся в целых числах с точностью до 0.01 градуса
/*		inline void CalcAllInclDeg(const MeasureValue& mv, const uint16_t const_total_g, uint16_t *tfg, uint16_t *tfm, uint16_t *zeni, uint16_t* zeni_ax, uint16_t *azim, uint16_t *dip, uint16_t *total_b, uint16_t *total_g){
			CalcOnlyTFDeg(mv, tfg, tfm);
			*total_g = CalcG(mv.A[0], mv.A[1], mv.A[2]);
			*zeni = (uint16_t)Round(Rad2DegF(CalcZeni(mv.A[0], *total_g)) * 100);
			*zeni_ax = (uint16_t)Round(Rad2DegF(CalcZeni(mv.A[0], const_total_g)) * 100);
			*total_b = CalcH(mv.H[0], mv.H[1], mv.H[2]);
			*azim = (uint16_t)Round(Rad2DegF(CalcAzim(mv.A[0], mv.A[1], mv.A[2], mv.H[0], mv.H[1], mv.H[2], *total_g)) * 100);
			*dip = (uint16_t)Round(Rad2DegF(CalcDip(mv.A[0], mv.A[1], mv.A[2], mv.H[0], mv.H[1], mv.H[2], *total_g, *total_b)) * 100);
		}*/

		typedef struct {
			uint16_t tfg;		// Гравитационный отклонитель с точностью до 0.01 градуса
			uint16_t tfm;		// Магнитный отклонитель с точностью до 0.01 градуса
			uint16_t zeni;		// Зенитный угол нормированный на TotalG с точностью до 0.01 градуса
			uint16_t zeni_ax;	// Зенитный угол расчитанный только из продольного акселерометра с точностью до 0.01 градуса
			uint16_t azim;      // Азимутальный угол с точностью до 0.01 градуса
			uint16_t dip;       // Наклонение с точностью до 0.01 градуса
			uint16_t total_b;   // Модуль вектора магнитного поля в нанотеслах
			uint16_t total_g;   // Модуль вектора силы тяжести 10000 - g
		}InclData;

		// расчет всех показателей инклинометрии за один вызов, углы хранятся в целых числах с точностью до 0.01 градуса
		inline void CalcAllInclDeg(const MeasureValue& mv, const uint16_t const_total_g, InclData *incl_data){
			CalcOnlyTFDeg(mv, &(incl_data->tfg), &(incl_data->tfm));
			incl_data->total_g = CalcG(mv.A[0], mv.A[1], mv.A[2]);
			incl_data->zeni = (uint16_t)Round(Rad2DegF(CalcZeni(mv.A[0], incl_data->total_g)) * 100);
			incl_data->zeni_ax = (uint16_t)Round(Rad2DegF(CalcZeni(mv.A[0], const_total_g)) * 100);
			incl_data->total_b = CalcH(mv.H[0], mv.H[1], mv.H[2]);
			incl_data->azim = (uint16_t)Round(Rad2DegF(CalcAzim(mv.A[0], mv.A[1], mv.A[2], mv.H[0], mv.H[1], mv.H[2], incl_data->total_g)) * 100);
			incl_data->dip = (uint16_t)Round(Rad2DegF(CalcDip(mv.A[0], mv.A[1], mv.A[2], mv.H[0], mv.H[1], mv.H[2], incl_data->total_g, incl_data->total_b)) * 100);
		}

#endif