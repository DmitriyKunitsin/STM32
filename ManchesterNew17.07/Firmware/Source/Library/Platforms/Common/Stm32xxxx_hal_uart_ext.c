// Stm32xxxx_hal_uart_ext.c, ���������� �������� Stm32xxxx_hal_uart
// (�������� ���� ����������)
#include "PlatformConfig.h"					// ��������� �����������������
#include "stm32xxxx_hal.h"					// ����� ���������, � �.�. 
#include "Stm32xxxx_hal_uart_ext.h"			// ������

// ��������� ���������� ������� ��������
static void UART_Ext_DMATransmitCplt( DMA_HandleTypeDef *hdma );				// [������]	DMA UART transmit process complete callback
static void UART_Ext_DMAError( DMA_HandleTypeDef *hdma );   						// [������]	DMA UART communication error callback
static void UART_Ext_Transmit_Complete_IT( UART_HandleTypeDef *huart );		// [������]	UART transmit process complete callback
static void UART_Ext_Transmit_Empty8_IT( UART_HandleTypeDef *huart );			// [������]	UART_TxISR_8BIT
static void UART_Ext_Receive_Idle_IT( UART_HandleTypeDef *huart );				// [�����]	UART receive idle callback
static void UART_Ext_RxDMABuffer_Update( UART_Ext_HandleTypeDef *huart_ext );	// ����������� ������ ��������� ������ �� ��������� ������ DMA
static void UART_Ext_DMAReceiveCyclicCplt( DMA_HandleTypeDef *hdma );			// ���������� UART.Rx.DMA.Complete � UART.Rx.DMA.HalfComplete - ���������� ������ ������� ������. ����� ������������ ��� ������ ������� ����������� �������
static void UART_Ext_DMAReceiveCplt( DMA_HandleTypeDef *hdma );					// ���������� UART.Rx.DMA.Complete - ���������� ���� ���������� ����� ������������ ������
static void UART_Ext_EndTxTransfer(UART_HandleTypeDef *huart);
static void UART_Ext_EndRxTransfer(UART_HandleTypeDef *huart);


// ��������, ��� ������� huart �������� ����������� ������� ������������ ��������
BaseType_t HAL_UART_Ext_ValidateHdl( UART_Ext_HandleTypeDef *huart )
{
//	return ( ( NULL != huart ) && ( huart->Instance == huart->Common.Instance ) );
	bool Result = false;
	do
	{
		if( NULL == huart )
			break;
		if( !IS_UART_INSTANCE( huart->Instance ) )
			break;
		if( huart->Instance != huart->Common.Instance )
			break;
		Result = true;
	} while( 0 );
	return Result;
}

// ������������� ������������ �������� UART
HAL_StatusTypeDef HAL_UART_Ext_Init( UART_Ext_HandleTypeDef *huart_ext )
{
	// !!!��-��������, ������� ���� ��������� - �������������� �� �����, ��������� � huart->Init, ����������� ���������.
	UART_HandleTypeDef *huart = ( UART_HandleTypeDef * ) huart_ext;
	huart_ext->Instance = huart->Instance;		// ������� ��������� �������� ��� ���������� ������ HAL_UART_MspInit()
	huart_ext->TXEN_GPIO = NULL;				// ��������. � ������������ � ������������� �������, ����� ���� ���������������� � ��������-��������� HAL_UART_MspInit()
	huart->hdmatx = NULL;													\
	huart->hdmarx = NULL;													\

	HAL_StatusTypeDef ReturnValue;
	ReturnValue = HAL_UART_Init( huart );		// ���������� ����������� ������������
	if( HAL_OK == ReturnValue )
	{	// ������ ����������� �������������. �������� ����������� �������������
		// ������������� ����� RS485.TxEn ��� ������ ���� ����������� � HAL_UART_MspInit()
#ifdef	USE_FREERTOS
		assert_param( NULL != ( huart_ext->EventGroup = xEventGroupCreate( ) ) );		// ���������������� ������� �������
		( void ) xEventGroupSetBits( huart_ext->EventGroup, EVENT_UART_EXT_INIT_COMPLETE | EVENT_UART_EXT_ACCESS_READY );
#endif	// USE_FREERTOS
		huart_ext->EventBitsMask = EVENT_UART_EXT_INIT_COMPLETE;
		huart_ext->pByteQueueRx = NULL;
		huart_ext->TxCompleteCallback = NULL;
		huart_ext->IRQ_Handlers = ( UART_Ext_IRQ_Handlers_t ) { 0 };
		// ���� ��� ������������ �������� ������, �� ����� ���������, ���� �������� huart->Common.Instance � huart->Common.State

	}

	return ReturnValue;
}

// ��������������� ������������ ������� UART (�� ������������, ����� ���� ������)
HAL_StatusTypeDef HAL_UART_Ext_DeInit( UART_Ext_HandleTypeDef *huart )
{
	HAL_StatusTypeDef ReturnValue;
	ReturnValue = HAL_UART_DeInit( &huart->Common );
#ifdef	USE_FREERTOS
	vEventGroupDelete( huart->EventGroup );
#endif	// USE_FREERTOS
	return ReturnValue;
}


// ��� ������ � ������� SIG60 ���� ��� �������, � �.�. ��� ���� ������
// (����., �� �����, �������) ����� ������������, � �������� ����� ��� � ����� �� ��������.
// ��� �������������� ��������� ���������� ����������, ��� ������� ��������.
// ���� �� ������� - �������������� ���������� ������ �� ����� �������� ������ ����� �����.
// ����� ���������� �������� ������ (������ ��� ������) ���������� ������� �������� ����������.
// ����� ����, ��� ������� ������ ���������� ���������, ��� �������� �������.
// !!! ���������� ������ ������ ���������� ����� huart_ext->EventBitsMask, �������� ������� ����������� ������!
static void HAL_UART_Ext_RxForModemDisable( UART_Ext_HandleTypeDef *huart_ext )
{	// ���� � ������ ������ ������� �������� - ��������� ���, �� ���������, ��� ��� �������.
	if( huart_ext->EventBitsMask & EVENT_UART_EXT_MODEM_INIT_COMPLETE )
	{
		if( huart_ext->Instance->CR1 & USART_CR1_RE )
		{
			huart_ext->EventBitsMask |= EVENT_UART_EXT_RX_ENABLED;
			huart_ext->Instance->CR1 &= ~USART_CR1_RE;
		}
		else
			huart_ext->EventBitsMask &= ~EVENT_UART_EXT_RX_ENABLED;
	}
}

static void HAL_UART_Ext_RxForModemRestore( UART_Ext_HandleTypeDef *huart_ext )
{	// ���� � ������ ������ ����� ��� �������� ����� - �������� ��������
	if( huart_ext->EventBitsMask & EVENT_UART_EXT_MODEM_INIT_COMPLETE )
	{
		if( huart_ext->EventBitsMask & EVENT_UART_EXT_RX_ENABLED )
			huart_ext->Instance->CR1 |= USART_CR1_RE;
		huart_ext->EventBitsMask &= ~EVENT_UART_EXT_RX_ENABLED;
	}
}

