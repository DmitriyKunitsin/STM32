// RUS_Regul_Main.c
// Основной алгоритм работы плат регуляторов
#include "ProjectConfig.h"			// конфиг платформы
#include "stm32xxxx_hal.h"			// дрова периферии
#include "stm32l4xx_ll_tim.h"
#include "platform_common.h"
#include "common_gpio.h"
#include "common_rcc.h"				// DWT_TimerGet()
#include "RebootUtils.h"
#include "RUS_Regul_Common.h"
//#include "Logger.h"					// Logger_WriteRecord()
//#include "MathUtils.h"				// DIVIDE()
#include "Utils.h"					// CalcCRC16SKLP()
//#include "NVM.h"
//#include "eeprom_emul.h"
//#include <string.h>

// Отладочный вывод
#include "common_uart.h"			// HAL_UART_Ext_Transmit()
#include <stdio.h>					// snprintf()
#include "RUS_Regul_Events.h"
#include "DRIVER_Motor.h"
#include "RUS_Regul_Main.h"
#include "Driver_ADS1231.h"
#include "RUS_Regul_ADC.h"
//#include "SKLP_Interface_RUS_Regul.h"

static void WatchDog_TimerCallback( TimerHandle_t xTimer );
static void RUS_Regul_ServiceTimerCallback( TimerHandle_t xTimer );
static void RUS_Regul_StartDelayTimerCallback( TimerHandle_t xTimer );

static void RUS_Regul_MainTask (void *pParameters);

EventGroupHandle_t RUS_Regul_EventGroup;
TimerHandle_t WatchDog_xTimer;
RUS_Regul_t RUS_Regul;


#if defined(DEBUG_OUTPUT)
Debug_Output_t debug_output = {'S', 'T', 0};

static uint8_t ReadyCNT = 0;

static void Debug_Timer_Callback(TimerHandle_t pTimer)
{
	assert_param( NULL != RUS_Regul_EventGroup );
	xEventGroupSetBits( RUS_Regul_EventGroup, EVENTS_RUS_REGUL_DEBUG_OUTPUT );
}

void Debug_Timer_Start(void){
	TimerHandle_t DebugTimer = xTimerCreate("DebugTimer", pdMS_TO_TICKS(100.0f), pdTRUE, NULL, Debug_Timer_Callback);
	assert_param(NULL != DebugTimer);
	assert_param( pdPASS == xTimerStart(DebugTimer, 0) );
}

#endif

extern RTOS_Timers_t Timers;
extern Motor_Main_t Motor;

const RUS_Regul_Calibr_Press_t RUS_Regul_Press_CalibrDefault = { {0}, 0, 1, 0 };		// единичный калибровочный коэффициент по-умолчанию
const RUS_Regul_Calibr_Motor_t RUS_Regul_Motor_CalibrDefault = { {0}, 1, 100};			// 1 А потребления и 100% максимальная скорость по умолчанию

const NVM_Tag_t NVM_Tag_RUS_Regul_PressCalibr	= {	EEPROM_PRESS_CALIBR,	sizeof( Motor.Settings ),	&Motor.Settings,	&RUS_Regul_Press_CalibrDefault,		NULL,	NULL };
const NVM_Tag_t NVM_Tag_RUS_Regul_MotorCalibr	= {	EEPROM_MOTOR_CALIBR,	sizeof( Motor.Settings ),	&Motor.Settings,	&RUS_Regul_Motor_CalibrDefault,		NULL,	NULL };


void Motor_Press_Init(void)
{
	ADS1231_Init();
	assert_param( NVM_Result_Ok == NVM_TagLoad(&NVM_Tag_RUS_Regul_PressCalibr) );
	assert_param( NVM_Result_Ok == NVM_TagLoad(&NVM_Tag_RUS_Regul_MotorCalibr) );
}

bool RUS_Regul_TaskInit(void)
{
	assert_param(RUS_Regul_SKLP_InitCallbacks());
	assert_param(taskSCHEDULER_NOT_STARTED == xTaskGetSchedulerState());

	// Создать задачу
	TaskHandle_t RUS_Regul_MainTaskHandle;
	if( pdTRUE != xTaskCreate( RUS_Regul_MainTask, TASK_REGUL_MAIN_NAME, TASK_REGUL_MAIN_STACK_SIZE, NULL, TASK_REGUL_MAIN_PRIORITY, &RUS_Regul_MainTaskHandle ) )
		return false;
	if( NULL == RUS_Regul_MainTaskHandle )
		return false;

	// Инициализировать таймер RTOS, однократный таймер
	RUS_Regul.xStartDelayTimer = xTimerCreate( "REGUL_StartDelay", (TickType_t)( RUS_REGUL_STARTDELAY1_MS ), pdFALSE, NULL, RUS_Regul_StartDelayTimerCallback );
	assert_param( NULL != RUS_Regul.xStartDelayTimer );
	// Запустить таймер RTOS
	assert_param( pdFAIL != xTimerStart( RUS_Regul.xStartDelayTimer, 0 ) );

	// Инициализировать хендлер событий
	if( NULL == ( RUS_Regul_EventGroup = xEventGroupCreate( ) ) )
		return false;
	xEventGroupClearBits( RUS_Regul_EventGroup, EVENT_RUS_REGUL_EVENTS_ALL );
		
	return true;
	
}

