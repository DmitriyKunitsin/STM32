bool Small_Motor_Speed_Set(float speed_percent);
void Small_Motor_Stop(void);
bool Small_Motor_Angle_Set(int16_t angle, float speed_percent);
void Small_Motor_Sync_Set(void);
void Small_Motor_Ovrelap_Set(void);

void SPEED_COUNTER_TIM_MspInit( void );
void SPEED_COUNTER_TIM_MspDeInit(void);
void Motor_SpeedCounter_Init(void);
void MOTOR_SPEED_CONTROL_TIM_MspInit(void);
void Motor_SpeedControl_Init(void);
void CurrentSpeed_Get(void);
void OPAMP_Init(void);
void HAL_OPAMP_MspInit(OPAMP_HandleTypeDef *hopamp);
void COMP_Init(void);
void HAL_COMP_MspInit(COMP_HandleTypeDef* hcomp);
void DAC_Init(void);
void DAC_Voltage_Set( float Voltage, float Vref );
void SET_CCR1_SPEED_COUNTER(uint32_t CompareValue);




#define DAC_VREF_V	(VDDA_VALUE / 1000.0f)


#define PERIOD_FOR_SPEED_CONTROL_TIM		99
#define PRESCALER_FOR_SPEED_CONTROL_TIM		33
#define GEAR_RATIO_SMALL_MOTOR				721  // Узнать число 
#define SPEED_COUNTER_TIMER_MS				100.0f
#define CALIBRATION_TIMER_MS				50.0f
#define CALIBRATION_PRESS_TIMER_MS			5.0f
#define STABLIZATION_PRESS_TIMER_MS			(100.0f)
#define PULSE_PER_ROUND_MOTOR_SMALL_MOTOR	6 	 // Узнать число
#define MAX_SPEED_SMALL_MOTOR				0.0f // Проверить на практике
#define PULSE_PER_ONE_TURN_SMALL_MOTOR		( (PULSE_PER_ROUND_MOTOR_SMALL_MOTOR) * (GEAR_RATIO_SMALL_MOTOR) )
#define MIN_SPEED_SMALL_MOTOR				100.0f
#define CALIBR_TURN_SMALL_MOTOR				3
#define INIT_TURNOVERS_SMALL_MOTOR			15
#define ARR_VALUE_SMALL_MOTOR				( (INIT_TURNOVERS_SMALL_MOTOR) * (PULSE_PER_ROUND_MOTOR_SMALL_MOTOR) )
#define OPAMP_TIMER_MS_SMALL_MOTOR			(5.0f)


