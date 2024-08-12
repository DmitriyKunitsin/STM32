bool CM36_3650_Motor_Speed_Set(float speed_percent);
void CM36_3650_Motor_Stop(void);
bool CM36_3650_Motor_Angle_Set(int16_t angle, float speed_percent);
void CM36_3650_Motor_Sync_Set(void);
void CM36_3650_Motor_Ovrelap_Set(void);

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
#define GEAR_RATIO_CM36_3650				721 
#define SPEED_COUNTER_TIMER_MS				500.0f
#define CALIBRATION_TIMER_MS				50.0f
#define CALIBRATION_PRESS_TIMER_MS			5.0f
#define STABLIZATION_PRESS_TIMER_MS			(500.0f)
#define PULSE_PER_ROUND_MOTOR_CM36_3650		6
#define MAX_SPEED_CM36_3650					100.0f
#define PULSE_PER_ONE_TURN_CM36_3650		( (PULSE_PER_ROUND_MOTOR_CM36_3650) * (GEAR_RATIO_CM36_3650) )
#define MIN_SPEED_CM36_3650					30.0f
#define CALIBR_TURN_CM36_3650				3
#define INIT_TURNOVERS_CM36_3650			15
#define ARR_VALUE_CM36_3650					( (INIT_TURNOVERS_CM36_3650) * (PULSE_PER_ONE_TURN_CM36_3650) )
#define OPAMP_TIMER_MS_MOTOR_CM36_3650		(5.0f)

