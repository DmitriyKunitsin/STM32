// Драйвер управления мотором 
// Провода:
//	- Питание "+"
//	- Питание "-"
//	- Управление скорсотью (ШИМ)
//	- Обратная свзяь (6 импульсов на оборот вала мотора)
//	- Управление направлением вращения
#include "ProjectConfig.h"
#include "stm32xxxx_hal.h"		// дрова периферии
#include "stm32l4xx_hal_opamp.h"
#include "stm32l4xx_hal_comp.h"
#include "stm32l4xx_hal_dac.h"
//#include "DRIVER_CM36-3650.h"
#include "DRIVER_Motor.h"		// родной
#include "platform_common.h"
#include "Common_GPIO.h"
#include "Common_tim.h"
#include "RUS_Regul_Main.h"
#include "stm32l4xx_ll_tim.h"
#include "RUS_Regul_ADC.h"
#include "RUS_Regul_Events.h"

Motor_Main_t Motor;
RTOS_Timers_t Timers;

extern RUS_Regul_t RUS_Regul;


// Инициализация операционного усилителя для измерения тока потребления мотора
void OPAMP_Init(void)
{
	OPAMP_HandleTypeDef OPAMP_Hdl;
	OPAMP_Hdl.Instance = RUS_REGUL_OPAMP;
	OPAMP_Hdl.Init.PowerSupplyRange = OPAMP_POWERSUPPLY_LOW;
	OPAMP_Hdl.Init.Mode = OPAMP_PGA_MODE;
	OPAMP_Hdl.Init.NonInvertingInput = OPAMP_NONINVERTINGINPUT_IO0;
	OPAMP_Hdl.Init.InvertingInput = OPAMP_INVERTINGINPUT_IO0;
	OPAMP_Hdl.Init.PgaGain = OPAMP_PGA_GAIN_16;
	OPAMP_Hdl.Init.PowerMode = OPAMP_POWERMODE_NORMAL;
 	OPAMP_Hdl.Init.UserTrimming = OPAMP_TRIMMING_FACTORY;
	assert_param(HAL_OK == HAL_OPAMP_Init(&OPAMP_Hdl));
	assert_param(HAL_OK == HAL_OPAMP_Start(&OPAMP_Hdl));
}

void HAL_OPAMP_MspInit(OPAMP_HandleTypeDef *hopamp)
{
	GPIO_InitTypeDef GPIO_Init;
	
	__HAL_RCC_OPAMP_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	
	GPIO_Init.Pin = RUS_REGUL_OPAMP_PINS;
	GPIO_Init.Mode = RUS_REGUL_OPAMP_MODE;
	GPIO_Init.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(RUS_REGUL_OPAMP_GPIO, &GPIO_Init);
}


bool Motor_Speed_Set(float speed_percent){
	#if defined(USE_MOTOR_CM36_3650) 
	if((speed_percent >= 30.0f) && (speed_percent <= 100.0f)){
		Motor.SpeedSet = speed_percent;
		CM36_3650_Motor_Speed_Set(speed_percent);
		return true;
	}	
	else{
		return false;
	}
	#elif defined(USE_SMALL_MOTOR)
	speed_percent = 100.0f - speed_percent;
	if( (speed_percent > 5.0f) && (speed_percent <= 100.0f) ){
		Motor.SpeedSet = 100.0f - speed_percent;
		Small_Motor_Speed_Set(speed_percent);
		return true;
	}
	else{
		return false;
	}
	#endif
}

void Motor_Stop()
{
	#if defined(USE_MOTOR_CM36_3650)
	CM36_3650_Motor_Stop();
	#elif defined(USE_SMALL_MOTOR)
	Small_Motor_Stop();
	#endif
	Motor.SpeedSet = 0;
	Motor.AngleSet = 0;
}

// Функция измерения скорости оборотов вала мотора (пересчитывается через передаточное число в обороты вала редуктора)
static void Motor_CurrentSpeed_Get_Callback(TimerHandle_t pTimer){ 

	assert_param( NULL != RUS_Regul_EventGroup );
	xEventGroupSetBits( RUS_Regul_EventGroup, EVENTS_RUS_REGUL_TIMER_SPEED_COUNT_X_READY );
}

