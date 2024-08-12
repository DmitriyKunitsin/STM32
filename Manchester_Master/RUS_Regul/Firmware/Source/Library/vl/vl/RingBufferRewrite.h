#ifndef __VLASOV_LOOCH_UTIL_RING_BUFFER_REWRITE_H__
#define __VLASOV_LOOCH_UTIL_RING_BUFFER_REWRITE_H__

#include <cstdint>

namespace vl
{

		/*
			Циклический буффер записывающий данные по кругу
			param size_step2 - степень двойки размера буфера накопления
			param base_type  - тип значений циклического буфера
			TODO нужно добавить копирование в массив
			*/
		template <int SIZE_CBUF, typename base_type> class RingBufferRewrite{
		public:
			inline RingBufferRewrite(){
				Init();
			}

			inline void Init(){
				memset(buf, 0, sizeof(base_type)*SIZE_CBUF);
				idx = Size()-1;
			}			

			inline void Write(const base_type &value){
				idx++;
				idx %= Size();
				buf[idx] = value;
			}

			//! Получить размер циклического буффера
			inline uint16_t Size()const{
				return (uint16_t)SIZE_CBUF;
			}

			inline base_type& operator[](const uint16_t idx_from_last)const {
				return ((base_type*)this)[(idx + Size() - idx_from_last) % Size()];
			}

			inline void CopyToArray(base_type* arr)const {
				memcpy(arr, buf+idx+1, sizeof(base_type)*(Size()-idx-1));
				memcpy(arr+ Size() - idx -1, buf, sizeof(base_type)*(idx + 1));
			}

			/*
			inline base_type LastValue()const{
				return buf[idx];
			}

			inline base_type LastValue(const uint16_t idx_from_last)const{
				return buf[(idx + Size() - idx_from_last) % Size()];
			}*/

		private:
			base_type buf[SIZE_CBUF];   //циклический буфер с углами
			uint16_t idx;	    //текущий индекс в циклическом буфере
		};

		//! Циклический буффер с максимальным размером 0xFFFF байт
		//template < int size > class RingBufferUChar :public RingBuffer< size, uint8_t>{};

};


#endif