// ������������ HAL_UART_Transmit()
// �������� ����� �� ��� � UART ����� DMA.
// ����������� ��� ���������� - UART.Tx.DMA.TxC � UART.TxC.
// � ������ ����������� ���������� ������������ ��������������� �������.
// ����� ����, � ����������� UART.TxC ����������� �������� ����� ������� ������� RS-485 (���� ������������).
HAL_StatusTypeDef HAL_UART_Ext_Transmit( UART_Ext_HandleTypeDef *huart_ext, uint8_t *pData, uint16_t Size, EventBits_t EventBitsMask )
{
  uint32_t *tmp;
  UART_HandleTypeDef *huart = ( UART_HandleTypeDef * ) huart_ext;
  assert_param( HAL_UART_Ext_ValidateHdl( huart_ext ) );
  
  if(huart->gState == HAL_UART_STATE_READY) 
  {
    if((pData == NULL ) || (Size == 0)) 
    {
      return HAL_ERROR;
    }
	// ��������, ��� ���������� UART �������.
	// ��������������, ��� ���������� ���������� ��� �������������, � �� ��� ������� ��������
    if( 0 == ( huart_ext->Instance->CR1 & USART_CR1_TE ) )
    {
      return HAL_ERROR;
    }
    
    /* Process Locked */
    __HAL_LOCK(huart);
    
    huart->pTxBuffPtr = pData;
    huart->TxXferSize = Size;
    huart->TxXferCount = Size;
    
    huart->ErrorCode = HAL_UART_ERROR_NONE;
    huart->gState = HAL_UART_STATE_BUSY_TX;
    
    // ��������� ���������� USART.TxC �� ������, ���� ������ ���������� �� ���������� �������� ����������� ������
    __HAL_UART_DISABLE_IT( huart, UART_IT_TC );

	
	if( NULL != huart->hdmatx )
	{	// ����������� �������� ������ ����� DMA, ���� ����������� ��� ������������
		/* Set the UART DMA transfer complete callback */
		huart->hdmatx->XferCpltCallback = UART_Ext_DMATransmitCplt;
		
		/* Set the UART DMA Half transfer complete callback */
		huart->hdmatx->XferHalfCpltCallback = NULL;
		// �� ������ �� ��, ��� ���������� DMA_IT_HT �� ���������������, ���������� ��� ����� ����� �������, �.�. ��� ������ ����������� � HAL_DMA_Start_IT().
		// ����� ������������� DMA_IT_HT � �� ����� � HAL_DMA_Start_IT(), ������������ ���������� ����� ����� HAL_DMA_Start_IT().
		
		/* Set the DMA error callback */
		huart->hdmatx->XferErrorCallback = UART_Ext_DMAError;

	    /* Enable the UART transmit DMA Stream */
    	tmp = (uint32_t*)&pData;
#if		defined( STM32F4 )
		__HAL_UART_CLEAR_FLAG( huart, UART_FLAG_TC );
#elif	defined( STM32L4 ) || defined( STM32F3 )
		__HAL_UART_CLEAR_FLAG( huart, UART_CLEAR_TCF );
#else
#error "Select Target Family!"
#endif
		assert_param( HAL_OK == HAL_DMA_Start_IT(huart->hdmatx, *(uint32_t*)tmp, ( uint32_t ) &UART_INSTANCE_TDR( huart->Instance ), Size ) );
		// ��������� ��������� ����������, ����������� � HAL_DMA_Start_IT() - ����� �� �������������� HAL
		__HAL_DMA_DISABLE_IT( huart->hdmatx, DMA_IT_HT );  // Half Transmit	- �� ������������
#if		defined( STM32F4 )
		__HAL_DMA_DISABLE_IT( huart->hdmatx, DMA_IT_FE );  // FIFO Error	- �� ������������, ��� ���� ��� ��� ������������ (���, �.�. ���), ����� ���������� FIFO Error, �� ����� ��������� �����������.
#endif
	}

#ifdef	USE_FREERTOS
	// �������� ������������ ������� ��� ��������
	if( 0 != ( EventBitsMask & EVENT_UART_EXT_ALL ) )
		( void ) xEventGroupClearBits( huart_ext->EventGroup, EVENT_UART_EXT_TX_ALL );
#endif	// USE_FREERTOS

	// ��������� �������� RS-485 (���� ����������������)
	if( huart_ext->TXEN_GPIO != NULL )
	{
		HAL_GPIO_WritePin( huart_ext->TXEN_GPIO, huart_ext->TXEN_Pin, GPIO_PIN_SET );
	}

	// ��������� ����� ������� �� ��������, ������� ���� ����� ������������
	ENTER_CRITICAL_SECTION( );		// !! �������� � huart_ext->EventBitsMask ����� ����.������, �.�. ��� ���� �������������� � �����������!!
	WRITE_BIT_MASKED( huart_ext->EventBitsMask, EventBitsMask, EVENT_UART_EXT_TX_ALL );
	HAL_UART_Ext_RxForModemDisable( huart_ext );
	EXIT_CRITICAL_SECTION( );

	if( NULL != huart->hdmatx )
	{	// ��������� �������� ������ ����� DMA, ���� ����������� ��� ������������
		ENTER_CRITICAL_SECTION( );
	    // Enable the DMA transfer for transmit request by setting the DMAT bit in the UART CR3 register
    	huart->Instance->CR3 |= USART_CR3_DMAT;
	    // ��������� ���������� USART.TxC
    	// !!! � ����� ������� ����� ���� ���������, ��� ������ ���� �� ������ ��� ����� � USART Data register,
		// � �� ����� ���������� ���������� USART.TxC ��� ���������� �������� ���������� ����� �� ����������� ������.
		huart_ext->IRQ_Handlers.xTxComplete = UART_Ext_Transmit_Complete_IT;
#if		defined( STM32F4 )
		__HAL_UART_CLEAR_FLAG( huart, UART_FLAG_TC );
		__HAL_UART_CLEAR_FLAG( huart, UART_FLAG_TC );
#elif	defined( STM32L4 ) || defined( STM32F3 )
		__HAL_UART_CLEAR_FLAG( huart, UART_CLEAR_TCF );
		__HAL_UART_CLEAR_FLAG( huart, UART_CLEAR_TCF );
#else
#error "Select Target Family!"
#endif
		__HAL_UART_ENABLE_IT( huart, UART_IT_TC );
		// ����� ����� �� ������ ��������� �������� (����������), �������������� ������ ���������� �������� ������ ������ ������, ��� ����� ������� ���� USART.TxC
		EXIT_CRITICAL_SECTION( );
	}
	else
	{	// ���������� �������� ������ ��� DMA, ���� �� ����������� ��� ������������
#ifdef	USART_CR1_FIFOEN
#error	"��������!"
#endif
		// Set the Tx ISR function pointer according to the data word length
		if ((huart->Init.WordLength == UART_WORDLENGTH_9B) && (huart->Init.Parity == UART_PARITY_NONE))
			assert_param( 0 );
		huart_ext->IRQ_Handlers.xTxEmpty = UART_Ext_Transmit_Empty8_IT;
		__HAL_UART_ENABLE_IT( huart, UART_IT_TXE );
	}
    
    /* Process Unlocked */
    __HAL_UNLOCK(huart);
    
    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;   
  }
}

