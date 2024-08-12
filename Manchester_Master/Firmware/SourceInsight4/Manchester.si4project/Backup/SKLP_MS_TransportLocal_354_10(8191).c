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
#include "Manchester.h"
//#include "RUS_Regul_Main.h"

//extern RUS_Regul_t RUS_Regul;

#if	!defined ( USE_PLATFORM_OKR_354_10 ) && !defined ( USE_PLATFORM_LOOCH_601_03 )
#error "Localization for project OKR_354_10 only!"
#endif

// ************************* Статические данные *************************
// Приемный циклический буфер UART для протокола СКЛ
BYTE_QUEUE_CREATE( SKLP_RxQueue_USART, SKLP_RX_BUFFER_SIZE );
BYTE_QUEUE_CREATE( SKLP_RxQueue_Manchester, SKLP_RX_BUFFER_SIZE );

// Структура данных протокола СКЛ - инициализация в SKLP_Init()
#define	iSKLP_Machester	0
#define	iSKLP_USART		1
static __no_init SKLP_Interface_t aSKLP_Interfaces[2];


// Коллбек из обработчика прерывания UART.RxIdle от любых UART
void HAL_UART_RxIdleCallback( UART_HandleTypeDef *huart )
{
	if( huart == &aSKLP_Interfaces[iSKLP_Machester].pUART_Hdl->Common )
		SKLP_ReceiveFragment( &aSKLP_Interfaces[iSKLP_Machester] );
	else if( huart == &aSKLP_Interfaces[iSKLP_USART].pUART_Hdl->Common )
		SKLP_ReceiveFragment( &aSKLP_Interfaces[iSKLP_USART] );
	else
		assert_param( 0 );
}


// Коллбеки таймеров контроля пауз
void SKLP_Timer_Service_PeriodElapsedCallback( void )
{
	SKLP_TimerElapsed( &aSKLP_Interfaces[iSKLP_Machester] );
}

void SKLP_Timer_Aux_PeriodElapsedCallback( void )
{
	SKLP_TimerElapsed( &aSKLP_Interfaces[iSKLP_USART] );
}

// Инициализация периферии интерфейсов SKLP
static void SKLP_InterfacePeripheryInit( void )
{
	SKLP_Interface_t * pInterface;
	// ПрЯмое соединение с головной платой
	pInterface = &aSKLP_Interfaces[iSKLP_USART];
	pInterface->pUART_Hdl				= &COM_SKLP_SERVICE_UART_EXT_HDL;
	pInterface->pTimerRxHdl				= &SKLP_TIMER_SERVICE_hdl;
	pInterface->pTimerRxHdl->Instance	= SKLP_TIMER_SERVICE;
	// Модем в составе коловной платы,
	pInterface = &aSKLP_Interfaces[iSKLP_Machester];
	pInterface->pUART_Hdl				= &COM_SKLP_AUX_UART_EXT_HDL;
	pInterface->pTimerRxHdl				= &SKLP_TIMER_AUX_hdl;
	pInterface->pTimerRxHdl->Instance	= SKLP_TIMER_AUX;

	// Инициализация последовательных интерфейсов
	// ******************************************
	// Обмен с головным микроконтроллером напрЯмую
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

	// Дополнительный UART
	aSKLP_Interfaces[iSKLP_Machester].pUART_Hdl->Common.Init = *pInit;
	pInit = &aSKLP_Interfaces[iSKLP_Machester].pUART_Hdl->Common.Init;
	pInit->BaudRate	 		= 57600;
#ifdef	STM32L4
	pAdvInit = &aSKLP_Interfaces[iSKLP_Machester].pUART_Hdl->Common.AdvancedInit;
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

/*	Изменение конфигурация портов для работы с Манчестером	*/
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void change_Configure_GPIO_Read();

void change_Conffigure_GPIO_Write();

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */



// Задача реализации протокола SKLP.Slave
static void SKLP_SlaveTask( void *pParameters );
static QueueHandle_t pSKLP_ManchesterMessageQueueHdl;
static QueueHandle_t pSKLP_UsartMessageQueueHdl;

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
		pSKLP_UsartMessageQueueHdl = xQueueCreate( 6, sizeof( SKLP_Message_t ) );
		pSKLP_ManchesterMessageQueueHdl = xQueueCreate(6, sizeof( SKLP_Message_t) );
		if( NULL == pSKLP_UsartMessageQueueHdl )
			break;

		// Произвести инициализацию последовательных интерфейсов
		// *****************************************
		// Обнулить структуры обоих интерфейсов
		memset( aSKLP_Interfaces, 0, sizeof( aSKLP_Interfaces ) );
		aSKLP_Interfaces[iSKLP_Machester].pName	= "Service";
		aSKLP_Interfaces[iSKLP_USART].pName		= "Modem SIG60";
		// Инициализировать периферию интерфейсов (UART & Timer-Counter)
		SKLP_InterfacePeripheryInit( );
//		RUS_Regul_BoardID_Get();
		// Прием пакетов будет запущен после запуска задачи
		Result = true;
	} while( 0 );
	// Следовало бы освободить ресурсы, если инициализация не удалась - но нет.
	return Result;
}