void RUS_Regul_Motor_State_Get(void){
	RUS_Regul.SpeedSet = Motor.SpeedSet;
	RUS_Regul.SpeedMeas = Motor.SpeedMeas;
	RUS_Regul.AngleSet = Motor.AngleSet;
	RUS_Regul.CurPosition = RUS_Regul_Motor_Angle_Get();
	RUS_Regul.longCurPosition = RUS_Regul_Motor_Long_Angle_Get();
	RUS_Regul.TimerCNTValue = LL_TIM_GetCounter(SPEED_COUNTER_TIM);
	RUS_Regul.Press = Motor.Press;
	RUS_Regul.MotorPowerSupply = ADC_VSUP_Value_Get();
	RUS_Regul.MotorCurrent = ADC_OPAMP_Value_Get();
	RUS_Regul.CalibrNull = Motor.CalibrNull;
	RUS_Regul.MinAngle = Motor.MinAngle;
	RUS_Regul.MaxAngle = Motor.MaxAngle;
	RUS_Regul.MinPress = Motor.MinPress;
	RUS_Regul.MaxPress = Motor.MaxPress;
	RUS_Regul.WorkZone = Motor.WorkZone;
	RUS_Regul.WorkMode = Motor.WorkMode;
	RUS_Regul.WorkState = Motor.WorkState;
	RUS_Regul.ErrorFlag = Motor.ErrorFlag;
	RUS_Regul.LogicalFlag = Motor.LogicalFlag;
}

void RUS_Regul_Main_Struct_Update(void){
	RUS_Regul_Motor_State_Get();
}

void EX_RUS_Regul_Motor_Speed_Set(float speed_percent)
{
	Motor.WorkMode = Ready;
	if(speed_percent > 0){
		Motor_Rotation_Set(Clockwise);
		Motor_Speed_Set(speed_percent);
	}
	else if(speed_percent < 0){
		Motor_Rotation_Set(CounterClockWise);
		Motor_Speed_Set(-speed_percent);
	}
	else if(0 == speed_percent){
		RUS_Regul_Motor_Stop();
	}
}

void EX_RUS_Regul_Motor_Angle_Set(int16_t angle, float speed_percent)
{
	if(0 == angle)
	{
		Motor_Stop();
	}
	else if(angle > 0)
	{
		Motor_Rotation_Set(Clockwise);
		if( angle <= 30 ){
			speed_percent = 30;
			Motor_Angle_Set( angle, speed_percent);
		}
		else{
			Motor_Angle_Set(angle, speed_percent);
		}
	}
	else if(angle < 0)
	{
		Motor_Rotation_Set(CounterClockWise);
		if( (-angle) <= 30){
			speed_percent = 30;
			Motor_Angle_Set( (-angle), speed_percent);
		}
		else{
			Motor_Angle_Set( (-angle), speed_percent);
		}
	}	
		
}

// Функция выставления скорости мотора. Реализуется с спомощью выставления необходимой скважности ШИМ. 
int RUS_Regul_Motor_Speed_Set(float speed_percent)
{
	if( (Motor.WorkMode == HoldingPressure) || (Motor.WorkMode == CalibratingConsumption) )
	{
		Motor_Speed_Set(speed_percent);
	}
	else
	{
		if(speed_percent > 0){
			Motor_Rotation_Set(Clockwise);
			Motor_Speed_Set(speed_percent);
		}
		if(speed_percent < 0){
			Motor_Rotation_Set(CounterClockWise);	
			Motor_Speed_Set(-speed_percent);
		}
		if(speed_percent == 0){
			RUS_Regul_Motor_Stop();
		}
	}
	return(0);
}

void RUS_Regul_Motor_Stop(void){
	Motor_Stop();
}

void RUS_Regul_Absolute_Angle_Set(int16_t angle, float speed_percent)
{	
	if(angle == 0){
		RUS_Regul_Motor_Stop();
		if(Motor.fSetCalNullPos == 1){
			LL_TIM_SetCounter(SPEED_COUNTER_TIM, 0);
			Motor.fSetCalNullPos = 0;
			Motor.fCalibrStart = 0;
			Motor.fCalibrated = 1;
			Motor.WorkMode = Ready;
		}
	}
	
	else if(angle < 0)
	{
		Motor_Rotation_Set(CounterClockWise);
		if( (-angle) <= 30 )
		{
			speed_percent = 30.0f;
			Motor_Angle_Set( (-angle), speed_percent);
		}
		else
		{
			Motor_Angle_Set( (-angle), speed_percent);
		}
	}
	
	else if(angle > 0)
	{
		Motor_Rotation_Set(Clockwise);
		if(angle <= 30)
		{
			speed_percent = 30.0f;
			Motor_Angle_Set(angle, speed_percent);
		}
		else
		{
			Motor_Angle_Set(angle, speed_percent);
		}
	}

}

