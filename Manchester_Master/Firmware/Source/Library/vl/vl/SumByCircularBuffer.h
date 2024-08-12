#ifndef __VLASOV_LOOCH_UTIL_SUM_BY_CIRCULAR_BUFFER_H__
#define __VLASOV_LOOCH_UTIL_SUM_BY_CIRCULAR_BUFFER_H__

#include <cstdint>
#include <cstring>

namespace vl{

#define MIN(a,b) ((a)<(b) ? (a) : (b))

		/**
			\brief Циклический буффер c автоматической суммой

			\tparam SIZE_CBUF размер циклического буфера
			\tparam base_type тип значений циклического буфера
			\tparam sum_type  тип суммы значений
			*/
		template <int SIZE_CBUF, typename base_type, typename sum_type > class SumByCircularBuffer{

		public:
                  enum {SIZE_CBUF_VAL = SIZE_CBUF};
			inline SumByCircularBuffer(){
				Init();
			}
			//! Выполнить начальную инициализации
			inline void Init(){
				memset(buf, 0, sizeof(base_type)*SIZE_CBUF);
				idx_buf = 0;
				sum = 0;
                                sz = 0;
			}
			//! Добавить значение в циклический буфер
			inline void Add(base_type value){
				sum -= buf[idx_buf];
				sum += value;
				buf[idx_buf] = value;
				idx_buf++;
				idx_buf %= SIZE_CBUF;
                                sz = MIN((sz + 1), SIZE_CBUF);
			}

                        inline int GetSize() const {
                          return sz;
                        }

			//! Получить сумму
			inline sum_type GetSum()const{
				return sum;
			}
			//! Получить среднее значение циклического буфера
			inline base_type GetAvg()const{
				//Компилятор должен оптимизировать этот учаток кода на сдвиг для целочисленных операций кратных степени двойки
				return (sum / SIZE_CBUF);
			}

			//! Получить значение из середины
			inline base_type GetMidValue()const{
				return buf[(idx_buf + (SIZE_CBUF >> 1)) % SIZE_CBUF];
			}

			inline base_type * GetBuf() { return buf; }
			inline const base_type * GetBuf() const { return buf; }

			//! Получить размер циклического буффера
			inline uint16_t Size()const{
				return SIZE_CBUF;
			}
			//! Получить отсортированный массив по событиям (надо тестить этот код)
			inline void GetSortBuffer(base_type* buf_sort)const{
				memcpy(buf_sort, buf + idx_buf, (SIZE_CBUF - (idx_buf))*sizeof(base_type));
				memcpy(buf_sort + SIZE_CBUF - (idx_buf), buf, idx_buf*sizeof(base_type));
			}
		private:
			base_type buf[SIZE_CBUF];   //!< Массив для циклического буфера
			uint16_t idx_buf;			//!< Текущий индекс в циклическом буфере
			sum_type sum;				//!< Текущая накопленная сумма в циклическом буфере
                        int sz;
		};


		//! Циклический буфер для float
		template < int size > class SumByCircularBufferFloat :public SumByCircularBuffer < size, float, float>{};
		//! Циклический буфер для int16
		template < int size > class SumByCircularBufferInt16 :public SumByCircularBuffer < size, int16_t, int32_t>{};
		//! Циклический буфер для uint16
		template < int size > class SumByCircularBufferUInt16 :public SumByCircularBuffer < size, uint16_t, uint32_t>{};
		//! Циклический буфер для int32
		template < int size > class SumByCircularBufferInt32 :public SumByCircularBuffer < size, int32_t, int64_t>{};
		//! Циклический буфер для uint32
		template < int size > class SumByCircularBufferUInt32 :public SumByCircularBuffer < size, uint32_t, uint64_t>{};

};


#endif