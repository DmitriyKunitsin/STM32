// SADC_SKLP_Interface.c
// РеализациЯ функций протокола СКЛ длЯ проекта спектрометрического АЦП
#include "ProjectConfig.h"				// конфиг платформы
#include "stm32xxxx_hal.h"				// дрова периферии
#include "SKLP_Interface_RUS_Regul_3540.h"	// родной
#include "SKLP_MS_TransportInterface.h"	// SKLP_SetTxCompleteCB()
#include "RUS_Regul_Common.h"				// доступ к данным проекта
#include "Utils.h"						// Round_UI16()
#include "MathUtils.h"					// Float16_t
#include <stdio.h>
#include <stdint.h>
#include "RUS_Regul_Main.h"				// доступ к основным данным проект
#include "DRIVER_Motor.h"
#include "Common_GPIO.h"
#include "NVM.h"						// NVM_Tag_t
#include "stm32xxxx_hal_conf.h"
//#include "timers.h"


extern RUS_Regul_t RUS_Regul;
extern Motor_Main_t Motor;

typedef struct SKLP_NVM_Tag_struct
{
	SKLP_NVM_ID_t	SKLP_ID;		// Идентификатор для работы через команды [0x04] и [0x05]
	NVM_Tag_t const *pTag;
} SKLP_NVM_Tag_t;

extern NVM_Tag_t NVM_Tag_RUS_Regul_PressCalibr;
extern NVM_Tag_t NVM_Tag_RUS_Regul_MotorCalibr;
// Список всех блоков, доступных длЯ команд [0x04], [0x05]
const SKLP_NVM_Tag_t aSKLP_NVM_Tags[] = 
{
	{ eSKLP_NVM_PressCoeff,			&NVM_Tag_RUS_Regul_PressCalibr},			// [SKLP_NVM.0]
	{ eSKLP_NVM_MotorCalibr,		&NVM_Tag_RUS_Regul_MotorCalibr},			// [SKLP.NVM.1]			
};


// Команды SKLP длЯ проверки работоспособности через терминал

// Стандартные команды
//static SKLP_CommandResult_t SKLP_ProcessCommand_RUS_Regul_ID_Get(uint8_t *pQuery, uint8_t **ppAnswer); // Отдать ID
static SKLP_CommandResult_t SKLP_ProcessCommand_RUS_Regul_NVM_Get(uint8_t *pQuery, uint8_t **ppAnswer);; // Отдать данные из NVM
static SKLP_CommandResult_t SKLP_ProcessCommand_RUS_Regul_NVM_Set(uint8_t *pQuery, uint8_t **ppAnswer);; // Прочитать данные из NVM
static SKLP_CommandResult_t SKLP_ProcessCommand_RUS_Regul_MainData_Get(uint8_t *pQuery, uint8_t **ppAnswer); // Чтение основного блока данных (запись на SD)
static SKLP_CommandResult_t SKLP_ProcessCommand_RUS_Regul_Extendeed_Data_Get(uint8_t *pQuery, uint8_t **ppAnswer); // Чтение расширенного блока данных
// Команды управления регулятором в процессе бурения
static SKLP_CommandResult_t SKLP_ProcessCommand_RUS_Regul_Motor_Power_Managment(uint8_t *pQuery, uint8_t **ppAnswer); // Управление (Вкл/выкл) питанием мотора, в зависимости от параметра
static SKLP_CommandResult_t SKLP_ProcessCommand_RUS_Regul_Overlap_Set(uint8_t *pQuery, uint8_t **ppAnswer); // Выставить процент перекрытия заслонки
static SKLP_CommandResult_t SKLP_ProcessCommand_RUS_Regul_Start_Pressure_Calibr(uint8_t *pQuery, uint8_t **ppAnswer); // Запустить калибровку по давлению
static SKLP_CommandResult_t SKLP_ProcessCommand_RUS_Regul_Start_Current_Calibr(uint8_t *pQuery, uint8_t **ppAnswer); // Запустить калибровку по току потребления (если есть упоры)
static SKLP_CommandResult_t SKLP_ProcessCommand_RUS_Regul_Press_Stabiliztion(uint8_t *pQuery, uint8_t **ppAnswer); // Запустить режим стабилизации давления (удержание выставленного давления)
// Переход к расширенным командам
static SKLP_CommandResult_t SKLP_ProcessCommand_RUS_Regul_Advanced_Managment(uint8_t *pQuery, uint8_t **ppAnswer); // Переход к расширенному набору команд (обычные команды не запрещены, продолжают выполняться)
// Расширенные команды управления регулятором
static SKLP_CommandResult_t EX_SKLP_ProcessCommand_RUS_Regul_Speed_Set(uint8_t *pQuery, uint8_t **ppAnswer); // Установка любой скорости и направления вразения мотора (скорость фактически 0 - 100, но float)
static SKLP_CommandResult_t EX_SKLP_ProcessCommand_RUS_Regul_Angle_Set(uint8_t *pQuery, uint8_t **ppAnswer); // Установка любого угла с любой скоростью поворота в любую сторону