float RUS_Regul_Motor_Angle_Get(void){

	static uint32_t curCount;
	static float curAngle;

	
	curCount = LL_TIM_GetCounter(SPEED_COUNTER_TIM);
	curAngle = (curCount % PULSE_PER_ONE_TURN) * (360.0f / PULSE_PER_ONE_TURN);
	
	Motor.CurPosition = curAngle;

	return(Motor.CurPosition);
}

float RUS_Regul_Motor_Long_Angle_Get(void){
	static uint32_t curCount;
	static float  longCurAngle;
	
	curCount = LL_TIM_GetCounter(SPEED_COUNTER_TIM);
	longCurAngle = curCount * 360.0f / PULSE_PER_ONE_TURN;
	
	Motor.longCurPosition = longCurAngle;

	return(Motor.longCurPosition);

}

void RUS_Regul_Motor_Calibration_Set(void)
{
	Motor_Rotation_Set(Clockwise);
	Motor_Calibration_Current_Start();
}

void RUS_Regul_Press_Calibration_Start(void)
{
	Motor_Calibration_Press_Start();
}

void RUS_Regul_Overlap_Set(int16_t percent)
{
	static int16_t prevAngle = 0;
	int16_t targetAngle = (Motor.WorkZone * percent / 100);
	int16_t angle;
	float speed;
	if(0 == Motor.fReverseLogic){
		if(targetAngle > prevAngle){
			Motor_Rotation_Set(Clockwise);
			angle = targetAngle - prevAngle;
			if((targetAngle - prevAngle) <= 30){
				speed = MIN_SPEED;
			}
			else{
				speed = MAX_SPEED;
			}
		}
		else if(targetAngle < prevAngle){
			Motor_Rotation_Set(CounterClockWise);
			angle = prevAngle - targetAngle;
			if((prevAngle - targetAngle) <= 30){
				speed = MIN_SPEED;
			}
			else{
				speed = MAX_SPEED;
			}
		}
		else if(targetAngle == prevAngle){
			angle = 0;
			speed = 0.0f;
		}
	}
	else{
		if(targetAngle > prevAngle){
			Motor_Rotation_Set(CounterClockWise);
			angle = targetAngle - prevAngle;
			if((targetAngle - prevAngle) <= 30){
				speed = MIN_SPEED;			
			}
			else{
				speed = MAX_SPEED;
			}
		}
		else if(targetAngle < prevAngle){
			Motor_Rotation_Set(Clockwise);
			angle = prevAngle - targetAngle;
			if( (prevAngle - targetAngle) <= 30 ){
				speed = MIN_SPEED;
			}
			else{
				speed = MAX_SPEED;
			}
		}
		else if(targetAngle == prevAngle){
			angle = 0;
			speed = 0.0f;
		}
	}

	prevAngle = targetAngle;
	Motor_Angle_Set(angle, speed);
}
	

// Существование этой функции пока под вопросом
void RUS_Regul_Motor_Toggle_Rotation(void){
	if(Motor.Rotation == Clockwise){
		Motor_Rotation_Set(CounterClockWise);
	}
	else if(Motor.Rotation == CounterClockWise){
		Motor_Rotation_Set(Clockwise);
	}
}