//TIM_HandleTypeDef htim1;
#define	htim1	TIM1_hdl

void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(htim->Instance==TIM1)
  {
  /* USER CODE BEGIN TIM1_MspPostInit 0 */

  /* USER CODE END TIM1_MspPostInit 0 */

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**TIM1 GPIO Configuration
    PB0     ------> TIM1_CH2N
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN TIM1_MspPostInit 1 */

  /* USER CODE END TIM1_MspPostInit 1 */
  }

}

void TIM1_MspInit( void )
{
    __HAL_RCC_TIM1_CLK_ENABLE();
}

static void MX_TIM1_Init(void) {

	/* USER CODE BEGIN TIM1_Init 0 */

	/* USER CODE END TIM1_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
	TIM_MasterConfigTypeDef sMasterConfig = { 0 };
	TIM_OC_InitTypeDef sConfigOC = { 0 };
	TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = { 0 };

	/* USER CODE BEGIN TIM1_Init 1 */

	/* USER CODE END TIM1_Init 1 */
	htim1.Instance = TIM1;
	htim1.Init.Prescaler = 1;
	htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim1.Init.Period = 79;
	htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim1.Init.RepetitionCounter = 0;
	htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim1) != HAL_OK) {
		//Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK) {
		//Error_Handler();
	}
	if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) {
		//Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig)
			!= HAL_OK) {
		//Error_Handler();
	}
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = 39;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
	sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2)
			!= HAL_OK) {
		//Error_Handler();
	}
	sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
	sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
	sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
	sBreakDeadTimeConfig.DeadTime = 0;
	sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
	sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
	sBreakDeadTimeConfig.BreakFilter = 0;
	sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
	sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
	sBreakDeadTimeConfig.Break2Filter = 0;
	sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
	if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig)
			!= HAL_OK) {
	//	Error_Handler();
	}
	/* USER CODE BEGIN TIM1_Init 2 */
	
	/* USER CODE END TIM1_Init 2 */
	HAL_TIM_MspPostInit(&htim1);

}
/**
* Режим чтения с портов
*/
void change_Configure_GPIO_Read() {
	GPIO_InitTypeDef GPIO;	
	

	/* Пины для чтения битов с 588ВГ6 */
	GPIO.Pin = DA_ALL | AINTTX_Pin;
	GPIO.Mode = GPIO_MODE_INPUT;\
	GPIO.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO);
	HAL_GPIO_WritePin(GPIOA, DA0_Pin | DA1_Pin | DA2_Pin | DA3_Pin | DA4_Pin | DA5_Pin | DA6_Pin | DA7_Pin | AINTTX_Pin , GPIO_PIN_RESET);
	
	/* Пин для реаизации прерывания по спаду фронта при чтении */
	GPIO.Pin = AINTRX_Pin;
	GPIO.Mode = GPIO_MODE_IT_FALLING;
	GPIO.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO);
	HAL_NVIC_SetPriority( EXTI9_5_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ( EXTI9_5_IRQn );

	/* Пины для работы с 588ВГ6	*/
	GPIO.Pin = ASD_Pin | AWRH_2byte_Pin | AWRL_1byte_Pin | ARDL_1byte_Pin | ANED_Pin | AMODE_Pin | ARDH_2byte_Pin;
	GPIO.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO.Pull = GPIO_NOPULL;
	GPIO.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO);
	
	HAL_GPIO_WritePin(GPIOB, ASD_Pin | AWRH_2byte_Pin | AWRL_1byte_Pin | ARDL_1byte_Pin | ANED_Pin |ARDH_2byte_Pin , GPIO_PIN_RESET);