// ВСЕГО 20 ДОП команд в ProjcetSoftConfig. Искать SKLP_CALLBACKS_AUX_COUNT

bool RUS_Regul_SKLP_InitCallbacks(void)
{
	// Стандартные команды
	//assert_param( NULL == SKLP_ServiceSetCallback( SKLP_COMMAND_RUS_REGUL_ID_GET,					SKLP_ProcessCommand_RUS_Regul_ID_Get ) );					//1
	assert_param( NULL == SKLP_ServiceSetCallback( SLKP_COMMAND_RUS_REGUL_NVM_GET,					SKLP_ProcessCommand_RUS_Regul_NVM_Get ) );					//2
	assert_param( NULL == SKLP_ServiceSetCallback( SLKP_COMMAND_RUS_REGUL_NVM_SET,					SKLP_ProcessCommand_RUS_Regul_NVM_Set ) );					//3
	assert_param( NULL == SKLP_ServiceSetCallback( SKLP_COMMAND_RUS_REGUL_MAIN_DATA_GET,			SKLP_ProcessCommand_RUS_Regul_MainData_Get ) );				//4
	assert_param( NULL == SKLP_ServiceSetCallback( SKLP_COMMAND_RUS_REGUL_EXTENDEED_DATA_GET,		SKLP_ProcessCommand_RUS_Regul_Extendeed_Data_Get ) );		//5
	// Команды управления регулятором
	assert_param( NULL == SKLP_ServiceSetCallback( SKLP_COMMAND_RUS_REGUL_MOTOR_POWER_MANAGE,		SKLP_ProcessCommand_RUS_Regul_Motor_Power_Managment ) );	//6
	assert_param( NULL == SKLP_ServiceSetCallback( SKLP_COMMAND_RUS_REGUL_OVERLAP_SET,				SKLP_ProcessCommand_RUS_Regul_Overlap_Set ) );				//7
	assert_param( NULL == SKLP_ServiceSetCallback( SKLP_COMMAND_RUS_REGUL_CALIBR_PRESS_SET,			SKLP_ProcessCommand_RUS_Regul_Start_Pressure_Calibr ) );	//8
	assert_param( NULL == SKLP_ServiceSetCallback( SKLP_COMMAND_RUS_REGUL_CALIBR_CURRENT_SET,		SKLP_ProcessCommand_RUS_Regul_Start_Current_Calibr ) );		//9
	assert_param( NULL == SKLP_ServiceSetCallback( SKLP_COMMAND_RUS_REGUL_PRESS_STABILIZATION,		SKLP_ProcessCommand_RUS_Regul_Press_Stabiliztion ) );		//10
	assert_param( NULL == SKLP_ServiceSetCallback( SKLP_COMMAND_RUS_REGUL_ADVANCED_MANAGMENT,		SKLP_ProcessCommand_RUS_Regul_Advanced_Managment ) );		//11
	// Расширенные команды
	assert_param( NULL == SKLP_ServiceSetCallback( EX_SKLP_COMMAND_RUS_REGUL_ANGLE_SET,				EX_SKLP_ProcessCommand_RUS_Regul_Angle_Set ) );				//12
	assert_param( NULL == SKLP_ServiceSetCallback( EX_SKLP_COMMAND_RUS_REGUL_SPEED_SET,				EX_SKLP_ProcessCommand_RUS_Regul_Speed_Set ) );				//13

	return true;
}


