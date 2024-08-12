// ������� ���������� ������
// 
// 
// 
// 
//


#include "ProjectConfig.h"
#include "platform_common.h"

#include "stm32xxxx_hal.h"			// ����� ���������
#include "stm32l4xx_hal_comp.h"
#include "stm32l4xx_hal_dac.h"
#include "DRIVER_Small_Motor.h"		// ������

#include "Common_GPIO.h"
#include "Common_tim.h"
#include "RUS_Regul_Main.h"
#include "stm32l4xx_ll_tim.h"
#include "DRIVER_Motor.h"
#include "Common_dac.h"
#include "Utils.h"				// isfinite()
#include "RUS_Regul_Events.h"


typedef struct Small_Motor_struct{
	float CurrentSpeed;
	float CurrentAngle;
	uint32_t CurrentPulsesPerAngle;
} Small_t;

Small_t Small_Motor;
extern Motor_Main_t Motor;
extern RUS_Regul_t RUS_Regul;



// ������������� ����� �������-�������� �������� �������� 
void SPEED_COUNTER_TIM_MspInit( void )
{
	// �������� ������������ �������
	TIM_CLK_ENABLE(SPEED_COUNTER_TIM);

	//  ���������������� ��� - ���� ��������
	GPIO_CLK_ENABLE(SPEED_COUNTER_TIM_GPIO_PORT);
	GPIO_InitTypeDef GPIO_Init;
	GPIO_Init.Pin		= SPEED_COUNTER_TIM_GPIO_PIN;
	GPIO_Init.Mode		= GPIO_MODE_AF_OD;
	GPIO_Init.Pull		= GPIO_PULLDOWN;
	GPIO_Init.Speed		= GPIO_SPEED_FREQ_LOW;
	GPIO_Init.Alternate	= SPEED_COUNTER_TIM_AF;
	HAL_GPIO_Init(SPEED_COUNTER_TIM_GPIO_PORT, &GPIO_Init);

	// ��������� ���������� �� ������� 2
	HAL_NVIC_SetPriority(TIM2_IRQn, SPEED_COUNTER_TIM_IRQ_PREEMTPRIORITY, SPEED_COUNTER_TIM_IRQ_SUBPRIORITY);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

// ��������������� �������-�������� (�������)
void SPEED_COUNTER_TIM_MspDeInit(void)
{
	assert_param(0);
}


// ������������� �������-�������� �������� �������� ������ �� �������� �����
void Motor_SpeedCounter_Init(void)
{
	TIM_HandleTypeDef *pTim_hdl = &SPEED_COUNTER_TIM_HDL;
	pTim_hdl->Instance = SPEED_COUNTER_TIM;

	TIM_MasterConfigTypeDef sMasterConfig = {0};

	TIM_OC_InitTypeDef sConfigOC = {0};
	
	// ���������� ������� ������������� (������� ����� ��� ������������)
	TIM_Base_InitTypeDef *pTim_Init = &pTim_hdl->Init;
	*pTim_Init = (TIM_Base_InitTypeDef){0};
	pTim_Init->CounterMode = 		TIM_COUNTERMODE_UP;
	pTim_Init->ClockDivision =		TIM_CLOCKDIVISION_DIV1;
	pTim_Init->Prescaler = 			0;
	pTim_Init->Period = 			ARR_VALUE_SMALL_MOTOR;
	pTim_Init->AutoReloadPreload	= TIM_AUTORELOAD_PRELOAD_DISABLE;
	__HAL_TIM_RESET_HANDLE_STATE( pTim_hdl );
	assert_param( HAL_OK == HAL_TIM_OC_Init( pTim_hdl ) );
	//assert_param( HAL_OK == HAL_TIM_Base_Init( pTim_hdl ) );

	// �������� ������������ - ����� ����� �������� ������� (ETR2)
	TIM_ClockConfigTypeDef TIM_ClockConfig = { 0 };
	TIM_ClockConfig.ClockSource		= TIM_CLOCKSOURCE_ETRMODE2;
	TIM_ClockConfig.ClockPolarity	= TIM_CLOCKPOLARITY_RISING; //TIM_CLOCKPOLARITY_BOTHEDGE;
	TIM_ClockConfig.ClockPrescaler	= TIM_CLOCKPRESCALER_DIV1;
	TIM_ClockConfig.ClockFilter		= 15;
	assert_param( HAL_OK == HAL_TIM_ConfigClockSource( pTim_hdl, &TIM_ClockConfig ) );

	// ����� ��������� � ������� 1 (Output Compare Channel 1)
	//sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC1REF;
  	//sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	//assert_param(HAL_OK == HAL_TIMEx_MasterConfigSynchronization(&SPEED_COUNTER_TIM_HDL, &sMasterConfig));

	// ��������� ����� ���������
	sConfigOC.OCMode = TIM_OCMODE_TIMING;
 	sConfigOC.Pulse = 0;
  	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	assert_param(HAL_OK == HAL_TIM_OC_ConfigChannel(&SPEED_COUNTER_TIM_HDL, &sConfigOC, TIM_CHANNEL_1));
	
	// ��������� ����
	//assert_param( HAL_OK == HAL_TIM_Base_Start( pTim_hdl ) );
	assert_param( HAL_OK == HAL_TIM_OC_Start_IT( pTim_hdl, TIM_CHANNEL_1 ) );
}

// ������������� ����� �������-��������� ���-������� ��� ���������� ��������� ���������
void MOTOR_SPEED_CONTROL_TIM_MspInit(void)
{
	// �������� ������������ �������
	TIM_CLK_ENABLE(MOTOR_SPEED_CONTROL_TIM);

	//  ���������������� ��� - ���� ��������
	GPIO_CLK_ENABLE(MOTOR_SPEED_CONTROL_TIM_GPIO_PORT);
	GPIO_InitTypeDef GPIO_Init;
	GPIO_Init.Pin		= MOTOR_SPEED_CONTROL_TIM_GPIO_PIN;
	GPIO_Init.Mode		= GPIO_MODE_AF_PP;
	GPIO_Init.Pull		= GPIO_PULLDOWN;
	GPIO_Init.Speed		= GPIO_SPEED_FREQ_LOW;
	GPIO_Init.Alternate	= MOTOR_SPEED_CONTROL_TIM_AF;
	HAL_GPIO_Init(MOTOR_SPEED_CONTROL_TIM_GPIO_PORT, &GPIO_Init);
}

// ������������� �������-�������� �������� �� ���������� ��������� �� ���� ������
void Motor_SpeedControl_Init(void)
{
	TIM_HandleTypeDef *pTim_hdl = &MOTOR_SPEED_CONTROL_TIM_HDL;
	pTim_hdl->Instance = MOTOR_SPEED_CONTROL_TIM;
	
	// ���������� ������� �������������. ������� ������� 65536 ���, ����� ������������ 65536/32758 = 2 ���, ����� ������� 2/100 = 20 ���
	TIM_Base_InitTypeDef *pTim_Init = &pTim_hdl->Init;
	
	*pTim_Init = (TIM_Base_InitTypeDef){0};
	pTim_Init->CounterMode = 		TIM_COUNTERMODE_UP;
	pTim_Init->ClockDivision =		TIM_CLOCKDIVISION_DIV1;
	pTim_Init->Prescaler = 			PRESCALER_FOR_SPEED_CONTROL_TIM;
	pTim_Init->Period = 			PERIOD_FOR_SPEED_CONTROL_TIM;
	pTim_Init->AutoReloadPreload = 	TIM_AUTORELOAD_PRELOAD_DISABLE;
	pTim_Init->RepetitionCounter =	0;
	__HAL_TIM_RESET_HANDLE_STATE( pTim_hdl );
	assert_param( HAL_OK == HAL_TIM_PWM_Init( pTim_hdl ) );

	TIM_MasterConfigTypeDef sMasterConfig = {0}; 
	
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;		// ������ ����������, ����� 
  	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	assert_param( HAL_OK == HAL_TIMEx_MasterConfigSynchronization( pTim_hdl, &sMasterConfig ) );

	TIM_OC_InitTypeDef sConfigOC = {0};
	
  	sConfigOC.OCMode = TIM_OCMODE_PWM1;						// ����� ������ - ��������� ��� �������
  	sConfigOC.Pulse = 0; 									// ����� ��������				
  	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;				// ������� - �����
  	sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;			// ������� - �����
  	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;				// �������� ����� ������� Enable
  	sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;			// ��������� ���� ����� �������� � ��������� ���� ��� ������� ���������� (�������� �������� ������� ����� �������� ���������)
  	sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;		// ��������� ��������� ������ ����� (0)
  	assert_param( HAL_OK == HAL_TIM_PWM_ConfigChannel(pTim_hdl, &sConfigOC, TIM_CHANNEL_1));

	TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};
	
	sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;		// ���������� ������������� 
  	sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  	sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  	sBreakDeadTimeConfig.DeadTime = 0; 								// �������� ���������
  	sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;			// �� ����������� ������ �� �������
  	sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;	// ���������� ������� ����������
  	sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;	//��������� �������������� ���������� �������� (���������� ������ ����������)
	assert_param(HAL_OK == HAL_TIMEx_ConfigBreakDeadTime(pTim_hdl, &sBreakDeadTimeConfig));
	// ��������� ����
	assert_param( HAL_OK == HAL_TIM_PWM_Start( pTim_hdl,  TIM_CHANNEL_1) );
}