//	HAL_GPIO_WritePin(GPIOB, AMODE_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, AMODE_Pin, GPIO_PIN_SET);
	
	GPIO.Pin = ACHD_Pin;
	GPIO.Mode = GPIO_MODE_INPUT;
	GPIO.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO);

	MX_TIM1_Init();
	HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);

	
	// Сразу считать, чтобы сбросить готовность
	readDatesToPorts();
}

/**
* Режим записи в порты
*/
void change_Conffigure_GPIO_Write() {
	GPIO_InitTypeDef GPIO;

	/* Пины для записи в 588ВГ6 */
	GPIO.Pin = DA_ALL;
	GPIO.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO.Pull = GPIO_NOPULL;
	GPIO.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO);
	
}
// Задача приема по протоколу СКЛ, нормально заблокирована ожиданием сообщений от последовательного канала.
// Прием пакетов производится в обработчике прерывания UART.Rx.Idle - SKLP_ReceivePacketFragment(), который добавляет в очередь сообщение о приеме.
// Коллбеки ошибок UART добавляют сообщения об ошибках, которые также надо обрабатывать.
typedef struct {
	uint8_t data[36];
	uint16_t count;
	uint8_t rxEventFull;
	uint8_t stopRead;
} DataBuffer;

DataBuffer* getDataBuffer() {
	static DataBuffer Buffer;
	return &Buffer;
}