/*static SKLP_CommandResult_t SKLP_ProcessCommand_RUS_Regul_ID_Get(uint8_t *pQuery, uint8_t **ppAnswer) // Отдать ID
{
	return 0;
}*/

static SKLP_CommandResult_t SKLP_ProcessCommand_RUS_Regul_NVM_Get(uint8_t *pQuery, uint8_t **ppAnswer) // Отдать данные из NVM
{
	return 0;
}

static SKLP_CommandResult_t SKLP_ProcessCommand_RUS_Regul_NVM_Set(uint8_t *pQuery, uint8_t **ppAnswer) // Прочитать данные из NVM
{
#if 1
	assert_param( sizeof( uint8_t ) == sizeof ( SKLP_NVM_ID_t ) );
	assert_param( sizeof( uint8_t ) == sizeof ( NVM_Result_t ) );

	SKLP_CommandResult_t ReturnValue = SKLP_COMMAND_RESULT_ERROR;
	do
	{
		if( SKLP_ADDRESS_MYSELF != pQuery[SKLP_OFFSET_ADDRESS] )
			break;
		SKLP_RUS_Regul_NVM_Set_Query_t *pQueryBody = ( SKLP_RUS_Regul_NVM_Set_Query_t * ) ( pQuery + SKLP_OFFSET_DATA_QUERY );
		SKLP_NVM_ID_t SKLP_NVM_ID = pQueryBody->SKLP_NVM_ID;
		int16_t SKLP_NVM_BlockSize = ( int16_t ) SKLP_SIZE_DATA_QUERY( pQuery ) - sizeof( pQueryBody->SKLP_NVM_ID ) - sizeof( pQueryBody->CRC16 );
		if( SKLP_NVM_BlockSize <= 0 )
			break;
		uint16_t CRC16 = *( uint16_t * ) ( &pQueryBody->xData + SKLP_NVM_BlockSize );

		*ppAnswer = pQuery;
		SKLP_Command_RUS_Regul_NVM_GetSet_Answer_t *pAnswerBody = ( SKLP_Command_RUS_Regul_NVM_GetSet_Answer_t * ) ( *ppAnswer + SKLP_OFFSET_DATA_ANSWER );
		pAnswerBody->Result = NVM_Result_FailID;
		ReturnValue = sizeof( pAnswerBody->Result );
		for(uint8_t i = 0; i < SIZEOFARRAY( aSKLP_NVM_Tags ); i++)
			if( aSKLP_NVM_Tags[i].SKLP_ID == SKLP_NVM_ID )
			{
				NVM_Tag_t const *pTag = aSKLP_NVM_Tags[i].pTag;
				uint16_t BlockSize = pTag->Size;
				if( SKLP_NVM_BlockSize != BlockSize )
					break;
				pAnswerBody->Result = NVM_Result_FailData;
				if( CRC16 != CalcCRC16SKLP( &pQueryBody->xData, SKLP_NVM_BlockSize ) )
					break;
				assert_param( ( sizeof( pAnswerBody->Result ) + BlockSize ) <= SKLP_SIZE_DATA_MAX_ANSWER );
				switch( SKLP_NVM_ID )
				{
				default:
					pAnswerBody->Result = NVM_TagSaveNew( pTag, &pQueryBody->xData, &pAnswerBody->xData );
					//pAnswerBody->Result = NVM_TagSaveNew( pTag, &pQueryBody->xData, NULL );
				}
//				if( NVM_Result_FailNVM != pAnswerBody->Result )
//					ReturnValue += BlockSize;
				ReturnValue += BlockSize;		// Вернуть считанный блок, даже если были ошибки при записи
				break;
			}
	} while( 0 );
	return ReturnValue;
#endif
	//return 0;
}

static SKLP_CommandResult_t SKLP_ProcessCommand_RUS_Regul_MainData_Get( uint8_t *pQuery, uint8_t **ppAnswer ) // Чтение основного блока данных
{
	return 0;
}