void Motor_Rotation_Set(DirRotation_t Rotation)
{
	if(Clockwise == Rotation)
	{
		GPIO_Common_Write(iGPIO_MOTOR_REVERSE_ROT, GPIO_PIN_SET); // Движение по часовой
		LL_TIM_SetCounterMode(SPEED_COUNTER_TIM, LL_TIM_COUNTERMODE_UP);
	}
	else if(CounterClockWise == Rotation)
	{
		GPIO_Common_Write(iGPIO_MOTOR_REVERSE_ROT, GPIO_PIN_RESET); // Движение против часовой
		LL_TIM_SetCounterMode(SPEED_COUNTER_TIM, LL_TIM_COUNTERMODE_DOWN);
	}
	Motor.Rotation = Rotation;
	return;
}


void Motor_CreateSpeedTimerTask(void){
		TimerHandle_t SpeedCounterTimer = xTimerCreate( "SPEEDCOUNTER", pdMS_TO_TICKS(SPEED_COUNTER_TIMER_MS), pdTRUE, NULL,  Motor_CurrentSpeed_Get_Callback);
		assert_param( pdPASS == xTimerStart( SpeedCounterTimer, 0 ) );
}

void Motor_Angle_Set(int16_t angle, float speed_percent){
	Motor.AngleSet = angle;
	
	#if defined(USE_MOTOR_CM36_3650)
	CM36_3650_Motor_Angle_Set(angle, speed_percent);
	
	#elif defined(USE_SMALL_MOTOR)
	speed_percent = 100.0f - speed_percent;
	Small_Motor_Angle_Set(angle, speed_percent);
	#endif
}


static void Motor_Calibration_Callback(TimerHandle_t pTimer)
{
	assert_param( NULL != RUS_Regul_EventGroup );
	xEventGroupSetBits( RUS_Regul_EventGroup, EVENTS_RUS_REGUL_OPAMP_VALUE_READY );
}

void Motor_Calibration_Current_Create(void){
	TimerHandle_t CalibrTimer = xTimerCreate("CALIBRATION", pdMS_TO_TICKS(CALIBRATION_TIMER_MS), pdTRUE, NULL,  Motor_Calibration_Callback);
	assert_param(NULL != CalibrTimer);
	Motor.Timers.CalibrTimer = CalibrTimer;
}

void Motor_Calibration_Current_Start(void)
{
	assert_param( pdPASS == xTimerStart( Motor.Timers.CalibrTimer, 0 ) );
	Motor_Speed_Set(MAX_SPEED);
}


static void Motor_Calibration_Press_Callback(TimerHandle_t pTimer)
{
	assert_param( NULL != RUS_Regul_EventGroup );
	xEventGroupSetBits( RUS_Regul_EventGroup, EVENTS_RUS_REGUL_CALIBR_PRESS_PROCESS );
}

void Motor_Calibration_Press_Create(void){
	TimerHandle_t CalibrPressTimer = xTimerCreate("CALIBRATION_PRESS", pdMS_TO_TICKS(CALIBRATION_PRESS_TIMER_MS), pdTRUE, NULL,  Motor_Calibration_Press_Callback);
	assert_param(NULL != CalibrPressTimer );
	Motor.Timers.CalibrPressTimer = CalibrPressTimer;

}

static void Motor_Stabilization_Press_Callback(TimerHandle_t pTimer)
{
	assert_param(NULL != RUS_Regul_EventGroup);
	xEventGroupSetBits(RUS_Regul_EventGroup, EVENTS_RUS_REGUL_HOLD_PRESS);
}

void Motor_Stabilization_Press_Create(void){
	TimerHandle_t StabPressTimer = xTimerCreate("STABILIZATION_PRESS", pdMS_TO_TICKS(STABLIZATION_PRESS_TIMER_MS), pdTRUE, NULL, Motor_Stabilization_Press_Callback);
	assert_param(NULL != StabPressTimer );
	Motor.Timers.StabPressTimer = StabPressTimer;
}

void Motor_Stabilization_Press_Start(void)
{
	assert_param( pdPASS == xTimerStart(Motor.Timers.StabPressTimer, 0) );
	Motor.fHoldPress = 1;
	Motor.WorkMode = HoldingPressure;
}

void Motor_Calibration_Press_Start(void)
{
	assert_param( pdPASS == xTimerStart( Motor.Timers.CalibrPressTimer, 0 ) );
	Motor_Rotation_Set(Clockwise);
	Motor_Angle_Set(1080, MAX_SPEED);
}