#include "ProjectConfig.h"
#include "common_gpio.h"
// ��������� �� UART �� DMA � ����� pByteQueueRx ����������.
// ����������� ������� ��� UART.RxIdle � UART.RxError.
// ������ ������������ � ����������� ����� pByteQueueRx �� DMA, �� ������ pByteQueueRx ������������� � ������������ UART.Rx.Idle, UART.Rx.DMA.Complete � UART.Rx.DMA.Complete.
// ���������� ���������� ������ ���������� ��������, ��������� � pByteQueueRx.
// !! �������� ������������ ����� ������� � ����������� UART.Rx.Idle, �� ��� �� ����� �������� ��� ������ ������������ ������.
// !! ��� ������ �������������, ��� �������� ������������ ����� ������������ UART.Rx.DMA.Complete � UART.Rx.DMA.Complete.
// !! ������ �������� ������������ ������ �� ������������, ������� ������� ����������� ������ ����� ���� ��������!! (������ ������ ��� ���������)
HAL_StatusTypeDef HAL_UART_Ext_ReceiveCyclicStart( UART_Ext_HandleTypeDef *huart_ext, ByteQueue_t *pByteQueueRx, EventBits_t EventBitsMask )
{
  uint32_t *tmp;
  UART_HandleTypeDef *huart = ( UART_HandleTypeDef * ) huart_ext;
  assert_param( HAL_UART_Ext_ValidateHdl( huart_ext ) );
  assert_param( NULL != pByteQueueRx );
  huart_ext->pByteQueueRx = pByteQueueRx;

  
  /* Check that a Rx process is not already ongoing */
  if(huart->RxState == HAL_UART_STATE_READY) 
  {
    if( !ByteQueue_Validate( huart_ext->pByteQueueRx ) )
    {
      return HAL_ERROR;
    }
	// ��������, ��� �������� UART �������.
	// ��������������, ��� �������� ���������� ��� �������������, � �� ��� ������� ������
    if( 0 == ( huart_ext->Instance->CR1 & USART_CR1_RE ) )
    {
      return HAL_ERROR;
    }
    
    /* Process Locked */
    __HAL_LOCK(huart);
    
	ByteQueue_Clear( huart_ext->pByteQueueRx );
    huart->pRxBuffPtr = huart_ext->pByteQueueRx->pBuffer;
    huart->RxXferSize = huart_ext->pByteQueueRx->Size;
    
    huart->ErrorCode = HAL_UART_ERROR_NONE;
    huart->RxState = HAL_UART_STATE_BUSY_RX;

	huart_ext->IRQ_Handlers.xRxIdle = UART_Ext_Receive_Idle_IT;
	
	// �������������������� DMA � ��������� �����
	huart->hdmarx->Init.Mode = DMA_CIRCULAR;
	assert_param( HAL_OK == HAL_DMA_Init( huart->hdmarx ) );

    /* Set the UART DMA transfer complete callback */
//    huart->hdmarx->XferCpltCallback = UART_DMAReceiveCplt;
    huart->hdmarx->XferCpltCallback = UART_Ext_DMAReceiveCyclicCplt;		// �������� ������� �������� ����
    
    /* Set the UART DMA Half transfer complete callback */
//    huart->hdmarx->XferHalfCpltCallback = UART_DMARxHalfCplt;
	huart->hdmarx->XferHalfCpltCallback = UART_Ext_DMAReceiveCyclicCplt;	// �������� ������� �������� ����
    
    /* Set the DMA error callback */
    huart->hdmarx->XferErrorCallback = UART_Ext_DMAError;

    /* Enable the DMA Stream */
    tmp = (uint32_t*)&huart_ext->pByteQueueRx->pBuffer;
    assert_param( HAL_OK == HAL_DMA_Start_IT(huart->hdmarx, ( uint32_t ) &UART_INSTANCE_RDR( huart->Instance ), *(uint32_t*)tmp, huart_ext->pByteQueueRx->Size) );

	// ���������������� ������� �������� ����
    huart->RxXferCount = 0;
    
#ifdef	USE_FREERTOS
	// �������� ������������ �������
	xEventGroupClearBits( huart_ext->EventGroup, EVENT_UART_EXT_RX_ALL );
#endif	// USE_FREERTOS

	// ��������� ����� ������� �� ������, ������� ���� ����� ������������
	ENTER_CRITICAL_SECTION( );
	WRITE_BIT_MASKED( huart_ext->EventBitsMask, EventBitsMask, EVENT_UART_EXT_RX_ALL );

    /* Enable the DMA transfer for the receiver request by setting the DMAR bit 
    in the UART CR3 register */
    huart->Instance->CR3 |= USART_CR3_DMAR;
    
	// �������� ������������ ����� ����������
	//	__HAL_UART_CLEAR_FLAG( huart, UART_IT_RXNE );		// ����� RXNE. ���������� �� ����� F4 � ���. ��������, �������� �� ������� ����������
	//	__HAL_UART_CLEAR_FLAG( huart, UART_FLAG_RXNE ); 	// ����� RXNE. ��������� ��� ����� F4
	READ_REG( UART_INSTANCE_RDR( huart->Instance ) );		// ����� RXNE. ��������� ��� ����1 F4 � L4
	__HAL_UART_CLEAR_OREFLAG( huart );						// ����� ORE. ��������� ��� ����1 F4 � L4
	__HAL_UART_CLEAR_IDLEFLAG( huart ); 					// ����� IDLE. ��������� ��� ����1 F4 � L4
    // ��������� ���������� USART.RxIdle
    __HAL_UART_ENABLE_IT( huart, UART_IT_IDLE );
    EXIT_CRITICAL_SECTION( );

    // ��� ������ � �������� �� ���� - ��������� ����������? 

    /* Process Unlocked */
    __HAL_UNLOCK(huart);
    
    return HAL_OK;
  }
  else
  {
    return HAL_BUSY; 
  }
}

// ��������� ����� �� UART �� DMA � ����������� �����
HAL_StatusTypeDef HAL_UART_Ext_ReceiveCyclicStop( UART_Ext_HandleTypeDef *huart_ext )
{
	UART_HandleTypeDef *huart = ( UART_HandleTypeDef * ) huart_ext;

    __HAL_LOCK( huart );	// ������������� �������

	if( huart->RxState & HAL_UART_STATE_BUSY_RX )
	{
		__HAL_UART_DISABLE_IT( huart, UART_IT_IDLE );		// ��������� ���������� USART.RxIdle
		HAL_UART_DMAStop( huart );							// ���������� ����� �� UART �� DMA

		huart_ext->IRQ_Handlers.xRxIdle =  NULL;

		// �������������������� DMA � ������� ����� (�� ������ ���������� ������� ������ ����� ����������� API)
		huart->hdmarx->Init.Mode = DMA_NORMAL;
		assert_param( HAL_OK == HAL_DMA_Init( huart->hdmarx ) );

		// ����� �� ������ ������
		huart->RxState = HAL_UART_STATE_READY;

		huart_ext->pByteQueueRx = NULL;
	}

	__HAL_UNLOCK(huart);	// �������������� �������

	return HAL_OK;
}

// �������� �������� �����
HAL_StatusTypeDef HAL_UART_Ext_ReceiveCyclicReset( UART_Ext_HandleTypeDef *huart_ext )
{
	assert_param( HAL_UART_Ext_ValidateHdl( huart_ext ) );
	UART_HandleTypeDef *huart = ( UART_HandleTypeDef * ) huart_ext;
	HAL_StatusTypeDef Result = HAL_ERROR;
	do
	{
		if( 0 == ( huart->RxState & HAL_UART_STATE_BUSY_RX ) )
			break;
		ENTER_CRITICAL_SECTION( );
		// �������� ����� ������ UART
	    __HAL_UART_CLEAR_OREFLAG( huart );
		__HAL_UART_CLEAR_IDLEFLAG( huart );
		// ���������������� �������� ����� �� ��������� DMA
		UART_Ext_RxDMABuffer_Update( huart_ext );
		// �������� �������� �����
		ByteQueue_AdvanceTailTo( huart_ext->pByteQueueRx, huart_ext->pByteQueueRx->iHead );
		EXIT_CRITICAL_SECTION( );
#ifdef	USE_FREERTOS
		// �������� ������������ �������
		xEventGroupClearBits( huart_ext->EventGroup, EVENT_UART_EXT_RX_ALL );
#endif	// USE_FREERTOS
		Result = HAL_OK;
	}
	while( 0 );
	return Result;
}