static SKLP_CommandResult_t SKLP_ProcessCommand_RUS_Regul_Extendeed_Data_Get( uint8_t *pQuery, uint8_t **ppAnswer ) // Чтение расширенного блока данных (запись на SD)
{
	SKLP_CommandResult_t ReturnValue = SKLP_COMMAND_RESULT_ERROR;
	*ppAnswer = pQuery;
	do
	{
		if(SKLP_ADDRESS_BROADCAST == pQuery[SKLP_OFFSET_ADDRESS])
			break;
		
		SKLP_Command_RUS_Regul_Extendeed_Data_Get_Answer_t *pAnswerBody = (SKLP_Command_RUS_Regul_Extendeed_Data_Get_Answer_t*)(*ppAnswer + SKLP_OFFSET_DATA_ANSWER);
		
		pAnswerBody->ErrorFlag = RUS_Regul.ErrorFlag;										// 	[1]		U
		pAnswerBody->LogicalFlag = RUS_Regul.LogicalFlag;									// 	[2]		U
		pAnswerBody->SpeedSet =	Round_SI8(RUS_Regul.SpeedSet);								// 	[1] 	S
		pAnswerBody->SpeedMeas = Round_UI16(RUS_Regul.SpeedMeas * 100);						//	[2]		U
		pAnswerBody->AngleSet = Round_UI16(RUS_Regul.AngleSet * 100.0f);					//	[2]		U
		pAnswerBody->CurPosition = Round_UI16(RUS_Regul.CurPosition * 100.0f);				//	[2]		U
		pAnswerBody->TimerCNTValue = RUS_Regul.TimerCNTValue;								//	[2]		U
		pAnswerBody->Press = Round_SI16(RUS_Regul.Press * 100.0f);							//	[2]		S
		pAnswerBody->ADC_code = RUS_Regul.ADC_Code;											//	[4]		S
		pAnswerBody->CurPress = Round_SI16(RUS_Regul.CurPress * 100.0f);					//	[2]		S
		pAnswerBody->TarPress = RUS_Regul.PressSet;											//	[2]		U
		pAnswerBody->MotorPowerSupply = Round_UI8(RUS_Regul.MotorPowerSupply * 100.0f);		//	[1]		U
		pAnswerBody->MotorCurrent = Round_UI8(RUS_Regul.MotorCurrent * 100.0f);				//	[1]		U		
		pAnswerBody->CalibrNull = Round_UI16(RUS_Regul.CalibrNull * 100.0f);				//	[2]		U
		pAnswerBody->MinAngle = Round_UI16(RUS_Regul.MinAngle * 100.0f);					//	[2]		U
		pAnswerBody->MaxAngle = Round_UI16(RUS_Regul.MaxAngle * 100.0f);					//	[2]		U
		pAnswerBody->MinPress = Round_SI16(RUS_Regul.MinPress * 100.0f);					//	[2]		U
		pAnswerBody->MaxPress = Round_SI16(RUS_Regul.MaxPress * 100.0f);					//	[2]		U
		pAnswerBody->WorkZone = Round_UI16(RUS_Regul.WorkZone * 100.0f);					//	[2]		U
		pAnswerBody->WorkState = RUS_Regul.WorkState;										//	[1]		U
		pAnswerBody->WorkMode = RUS_Regul.WorkMode;											//	[1]		U

		ReturnValue = sizeof(SKLP_Command_RUS_Regul_Extendeed_Data_Get_Answer_t);
																							// TOTAL [38]
	} while(0);
	return ReturnValue;
}

