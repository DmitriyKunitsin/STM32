#pragma once
#include <cstdint>

namespace vl{

		//! Поиск наиболее подходящего индекса по массиву отсортированному по возрастанию
		/*!
			\tparam table_type тип массива
			\tparam search_type - тип элемента который будем искаться в таблице
		*/
		template<typename table_type, typename search_type> uint16_t FindIdxByTableUp(const uint16_t n,const table_type table[],const search_type &value){

			uint16_t first = 0, last = n - 1;
			
			if (value <= table[first])
				return first;
			if (value >= table[last])
				return last;
			

			while (last - first > 1) {
				int mid = (first + last) >> 1;

				if (value <= table[mid])
					last = mid;
				else
					first = mid;
			}
			if (value - table[first] < table[last] - value){
				return first;
			}
			else{
				return last;
			}
		}

		//! Поиск наиболее подходящего индекса по массиву отсортированному по убыванию
		template<typename table_type, typename search_type> uint16_t FindIdxByTableDown(const uint16_t n,const table_type table[],const search_type& value){
			uint16_t first = 0, last = n - 1;

			if (value >= table[first])
				return first;
			if (value <= table[last])
				return last;

			while (last - first > 1) {
				int mid = (first + last) >> 1;

				if (value >= table[mid])
					last = mid;
				else
					first = mid;
			}
			if (table[first] - value < value - table[last]){
				return first;
			}
			else{
				return last;
			}
		}

};