static void RUS_Regul_MainTask (void *pParameters)
{
	RUS_Regul_ADC_Init();
	RUS_Regul_ADC_Start();
	Motor_SpeedCounter_Init();
	Motor_CreateSpeedTimerTask();
	Motor_Press_Init();
	//Debug_Timer_Start();
	//Motor_SpeedControl_Init();
	
	
	RUS_Regul_ADC_AI_t adc_struct = {0};
	static float PressMedianFiltred[PRESS_MEDIAN_SIZE];

	
	// Основной цикл
	while( 1 )
	{         
		RUS_Regul_Main_Struct_Update();
	
		// Ожидание событий
		EventBits_t EventBitsWait = ( EVENT_RUS_REGUL_EVENTS_ALL );
		EventBits_t EventBits = EventBitsWait & xEventGroupWaitBits( RUS_Regul_EventGroup, EventBitsWait, true, false, portMAX_DELAY );

		//if(EVENTS_RUS_REGUL_DEBUG_OUTPUT & EventBits ){
		//	HAL_UART_Ext_Transmit(&COM1_UART_Ext_hdl, (uint8_t*)&debug_output, sizeof(debug_output), 100);
		//}

		if( EVENTS_RUS_REGUL_CURRENT_CHECK & EventBits )
		{
			if(ADC_OPAMP_Value_Get() >= CURRENT_LIMITER_VALUE){
				Motor.fStuck = 1;
				RUS_Regul_Motor_Stop();
			}

#if 0
			static int OverCurrentCount = 0;
			static float OverPosition = 0.0f;
			
			do{
				if(ADC_OPAMP_Value_Get() > CURRENT_LIMITER_VALUE){
					Motor.fStuck = 1;											// Выставить флаг, что застрял
					Motor.WorkMode = DiscoverOverCurrent; 						// Выставить режим работы отработки превышения тока потребления
					// Если были запущены какие-либо таймеры, остановить их
					OverPosition = RUS_Regul_Motor_Long_Angle_Get();
					if( xTimerIsTimerActive(Motor.Timers.CalibrPressTimer))
						xTimerStop(Motor.Timers.CalibrPressTimer, 0);
					if( xTimerIsTimerActive(Motor.Timers.CalibrTimer))
						xTimerStop(Motor.Timers.CalibrTimer, 0);
					if( xTimerIsTimerActive(Motor.Timers.StabPressTimer))
						xTimerStop(Motor.Timers.StabPressTimer, 0);

					// Счетчик попыток "выбить" застрявший предмет, который не дает вращаться и вызывает повышенное потребление
					OverCurrentCount++;			

					// Повернуться в другом направлении на 30 градусов с минимальной скоростью
					RUS_Regul_Motor_Toggle_Rotation();
					Motor_Angle_Set(30, MIN_SPEED);

					if( (OverCurrentCount > 2) && (OverCurrentCount <= 4) )
					{
						RUS_Regul_Motor_Toggle_Rotation();
						Motor_Angle_Set(365, MAX_SPEED);
					}
					
					if(OverCurrentCount > 4) // Если счетчик попыток превысил 4, сбросить питание с мотора,  
					{
						RUS_Regul_Motor_Stop();
						GPIO_Common_Write(iGPIO_MOTOR_PWR, GPIO_PIN_RESET);
						OverCurrentCount = 0;
						break;
					}
					
				}
				else{
					if(0 == Motor.fStuck){
						break;
					}
					else{
						Motor.fStuck = 0;
						//Motor.WorkMode = Ready;
					}
					break;
				}
			}while(0);
#endif
			
		}

		if ( EVENTS_RUS_REGUL_ADC1_FIRSTHALFBUFFER_READY & EventBits )		// Готовность первой половины буфера чтения данных с ADS1231
		{
			// Вычислить постоЯнные составлЯющие сигналов АЦП1 по первой половине буфера
			ADC1_AllChAvgHalfBuffer( true );
		}
		if ( EVENTS_RUS_REGUL_ADC1_SECONDHALFBUFFER_READY & EventBits )		// Готовность второй половины буфера чтения данных с ADS1231
		{
			// Вычислить постоЯнные составлЯющие сигналов АЦП1 по второй половине буфера
			ADC1_AllChAvgHalfBuffer( false );
			RUS_Regul_ADC_GetVoltages(&adc_struct);
		}
		if ( EVENTS_RUS_REGUL_TIMER_SPEED_COUNT_X_READY & EventBits )		// Вычислить скорость за X мс
		{
			float CurrentSpeed;
			static uint32_t curCount, prevCount, delta;
			prevCount = curCount;
			curCount = LL_TIM_GetCounter(SPEED_COUNTER_TIM);

			if(curCount > prevCount){
				delta = curCount - prevCount;
			}
			else if(curCount < prevCount){
				delta = prevCount - curCount;
			}
			else{
				delta = 0;
			}
			CurrentSpeed = (delta / (float)(PULSE_PER_ONE_ROUND_INTERNAL) / (float)(SPEED_COUNTER_TIMER_MS) * 1000.0f * 60.0f) / (float)(GEAR_RATIO);  // Скорость вала редуктора в минуту. 
			Motor.SpeedMeas = CurrentSpeed;

		}
		if ( EVENTS_RUS_REGUL_TIMER_SPEED_COUNT_Y_READY & EventBits )		// Вычислить скорость за Y мс
		{
			
		}
		if ( EVENTS_RUS_REGUL_TIMER_SPEED_COUNT_Z_READY & EventBits )		// Вычислить скорость за Z мс
		{
			
		}
		if ( EVENTS_RUS_REGUL_SERVICETIMER & EventBits )		// 
		{
			
		}
		if ( EVENTS_RUS_REGUL_START_DELAY & EventBits )		// 
		{

		}
		if ( EVENTS_RUS_REGUL_STATE_TELE & EventBits )		// 
		{
			
		}
		if ( EVENTS_RUS_REGUL_MOTOR_POWER_ON & EventBits )		// Подать питание на мотор
		{
			
		}
		if ( EVENTS_RUS_REGUL_PROCESS_CALLBACK & EventBits )		// Готовность сдлеать действие при CNT = CCR1
		{
			switch(Motor.WorkMode)
			{
				case(Ready): // В состоянии готовности выполнения команд при попадании в колбек - ниего не делать. Регистр CCR1 игнорируется.
					//ReadyCNT++;
					//debug_output.ReadyCount = ReadyCNT;
					break;

				case(HoldingPressure):
					break;
				
				case(CalibratingConsumption):
					if(0 == Motor.fCalibrated){
						break;
					}
					if(1 == Motor.fCalibrated){
						RUS_Regul_Motor_Stop();
						Motor.AngleSet = 0;
					}
					break;
					
				case(CalibratingPress):
					// В начале калибровки CNT сбрасывается в ноль, а затем выставляется WorkMode, из-за чего CNT становится равным CCR1 и попадаем в перывание
					// (обработка прерывания здесь). Сначала выставить CCR1, а потом сбросить CNT в 0 нельзя, потому что CCR1 выставляется от текущего положения.
					// Если игнорировать ситуацию CCR1 = CNT = 0 все работает корректно.
					if(LL_TIM_GetCounter(SPEED_COUNTER_TIM) == 0) 
					{
						break;
					}
					if(Motor.SpeedSet == 30)
					{
						RUS_Regul_Motor_Stop();
						if(Motor.fSetCalNullPos == 1)
						{
							Motor.fSetCalNullPos = 0;
							Motor.fCalibrStart = 0;
							Motor.fCalibrated = 1;
							Motor.AngleSet = 0;
							Motor.WorkMode = Ready;
						}
					}
					else
					{
						Motor.SpeedSet = 30;
						RUS_Regul_Motor_Speed_Set(30);
						LL_TIM_OC_SetCompareCH1(SPEED_COUNTER_TIM, (LL_TIM_GetCounter(SPEED_COUNTER_TIM) + 200));
					}
					break;
					
				case(RotatingByAngle): // В режиме поворота на угол, плавно сделать доворот. CCR1 выставляется в зависимости от направления вращения
					if(Motor.SpeedSet == 30){
						RUS_Regul_Motor_Stop();
						Motor.WorkMode = Ready;
						Motor.AngleSet = 0;
						Motor.SpeedSet = 0;
					}
					else{
						Motor.SpeedSet = 30;
						if(Motor.Rotation == Clockwise){
							LL_TIM_OC_SetCompareCH1(SPEED_COUNTER_TIM, (LL_TIM_GetCounter(SPEED_COUNTER_TIM) + 200));
							RUS_Regul_Motor_Speed_Set(30);
						}
						else{
							LL_TIM_OC_SetCompareCH1(SPEED_COUNTER_TIM, (LL_TIM_GetCounter(SPEED_COUNTER_TIM) - 200));
							RUS_Regul_Motor_Speed_Set(-30);
						}
					}
					break;
				case(DiscoverOverCurrent):
					break;
					// Пока не ясно, каким образом действовать в ситуации застревания

			}
		}
		
		if ( EVENTS_RUS_REGUL_ADS1231_READY & EventBits )		// Готовность внешнего АЦП отдать данные 
		{
			static int32_t ADSCode, i = 0;
			static uint8_t count = 0;
			static uint8_t Transmit_Buf[62];
			float PressBar = 0;
			int16_t SIPressBar;
			static float avgPress = 0;
			static float NewPress;

			static uint32_t milliseconds = 0;

			static uint8_t arr_count_LSB = 2;
			static uint8_t arr_count_MSB = 3;

			static uint8_t time_idx_0 = 4;
			static uint8_t time_idx_1 = 5;
			static uint8_t time_idx_2 = 6;
			static uint8_t time_idx_3 = 7;
			

			Transmit_Buf[0] = 0xFF;
			Transmit_Buf[1] = 0xFF;
			
			ADS1231_DataRead(&ADSCode);
			RUS_Regul.ADC_Code = ADSCode;
			PressBar = ADSCode * CONVERT_TO_BAR * ADS1231_CALIBR_COEF_A - ADS1231_CALIBR_COEF_B;
			#if defined(TEST_BOARD)
				PressBar = ((((((ADSCode * 0.5f * ADS1231_VDDA) / ADS1231_GAIN) / ADS1231_MAX_CODE) * ADS1231_V_TO_MV) / ADS1231_VDDA) * TEST_COEF_A - TEST_COEF_B);
				SIPressBar = (int16_t)(PressBar * 100);
				milliseconds = HAL_GetTick();
				#if defined(UART_DEBUG_LOG)	
				
					Transmit_Buf[arr_count_LSB] = (uint8_t)(SIPressBar>>8);
					Transmit_Buf[arr_count_MSB] = (uint8_t)(SIPressBar);

					Transmit_Buf[time_idx_0] = (uint8_t)(milliseconds>>24);					
					Transmit_Buf[time_idx_1] = (uint8_t)(milliseconds>>16);
					Transmit_Buf[time_idx_2] = (uint8_t)(milliseconds>>8);
					Transmit_Buf[time_idx_3] = (uint8_t)(milliseconds);

					
					if (time_idx_3 == 61){
						HAL_UART_Ext_Transmit(&COM1_UART_Ext_hdl, Transmit_Buf, sizeof(Transmit_Buf), 0);
						arr_count_LSB = 2;
						arr_count_MSB = 3;
						time_idx_0 = 4;
						time_idx_1 = 5;
						time_idx_2 = 6;
						time_idx_3 = 7;						
					}
					else 
					{
						arr_count_LSB += 6;
						arr_count_MSB += 6;
						time_idx_0 += 6;
						time_idx_1 += 6;
						time_idx_2 += 6;
						time_idx_3 += 6;
					}
				#endif
			#endif
			
			RUS_Regul.CurPress = PressBar;
			if( (i < PRESS_MEDIAN_SIZE) )
			{
				PressMedianFiltred[i++] = PressBar;
			}
			if( (i % PRESS_MEDIAN_SIZE) == 0)
			{			
            	heapsort_2(PressMedianFiltred, (uint16_t)(PRESS_MEDIAN_SIZE));

				if(1 == count){
					NewPress = PressMedianFiltred[PRESS_MEDIAN_HALF_SIZE];
					avgPress = (1 - BETA_COEF_FILTER) * avgPress + BETA_COEF_FILTER * NewPress;
				}

				if(0 == count){
					avgPress = PressMedianFiltred[PRESS_MEDIAN_HALF_SIZE];
					count = 1;
				}
				i = 0;
			}
			
			Motor.Press = avgPress;
			
		}

		if ( EVENTS_RUS_REGUL_CALIBR_OPAMP_START & EventBits )		// Начать процедуру поиска нулевого положения по потреблению
		{
			// Если был проинициализивроан процесс калибровки, сбросить флаги в исходное состояние
			Motor.fCalibrStart = 1;
			Motor.fCalibrated = 0;
			Motor.OPAMPThreshold = 0;
			Motor.MinAngle = 0;
			Motor.MaxAngle = 0;
			Motor.WorkZone = 0;
			Motor.fFoundLimit = 0;
			Motor.fCheckOPAMPValue = 1;
			RUS_Regul_Motor_Calibration_Set();
			LL_TIM_SetCounter(SPEED_COUNTER_TIM, 0);
		}		
		
		if (EVENTS_RUS_REGUL_OPAMP_VALUE_READY & EventBits)
		{
			if(Motor.fCalibrStart == 1)
			{		
				
				static uint16_t leftCount;
				static uint16_t rightCount;
				//CurCount = LL_TIM_GetCounter(SPEED_COUNTER_TIM);
				uint16_t workArea;
				
				float OpampValue = ADC_OPAMP_Value_Get();
				
				if(Motor.fCheckOPAMPValue == 1)
				{
					Motor.OPAMPThreshold = OpampValue * 1.3f;
					Motor.fCheckOPAMPValue = 0;
				}
							
				do
				{
					
					if( (OpampValue > Motor.OPAMPThreshold) && (Motor.fFoundLimit == 0) )
					{
						//rightCount = LL_TIM_GetCounter(SPEED_COUNTER_TIM);
						Motor.fFoundLimit = 1;
						Motor.MaxAngle = (LL_TIM_GetCounter(SPEED_COUNTER_TIM) % PULSE_PER_ONE_TURN) * (360.0f / PULSE_PER_ONE_TURN);
						Motor_Rotation_Set(CounterClockWise);
						break;
					}

					if( (OpampValue > Motor.OPAMPThreshold) && (Motor.fFoundLimit == 1) )
					{
						RUS_Regul_Motor_Stop();
						//leftCount = LL_TIM_GetCounter(SPEED_COUNTER_TIM);
						
						Motor.fFoundLimit = 0; 
						Motor.fCalibrStart = 0;
						Motor.fCalibrated = 1;

						
						Motor.MinAngle = (LL_TIM_GetCounter(SPEED_COUNTER_TIM) % PULSE_PER_ONE_TURN) * (360.0f / PULSE_PER_ONE_TURN);
						
						Motor_Rotation_Set(Clockwise);

						if(Motor.MinAngle >= Motor.MaxAngle){
							Motor.MaxAngle += 360.0f;
						}
						//else if(Motor.MaxAngle > Motor.MinAngle){
						//	Motor.WorkZone = Motor.MaxAngle - Motor.MinAngle;
						//}

						//workArea = ((rightCount - leftCount) % PULSE_PER_ONE_TURN) * (360.0f / PULSE_PER_ONE_TURN);
						//Motor.WorkZone = workArea;

						Motor.WorkZone = Motor.MaxAngle - Motor.MinAngle;

						
						LL_TIM_SetCounter(SPEED_COUNTER_TIM, 0);
						xTimerStop(Motor.Timers.CalibrTimer , NULL);
						Motor.WorkMode = Ready;

						break;
					}

				} while(0);
			}
		}
		
		
		if( EVENTS_RUS_REGUL_CALIBR_PRESS_START & EventBits )		// Подготовить процесс калибровки по давлению
		{
			LL_TIM_SetCounter(SPEED_COUNTER_TIM, 0); // Перед каждой калибровкой принять текущее положение за 0	
			Motor.fReverseLogic = 0;
			Motor.fCalibrStart = 1;
			Motor.fCalibrated = 0;
			Motor.WorkZone = 0;
			Motor.MinAngle = 0;
			Motor.MaxAngle = 0;
			Motor.MinPress = 0;
			Motor.MaxPress = 0;
			RUS_Regul_Press_Calibration_Start();
			Motor.WorkMode = CalibratingPress;
		}

		if ( EVENTS_RUS_REGUL_CALIBR_PRESS_PROCESS & EventBits ) 	// Запустить процесс калибровки по давлению
		{

			static float CurAngle = 0;
			static float minAngle, maxAngle = 0; 
			static float minPress = 90000000;
			static float maxPress = -90000000;

			CurAngle = (LL_TIM_GetCounter(SPEED_COUNTER_TIM) % PULSE_PER_ONE_TURN) * (360.0f / PULSE_PER_ONE_TURN); 

			if(RUS_Regul.CurPress < minPress){
				minPress = RUS_Regul.CurPress;
				minAngle = CurAngle;
			}

			if(RUS_Regul.CurPress > maxPress){
				maxPress = RUS_Regul.CurPress;
				maxAngle = CurAngle;
			}

			if(Motor.WorkState == Standing){

				xTimerStop(Motor.Timers.CalibrPressTimer, NULL);
				
				Motor.MinAngle = minAngle;
				Motor.MaxAngle = maxAngle;

				Motor.MinPress = minPress;
				Motor.MaxPress = maxPress;
				if(maxAngle > minAngle){
					Motor.fReverseLogic = 0;
					Motor.WorkZone = maxAngle - minAngle;
				}
				else if(maxAngle < minAngle){
					Motor.fReverseLogic = 1;
					Motor.WorkZone = minAngle - maxAngle;
				}
				else if(minAngle == maxAngle){
					Motor.fBadCalibr = 1;
					Motor.CalibrNull = -100.0f;
				}
								
				Motor.CalibrNull = Motor.MinAngle; 				// Получен калибровочный ноль
				Motor.fSetCalNullPos = 1;						// Флаг выставления в положение минимума давления
				
				RUS_Regul_Absolute_Angle_Set(Motor.CalibrNull, MAX_SPEED); 
				minPress = 90000000;
				maxPress = -90000000;								
	
			}			
		} 	

		// Finish
		
		if( EVENTS_RUS_REGUL_HOLD_PRESS & EventBits )		// Запустить процесс стабилизации установленного давления
		{
			uint16_t TargetPress = RUS_Regul.PressSet;
			float CurrentPress = 0;
			CurrentPress = RUS_Regul.CurPress;
							
			do
			{
				if( (TargetPress <= Motor.MinPress) || (TargetPress >= Motor.MaxPress) )
				{
					break;
				}
				else
				{
					if(CurrentPress != TargetPress)
					{
						if(Motor.fReverseLogic == 0)
						{
							if(CurrentPress < TargetPress)
							{
								if(Motor.CurPosition >= Motor.MaxAngle)
								{
									RUS_Regul_Motor_Stop();
									break;
								}
								else
								{
									Motor_Rotation_Set(Clockwise);
									if( (TargetPress - CurrentPress) <= 2.0f ){
										RUS_Regul_Motor_Speed_Set(MIN_SPEED);
										break;
									}
									else if ( (TargetPress - CurrentPress) > 2.0f ){									
										RUS_Regul_Motor_Speed_Set(MAX_SPEED);
										break;
									}
									
									if( (CurrentPress >= (TargetPress - 0.5f)) && (CurrentPress <= (TargetPress + 0.5f)) )
									{
										RUS_Regul_Motor_Stop();
										break;
									}
								}
							}

							if(CurrentPress > TargetPress)
							{
								if(Motor.CurPosition <= Motor.MinAngle)
								{
									RUS_Regul_Motor_Stop();
									break;
								}
								else
								{
									Motor_Rotation_Set(CounterClockWise);
									if( (CurrentPress - TargetPress) <= 2.0f ){
										RUS_Regul_Motor_Speed_Set(MIN_SPEED);
										break;
									}
									else if( (CurrentPress - TargetPress) > 2.0f ){
										RUS_Regul_Motor_Speed_Set(MAX_SPEED);
										break;
									}			
									
									if( (CurrentPress >= (TargetPress - 0.5f)) && (CurrentPress <= (TargetPress + 0.5f)) )
									{
										RUS_Regul_Motor_Stop();
										break;
									}
								}
							}
						}



						if(Motor.fReverseLogic == 1)
						{
							if(CurrentPress < TargetPress)
							{
								if(Motor.CurPosition <= Motor.MaxAngle)
								{
									RUS_Regul_Motor_Stop();
									break;
								}
								else
								{
									Motor_Rotation_Set(CounterClockWise);
									if( (TargetPress - CurrentPress) <= 2.0f ){
										RUS_Regul_Motor_Speed_Set(MIN_SPEED);
										break;
									}
									else if ( (TargetPress - CurrentPress) > 2.0f ){									
										RUS_Regul_Motor_Speed_Set(MAX_SPEED);
										break;
									}

									if( (CurrentPress >= (TargetPress - 0.5f)) && (CurrentPress <= (TargetPress + 0.5f)) )
									{
										RUS_Regul_Motor_Stop();
										break;
									}
								}
							}


							if(CurrentPress > TargetPress)
							{
								if(Motor.CurPosition >= Motor.MinAngle)
								{
									RUS_Regul_Motor_Stop();
									break;
								}
								else
								{
									Motor_Rotation_Set(CounterClockWise);
									if( (CurrentPress - TargetPress) <= 2.0f ){
										RUS_Regul_Motor_Speed_Set(MIN_SPEED);
										break;
									}
									else if( (CurrentPress - TargetPress) > 2.0f ){
										RUS_Regul_Motor_Speed_Set(MAX_SPEED);
										break;
									}	


									if( (CurrentPress >= (TargetPress - 0.5f)) && (CurrentPress <= (TargetPress + 0.5f)) )
									{
										RUS_Regul_Motor_Stop();
										break;
									}
								}
							}
						}
					}
					else
					{
						RUS_Regul_Motor_Stop();
						break;
						// Add answer like "Can't rotate more"
					}
				}
			}while(0);
		}
	}
}