static SKLP_CommandResult_t SKLP_ProcessCommand_RUS_Regul_Motor_Power_Managment( uint8_t *pQuery, uint8_t **ppAnswer ){
	SKLP_CommandResult_t ReturnValue = SKLP_COMMAND_RESULT_ERROR;
	*ppAnswer = pQuery;
	do
	{
		if(SKLP_ADDRESS_BROADCAST == pQuery[SKLP_OFFSET_ADDRESS])
			break;
		SKLP_Command_RUS_Regul_Motor_Power_Managment_Query_t *pQueryBody = (SKLP_Command_RUS_Regul_Motor_Power_Managment_Query_t*)(pQuery + SKLP_OFFSET_DATA_QUERY);
		if(sizeof(*pQueryBody) != SKLP_SIZE_DATA_QUERY(pQuery))
			break;

		if(Motor.fHoldPress == 1){
			xTimerStop(Motor.Timers.StabPressTimer, NULL);
			Motor.fHoldPress = 0;
		}

			
		RUS_Regul.fPowerManage = pQueryBody->pwrOn;
		
		SKLP_Command_RUS_Regul_Motor_Power_Managment_Answer_t *pAnswerBody = (SKLP_Command_RUS_Regul_Motor_Power_Managment_Answer_t*)(*ppAnswer + SKLP_OFFSET_DATA_ANSWER);

		if(0x00 == RUS_Regul.fPowerManage){
			GPIO_Common_Write(iGPIO_MOTOR_PWR, GPIO_PIN_RESET);
			pAnswerBody->Power = 0x00;
		}
		else if(0x01 == RUS_Regul.fPowerManage){
			GPIO_Common_Write(iGPIO_MOTOR_PWR, GPIO_PIN_SET);
			pAnswerBody->Power = 0x01;
		}
		else{
			break;
		}
		
		ReturnValue = sizeof(SKLP_Command_RUS_Regul_Motor_Power_Managment_Answer_t);
		
	}while(0);
	return ReturnValue;
}

static SKLP_CommandResult_t SKLP_ProcessCommand_RUS_Regul_Overlap_Set( uint8_t *pQuery, uint8_t **ppAnswer ){
	SKLP_CommandResult_t ReturnValue = SKLP_COMMAND_RESULT_ERROR;
	*ppAnswer = pQuery;
	do
	{
		if(SKLP_ADDRESS_BROADCAST == pQuery[SKLP_OFFSET_ADDRESS])
			break;
		
		SKLP_Command_RUS_Regul_Overlap_Set_Query_t *pQueryBody = (SKLP_Command_RUS_Regul_Overlap_Set_Query_t*)(pQuery + SKLP_OFFSET_DATA_QUERY);
		if(sizeof(*pQueryBody) != SKLP_SIZE_DATA_QUERY(pQuery))
			break;

		if(Motor.fHoldPress == 1){
			xTimerStop(Motor.Timers.StabPressTimer, NULL);
			Motor.fHoldPress = 0;
		}

		
		uint8_t OverlapPercent = pQueryBody->OverlapPercent;

		SKLP_Command_RUS_Regul_Overlap_Set_Answer_t *pAnswerBody = (SKLP_Command_RUS_Regul_Overlap_Set_Answer_t*)(**ppAnswer + SKLP_OFFSET_DATA_ANSWER);
		
		if(Motor.fCalibrated == 1){
			Motor.WorkMode = RotatingByAngle;
			RUS_Regul_Overlap_Set(OverlapPercent);
			pAnswerBody->error = 0;
		}
		else{
			pAnswerBody->error = 1;
		}
		ReturnValue = sizeof(SKLP_Command_RUS_Regul_Overlap_Set_Answer_t);
		
		
	}while(0);
	return ReturnValue;
}

static SKLP_CommandResult_t SKLP_ProcessCommand_RUS_Regul_Start_Current_Calibr(uint8_t *pQuery, uint8_t **ppAnswer){
	
	SKLP_CommandResult_t ReturnValue = SKLP_COMMAND_RESULT_ERROR;
	*ppAnswer = pQuery;
	do
	{
		if(SKLP_ADDRESS_BROADCAST == pQuery[SKLP_OFFSET_ADDRESS])
			break;
		// Инициируем процедуру поиска нулевого положения
		assert_param( NULL != RUS_Regul_EventGroup );
		xEventGroupSetBits( RUS_Regul_EventGroup, EVENTS_RUS_REGUL_CALIBR_OPAMP_START );

		if(Motor.fHoldPress == 1){
			xTimerStop(Motor.Timers.StabPressTimer, NULL);
			Motor.fHoldPress = 0;
		}

		Motor.WorkMode = CalibratingConsumption;
		
		ReturnValue = SKLP_COMMAND_RESULT_NO_REPLY;
	}
	while(0);
	return ReturnValue;
}