// ��������� ����� �� UART �� DMA � ����� �������������� ������� (�� ���������� ������ ��� Idle). ��� �������������� ���������� �������� ����?
HAL_StatusTypeDef HAL_UART_Ext_ReceiveStart( UART_Ext_HandleTypeDef *huart_ext, uint8_t *pData, uint16_t Size, EventBits_t EventBitsMask )
{
  uint32_t *tmp;
  assert_param( HAL_UART_Ext_ValidateHdl( huart_ext ) );
  UART_HandleTypeDef *huart = ( UART_HandleTypeDef * ) huart_ext;
  assert_param( NULL != pData );
  
  if( huart->RxState == HAL_UART_STATE_READY )
  {
	  // ��������, ��� �������� UART �������.
	  // ��������������, ��� �������� ���������� ��� �������������, � �� ��� ������� ������
	  if( 0 == ( huart_ext->Instance->CR1 & USART_CR1_RE ) )
	  {
		return HAL_ERROR;
	  }

    /* Process Locked */
    __HAL_LOCK(huart);
    
    huart->pRxBuffPtr = pData;
    huart->RxXferSize = Size;
    huart_ext->pByteQueueRx = NULL;

    huart->ErrorCode = HAL_UART_ERROR_NONE;
    huart->RxState = HAL_UART_STATE_BUSY_RX;
    
	huart_ext->IRQ_Handlers.xRxIdle = UART_Ext_Receive_Idle_IT;

	// �������������������� DMA � ������� �����
	huart->hdmarx->Init.Mode = DMA_NORMAL;
	assert_param( HAL_OK == HAL_DMA_Init( huart->hdmarx ) );

    /* Set the UART DMA transfer complete callback */
//    huart->hdmarx->XferCpltCallback = UART_DMAReceiveCplt;
    huart->hdmarx->XferCpltCallback = UART_Ext_DMAReceiveCplt;		// ��������� ������� �� ���������� ������ � ��������� �����
    
    /* Set the UART DMA Half transfer complete callback */
//    huart->hdmarx->XferHalfCpltCallback = UART_DMARxHalfCplt;
//	huart->hdmarx->XferHalfCpltCallback = UART_Ext_DMAReceiveCyclicCplt;	// �������� ������� �������� ����
	huart->hdmarx->XferHalfCpltCallback = NULL;
    
    /* Set the DMA error callback */
    huart->hdmarx->XferErrorCallback = UART_Ext_DMAError;

	// ���������������� ������� �������� ����
    huart->RxXferCount = 0;
    
#ifdef	USE_FREERTOS
	// �������� ������������ �������
	xEventGroupClearBits( huart_ext->EventGroup, EVENT_UART_EXT_RX_ALL );
#endif	// USE_FREERTOS

    /* Enable the DMA Stream */
    tmp = (uint32_t*)&huart->pRxBuffPtr;
    assert_param( HAL_OK == HAL_DMA_Start_IT(huart->hdmarx, ( uint32_t ) &UART_INSTANCE_RDR( huart->Instance ), *(uint32_t*)tmp, huart->RxXferSize ) );

	ENTER_CRITICAL_SECTION( );
	// ��������� ����� ������� �� ������, ������� ���� ����� ������������
	WRITE_BIT_MASKED( huart_ext->EventBitsMask, EventBitsMask, EVENT_UART_EXT_RX_ALL );

    /* Clear the Overrun flag just before enabling the DMA Rx request: mandatory for the second transfer
       when using the USART in circular mode */
    __HAL_UART_CLEAR_OREFLAG( huart );

    /* Enable the DMA transfer for the receiver request by setting the DMAR bit 
    in the UART CR3 register */
    huart->Instance->CR3 |= USART_CR3_DMAR;
    
    // ��������� ���������� USART.RxIdle
	__HAL_UART_CLEAR_IDLEFLAG( huart );
    __HAL_UART_ENABLE_IT( huart, UART_IT_IDLE );
	EXIT_CRITICAL_SECTION( );

    // ��� ������ � �������� �� ���� - ��������� ����������? 

    /* Process Unlocked */
    __HAL_UNLOCK(huart);
    
    return HAL_OK;
  }
  else
  {
    return HAL_BUSY; 
  }
}

/*HAL_StatusTypeDef HAL_UART_Ext_Receive( UART_Ext_HandleTypeDef *huart_ext, uint8_t *pData, uint16_t Size, EventBits_t EventBitsMask )
{
	assert_param( HAL_UART_Ext_ValidateHdl( huart_ext ) );
	huart_ext->EventBitsResult = 0;
	BootEvents = 0;
	EventBits_t BootEventsNew;
	HAL_UART_Ext_ReceiveStart( huart_ext, pData, Size, EVENT_UART_EXT_RX_ALL );
	do
	{
		ATOMIC_WRITE( BootEventsNew, BootEvents );
		extern void WatchdogReset( void );
		WatchdogReset( );
	}
	while( 0 == ( BootEventsNew & EVENT_UART_EXT_RX_ALL ) );

	{
		if( EVENT_UART_EXT_RX_IDLE != BootEventsNew )
			break;
	}
}
*/

// ��������� ����� �� UART �� DMA � �����
HAL_StatusTypeDef HAL_UART_Ext_ReceiveStop( UART_Ext_HandleTypeDef *huart_ext )
{
	assert_param( HAL_UART_Ext_ValidateHdl( huart_ext ) );
	UART_HandleTypeDef *huart = ( UART_HandleTypeDef * ) huart_ext;

    __HAL_LOCK( huart );	// ������������� �������

	if( huart->RxState == HAL_UART_STATE_BUSY_RX )
	{
		__HAL_UART_DISABLE_IT( huart, UART_IT_IDLE );		// ��������� ���������� USART.RxIdle
		HAL_UART_DMAStop( huart );							// ���������� ����� �� UART �� DMA

		// �������������������� DMA � ������� ����� (�� ������ ���������� ������� ������ ����� ����������� API)
//		huart->hdmarx->Init.Mode = DMA_NORMAL;
//		assert_param( HAL_OK == HAL_DMA_Init( huart->hdmarx ) );

		huart_ext->IRQ_Handlers.xRxIdle = NULL;

		// ����� �� ������ ������
		huart->RxState = HAL_UART_STATE_READY;
	}

	__HAL_UNLOCK(huart);	// �������������� �������

	return HAL_OK;
}


