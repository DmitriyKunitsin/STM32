// SKLP_MS_TransportLocal_641_05.c
// ����������� SKLP_MS_Transport.c ��� ������
// - ������������� ����������� ���������
// - ���������� ������������ ����������
// - ���������� ��������� ����� ������� � ������

// ������ ����������� ������������������ ��� ���.641.00.05.00/���.641.00.06.00 ��� ���-�� ���.641.00.00.00
// ���� ����� UART, �������� SKLP �� 460800, ��� ����� � ������ GGLP_Tele.
// ������ ����� UART, �������� SKLP �� 57600, ��� ����� � ���/Colibri ����� ����� �� ����� GGLP_Tele.

// ����� ���� ��� ����� 641_05/641_06 ��� ���-�� ���.641.00.00.00 -
// � ������, ��� ���� ������ ������������� SKLP_MS_TransportLocal_641_05.c � ����� UART.

#include "ProjectConfig.h"		// ������ ���������
#include "stm32xxxx_hal.h"		// ����� ���������
#include "Platform_common.h"
#include "SKLP_MS_Transport.h"
#include "SKLP_MS_TransportInterface.h"
#include "TaskConfig.h"
#include "SKLP_Service.h"
#include "Common_gpio.h"
//#include "RUS_Regul_Main.h"

//extern RUS_Regul_t RUS_Regul;

#if	!defined ( USE_PLATFORM_OKR_354_10 ) && !defined ( USE_PLATFORM_LOOCH_601_03 )
#error "Localization for project OKR_354_10 only!"
#endif

// ************************* ����������� ������ *************************
// �������� ����������� ����� UART ��� ��������� ���
BYTE_QUEUE_CREATE( SKLP_RxQueue_Modem, SKLP_RX_BUFFER_SIZE );
BYTE_QUEUE_CREATE( SKLP_RxQueue_Service, SKLP_RX_BUFFER_SIZE );

// ��������� ������ ��������� ��� - ������������� � SKLP_Init()
#define	iSKLP_Machester	0
#define	iSKLP_USART		1
static __no_init SKLP_Interface_t aSKLP_Interfaces[2];


// ������� �� ����������� ���������� UART.RxIdle �� ����� UART
void HAL_UART_RxIdleCallback( UART_HandleTypeDef *huart )
{
	if( huart == &aSKLP_Interfaces[iSKLP_Machester].pUART_Hdl->Common )
		SKLP_ReceiveFragment( &aSKLP_Interfaces[iSKLP_Machester] );
	else if( huart == &aSKLP_Interfaces[iSKLP_USART].pUART_Hdl->Common )
		SKLP_ReceiveFragment( &aSKLP_Interfaces[iSKLP_USART] );
	else
		assert_param( 0 );
}


// �������� �������� �������� ����
void SKLP_Timer_Service_PeriodElapsedCallback( void )
{
	SKLP_TimerElapsed( &aSKLP_Interfaces[iSKLP_Machester] );
}

void SKLP_Timer_Aux_PeriodElapsedCallback( void )
{
	SKLP_TimerElapsed( &aSKLP_Interfaces[iSKLP_USART] );
}

