// SKLP_MS_TransportLocal_641_05.c
// Локализация SKLP_MS_Transport.c под проект
// - инициализация необходимой периферии
// - реализация обработчиков прерываний
// - реализация требуемых задач мастера и слейва

// Проект контроллера спектрометрических АЦП ЛУЧ.641.00.05.00/ЛУЧ.641.00.06.00 длЯ ГГК-ЛП ЛУЧ.641.00.00.00
// Один голый UART, протокол SKLP на 460800, длЯ свЯзи с платой GGLP_Tele.
// Второй голый UART, протокол SKLP на 57600, длЯ свЯзи с МПИ/Colibri через модем на плате GGLP_Tele.

// Также клон длЯ СпАЦП 641_05/641_06 длЯ ГГК-ЛП ЛУЧ.641.00.00.00 -
// а вообще, под этот проект закладываетсЯ SKLP_MS_TransportLocal_641_05.c с двумЯ UART.

#include "ProjectConfig.h"		// конфиг платформы
#include "stm32xxxx_hal.h"		// дрова периферии
#include "Platform_common.h"
#include "SKLP_MS_Transport.h"
#include "SKLP_MS_TransportInterface.h"
#include "TaskConfig.h"
#include "SKLP_Service.h"
#include "Common_gpio.h"
#include "RUS_Regul_Main.h"

extern RUS_Regul_t RUS_Regul;

#if	!defined ( USE_PLATFORM_OKR_354_10 )
#error "Localization for project OKR_354_10 only!"
#endif

#if	defined( USE_PLATFORM_OKR_354_10 )
// Определение адреса на шине в зависимости от джамперов на плате
static uint8_t SKLP_CustomAddress = 0;
static void RUS_Regul_BoardID_Get(void)
{
	bool A0 = GPIO_Common_Read( iGPIO_ADDRESS_0 );
	bool A1 = GPIO_Common_Read( iGPIO_ADDRESS_1 );
	if		( !A0 && !A1 )	SKLP_CustomAddress = SKLP_ADDRESS_RUS_Pump0;
	else if (  A0 && !A1 )	SKLP_CustomAddress = SKLP_ADDRESS_RUS_Pump1;
	else if ( !A0 &&  A1 )	SKLP_CustomAddress = SKLP_ADDRESS_RUS_Pump2;
	else					SKLP_CustomAddress = SKLP_ADDRESS_RUS_Pump;
	RUS_Regul.BoardAdress = SKLP_CustomAddress;
}
uint8_t SKLP_CustomAddressGet( void )	{	return SKLP_CustomAddress;	}
#else
void SKLP_CustomAddressInit( void )	{}
#endif
// ************************* Статические данные *************************
// Приемный циклический буфер UART для протокола СКЛ
BYTE_QUEUE_CREATE( SKLP_RxQueue_Modem, SKLP_RX_BUFFER_SIZE );
BYTE_QUEUE_CREATE( SKLP_RxQueue_Service, SKLP_RX_BUFFER_SIZE );

// Структура данных протокола СКЛ - инициализация в SKLP_Init()
#define	iSKLP_Service	0
#define	iSKLP_Modem		1
static __no_init SKLP_Interface_t aSKLP_Interfaces[2];


// Коллбек из обработчика прерывания UART.RxIdle от любых UART
void HAL_UART_RxIdleCallback( UART_HandleTypeDef *huart )
{
	if( huart == &aSKLP_Interfaces[iSKLP_Service].pUART_Hdl->Common )
		SKLP_ReceiveFragment( &aSKLP_Interfaces[iSKLP_Service] );
	else if( huart == &aSKLP_Interfaces[iSKLP_Modem].pUART_Hdl->Common )
		SKLP_ReceiveFragment( &aSKLP_Interfaces[iSKLP_Modem] );
	else
		assert_param( 0 );
}


// Коллбеки таймеров контроля пауз
void SKLP_Timer_Service_PeriodElapsedCallback( void )
{
	SKLP_TimerElapsed( &aSKLP_Interfaces[iSKLP_Service] );
}

void SKLP_Timer_Aux_PeriodElapsedCallback( void )
{
	SKLP_TimerElapsed( &aSKLP_Interfaces[iSKLP_Modem] );
}