// ���������� ���������� UART ��� ������������ ��������.
// ����� ������������, �� ��������� �������� UART.TxC � UART.Idle
void HAL_UART_Ext_IRQHandler(UART_HandleTypeDef *huart)
{
	UART_Ext_HandleTypeDef* huart_ext = ( UART_Ext_HandleTypeDef * ) huart;
	assert_param( HAL_UART_Ext_ValidateHdl( huart_ext ) );

  uint32_t tmp1 = 0, tmp2 = 0;

  tmp1 = __HAL_UART_GET_FLAG(huart, UART_FLAG_PE);
  tmp2 = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_PE);  
  /* UART parity error interrupt occurred ------------------------------------*/
  if((tmp1 != RESET) && (tmp2 != RESET))
  { 
    __HAL_UART_CLEAR_PEFLAG(huart);
    
    huart->ErrorCode |= HAL_UART_ERROR_PE;
  }
  
  tmp1 = __HAL_UART_GET_FLAG(huart, UART_FLAG_FE);
  tmp2 = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_ERR);
  /* UART frame error interrupt occurred -------------------------------------*/
  if((tmp1 != RESET) && (tmp2 != RESET))
  { 
    __HAL_UART_CLEAR_FEFLAG(huart);
    
    huart->ErrorCode |= HAL_UART_ERROR_FE;
  }
  
  tmp1 = __HAL_UART_GET_FLAG(huart, UART_FLAG_NE);
  tmp2 = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_ERR);
  /* UART noise error interrupt occurred -------------------------------------*/
  if((tmp1 != RESET) && (tmp2 != RESET))
  { 
    __HAL_UART_CLEAR_NEFLAG(huart);
    
    huart->ErrorCode |= HAL_UART_ERROR_NE;
  }
  
  tmp1 = __HAL_UART_GET_FLAG(huart, UART_FLAG_ORE);
  tmp2 = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_ERR);
  /* UART Over-Run interrupt occurred ----------------------------------------*/
  if((tmp1 != RESET) && (tmp2 != RESET))
  { 
    __HAL_UART_CLEAR_OREFLAG(huart);
    
    huart->ErrorCode |= HAL_UART_ERROR_ORE;
  }
  
  tmp1 = __HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE);
  tmp2 = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_RXNE);
  /* UART in mode Receiver ---------------------------------------------------*/
  if((tmp1 != RESET) && (tmp2 != RESET))
  { 
//    UART_Receive_IT(huart);
//	if( NULL != huart_ext->IRQ_Handlers.xRxComplete )
	assert_param( NULL != huart_ext->IRQ_Handlers.xRxComplete );
		huart_ext->IRQ_Handlers.xRxComplete( huart );
  }
  
  tmp1 = __HAL_UART_GET_FLAG(huart, UART_FLAG_TXE);
  tmp2 = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_TXE);
  /* UART in mode Transmitter ------------------------------------------------*/
  if((tmp1 != RESET) && (tmp2 != RESET))
  {
//    UART_Transmit_IT(huart);
//	if( NULL != huart_ext->IRQ_Handlers.xTxEmpty )
	assert_param( NULL != huart_ext->IRQ_Handlers.xTxEmpty );
		huart_ext->IRQ_Handlers.xTxEmpty( huart );
  }
  
  tmp1 = __HAL_UART_GET_FLAG(huart, UART_FLAG_TC);
  tmp2 = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_TC);
  /* UART Transmitte Complete ------------------------------------------------*/
  if((tmp1 != RESET) && (tmp2 != RESET))
  {
	if( NULL != huart_ext->IRQ_Handlers.xTxComplete )
//	assert_param( NULL != huart_ext->IRQ_Handlers.xTxComplete );
		huart_ext->IRQ_Handlers.xTxComplete( huart );
	else
		__HAL_UART_DISABLE_IT( huart, UART_IT_TC );

//    UART_Ext_Transmit_Complete_IT(huart);
  }
  
  tmp1 = __HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE);
  tmp2 = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_IDLE);
  /* UART Receive Idle ------------------------------------------------*/
  if((tmp1 != RESET) && (tmp2 != RESET))
  {
	__HAL_UART_CLEAR_IDLEFLAG(huart);
//    UART_Ext_Receive_Idle_IT(huart);
//	if( NULL != huart_ext->IRQ_Handlers.xRxIdle )
	assert_param( NULL != huart_ext->IRQ_Handlers.xRxIdle );
		huart_ext->IRQ_Handlers.xRxIdle( huart );
  }
  
  if(huart->ErrorCode != HAL_UART_ERROR_NONE)
  {
    /* Set the UART state ready to be able to start again the process */
    huart->gState = HAL_UART_STATE_READY;
    huart->RxState = HAL_UART_STATE_READY;
    
    HAL_UART_ErrorCallback(huart);
  }  
}

/**
  * @brief  DMA UART transmit process complete callback. 
  * @param  hdma: DMA handle
  * @retval None
  */
static void UART_Ext_DMATransmitCplt(DMA_HandleTypeDef *hdma)
{
// !�����! ��������� ���������� �� UART �� ������ ���� ����, ��� �� DMA - ����� UART.TxC ����� ������������ ������, ��� DMA.TxC!!
  
  UART_HandleTypeDef* huart = ( UART_HandleTypeDef* )((DMA_HandleTypeDef* )hdma)->Parent;
  UART_Ext_HandleTypeDef* huart_ext = ( UART_Ext_HandleTypeDef * ) huart;
  assert_param( HAL_UART_Ext_ValidateHdl( huart_ext ) );
  
  /* DMA Normal mode*/
#if		defined( STM32F4 )
  if((hdma->Instance->CR & DMA_SxCR_CIRC) == 0)
#elif	defined( STM32L4 ) || defined( STM32F3 )
  if((hdma->Instance->CCR & DMA_CCR_CIRC) == 0)
#else
#error "Select Target Family!"
#endif	// STM32XX
  {
    huart->TxXferCount = 0;

    /* Disable the DMA transfer for transmit request by setting the DMAT bit
       in the UART CR3 register */
    huart->Instance->CR3 &= (uint32_t)~((uint32_t)USART_CR3_DMAT);

	// � ������� �� ������������ �����������, �� ����� ��������� ����� UART.TxC
	
	// �� ������ �� ��, ��� ��������� ���� ��� ����������, UART ��� ����� � �������� ������ ������
	huart->gState = HAL_UART_STATE_READY;

	// ������������ ������� �� ���������� �������� ������ DMA � UART
	if( huart_ext->EventBitsMask & EVENT_UART_EXT_TX_DMA_COMPLETE )
	{
		huart_ext->EventBitsMask &= ~EVENT_UART_EXT_TX_DMA_COMPLETE;
#ifdef	USE_FREERTOS
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		assert_param( pdPASS == xEventGroupSetBitsFromISR( huart_ext->EventGroup, EVENT_UART_EXT_TX_DMA_COMPLETE, &xHigherPriorityTaskWoken ) );
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
#endif	//USE_FREERTOS
	}
  }
  /* DMA Circular mode */
  else
  {
    HAL_UART_TxCpltCallback(huart);
  }

}

/**
  * @brief  DMA UART communication error callback.
  * @param  hdma: DMA handle
  * @retval None
  */
// ������� ����������, � ����������� ��������� ������� ��� ������
// ��������� ���������� � ����������� �� ������ DMA (RX ���� TX) 
static void UART_Ext_DMAError( DMA_HandleTypeDef *hdma )   
{
  UART_HandleTypeDef* huart = ( UART_HandleTypeDef* )((DMA_HandleTypeDef* )hdma)->Parent;
  UART_Ext_HandleTypeDef* huart_ext = ( UART_Ext_HandleTypeDef * ) huart;
  assert_param( HAL_UART_Ext_ValidateHdl( huart_ext ) );

  // ������������ ������� �� ������, � ����������� �� ������ DMA (RX ���� TX)
  EventBits_t EventBits = 0;
  if( hdma == huart->hdmatx )
  {
  	EventBits = EVENT_UART_EXT_TX_ERROR;
	huart->TxXferCount = 0;
//	huart->gState = HAL_UART_STATE_READY;
    UART_Ext_EndTxTransfer(huart);
  }
  else if( hdma == huart->hdmarx )
  {
  	EventBits = EVENT_UART_EXT_RX_ERROR;
	huart->RxXferCount = 0;
//	huart->RxState = HAL_UART_STATE_READY;
    UART_Ext_EndRxTransfer(huart);
  }
  else
  	assert_param( 0 );
  huart->ErrorCode |= HAL_UART_ERROR_DMA;		// ���� ������, � �� RX, � �� TX - ����� ���� ��������!
  	
  EventBits &= huart_ext->EventBitsMask;
  if( EventBits )
  {
	huart_ext->EventBitsMask &= ~EventBits;
#ifdef	USE_FREERTOS
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	assert_param( pdPASS == xEventGroupSetBitsFromISR( huart_ext->EventGroup, EventBits, &xHigherPriorityTaskWoken ) );
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
#endif	// USE_FREERTOS
  }

  HAL_UART_ErrorCallback(huart);
}