void fillDataBuffre(uint16_t value) {
	DataBuffer *rxBuffer = getDataBuffer();
	if(rxBuffer->stopRead == 0) {
		if(rxBuffer->count < (sizeof(rxBuffer->data) - 1) ) {
//			GPIO_Common_Toggle(iGPIO_KT2);
			rxBuffer->data[rxBuffer->count] = value;
			rxBuffer->data[rxBuffer->count+1] = value >> 8;
			rxBuffer->count += 2;
			rxBuffer->rxEventFull = 0;
//			GPIO_Common_Toggle(iGPIO_KT2);
			} else {
				
			GPIO_Common_Toggle(iGPIO_KT2);
			rxBuffer->count = 0;
			rxBuffer->rxEventFull = 1;


			assert_param( NULL != EventGroup_System );
			xEventGroupSetBitsFromISR( EventGroup_System, EVENTSYSTEM_MANCHESTER_RX_COMPLETE, NULL );

			GPIO_Common_Toggle(iGPIO_KT2);
			}
	}
}
static void SKLP_SlaveTask( void *pParameters )
{
	( void ) pParameters;
	assert_param( NULL != pSKLP_UsartMessageQueueHdl );

	// Приступить к приему пакетов SKLP по обоим интерфейсам
	// *****************************************************
	// Подключить последовательные интерфейсы к очереди SKLP_Slave
	aSKLP_Interfaces[iSKLP_Machester].pMessageQueue = pSKLP_ManchesterMessageQueueHdl;
	aSKLP_Interfaces[iSKLP_USART].pMessageQueue = pSKLP_UsartMessageQueueHdl;
	// Подписаться на события по приему пакетов, адресованнх этому модулю
	aSKLP_Interfaces[iSKLP_Machester].EventsAllowed	= EVENT_SKLP_QUERY_2ME |  EVENT_SKLP_QUERY_2OTHER ;
	aSKLP_Interfaces[iSKLP_USART].EventsAllowed		= EVENT_SKLP_ALL;
	// Приступить к ожиданию входящих пакетов
	aSKLP_Interfaces[iSKLP_Machester].State = aSKLP_Interfaces[iSKLP_USART].State = SKLP_STATE_WaitStart;
	// Включить UART на прием в очередь, событий по прерываниям не генерить, RxIdle обработать в коллбеке
	assert_param( HAL_OK == HAL_UART_Ext_ReceiveCyclicStart( aSKLP_Interfaces[iSKLP_Machester].pUART_Hdl, &SKLP_RxQueue_Manchester, 0 ) );
	assert_param( HAL_OK == HAL_UART_Ext_ReceiveCyclicStart( aSKLP_Interfaces[iSKLP_USART].pUART_Hdl, &SKLP_RxQueue_USART, 0 ) );
	DWT_AppendTimestampTag( "Start SKLP" );
	/*Включаю режим чтения с портов */
	change_Configure_GPIO_Read();
	
	
//	const char[10] const mTimerManchester = "Manchester";
//	xTimerCreate(mTimerManchester, pdMS_TO_TICKS(1), const UBaseType_t uxAutoReload, void * const pvTimerID, TimerCallbackFunction_t pxCallbackFunction)

	static uint8_t aTmpBuff[1024] = {0};
	static uint32_t startTime = 0;
	static uint32_t currentTime = 0;
        static uint16_t dates = 0;

	while( 1 )
	{
		SKLP_Message_t Message;
		DataBuffer *rxBuffer = getDataBuffer();
		// Ожидать событие от последовательных интерфейсов
//		aSKLP_Interfaces[iSKLP_Machester].State = SKLP_STATE_WaitStart;
//		xQueueReset(pSKLP_ManchesterMessageQueueHdl);
//		assert_param( pdTRUE == xQueueReceive( pSKLP_ManchesterMessageQueueHdl, &Message, portMAX_DELAY ) );
	
//		
//		if( 1  == HAL_GPIO_ReadPin(GPIOA, AINTTX_Pin)) {
////			xTimerCreate(mTimerManchester, pdMS_TO_TICKS(1), const UBaseType_t uxAutoReload, void * const pvTimerID, TimerCallbackFunction_t pxCallbackFunction)
////			xTimerStart(xTimer, xTicksToWait);
//			static uint16_t counterByte = 0; /// Счетчик байтов, что прочитаны
//			startTime = HAL_GetTick();
//			currentTime = startTime;
//			while((currentTime - startTime) < 10) {
//				if(1 == HAL_GPIO_ReadPin(GPIOA, AINTTX_Pin)) {
//						dates = readDatesToPorts();
//						aTmpBuff[counterByte] = dates >> 8;
//						aTmpBuff[counterByte] = dates;
//						counterByte += 2;
//						startTime = HAL_GetTick();
//						
//				}
//					currentTime = HAL_GetTick();
//			}
//			counterByte = 0;
//				
//			Message.mTxEventManchester = 1;
//		}
//		vTaskDelay(2);
//		if( 1 == rxBuffer->rxEventFull )
		GPIO_Common_Toggle(iGPIO_KT1);
		assert_param( NULL != EventGroup_System );
		EventBits_t EventsResult = xEventGroupWaitBits( EventGroup_System, EVENTSYSTEM_MANCHESTER_RX_COMPLETE, true, false, portMAX_DELAY );
		GPIO_Common_Toggle(iGPIO_KT1);
		if( EventsResult & EVENTSYSTEM_MANCHESTER_RX_COMPLETE )
		{	// На основной интерфейс допускаютсЯ запросы по всем поддерживаемым адресам
			rxBuffer->stopRead = 1;
			rxBuffer->rxEventFull = 0;
			rxBuffer->count = 0;
			rxBuffer->data[34] = '\r';
			rxBuffer->data[35] = '\n';
			HAL_UART_Ext_Transmit(aSKLP_Interfaces[iSKLP_Machester].pUART_Hdl, rxBuffer->data, 36, EVENT_UART_EXT_TX_COMPLETE );
			xEventGroupWaitBits( aSKLP_Interfaces[iSKLP_Machester].pUART_Hdl->EventGroup, EVENT_UART_EXT_TX_COMPLETE, true, false, portMAX_DELAY );
			for(int i = 0; i < 36; ++i) {
				rxBuffer->data[i] = 0;
				}
			rxBuffer->stopRead = 0;
/*	
			switch( Message.Event )
			{
			case EVENT_SKLP_QUERY_2OTHER: 	// Широковещательный пакет для всех слейвов
				{// Отправка запроса для МПИ
					bool ReceiverValid =  SKLP_ReceivePacket(aTmpBuff, sizeof(aTmpBuff) , &SKLP_RxQueue_Manchester , Message.Packet);
					if(true == ReceiverValid) {
						GPIO_Common_Toggle(iGPIO_KT2);
						HAL_UART_Ext_Transmit(aSKLP_Interfaces[iSKLP_USART].pUART_Hdl, aTmpBuff, Message.Packet.Size, 0);
						xQueueReset(pSKLP_UsartMessageQueueHdl);
						assert_param( pdTRUE == xQueueReceive( pSKLP_UsartMessageQueueHdl, &Message, pdMS_TO_TICKS( 100 ) ) );
					GPIO_Common_Toggle(iGPIO_KT2);
										switch ( Message.Event )
										{
											case EVENT_SKLP_ANSWER:
												bool ReceiverValid = SKLP_ReceivePacket(aTmpBuff, sizeof(aTmpBuff), &SKLP_RxQueue_USART , Message.Packet);
													if(true == ReceiverValid) {
													HAL_UART_Ext_Transmit(aSKLP_Interfaces[iSKLP_Machester].pUART_Hdl, aTmpBuff, Message.Packet.Size, 0);
													}
											
											break;
										}
					}
					}
			break;
			case EVENT_SKLP_QUERY_2ME: 		// Персональный пакет для этого слейва по любому из допустимых адресов
				SKLP_ProcessPacket( Message.pInterface, Message.Packet );
				break;
			default:						// Недопустимое событие
				assert_param( 0 );
			}
		*/
		}
	}			
}


void EXTI9_5_IRQHandler( void )
{
	HAL_NVIC_ClearPendingIRQ( EXTI9_5_IRQn );
	// Вызвать обработчик для каждого возможного источника
	for( uint32_t PinMask = GPIO_PIN_5; PinMask < GPIO_PIN_9; PinMask <<= 1 )
		HAL_GPIO_EXTI_IRQHandler( ( uint16_t ) PinMask );
}






void HAL_GPIO_EXTI_Callback( uint16_t GPIO_Pin )
{
//	static int count = 0;
//	__no_init  static uint8_t xRxBuffer[1024];// = {0};
	if( GPIO_Pin == GPIO_PIN_8 )
	{
	
												while(1 == HAL_GPIO_ReadPin(GPIOB, ACHD_Pin)){}
		GPIO_Common_Toggle(iGPIO_KT3);
		uint16_t dates = readDatesToPorts();
		fillDataBuffre(dates);                
												GPIO_Common_Toggle(iGPIO_KT3);
	}
}
