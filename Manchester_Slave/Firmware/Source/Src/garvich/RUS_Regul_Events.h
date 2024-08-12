// VIKPB_EVENTS.h
// **************************************************************************************************************************************

#ifndef	VIKPB_EVENTS_H
#define	VIKPB_EVENTS_H

#include "FreeRTOS.h"

#include "Event_groups.h"

// ***************************************************
// ����� ������� � ������ VIKPB_EventGroup
// ***************************************************


#define EVENTS_RUS_REGUL_ADC1_FIRSTHALFBUFFER_READY			( 1 << 1 )		// ���� ���������� ������ �������� ������ ���������
#define EVENTS_RUS_REGUL_ADC1_SECONDHALFBUFFER_READY		( 1 << 2 )		// ���� ���������� ������ �������� ������ ���������
#define EVENTS_RUS_REGUL_TIMER_SPEED_COUNT_X_READY			( 1 << 3 )		// ���� ��������� ����� ������� (������ � ��)
#define EVENTS_RUS_REGUL_TIMER_SPEED_COUNT_Y_READY			( 1 << 4 )		// ���� ��������� ����� ������� (������ Y ��)
#define EVENTS_RUS_REGUL_TIMER_SPEED_COUNT_Z_READY			( 1 << 5 )		// ���� ��������� ����� ������� (������ Z ��)
#define EVENTS_RUS_REGUL_SERVICETIMER						( 1 << 6 )		// ���� ����-��!!!! (������ �������� ����� ��������)
#define EVENTS_RUS_REGUL_START_DELAY						( 1 << 7 )		// ���� ������ �������� �� ���-��!!!
#define EVENTS_RUS_REGUL_STATE_TELE							( 1 << 8 )		// ���� ����� ��������� ������ � ������������ ���������
#define EVENTS_RUS_REGUL_MOTOR_POWER_ON						( 1 << 9 )		// ���� ��������� ������� �� ������
#define EVENTS_RUS_REGUL_MOTOR_STATE						( 1 << 10 )		// ���� ��������� ������
#define EVENTS_RUS_REGUL_ADS1231_READY						( 1 << 11 )		// ���� ���������� �������� ��� � ������
#define EVENTS_RUS_REGUL_OPAMP_VALUE_READY					( 1 << 12 )		// ���� ���������� ���������� �������� ����������� � ������������� ���������
#define EVENTS_RUS_REGUL_CALIBR_OPAMP_START					( 1 << 13 )		// ���� ������������� ��������� ������ �������� ���������
#define EVENTS_RUS_REGUL_CALIBR_PRESS_START					( 1 << 14 )		// ���� ������������� ���������� �� ��������
#define EVENTS_RUS_REGUL_CALIBR_PRESS_PROCESS				( 1 << 15 )		// ���� ���������� ������ ���������� �� ��������
#define EVENTS_RUS_REGUL_HOLD_PRESS							( 1 << 16 )		// ���� ����������� ������ ��������� ��������
#define EVENTS_RUS_REGUL_PROCESS_CALLBACK					( 1 << 17 )		// ���� ��������� ������� TIM2
#define EVENTS_RUS_REGUL_CURRENT_CHECK						( 1 << 18 )		// ������� ��� ������������ ����������� ��� � 5 ��. ����������� ����������������
//#define EVENTS_RUS_REGUL_DEBUG_OUTPUT						( 1 << 19 )		// ������ � ������ �����!!!!!!!!





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


// ������ �������, ������������ �� ���������� ������� � �������� �����
extern EventGroupHandle_t RUS_Regul_EventGroup;

#endif	