/**
  * @brief  UART transmit process complete callback. 
  * @param  huart: pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval None
  */
// ������������ ������� �� USART.TxC � ����������� ��������� �������� ��������� RS-485
static void UART_Ext_Transmit_Complete_IT( UART_HandleTypeDef *huart )
{
	UART_Ext_HandleTypeDef* huart_ext = ( UART_Ext_HandleTypeDef * ) huart;
	assert_param( HAL_UART_Ext_ValidateHdl( huart_ext ) );

	// ��������� ���������� USART.TxC. ����� ����������, � �������� �����, �������� � HAL_UART_Ext_Transmit()
#if		defined( STM32F4 )
	  __HAL_UART_DISABLE_IT( huart, UART_IT_TC );
	  __HAL_UART_CLEAR_FLAG( huart, UART_FLAG_TC );
#elif	defined( STM32L4 ) || defined( STM32F3 )
	  __HAL_UART_DISABLE_IT( huart, UART_IT_TC );
	  __HAL_UART_CLEAR_FLAG( huart, UART_CLEAR_TCF );
#else
#error "Select Target Family!"
#endif	// STM32XX

    huart_ext->IRQ_Handlers.xTxComplete = NULL;

	// ������������ �����, ���� ����� ��� ��������
	HAL_UART_Ext_RxForModemRestore( ( UART_Ext_HandleTypeDef * ) huart );

	/* Check if a receive process is ongoing or not */
	// ���������������, ��� �������� huart->State = HAL_UART_STATE_READY ��� ��������� � UART_Ext_DMATransmitCplt( )

	// ��������� �������� RS-485
	if( huart_ext->TXEN_GPIO != NULL )
	{
		HAL_GPIO_WritePin( huart_ext->TXEN_GPIO, huart_ext->TXEN_Pin, GPIO_PIN_RESET );
	}

	// ������������ ������� �� ���������� �������� ����� UART
	if( huart_ext->EventBitsMask & EVENT_UART_EXT_TX_COMPLETE )
	{
		huart_ext->EventBitsMask &= ~EVENT_UART_EXT_TX_COMPLETE;
#ifdef	USE_FREERTOS
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		assert_param( pdPASS == xEventGroupSetBitsFromISR( huart_ext->EventGroup, EVENT_UART_EXT_TX_COMPLETE, &xHigherPriorityTaskWoken ) );
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
#endif	// USE_FREERTOS
	}

	// ����� ������������� (__weak) ��������.
	// ���������������, ��� ������� ������������ ���������� RS485.TxEn � ��������� ������� RTOS, � ������� �� �����.
	HAL_UART_TxCpltCallback( huart );
	UART_Ext_Callback_t TxCompleteCallback = huart_ext->TxCompleteCallback;
	if( NULL != TxCompleteCallback )
		TxCompleteCallback( );
	
	/* At end of Tx process, restore huart->gState to Ready */
	huart->gState = HAL_UART_STATE_READY;
}

// ���������� ��������� UART.TxEmpty (��� ������ ������ ��� DMA).
// ���� ���� ������ � ������ - ����������.
// ��� ����������� ������ - ��������� ���������� UART.TxComplete
static void UART_Ext_Transmit_Empty8_IT( UART_HandleTypeDef *huart )			// [������]	UART_TxISR_8BIT
{
	// Check that a Tx process is ongoing
	if( huart->gState == HAL_UART_STATE_BUSY_TX )
	{
		if( huart->TxXferCount == 0 )
		{
			// Disable the UART Transmit Data Register Empty Interrupt
#ifdef	USART_CR1_FIFOEN
#error
#endif
#if		defined( STM32F4 )
			__HAL_UART_DISABLE_IT( huart, UART_IT_TXE );
			__HAL_UART_CLEAR_FLAG( huart, UART_FLAG_TC );
#elif	defined( STM32L4 ) || defined( STM32F3 )
			__HAL_UART_DISABLE_IT( huart, UART_IT_TXE );
			__HAL_UART_CLEAR_FLAG( huart, UART_CLEAR_TCF );
#else
#error "Select Target Family!"
#endif	// STM32XX
		  ( ( UART_Ext_HandleTypeDef * ) huart )->IRQ_Handlers.xTxEmpty = NULL;

		  // Enable the UART Transmit Complete Interrupt
		  ( ( UART_Ext_HandleTypeDef * ) huart )->IRQ_Handlers.xTxComplete = UART_Ext_Transmit_Complete_IT;
			__HAL_UART_ENABLE_IT( huart, UART_IT_TC );
		}
		else
		{
			UART_INSTANCE_TDR( huart->Instance ) = ( uint8_t ) ( *huart->pTxBuffPtr++ & ( uint8_t ) 0xFF );
			huart->TxXferCount--;
		}
	  }
}


// ������������ ������� �� USART.RxIdle
static void UART_Ext_Receive_Idle_IT( UART_HandleTypeDef *huart )
{
	UART_Ext_HandleTypeDef* huart_ext = ( UART_Ext_HandleTypeDef * ) huart;
	assert_param( HAL_UART_Ext_ValidateHdl( huart_ext ) );

	// �������� ������� �������� ����
	if( NULL != huart_ext->pByteQueueRx )
		UART_Ext_RxDMABuffer_Update( huart_ext );
	else
	{
		uint32_t DMA_RxCntRemain = __HAL_DMA_GET_COUNTER( huart_ext->Common.hdmarx );
		huart_ext->Common.RxXferCount = huart_ext->Common.RxXferSize - DMA_RxCntRemain;
	}

	// ������������ ������� �� USART.RxIdle
	if( huart_ext->EventBitsMask & EVENT_UART_EXT_RX_IDLE )
	{
		// �� ���������� EVENT_UART_EXT_RX_IDLE � EventBitsMask - ��� ������� ����� �������� ����������� (����?)
#ifdef	USE_FREERTOS
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		assert_param( pdPASS == xEventGroupSetBitsFromISR( huart_ext->EventGroup, EVENT_UART_EXT_RX_IDLE, &xHigherPriorityTaskWoken ) );
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
#endif	// USE_FREERTOS
	}

	// ����� ������������� (__weak) ��������.
	// (�������� � ����������� �������� �� �������������)
	HAL_UART_RxIdleCallback( huart );
}

/**
  * @brief  Receive Idle callbacks.
  * @param  huart: pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval None
  */
__weak void HAL_UART_RxIdleCallback( UART_HandleTypeDef *huart )
{
  /* NOTE: This function Should not be modified, when the callback is needed,
           the HAL_UART_RxIdleCallback could be implemented in the user file
   */ 
}

// ����������� ������ ��������� ������ �� ��������� ������ DMA (�������� ������ �� ����������!!)
// ����� ������������ �� ������������ UART.RxIdle, UART.RxDMA.RxC, UART.RxDMA.RxHalf
static void UART_Ext_RxDMABuffer_Update( UART_Ext_HandleTypeDef *huart_ext )
{
	assert_param( HAL_UART_Ext_ValidateHdl( huart_ext ) );
	assert_param( NULL != huart_ext->pByteQueueRx ); 
	// �������� ����� ������ ������
	ByteQueueIndex_t iHeadNew = huart_ext->pByteQueueRx->Size - __HAL_DMA_GET_COUNTER( huart_ext->Common.hdmarx );

	// �������� ������� � ��������� ������ (������������ �� ��������������!)
	ByteQueueIndex_t Increment = ByteQueue_AdvanceHeadTo( huart_ext->pByteQueueRx, iHeadNew );
	huart_ext->Common.RxXferCount += Increment;
}