// Инициализация периферии интерфейсов SKLP
static void SKLP_InterfacePeripheryInit( void )
{
	SKLP_Interface_t * pInterface;
	// ПрЯмое соединение с головной платой
	pInterface = &aSKLP_Interfaces[iSKLP_Service];
	pInterface->pUART_Hdl				= &COM_SKLP_SERVICE_UART_EXT_HDL;
	pInterface->pTimerRxHdl				= &SKLP_TIMER_SERVICE_hdl;
	pInterface->pTimerRxHdl->Instance	= SKLP_TIMER_SERVICE;
	// Модем в составе коловной платы,
	pInterface = &aSKLP_Interfaces[iSKLP_Modem];
	pInterface->pUART_Hdl				= &COM_SKLP_AUX_UART_EXT_HDL;
	pInterface->pTimerRxHdl				= &SKLP_TIMER_AUX_hdl;
	pInterface->pTimerRxHdl->Instance	= SKLP_TIMER_AUX;

	// Инициализация последовательных интерфейсов
	// ******************************************
	// Обмен с головным микроконтроллером напрЯмую
	UART_InitTypeDef *pInit = &aSKLP_Interfaces[iSKLP_Service].pUART_Hdl->Common.Init;
	pInit->BaudRate	 		= SKLP_BAUD_RUS_REGUL_DEFAULT;
	pInit->WordLength  		= UART_WORDLENGTH_8B;
	pInit->StopBits	 		= UART_STOPBITS_1;
	pInit->Parity	 		= UART_PARITY_NONE;
	pInit->Mode		 		= UART_MODE_TX_RX;
	pInit->HwFlowCtl		= UART_HWCONTROL_NONE;
	pInit->OverSampling		= UART_OVERSAMPLING_8;
	pInit->OneBitSampling	= UART_ONE_BIT_SAMPLE_DISABLE;
#ifdef	STM32L4
	UART_AdvFeatureInitTypeDef *pAdvInit = &aSKLP_Interfaces[iSKLP_Service].pUART_Hdl->Common.AdvancedInit;
	pAdvInit->AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
#endif	// STM32L4	
	COM_INIT( COM_SKLP_SERVICE );

	// Дополнительный UART
	aSKLP_Interfaces[iSKLP_Modem].pUART_Hdl->Common.Init = *pInit;
#ifdef	STM32L4
	pAdvInit = &aSKLP_Interfaces[iSKLP_Modem].pUART_Hdl->Common.AdvancedInit;
	pAdvInit->AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
#endif	// STM32L4	
	COM_INIT( COM_SKLP_AUX );

	// Инициализацией модема занимаетсЯ исключительно микроконтроллер на головной плате

	// Инициализация таймеров контроля пауз, аналогично длЯ обоих интерфейсов
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

// Низкоуровневые инициализаторы и деинициализаторы таймеров контроля пауз
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

// Задача реализации протокола SKLP.Slave
static void SKLP_SlaveTask( void *pParameters );
static QueueHandle_t pSKLP_SlaveMessageQueueHdl;
//static void SKLP_LedRxTimerCallback( TimerHandle_t pTimer );

// Инициализация протокола, до запуска операционки
bool SKLP_TaskInit( void )
{
	assert_param( taskSCHEDULER_NOT_STARTED == xTaskGetSchedulerState( ) );

	bool Result = false;
	do
	{
		// *****************************************
		// Инициализировать модуль обработки пакетов
		if( !SKLP_ServiceInit( ) )
			break;

		// Создать задачу и очередь SKLP_Slave
		// *****************************************
		// Создать задачу SKLP_Slave
		TaskHandle_t xTaskHandle;
		if( pdTRUE != xTaskCreate( SKLP_SlaveTask, TASK_SKLP_NAME, TASK_SKLP_STACK_SIZE, NULL, TASK_SKLP_PRIORITY, &xTaskHandle ) )
			break;
		if( NULL == xTaskHandle )
			break;
		SysState_AppendTaskHandler( xTaskHandle );
		// Создать очередь сообщений SKLP_Slave, одну на оба интерфейса
		pSKLP_SlaveMessageQueueHdl = xQueueCreate( 6, sizeof( SKLP_Message_t ) );
		if( NULL == pSKLP_SlaveMessageQueueHdl )
			break;

		// Произвести инициализацию последовательных интерфейсов
		// *****************************************
		// Обнулить структуры обоих интерфейсов
		memset( aSKLP_Interfaces, 0, sizeof( aSKLP_Interfaces ) );
		aSKLP_Interfaces[iSKLP_Service].pName	= "Service";
		aSKLP_Interfaces[iSKLP_Modem].pName		= "Modem SIG60";
		// Инициализировать периферию интерфейсов (UART & Timer-Counter)
		SKLP_InterfacePeripheryInit( );
		RUS_Regul_BoardID_Get();
		// Прием пакетов будет запущен после запуска задачи
		Result = true;
	} while( 0 );
	// Следовало бы освободить ресурсы, если инициализация не удалась - но нет.
	return Result;
}


// Задача приема по протоколу СКЛ, нормально заблокирована ожиданием сообщений от последовательного канала.
// Прием пакетов производится в обработчике прерывания UART.Rx.Idle - SKLP_ReceivePacketFragment(), который добавляет в очередь сообщение о приеме.
// Коллбеки ошибок UART добавляют сообщения об ошибках, которые также надо обрабатывать.
static void SKLP_SlaveTask( void *pParameters )
{
	( void ) pParameters;
	assert_param( NULL != pSKLP_SlaveMessageQueueHdl );

	// Приступить к приему пакетов SKLP по обоим интерфейсам
	// *****************************************************
	// Подключить последовательные интерфейсы к очереди SKLP_Slave
	aSKLP_Interfaces[iSKLP_Service].pMessageQueue = aSKLP_Interfaces[iSKLP_Modem].pMessageQueue = pSKLP_SlaveMessageQueueHdl;
	// Подписаться на события по приему пакетов, адресованнх этому модулю
	aSKLP_Interfaces[iSKLP_Service].EventsAllowed	= EVENT_SKLP_QUERY_2ME | EVENT_SKLP_QUERY_2ALL;
	aSKLP_Interfaces[iSKLP_Modem].EventsAllowed		= EVENT_SKLP_QUERY_2ME | EVENT_SKLP_QUERY_2ALL;
	// Приступить к ожиданию входящих пакетов
	aSKLP_Interfaces[iSKLP_Service].State = aSKLP_Interfaces[iSKLP_Modem].State = SKLP_STATE_WaitStart;
	// Включить UART на прием в очередь, событий по прерываниям не генерить, RxIdle обработать в коллбеке
	assert_param( HAL_OK == HAL_UART_Ext_ReceiveCyclicStart( aSKLP_Interfaces[iSKLP_Service].pUART_Hdl, &SKLP_RxQueue_Service, 0 ) );
	assert_param( HAL_OK == HAL_UART_Ext_ReceiveCyclicStart( aSKLP_Interfaces[iSKLP_Modem].pUART_Hdl, &SKLP_RxQueue_Modem, 0 ) );
	DWT_AppendTimestampTag( "Start SKLP" );

	while( 1 )
	{
		SKLP_Message_t Message;
		// Ожидать событие от последовательных интерфейсов
		assert_param( pdTRUE == xQueueReceive( pSKLP_SlaveMessageQueueHdl, &Message, portMAX_DELAY ) );
		// Проверить интерфейс, в который прилетел пакет
		if( &aSKLP_Interfaces[iSKLP_Service] == Message.pInterface )
		{	// На основной интерфейс допускаютсЯ запросы по всем поддерживаемым адресам
			switch( Message.Event )
			{
			case EVENT_SKLP_QUERY_2ALL: 	// Широковещательный пакет для всех слейвов
			case EVENT_SKLP_QUERY_2ME: 		// Персональный пакет для этого слейва по любому из допустимых адресов
				SKLP_ProcessPacket( Message.pInterface, Message.Packet );
				break;
			default:						// Недопустимое событие
				assert_param( 0 );
			}
		}
		if( &aSKLP_Interfaces[iSKLP_Modem] == Message.pInterface )
		{	// На основной интерфейс допускаютсЯ запросы по всем поддерживаемым адресам
			switch( Message.Event )
			{
			case EVENT_SKLP_QUERY_2ALL: 	// Широковещательный пакет для всех слейвов
			case EVENT_SKLP_QUERY_2ME: 		// Персональный пакет для этого слейва по любому из допустимых адресов
				SKLP_ProcessPacket( Message.pInterface, Message.Packet );
				break;
			default:						// Недопустимое событие
				assert_param( 0 );
			}
		}
		
	}			
}