// [1 с]	Коллбек таймера RTOS
// Периодический сброс WatchDog
static void WatchDog_TimerCallback( TimerHandle_t xTimer )
{
	assert_param( xTimer == WatchDog_xTimer );
	WatchdogReset( );
}


// [100 мс]	Коллбек таймера RTOS
/*static void RUS_Regul_ServiceTimerCallback( TimerHandle_t xTimer )
{
	assert_param( xTimer == RUS_Regul.xServiceTimer );

	// Выставить флаг готовности пакета данных к дальнейшей обработке
	assert_param( NULL != RUS_Regul_EventGroup );
	xEventGroupSetBits( RUS_Regul_EventGroup, EVENTS_RUS_REGUL_SERVICETIMER );
}*/
// [ мс]	Коллбек таймера RTOS
static void RUS_Regul_StartDelayTimerCallback( TimerHandle_t xTimer )
{
	assert_param( xTimer == RUS_Regul.xStartDelayTimer );

	// Выставить флаг готовности пакета данных к дальнейшей обработке
	assert_param( NULL != RUS_Regul_EventGroup );
	xEventGroupSetBits( RUS_Regul_EventGroup, EVENTS_RUS_REGUL_START_DELAY );
}

static void RUS_Regul_Check_Current_Timer_Callback(TimerHandle_t xTimer){
	assert_param( NULL != RUS_Regul_EventGroup );
	xEventGroupSetBits( RUS_Regul_EventGroup, EVENTS_RUS_REGUL_CURRENT_CHECK );
}