// ���������� UART.Rx.DMA.Complete � UART.Rx.DMA.HalfComplete - ���������� ������ ������� ������. ����� ������������ ��� ������ ������� ����������� �������
static void UART_Ext_DMAReceiveCyclicCplt( DMA_HandleTypeDef *hdma )
{
	UART_Ext_HandleTypeDef* huart_ext = ( UART_Ext_HandleTypeDef * )( ( DMA_HandleTypeDef *) hdma )->Parent;
	assert_param( HAL_UART_Ext_ValidateHdl( huart_ext ) );

	// �������� ������� �������� ����
	UART_Ext_RxDMABuffer_Update( huart_ext );
}

// ���������� UART.Rx.DMA.Complete - ���������� ���� ���������� ����� ������������ ������
static void UART_Ext_DMAReceiveCplt( DMA_HandleTypeDef *hdma )
{
	UART_Ext_HandleTypeDef* huart_ext = ( UART_Ext_HandleTypeDef * )( ( DMA_HandleTypeDef *) hdma )->Parent;
	assert_param( HAL_UART_Ext_ValidateHdl( huart_ext ) );

	// ������������ ������� �� DMA.RxComplete
	if( huart_ext->EventBitsMask & EVENT_UART_EXT_RX_COMPLETE )
	{
#ifdef	USE_FREERTOS
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		assert_param( pdPASS == xEventGroupSetBitsFromISR( huart_ext->EventGroup, EVENT_UART_EXT_RX_COMPLETE, &xHigherPriorityTaskWoken ) );
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
#endif	// USE_FREERTOS
		HAL_UART_RxCpltCallback( &huart_ext->Common );
	}
	else
		assert_param( 0 );
}

// �������� ���������� ��� ������ � ����������� ����� �� DMA - ��������� �������� DMA.NDTR � �����������, ������������ � *pPrevState
bool UART_Ext_RxDMA_CheckActivity( UART_Ext_HandleTypeDef *huart_ext, uint32_t *pPrevState )
{
	assert_param( ( NULL != huart_ext ) && ( NULL != pPrevState ) );

	uint32_t PrevState = *pPrevState;
	*pPrevState = __HAL_DMA_GET_COUNTER( huart_ext->Common.hdmarx );

	return *pPrevState != PrevState;
}

// �������� �������� UART
#if		defined( STM32F4 )
// �������� �������� UART ��� ������ �����������������.
// �������� �� �������� ������� UART_SetConfig( ) �������� HAL_UART ��� STM32F4
void HAL_UART_Ext_ResetBaudrate( UART_HandleTypeDef *huart )
{
	/* Check the parameters */
	assert_param( NULL != huart );
	assert_param(IS_UART_INSTANCE(huart->Instance));
	assert_param(IS_UART_BAUDRATE(huart->Init.BaudRate));  

	/* Check the Over Sampling */
	if(huart->Init.OverSampling == UART_OVERSAMPLING_8)
	{
	  /*-------------------------- USART BRR Configuration ---------------------*/
	  if((huart->Instance == USART1) || (huart->Instance == USART6))
	  {
		huart->Instance->BRR = UART_BRR_SAMPLING8(HAL_RCC_GetPCLK2Freq(), huart->Init.BaudRate);
	  }
	  else
	  {
		huart->Instance->BRR = UART_BRR_SAMPLING8(HAL_RCC_GetPCLK1Freq(), huart->Init.BaudRate);
	  }
	}
	else
	{
	  /*-------------------------- USART BRR Configuration ---------------------*/
	  if((huart->Instance == USART1) || (huart->Instance == USART6))
	  {
		huart->Instance->BRR = UART_BRR_SAMPLING16(HAL_RCC_GetPCLK2Freq(), huart->Init.BaudRate);
	  }
	  else
	  {
		huart->Instance->BRR = UART_BRR_SAMPLING16(HAL_RCC_GetPCLK1Freq(), huart->Init.BaudRate);
	  }
	}
}
#elif		defined( STM32L4 ) || defined( STM32F3 )
// �������� �������� UART ������ � ������ ������������������
// ������� UART_SetConfig( ) � �������� HAL_UART ��� STM32L4 �������.
void HAL_UART_Ext_ResetBaudrate( UART_HandleTypeDef *huart )
{
	UART_SetConfig( huart );
}
#else
#error "Select Target Family!"
#endif

// [����/�]	���������� �������� �������� ����������
uint32_t HAL_UART_Ext_CalcByteRate( UART_Ext_HandleTypeDef *huart_ext )
{
	assert_param( HAL_UART_Ext_ValidateHdl( huart_ext ) );
	UART_InitTypeDef *pInit = &huart_ext->Common.Init;
/*	assert_param( IS_UART_WORD_LENGTH	( pInit->WordLength ) );  
	assert_param( IS_UART_BAUDRATE		( pInit->BaudRate ) );  
	assert_param( IS_UART_STOPBITS		( pInit->StopBits ) );
	assert_param( IS_UART_PARITY		( pInit->Parity ) );
*/
	uint32_t Result = 0;
	do
	{
		uint8_t BitsInByte = 1;		// Start Bit

		if( UART_WORDLENGTH_8B == pInit->WordLength )
			BitsInByte += 8;
		else if( UART_WORDLENGTH_9B == pInit->WordLength )
			BitsInByte += 9;
		else
			break;
		
		if( UART_STOPBITS_1 == pInit->StopBits )
			BitsInByte += 1;
		else if( UART_STOPBITS_2 == pInit->StopBits )
			BitsInByte += 2;
		else
			break;
		
		if( UART_PARITY_NONE == pInit->Parity )
			;
		else if( ( UART_PARITY_EVEN == pInit->Parity ) || ( UART_PARITY_ODD == pInit->Parity ) )
			BitsInByte += 1;
		else
			break;
		
		Result = pInit->BaudRate / BitsInByte;
	} while( 0 );
	return Result;
}

// [�]	���������� ����� �� �������� ������
float HAL_UART_Ext_CalcTimeToTransmite( UART_Ext_HandleTypeDef *huart_ext, uint16_t PacketSize )
{
	float ResultTime = -1.0f;
	do
	{
		if( 0 == PacketSize )
			break;
		uint32_t ByteRate = HAL_UART_Ext_CalcByteRate( huart_ext );
		if( 0 == ByteRate )
			break;
		ResultTime = PacketSize / ( float ) ByteRate;
	}
	while( 0 );
	return ResultTime;
}