// ������������� ��������� ����������� SKLP
static void SKLP_InterfacePeripheryInit( void )
{
	SKLP_Interface_t * pInterface;
	// ������ ���������� � �������� ������
	pInterface = &aSKLP_Interfaces[iSKLP_USART];
	pInterface->pUART_Hdl				= &COM_SKLP_SERVICE_UART_EXT_HDL;
	pInterface->pTimerRxHdl				= &SKLP_TIMER_SERVICE_hdl;
	pInterface->pTimerRxHdl->Instance	= SKLP_TIMER_SERVICE;
	// ����� � ������� �������� �����,
	pInterface = &aSKLP_Interfaces[iSKLP_Machester];
	pInterface->pUART_Hdl				= &COM_SKLP_AUX_UART_EXT_HDL;
	pInterface->pTimerRxHdl				= &SKLP_TIMER_AUX_hdl;
	pInterface->pTimerRxHdl->Instance	= SKLP_TIMER_AUX;

	// ������������� ���������������� �����������
	// ******************************************
	// ����� � �������� ����������������� ��������
	UART_InitTypeDef *pInit = &aSKLP_Interfaces[iSKLP_USART].pUART_Hdl->Common.Init;
	pInit->BaudRate	 		= SKLP_BAUD_DEFAULT;
	pInit->WordLength  		= UART_WORDLENGTH_8B;
	pInit->StopBits	 		= UART_STOPBITS_1;
	pInit->Parity	 		= UART_PARITY_NONE;
	pInit->Mode		 		= UART_MODE_TX_RX;
	pInit->HwFlowCtl		= UART_HWCONTROL_NONE;
	pInit->OverSampling		= UART_OVERSAMPLING_8;
	pInit->OneBitSampling	= UART_ONE_BIT_SAMPLE_DISABLE;
#ifdef	STM32L4
	UART_AdvFeatureInitTypeDef *pAdvInit = &aSKLP_Interfaces[iSKLP_USART].pUART_Hdl->Common.AdvancedInit;
	pAdvInit->AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
#endif	// STM32L4	
	COM_INIT( COM_SKLP_SERVICE );

	// �������������� UART
	aSKLP_Interfaces[iSKLP_Machester].pUART_Hdl->Common.Init = *pInit;
	pInit = &aSKLP_Interfaces[iSKLP_Machester].pUART_Hdl->Common.Init;
	pInit->BaudRate	 		= 57600;
#ifdef	STM32L4
	pAdvInit = &aSKLP_Interfaces[iSKLP_Machester].pUART_Hdl->Common.AdvancedInit;
	pAdvInit->AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
#endif	// STM32L4	
	COM_INIT( COM_SKLP_AUX );

	// �������������� ������ ���������� ������������� ��������������� �� �������� �����

	// ������������� �������� �������� ����, ���������� ��� ����� �����������
	// ************************************
	for( int i = 0; i < SIZEOFARRAY( aSKLP_Interfaces ); i++ )
	{
		TIM_HandleTypeDef * const pTimer_hdl = aSKLP_Interfaces[i].pTimerRxHdl;
		pTimer_hdl->Init.CounterMode = TIM_COUNTERMODE_UP;
		uint32_t ByteRate = HAL_UART_Ext_CalcByteRate( aSKLP_Interfaces[i].pUART_Hdl );
		assert_param( 0 != ByteRate );
		uint32_t TimerPeriod_us = ( uint32_t ) ( ( 1000000 * SKLP_TIMERRX_PAUSE_BYTES ) / ByteRate );
		assert_param( TIM_Common_SetupPrescalers( pTimer_hdl, TimerPeriod_us, SKLP_TIMER_CLOCKDIVISION ) );
		__HAL_TIM_RESET_HANDLE_STATE( pTimer_hdl );
		assert_param( HAL_OK == HAL_TIM_Base_Init( pTimer_hdl ) );
	}
}

// �������������� �������������� � ���������������� �������� �������� ����
void SKLP_Timer_Service_MspInit( void )
{
	TIM_CLK_ENABLE( SKLP_TIMER_SERVICE );
	HAL_NVIC_SetPriority( SKLP_TIMER_SERVICE_IRQn, SKLP_TIMER_IRQ_PREEMTPRIORITY, SKLP_TIMER_IRQ_SUBPRIORITY );
	HAL_NVIC_EnableIRQ( SKLP_TIMER_SERVICE_IRQn );
}

void SKLP_Timer_Service_MspDeInit( void )
{
	assert_param( 0 );
}

void SKLP_Timer_Aux_MspInit( void )
{
	TIM_CLK_ENABLE( SKLP_TIMER_AUX );
	HAL_NVIC_SetPriority( SKLP_TIMER_AUX_IRQn, SKLP_TIMER_IRQ_PREEMTPRIORITY, SKLP_TIMER_IRQ_SUBPRIORITY );
	HAL_NVIC_EnableIRQ( SKLP_TIMER_AUX_IRQn );
}

void SKLP_Timer_Aux_MspDeInit( void )
{
	assert_param( 0 );
}