void RUS_Regul_OPAMP_Timer_Create(void){
	TimerHandle_t OPAMPTimer = xTimerCreate("OPAMPTimer", pdMS_TO_TICKS(OPAMP_TIMER_MS), pdTRUE, NULL,  RUS_Regul_Check_Current_Timer_Callback);
	assert_param(NULL != OPAMPTimer );
	assert_param( pdPASS == xTimerStart( OPAMPTimer, 0 ) );
	Motor.Timers.OPAMPTimer = OPAMPTimer;
}

void heapsort_2(float* arr, uint16_t N)  
{
	GPIO_Common_Write(iGPIO_ADDRESS_0, GPIO_PIN_SET);
    assert_param (arr != NULL);
    assert_param (N > 1);
    
    register uint16_t n = N, i = n/2, parent, child;  
    register float t;  
  
    for (;;) { /* Loops until arr is sorted */  
        if (i > 0) { /* First stage - Sorting the heap */  
            i--;           /* Save its index to i */  
            t = arr[i];    /* Save parent value to t */  
        } else {     /* Second stage - Extracting elements in-place */  
            n--;           /* Make the new heap smaller */  
            if (n == 0) return; /* When the heap is empty, we are done */  
            t = arr[n];    /* Save last value (it will be overwritten) */  
            arr[n] = arr[0]; /* Save largest value at the end of arr */  
        }  
  
        parent = i; /* We will start pushing down t from parent */  
        child = i*2 + 1; /* parent's left child */  
  
        /* Sift operation - pushing the value of t down the heap */  
        while (child < n) {  
            if (child + 1 < n  &&  arr[child + 1] > arr[child]) {  
                child++; /* Choose the largest child */  
            }  
            if (arr[child] > t) { /* If any child is bigger than the parent */  
                arr[parent] = arr[child]; /* Move the largest child up */  
                parent = child; /* Move parent pointer to this child */  
                //child = parent*2-1; /* Find the next child */  
                child = parent*2+1; /* the previous line is wrong*/  
            } else {  
                break; /* t's place is found */  
            }  
        }  
        arr[parent] = t; /* We save t in the heap */  
    }  
	GPIO_Common_Write(iGPIO_ADDRESS_0, GPIO_PIN_RESET);
}