static SKLP_CommandResult_t SKLP_ProcessCommand_RUS_Regul_Start_Pressure_Calibr(uint8_t *pQuery, uint8_t **ppAnswer){
	
	SKLP_CommandResult_t ReturnValue = SKLP_COMMAND_RESULT_ERROR;
	*ppAnswer = pQuery;
	do
	{
		if(SKLP_ADDRESS_BROADCAST == pQuery[SKLP_OFFSET_ADDRESS])
			break;

		if(Motor.fHoldPress == 1){
			xTimerStop(Motor.Timers.StabPressTimer, NULL);
			Motor.fHoldPress = 0;
		}
	
		assert_param( NULL != RUS_Regul_EventGroup );
		xEventGroupSetBits( RUS_Regul_EventGroup, EVENTS_RUS_REGUL_CALIBR_PRESS_START );
		
		
		ReturnValue = SKLP_COMMAND_RESULT_NO_REPLY;
	}
	while(0);
	return ReturnValue;
}

static SKLP_CommandResult_t SKLP_ProcessCommand_RUS_Regul_Press_Stabiliztion(uint8_t *pQuery, uint8_t **ppAnswer){
	SKLP_CommandResult_t ReturnValue = SKLP_COMMAND_RESULT_ERROR;
	do
	{
		if(SKLP_ADDRESS_BROADCAST == pQuery[SKLP_OFFSET_ADDRESS])
			break;
		
		SKLP_HoldPress_Query_t *pQueryBody = (SKLP_HoldPress_Query_t*)(pQuery + SKLP_OFFSET_DATA_QUERY);
		
		RUS_Regul.PressSet = pQueryBody->tarPress;

		Motor_Stabilization_Press_Start();		
		
		
		ReturnValue = SKLP_COMMAND_RESULT_NO_REPLY;
		
	}while(0);
	return ReturnValue;
}

static SKLP_CommandResult_t SKLP_ProcessCommand_RUS_Regul_Advanced_Managment( uint8_t *pQuery, uint8_t **ppAnswer ){
	return 0;
}

static SKLP_CommandResult_t EX_SKLP_ProcessCommand_RUS_Regul_Speed_Set( uint8_t *pQuery, uint8_t **ppAnswer ){
	SKLP_CommandResult_t ReturnValue = SKLP_COMMAND_RESULT_ERROR;
	do
	{
		if(SKLP_ADDRESS_BROADCAST == pQuery[SKLP_OFFSET_ADDRESS])
			break;

		if(Motor.fHoldPress == 1){
			xTimerStop(Motor.Timers.StabPressTimer, NULL);
			Motor.fHoldPress = 0;
		}

		EX_SKLP_ProcessCommand_RUS_Regul_Speed_Set_Query_t *pQueryBody = (EX_SKLP_ProcessCommand_RUS_Regul_Speed_Set_Query_t*)(pQuery + SKLP_OFFSET_DATA_QUERY);
		float speed = pQueryBody->speed_percent;
		EX_RUS_Regul_Motor_Speed_Set(speed);

		ReturnValue = SKLP_COMMAND_RESULT_NO_REPLY;
		
	}while(0);
	return ReturnValue;
}

static SKLP_CommandResult_t EX_SKLP_ProcessCommand_RUS_Regul_Angle_Set( uint8_t *pQuery, uint8_t **ppAnswer ){
	SKLP_CommandResult_t ReturnValue = SKLP_COMMAND_RESULT_ERROR;
	do
	{
		if(SKLP_ADDRESS_BROADCAST == pQuery[SKLP_OFFSET_ADDRESS])
			break;

		if(Motor.fHoldPress == 1){
			xTimerStop(Motor.Timers.StabPressTimer, NULL);
			Motor.fHoldPress = 0;
		}
		
		EX_SKLP_Command_RUS_Regul_Angle_Set_Query_t *pQueryBody = (EX_SKLP_Command_RUS_Regul_Angle_Set_Query_t*)(pQuery + SKLP_OFFSET_DATA_QUERY);
		int16_t angle = pQueryBody->angle;
		float speed = pQueryBody->speed;
		Motor.WorkMode = RotatingByAngle;
		EX_RUS_Regul_Motor_Angle_Set(angle, speed);

		ReturnValue = SKLP_COMMAND_RESULT_NO_REPLY;
	}while(0);

	return ReturnValue;
}