// ������ ���������� ��������� SKLP.Slave
static void SKLP_SlaveTask( void *pParameters );
static QueueHandle_t pSKLP_ManchesterMessageQueueHdl;
static QueueHandle_t pSKLP_UsartMessageQueueHdl;

//static void SKLP_LedRxTimerCallback( TimerHandle_t pTimer );

// ������������� ���������, �� ������� �����������
bool SKLP_TaskInit( void )
{
	assert_param( taskSCHEDULER_NOT_STARTED == xTaskGetSchedulerState( ) );

	bool Result = false;
	do
	{
		// *****************************************
		// ���������������� ������ ��������� �������
		if( !SKLP_ServiceInit( ) )
			break;

		// ������� ������ � ������� SKLP_Slave
		// *****************************************
		// ������� ������ SKLP_Slave
		TaskHandle_t xTaskHandle;
		if( pdTRUE != xTaskCreate( SKLP_SlaveTask, TASK_SKLP_NAME, TASK_SKLP_STACK_SIZE, NULL, TASK_SKLP_PRIORITY, &xTaskHandle ) )
			break;
		if( NULL == xTaskHandle )
			break;
		SysState_AppendTaskHandler( xTaskHandle );
		// ������� ������� ��������� SKLP_Slave, ���� �� ��� ����������
		pSKLP_UsartMessageQueueHdl = xQueueCreate( 6, sizeof( SKLP_Message_t ) );
		pSKLP_ManchesterMessageQueueHdl = xQueueCreate(6, sizeof( SKLP_Message_t) );
		if( NULL == pSKLP_UsartMessageQueueHdl )
			break;

		// ���������� ������������� ���������������� �����������
		// *****************************************
		// �������� ��������� ����� �����������
		memset( aSKLP_Interfaces, 0, sizeof( aSKLP_Interfaces ) );
		aSKLP_Interfaces[iSKLP_Machester].pName	= "Service";
		aSKLP_Interfaces[iSKLP_USART].pName		= "Modem SIG60";
		// ���������������� ��������� ����������� (UART & Timer-Counter)
		SKLP_InterfacePeripheryInit( );
//		RUS_Regul_BoardID_Get();
		// ����� ������� ����� ������� ����� ������� ������
		Result = true;
	} while( 0 );
	// ��������� �� ���������� �������, ���� ������������� �� ������� - �� ���.
	return Result;
}


