// VIKPB_EVENTS.h
// **************************************************************************************************************************************

#ifndef	VIKPB_EVENTS_H
#define	VIKPB_EVENTS_H

#include "FreeRTOS.h"

#include "Event_groups.h"

// ***************************************************
// Флаги событий в группе VIKPB_EventGroup
// ***************************************************


#define EVENTS_RUS_REGUL_ADC1_FIRSTHALFBUFFER_READY			( 1 << 1 )		// Флаг готовности первой половины буфера оцифровки
#define EVENTS_RUS_REGUL_ADC1_SECONDHALFBUFFER_READY		( 1 << 2 )		// Флаг готовности второй половины буфера оцифровки
#define EVENTS_RUS_REGUL_TIMER_SPEED_COUNT_X_READY			( 1 << 3 )		// Флаг окончания счета таймера (каждый Х мс)
#define EVENTS_RUS_REGUL_TIMER_SPEED_COUNT_Y_READY			( 1 << 4 )		// Флаг окончания счета таймера (каждый Y мс)
#define EVENTS_RUS_REGUL_TIMER_SPEED_COUNT_Z_READY			( 1 << 5 )		// Флаг окончания счета таймера (каждый Z мс)
#define EVENTS_RUS_REGUL_SERVICETIMER						( 1 << 6 )		// Флаг чего-то!!!! (Таймер контроля уарта наверное)
#define EVENTS_RUS_REGUL_START_DELAY						( 1 << 7 )		// Флаг начала задержки на что-то!!!
#define EVENTS_RUS_REGUL_STATE_TELE							( 1 << 8 )		// Флаг смены состояния обмена с контроллером телемтрии
#define EVENTS_RUS_REGUL_MOTOR_POWER_ON						( 1 << 9 )		// Флаг состояния питания на моторе
#define EVENTS_RUS_REGUL_MOTOR_STATE						( 1 << 10 )		// Флаг состояния мотора
#define EVENTS_RUS_REGUL_ADS1231_READY						( 1 << 11 )		// Флаг готовности внешнего АЦП к опросу
#define EVENTS_RUS_REGUL_OPAMP_VALUE_READY					( 1 << 12 )		// Флаг готовности считывания значения потребления с операционного усилителя
#define EVENTS_RUS_REGUL_CALIBR_OPAMP_START					( 1 << 13 )		// Флаг инициирования процедуры поиска нулевого положения
#define EVENTS_RUS_REGUL_CALIBR_PRESS_START					( 1 << 14 )		// Фалг инициирования калибровки по давлению
#define EVENTS_RUS_REGUL_CALIBR_PRESS_PROCESS				( 1 << 15 )		// Флаг готовности начать калибровку по давлению
#define EVENTS_RUS_REGUL_HOLD_PRESS							( 1 << 16 )		// Флаг выставления режима удержания давления
#define EVENTS_RUS_REGUL_PROCESS_CALLBACK					( 1 << 17 )		// Флаг обработки колбека TIM2
#define EVENTS_RUS_REGUL_CURRENT_CHECK						( 1 << 18 )		// Событие для отслеживания потребления раз в 5 мс. Программный токоограничитель
//#define EVENTS_RUS_REGUL_DEBUG_OUTPUT						( 1 << 19 )		// Только в режиме дебуг!!!!!!!!





#define	EVENT_RUS_REGUL_EVENTS_ALL		(EVENTS_RUS_REGUL_ADC1_FIRSTHALFBUFFER_READY | \
										EVENTS_RUS_REGUL_ADC1_SECONDHALFBUFFER_READY | \
										EVENTS_RUS_REGUL_TIMER_SPEED_COUNT_X_READY   | \
										EVENTS_RUS_REGUL_TIMER_SPEED_COUNT_Y_READY   | \
										EVENTS_RUS_REGUL_TIMER_SPEED_COUNT_Z_READY   | \
										EVENTS_RUS_REGUL_SERVICETIMER 				 | \
										EVENTS_RUS_REGUL_START_DELAY 				 | \
										EVENTS_RUS_REGUL_STATE_TELE					 | \
										EVENTS_RUS_REGUL_MOTOR_POWER_ON 			 | \
										EVENTS_RUS_REGUL_MOTOR_STATE 			 	 | \
										EVENTS_RUS_REGUL_ADS1231_READY				 | \
										EVENTS_RUS_REGUL_OPAMP_VALUE_READY			 | \
										EVENTS_RUS_REGUL_CALIBR_OPAMP_START			 | \
										EVENTS_RUS_REGUL_CALIBR_PRESS_START			 | \
										EVENTS_RUS_REGUL_CALIBR_PRESS_PROCESS		 | \
										EVENTS_RUS_REGUL_HOLD_PRESS					 | \
										EVENTS_RUS_REGUL_PROCESS_CALLBACK			 | \
										EVENTS_RUS_REGUL_CURRENT_CHECK)				// | \
										//EVENTS_RUS_REGUL_DEBUG_OUTPUT)


// Группа событий, используемаЯ во внутренних модулЯх и основном цикле
extern EventGroupHandle_t RUS_Regul_EventGroup;

#endif	