// ������� ����������� �������� ������. ����������� � �������� ����������� ����������� ����������� ���.
// @param float speed_percent: +- 0...100.0 %
bool Small_Motor_Speed_Set(float speed_percent)
{
	speed_percent = 100.0f - speed_percent; // ��� ������ ���, ��� ������ ��������, ��� �������� ����������� �������� ���������� � ���������� ����������� 0 - 100, ��� 0 - ���, 100 - ����.
	uint16_t pulse = 0;
	pulse = (uint16_t)(PERIOD_FOR_SPEED_CONTROL_TIM * speed_percent / 100.0f + 0.5f);
	Motor.WorkState = Rotating;
	Motor.SpeedSet = 100.0f - speed_percent;
	LL_TIM_OC_SetCompareCH1(MOTOR_SPEED_CONTROL_TIM, pulse);
	return true;
}

void Small_Motor_Stop(void){
	LL_TIM_OC_SetCompareCH1(MOTOR_SPEED_CONTROL_TIM, 100.0f);
	Motor.WorkState = Standing;
}


// ������� ������������� ��������� ������
void Small_Motor_Sync_Set(void)
{
	
}


// ������ ������� ��������� ������ � ������� ����
bool Small_Motor_Angle_Set(int16_t angle, float speed_percent)
{
	int32_t CurrentPulses; // ������� ��������� ���������� ��������� �� ������� ���� �����������
	uint32_t PulsesPerAngle = (uint32_t)((PULSE_PER_ONE_TURN_SMALL_MOTOR / 360.0f) * angle + 0.5f);
	speed_percent = 100.0f - speed_percent;
		
	if(Motor.Rotation == Clockwise){
		CurrentPulses = LL_TIM_GetCounter(SPEED_COUNTER_TIM) + PulsesPerAngle;
		if(CurrentPulses > ARR_VALUE_SMALL_MOTOR)
		{
			CurrentPulses = CurrentPulses - ARR_VALUE_SMALL_MOTOR;
		}
	}
	else{
		CurrentPulses = LL_TIM_GetCounter(SPEED_COUNTER_TIM) - PulsesPerAngle;
		if(CurrentPulses < 0)
		{
			CurrentPulses = ARR_VALUE_SMALL_MOTOR + CurrentPulses;
		}
	}
	
	Small_Motor.CurrentSpeed = speed_percent;
	if(speed_percent <= 30){
		LL_TIM_OC_SetCompareCH1(SPEED_COUNTER_TIM, CurrentPulses);
	}
	if(speed_percent > 30){
		if(Motor.Rotation == Clockwise){
			LL_TIM_OC_SetCompareCH1(SPEED_COUNTER_TIM, (CurrentPulses - 200));
		}
		else{
			LL_TIM_OC_SetCompareCH1(SPEED_COUNTER_TIM, (CurrentPulses + 200));
		}
	}
	
	Small_Motor_Speed_Set(speed_percent);
	return true;
}

void SPEED_COUNTER_TIM_OC_DelayElapsedCallback(void) // ����� ����������� �������, � �������� ���������!
{
	assert_param( NULL != RUS_Regul_EventGroup );
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xEventGroupSetBitsFromISR( RUS_Regul_EventGroup, EVENTS_RUS_REGUL_PROCESS_CALLBACK, &xHigherPriorityTaskWoken );
}



// ������� ������� ��������� ������ � ������� �������� ����������
void Small_Motor_Ovrelap_Set(void)
{
	
}

// ������� ������ ���������� �� ������
void Small_Motor_Voltage_Get(void)
{
	
}

void SET_CCR1_SPEED_COUNTER(uint32_t CompareValue){
	LL_TIM_OC_SetCompareCH1(SPEED_COUNTER_TIM, CompareValue);
}