// ������ ������ �� ��������� ���, ��������� ������������� ��������� ��������� �� ����������������� ������.
// ����� ������� ������������ � ����������� ���������� UART.Rx.Idle - SKLP_ReceivePacketFragment(), ������� ��������� � ������� ��������� � ������.
// �������� ������ UART ��������� ��������� �� �������, ������� ����� ���� ������������.
static void SKLP_SlaveTask( void *pParameters )
{
	( void ) pParameters;
	assert_param( NULL != pSKLP_UsartMessageQueueHdl );

	// ���������� � ������ ������� SKLP �� ����� �����������
	// *****************************************************
	// ���������� ���������������� ���������� � ������� SKLP_Slave
	aSKLP_Interfaces[iSKLP_Machester].pMessageQueue = pSKLP_ManchesterMessageQueueHdl;
	aSKLP_Interfaces[iSKLP_USART].pMessageQueue = pSKLP_UsartMessageQueueHdl;
	// ����������� �� ������� �� ������ �������, ����������� ����� ������
	aSKLP_Interfaces[iSKLP_Machester].EventsAllowed	= EVENT_SKLP_QUERY_2ME |  EVENT_SKLP_QUERY_2OTHER ;
	aSKLP_Interfaces[iSKLP_USART].EventsAllowed		= EVENT_SKLP_ANSWER;
	// ���������� � �������� �������� �������
	aSKLP_Interfaces[iSKLP_Machester].State = aSKLP_Interfaces[iSKLP_USART].State = SKLP_STATE_WaitStart;
	// �������� UART �� ����� � �������, ������� �� ����������� �� ��������, RxIdle ���������� � ��������
	assert_param( HAL_OK == HAL_UART_Ext_ReceiveCyclicStart( aSKLP_Interfaces[iSKLP_Machester].pUART_Hdl, &SKLP_RxQueue_Service, 0 ) );
	assert_param( HAL_OK == HAL_UART_Ext_ReceiveCyclicStart( aSKLP_Interfaces[iSKLP_USART].pUART_Hdl, &SKLP_RxQueue_Modem, 0 ) );
	DWT_AppendTimestampTag( "Start SKLP" );

	static uint8_t aTmpBuff[1024] = {0};
	while( 1 )
	{
		SKLP_Message_t Message;
		// ������� ������� �� ���������������� �����������
		assert_param( pdTRUE == xQueueReceive( pSKLP_ManchesterMessageQueueHdl, &Message, portMAX_DELAY ) );
		// ��������� ���������, � ������� �������� �����
//		if( &aSKLP_Interfaces[iSKLP_USART] == Message.pInterface )
//		{	// �� �������� ��������� ����������� ������� �� ���� �������������� �������
//			switch( Message.Event ) // 485
//			{
//			case EVENT_SKLP_QUERY_2ALL: 	// ����������������� ����� ��� ���� �������
//			case EVENT_SKLP_QUERY_2OTHER:
//			case EVENT_SKLP_ANSWER: {
//					bool ReceiverValid = SKLP_ReceivePacket(aTmpBuff, sizeof(aTmpBuff), &SKLP_RxQueue_Service , Message.pInterface);
//					if(true == ReceiverValid) {
//						SKLP_SendPacket(aTmpBuff, Message.Packet.Size, SKLP_SendPacketFormat_Answer, aSKLP_Interfaces[iSKLP_USART].pUART_Hdl);
//					}
//				}
//				// ����������������� ����� ��� ���� �������
//			case EVENT_SKLP_QUERY_2ME:	// ����������������� ����� ��� ���� �������
//				break;
//			default:						// ������������ �������
//				assert_param( 0 );
//			}
//		}
		if( &aSKLP_Interfaces[iSKLP_Machester] == Message.pInterface )
		{	// �� �������� ��������� ����������� ������� �� ���� �������������� �������
			switch( Message.Event )
			{
			case EVENT_SKLP_QUERY_2OTHER: 	// ����������������� ����� ��� ���� �������
				{// �������� ������� ��� ���
					bool ReceiverValid =  SKLP_ReceivePacket(aTmpBuff, sizeof(aTmpBuff) , &SKLP_RxQueue_Modem , Message.Packet);
					if(true == ReceiverValid) {
						HAL_UART_Ext_Transmit(aSKLP_Interfaces[iSKLP_Machester].pUART_Hdl, aTmpBuff, Message.Packet.Size, 0);
//						SKLP_SendPacket(aTmpBuff, Message.Packet.Size, SKLP_SendPacketFormat_Query, aSKLP_Interfaces[iSKLP_Machester].pUART_Hdl);
						assert_param( pdTRUE == xQueueReceive( pSKLP_UsartMessageQueueHdl, &Message, pdMS_TO_TICKS( 100 ) ) );
					switch ( Message.Event )
						{
							case EVENT_SKLP_ANSWER:
								bool ReceiverValid = SKLP_ReceivePacket(aTmpBuff, sizeof(aTmpBuff), &SKLP_RxQueue_Service , Message.Packet);
									if(true == ReceiverValid) {
									HAL_UART_Ext_Transmit(aSKLP_Interfaces[iSKLP_Machester].pUART_Hdl, aTmpBuff, Message.Packet.Size, 0);
//									SKLP_SendPacket(aTmpBuff, Message.Packet.Size, SKLP_SendPacketFormat_Answer, aSKLP_Interfaces[iSKLP_USART].pUART_Hdl);
									}
							break;
						}
					}
					}
			break;
			case EVENT_SKLP_QUERY_2ME: 		// ������������ ����� ��� ����� ������ �� ������ �� ���������� �������
				SKLP_ProcessPacket( Message.pInterface, Message.Packet );
				break;
			default:						// ������������ �������
				assert_param( 0 );
			}
		}
		
	}			
}