// ������ UART �� ����� ��� �����-���� ���������.
// ����������� ���������� ����������� � ����������.
HAL_StatusTypeDef HAL_UART_Ext_ReceiveRawStart( UART_Ext_HandleTypeDef *huart_ext, UART_Ext_IT_t xRxIT, UART_Ext_IT_t xRxIdleIT )
{
	assert_param( HAL_UART_Ext_ValidateHdl( huart_ext ) );
	UART_HandleTypeDef *huart = ( UART_HandleTypeDef * ) huart_ext;
	
	if( huart->RxState == HAL_UART_STATE_READY )
	{
		// ��������, ��� �������� UART �������.
		// ��������������, ��� �������� ���������� ��� �������������, � �� ��� ������� ������
		if( 0 == ( huart_ext->Instance->CR1 & USART_CR1_RE ) )
		{
		  return HAL_ERROR;
		}

	  /* Process Locked */
	  __HAL_LOCK(huart);
	  
	  huart->pRxBuffPtr = NULL;
	  huart->RxXferSize = 0;
	  huart->RxXferCount = 0;
	  
	  huart->ErrorCode = HAL_UART_ERROR_NONE;
	  huart->RxState = HAL_UART_STATE_BUSY_RX;

	  /* Enable the UART Parity Error Interrupt */
//	  __HAL_UART_ENABLE_IT(huart, UART_IT_PE);
	  
	  /* Enable the UART Error Interrupt: (Frame error, noise error, overrun error) */
//	  __HAL_UART_ENABLE_IT(huart, UART_IT_ERR);
	  
		ENTER_CRITICAL_SECTION( );

	  // ��������� ���������� USART.RxC
	  if( NULL != xRxIT )
		  __HAL_UART_ENABLE_IT( huart, UART_IT_RXNE );
	  else
		  __HAL_UART_DISABLE_IT( huart, UART_IT_RXNE );
	  huart_ext->IRQ_Handlers.xRxComplete = xRxIT;
	  
	  // ��������� ���������� USART.RxIdle
	  if( NULL != xRxIdleIT )
		  __HAL_UART_ENABLE_IT( huart, UART_IT_IDLE );
	  else
		  __HAL_UART_DISABLE_IT( huart, UART_IT_IDLE );
	  huart_ext->IRQ_Handlers.xRxIdle = xRxIdleIT;

		// �������� ������������ ����� ����������
		//	__HAL_UART_CLEAR_FLAG( huart, UART_IT_RXNE );		// ����� RXNE. ���������� �� ����� F4 � ���. ��������, �������� �� ������� ����������
		//	__HAL_UART_CLEAR_FLAG( huart, UART_FLAG_RXNE );		// ����� RXNE. ��������� ��� ����� F4
		READ_REG( UART_INSTANCE_RDR( huart->Instance ) );		// ����� RXNE. ��������� ��� ����1 F4 � L4
		__HAL_UART_CLEAR_OREFLAG( huart );						// ����� ORE. ��������� ��� ����1 F4 � L4
		__HAL_UART_CLEAR_IDLEFLAG( huart );						// ����� IDLE. ��������� ��� ����1 F4 � L4

	  EXIT_CRITICAL_SECTION( );
	  
	  /* Process Unlocked */
	  __HAL_UNLOCK(huart);
	  
	  return HAL_OK;
	}
	else
	{
	  return HAL_BUSY; 
	}
}

// ��������� ����� �� UART, ������� � HAL_UART_Ext_ReceiveRawStart()
HAL_StatusTypeDef HAL_UART_Ext_ReceiveRawStop( UART_Ext_HandleTypeDef *huart_ext )
{
	assert_param( HAL_UART_Ext_ValidateHdl( huart_ext ) );
	UART_HandleTypeDef *huart = ( UART_HandleTypeDef * ) huart_ext;

    __HAL_LOCK( huart );	// ������������� �������

	if( huart->RxState == HAL_UART_STATE_BUSY_RX )
	{
		__HAL_UART_DISABLE_IT( huart, UART_IT_IDLE );		// ��������� ���������� USART.RxIdle
		__HAL_UART_DISABLE_IT( huart, UART_IT_RXNE );		// ��������� ���������� USART.RxRxNE

		huart_ext->IRQ_Handlers.xRxIdle = NULL;
		huart_ext->IRQ_Handlers.xRxComplete = NULL;

		// ����� �� ������ ������
		huart->RxState = HAL_UART_STATE_READY;
	}

	__HAL_UNLOCK(huart);	// �������������� �������

	return HAL_OK;
}

// ���������� ������� ��� ���������� �������� ������ ����� UART
// ������� ����� �������� �������
UART_Ext_Callback_t UART_Ext_SetTransferCompleteCallback( UART_Ext_HandleTypeDef *huart_ext, UART_Ext_Callback_t xCallback )
{
	assert_param( HAL_UART_Ext_ValidateHdl( huart_ext ) );
	UART_Ext_Callback_t xPevCallback;
	ENTER_CRITICAL_SECTION( );
	xPevCallback = huart_ext->TxCompleteCallback;
	huart_ext->TxCompleteCallback = xCallback;
	EXIT_CRITICAL_SECTION( );
	return xPevCallback;
}


// ����������� UART_EndTxTransfer() � UART_EndRxTransfer() �� ������������ ��������

#if		defined (STM32L4)
/**
  * @brief  End ongoing Tx transfer on UART peripheral (following error detection or Transmit completion).
  * @param huart UART handle.
  * @retval None
  */
static void UART_Ext_EndTxTransfer(UART_HandleTypeDef *huart)
{
  /* Disable TXEIE and TCIE interrupts */
#if defined(USART_CR1_FIFOEN)
  CLEAR_BIT(huart->Instance->CR1, (USART_CR1_TXEIE_TXFNFIE | USART_CR1_TCIE));
#else
  CLEAR_BIT(huart->Instance->CR1, (USART_CR1_TXEIE | USART_CR1_TCIE));
#endif

  // ������������ �����, ���� ����� ��� ��������
  HAL_UART_Ext_RxForModemRestore( ( UART_Ext_HandleTypeDef * ) huart );

  /* At end of Tx process, restore huart->gState to Ready */
  huart->gState = HAL_UART_STATE_READY;
}


/**
  * @brief  End ongoing Rx transfer on UART peripheral (following error detection or Reception completion).
  * @param huart UART handle.
  * @retval None
  */
static void UART_Ext_EndRxTransfer(UART_HandleTypeDef *huart)
{
  /* Disable RXNE, PE and ERR (Frame error, noise error, overrun error) interrupts */
#if defined(USART_CR1_FIFOEN)
  CLEAR_BIT(huart->Instance->CR1, (USART_CR1_RXNEIE_RXFNEIE | USART_CR1_PEIE));
#else
  CLEAR_BIT(huart->Instance->CR1, (USART_CR1_RXNEIE | USART_CR1_PEIE));
#endif
  CLEAR_BIT(huart->Instance->CR3, USART_CR3_EIE);

  /* At end of Rx process, restore huart->RxState to Ready */
  huart->RxState = HAL_UART_STATE_READY;
  
  /* Reset RxIsr function pointer */
  huart->RxISR = NULL;
}

#elif	defined (STM32F4)
/**
  * @brief  End ongoing Tx transfer on UART peripheral (following error detection or Transmit completion).
  * @param  huart: UART handle.
  * @retval None
  */
static void UART_Ext_EndTxTransfer(UART_HandleTypeDef *huart)
{
  /* Disable TXEIE and TCIE interrupts */
  CLEAR_BIT(huart->Instance->CR1, (USART_CR1_TXEIE | USART_CR1_TCIE));

  // ������������ �����, ���� ����� ��� ��������
  HAL_UART_Ext_RxForModemRestore( ( UART_Ext_HandleTypeDef * ) huart );

  /* At end of Tx process, restore huart->gState to Ready */
  huart->gState = HAL_UART_STATE_READY;
}

/**
  * @brief  End ongoing Rx transfer on UART peripheral (following error detection or Reception completion).
  * @param  huart: UART handle.
  * @retval None
  */
static void UART_Ext_EndRxTransfer(UART_HandleTypeDef *huart)
{
  /* Disable RXNE, PE and ERR (Frame error, noise error, overrun error) interrupts */
  CLEAR_BIT(huart->Instance->CR1, (USART_CR1_RXNEIE | USART_CR1_PEIE));
  CLEAR_BIT(huart->Instance->CR3, USART_CR3_EIE);

  /* At end of Rx process, restore huart->RxState to Ready */
  huart->RxState = HAL_UART_STATE_READY;
}

#else
#error "Select Target Family!"
#endif	// STM32XXX
