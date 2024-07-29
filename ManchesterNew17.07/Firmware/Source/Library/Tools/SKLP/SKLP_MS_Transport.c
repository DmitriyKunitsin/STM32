// SKLP_MS_Transport.h
// �������� ���������������� ���� ���, ������ ������� �� ����.
#include "ProjectConfig.h"		// ������ ���������
#include "stm32xxxx_hal.h"		// ����� ���������
#include "platform_common.h"	// GPIO_Common_Write()
#include "common_gpio.h"		// GPIO_Common_Write()
#include "SKLP_MS_Transport.h"	// ������
#include "SKLP_MS_TransportInterface.h"	// ������
#include "SKLP_Service.h"
#include "Logger.h"
#include <stdio.h>
#include "RebootUtils.h"		// ������������ ����� ResetInfo.aLogMessage � �������� ���������� ��� ������ �����������
#include "Utils.h"

// �������� ������������ ��������� "���������".
// ��� ���������� ������� ����� �� ������������, � ��� ������� - ����������
static bool SKLP_InterfaceValidate( SKLP_Interface_t *pInterface )
{
	bool Result = false;
	do
	{
		if( NULL == pInterface )
			break;
		if( !HAL_UART_Ext_ValidateHdl( pInterface->pUART_Hdl ) )
			break;
		// !!! ���� ����� �� UART ���������� � �������� �������, ��������� ����������� ���� �������!
		if( !ByteQueue_Validate( pInterface->pUART_Hdl->pByteQueueRx ) )
			break;
		// !!! �����, ���� ��� ���-������ ��������� �� ������������ �����������? ��� �������� pByteQueueRx ��� ����������!
		Result = true;
	} while( 0 );
	return Result;
}

// ������������ ������� ������������ ����� ����� ������� � ������
// ********************************
// ���������� �������
static void SKLP_TimerRx_Restart( SKLP_Interface_t *pInterface )
{
	UART_Ext_RxDMA_CheckActivity( pInterface->pUART_Hdl, &pInterface->RxBuffer_SavedState );	// ��������� ������� ��������� UART.Rx.DMA.NDTR, ����� ������������ ����� ���� ��������� ���������� ������
	__HAL_TIM_CLEAR_FLAG( pInterface->pTimerRxHdl, TIM_FLAG_UPDATE );
	__HAL_TIM_SET_COUNTER( pInterface->pTimerRxHdl, 0 );		// ����� ���� __HAL_TIM_SetCounter()
}

// ������ � ���������� ���������� �� �������
static void SKLP_TimerRx_Start( SKLP_Interface_t *pInterface )
{
	SKLP_TimerRx_Restart( pInterface );
	__HAL_TIM_ENABLE_IT( pInterface->pTimerRxHdl, TIM_IT_UPDATE );
	__HAL_TIM_ENABLE( pInterface->pTimerRxHdl );
}

// ��������� ������� � ���������� ����������
/*static*/ void SKLP_TimerRx_Stop( SKLP_Interface_t *pInterface )	
{
	__HAL_TIM_DISABLE( pInterface->pTimerRxHdl );
	__HAL_TIM_DISABLE_IT( pInterface->pTimerRxHdl, TIM_IT_UPDATE );
	__HAL_TIM_CLEAR_FLAG( pInterface->pTimerRxHdl, TIM_FLAG_UPDATE );
}

// ������� �� IRQ.TIM.Ovf. ����� �� ���� - ������� ����� ��� ��������������������.
void SKLP_TimerElapsed( SKLP_Interface_t *pInterface )
{
	assert_param( SKLP_InterfaceValidate( pInterface ) );
	ByteQueueIndex_t PendingBytesCount = ByteQueue_GetSpaceFilled( pInterface->pUART_Hdl->pByteQueueRx );				// ������ ���������� �������������� ������ � �������� ������ UART
	bool RxDMA_NoActivity = !UART_Ext_RxDMA_CheckActivity( pInterface->pUART_Hdl, &pInterface->RxBuffer_SavedState );	// ��������� ������� ������ ����� ���� � ����� � ������� ��������� ��������
	if( SKLP_STATE_WaitSync == pInterface->State )
	{	// ��������� � ������ �������� ������������� - � ������� �����. ������� �� ���� ������ ���� ������
		if( RxDMA_NoActivity )
		{	// �� ����� �������� ������������� �� ���� �������� ������ �� UART �� DMA
			pInterface->State = SKLP_STATE_WaitStart;		// ������� ������-�����, ������ ����� ����� �����
		}
		else
		{	// �� ����� �������� ������������� ���� �������� ����� �� UART �� DMA, ������������� �� �������.
			// �������� ��-�������� � ��������� �������� ������-�����.
			// ����� ����������� ����� ������ �����������, ����� ������ ���������� UART.RxIdle,
			// ��� ����� ��������� ��� �������� �����, � �������� ������-����� ����� �������� ������.
		}
	}
	else
	{	// ��������� � ������ ������ ������. ������ ��� ������� ����� �������� ����� ������ ������.
		// ��������� ���������� �� ���� - ��������, ����� ������ ��� �����������!
		if( RxDMA_NoActivity )
		{	// � ������� ������� ������� ����� ������ ��� � �� ������ - ����� ������!
			assert_param( ByteQueue_RemoveFromTail( pInterface->pUART_Hdl->pByteQueueRx, NULL, PendingBytesCount ) );	// ��������� ����� �������� ��������
			pInterface->State = SKLP_STATE_WaitStart;		// ������� � �������� ������ ������
		}
		else
		{	// ������������ ����� ������, ����� �� ������.
		}
	}
	// � ����� ������, ���������� ������. �� ����� ����� ������� ����� ������ ���������� ���������
	SKLP_TimerRx_Stop( pInterface );
}

#ifdef	SKLP_SIZE_PACKET_LONG_MAX
#warning "!!! ������������ ��������� ������� ������� SKLP !!!"
#endif

// ������������ UART
// ********************************
// ������� �� IRQ.USART.RxIdle.
// ������� �������� ������ �� ������ UART. ��� ��������� ������� ������ �������� ��������� � �������.
// �������� ��������� ���������� ��� 602_01 ��� - STM32F405 @ 67 ���:
// - ��������� ������ ����� - �� 12 ���
// - ������� �������� ����� - �� 20 ���
void SKLP_ReceiveFragment( SKLP_Interface_t *pInterface )
{
	assert_param( SKLP_InterfaceValidate( pInterface ) );
	ByteQueue_t * const pByteQueueRx = pInterface->pUART_Hdl->pByteQueueRx;	// �������� ��������� ����� UART
	
	// ������ ���������� �������������� ������ � �������� ������ UART
	ByteQueueIndex_t PendingBytesCount = ByteQueue_GetSpaceFilled( pByteQueueRx );	// !! ��������, ����� �� ��������� ������� (����� USART.RxIdle) �� ��� ������ �� ���� ����!
	assert_param( 0 != PendingBytesCount );
	pInterface->Statistic.FragmentsRecieved++;

	if ( PendingBytesCount == pByteQueueRx->Size )
	{
		// ������� ������ ����� �����! �� ������ ����� �������� ����������?
		// ���� ������� �������� ��� �������?
		// � ����� ������ ������� �������, ��� ����� ��� �� ����������.
		// ���������� ����� � ��������� ������, �� �������� ��������� ������
		assert_param( ByteQueue_RemoveFromTail( pByteQueueRx, NULL, PendingBytesCount ) );
		// ��������� ��������� �����������������
		SKLP_TimerRx_Start( pInterface );
		pInterface->State = SKLP_STATE_WaitSync;
		pInterface->Statistic.PacketsSkipped++;
	}

	bool bContinueParsing;				// ���� ������������� ��������� ��������� ���������� ������
	do
	{	// ���� ��� ������� ��������� �������
		bContinueParsing = false;
		SKLP_InterfaceEvent_t EventOccured;
		uint16_t PacketSizeExpected;

		// ��������� ������ ���������
#ifndef	SKLP_SIZE_PACKET_LONG_MAX
		if( PendingBytesCount >= SKLP_SIZE_PACKET_MAX ) 		// �������� ����� "�������" �������
#else
		if( PendingBytesCount > SKLP_SIZE_PACKET_LONG_MAX ) 	// ������� ��������� "�������" ������
#endif
		{	// ��������� ����� � �������������������� �����
			pInterface->State = SKLP_STATE_WaitSync;
			// !!! ��������� � ����� � ����������� ������������ �������� �� ���(�)
		}

		// ��������� ������ ���������
		// ��������� ��������� ������ �� ������ ������������� UART.RxIdle
		switch( pInterface->State )
		{
		case SKLP_STATE_WaitSync:	// �������� �������� �� ����� �������� ����� �� ����
			pInterface->State = SKLP_STATE_PacketReject;	// ��������� �������� � ���������� �������� �����
			break;

		case SKLP_STATE_WaitStart:	// �������� �������� �� ����� �������� ������ ������ ������
			pInterface->PacketHeader.Start = ByteQueue_Peek( pByteQueueRx, SKLP_OFFSET_START );
			if( ( SKLP_START_QUERY != pInterface->PacketHeader.Start ) && ( SKLP_START_ANSWER != pInterface->PacketHeader.Start ) )
			{	// ������ ������ ��������. ����� ��������� ��������
				pInterface->State = SKLP_STATE_PacketReject;
				break;
			}
			// ������ �����. ����� ����������� ������
			pInterface->State = SKLP_STATE_WaitSize;
			// ���������� ������ ���������
	
		case SKLP_STATE_WaitSize:	// �������� �������� �� ����� �������� ����� ������� ������
			if( PendingBytesCount <= SKLP_OFFSET_SIZE )
			{	// ������ ��������� ������ ������ ����������. �������� ���.
				break;
			}
			pInterface->PacketHeader.Size = ByteQueue_Peek( pByteQueueRx, SKLP_OFFSET_SIZE );
#pragma	diag_suppress=Pa084		// Warning[Pa084]: pointless integer comparison, the result is always false
			if( ( pInterface->PacketHeader.Size < ( ( SKLP_START_QUERY == pInterface->PacketHeader.Start ) ? SKLP_SIZE_MIN_QUERY : SKLP_SIZE_MIN_ANSWER ) )
#ifndef	SKLP_SIZE_PACKET_LONG_MAX
				|| ( pInterface->PacketHeader.Size >= SKLP_SIZE_MAX )
#endif	//SKLP_SIZE_PACKET_LONG_MAX
				)
#pragma diag_default=Pe084		// ������� ���������� � ��������
			{	// ������ ������������ ������ ������. ����� ��������� �����.
				pInterface->State = SKLP_STATE_PacketReject;
				break;
			}
			// ������ ������. ����� ��������� ������
			pInterface->State = SKLP_STATE_WaitTail;
			// !!! ����! ������������� �������, ��� ���� ������� ��� ������ ����� ���������.
			// !!! ������������ � ��� ��� ������� ��������� ���� ��� ������ ������� �� �������.
			pInterface->Statistic.HeadersRecieved++;
			// ���������� ������ ������
	
		case SKLP_STATE_WaitTail:	// �������� �������� �� ����� �������� ���������� ������
			PacketSizeExpected = pInterface->PacketHeader.Size + ( uint16_t ) SKLP_SIZE_TRIM;	// ���������� ��������� ������ ������. � ������ "��������" ������, ����� (255+3)
			if( PendingBytesCount < PacketSizeExpected )
			{	// ������ ��������� ������ ������ ����������. �������� ���.
				break;
			}
			if(
#ifdef	SKLP_SIZE_PACKET_LONG_MAX
				( PacketSizeExpected < SKLP_SIZE_PACKET_MAX ) &&	// ��������� "�������" ������, �� ���� ����� �� �������
#endif	//SKLP_SIZE_PACKET_LONG_MAX
				( PendingBytesCount > PacketSizeExpected ) )
			{	// ������ ��������� ������ ������ ����������. ��������� ���� �����, �� ������� ��������� �� ������ ������ ��� ������ � SIG60
				if( ( pInterface->pUART_Hdl->EventBitsMask & EVENT_UART_EXT_MODEM_INIT_COMPLETE ) &&	// UART �������� ����� ����� SIG60
					( ( PendingBytesCount == ( PacketSizeExpected + 1 ) ) ||	// ����� �� ������ ������ ������������� �� 1 ���� ��� ����� �� �����
					  ( PendingBytesCount == ( PacketSizeExpected + 2 ) ) ) )	// ������ ������ ������ - ����� �� �������� ��� �� ������� [0x73] ������ ��������� ������ ���� ����� CRC, � ����� ��� ���� ����� �������� ������ ������ ����
				{	// ���������� ��������� �������� ������. ������ ����� �� ������ ��������� (�����)
				}
				else
				{	// ������ ��������� ������ ������ ����������. ���������� ���� �����
					pInterface->State = SKLP_STATE_PacketSkip;
					// ����� ��� �.�. SKLP_STATE_PacketReject, �� ��. ���������� ��������� SKLP_STATE_PacketSkip:
					// ��� ��� �������� "������������" ������ ������������� ������ ��� ������ �� ������������� �������,
					// � ���������� ����� ����� �������������� �� ������� ������������ ���������.
					// �����-�� ��� ���� ����, ������ �� ���������.
					break;
				}
			}
			// ������ ��������� ������ ����� ����������. ���������, ���� ��������� �����,
			// � ����������� �� ��������� ������ � ����� �������.
			EventOccured = EVENT_SKLP_NONE;
			switch( pInterface->PacketHeader.Start )
			{
			case SKLP_START_ANSWER:		// ������ ����� �� ������
				if( pInterface->EventsAllowed & EVENT_SKLP_ANSWER )
					EventOccured = EVENT_SKLP_ANSWER;
				break;
				
			case SKLP_START_QUERY:		// ������ ������
				// ���������, ���� ��������� ������
				pInterface->PacketHeader.Address = ByteQueue_Peek( pByteQueueRx, SKLP_OFFSET_ADDRESS );
				switch( pInterface->PacketHeader.Address )
				{
				case SKLP_ADDRESS_BROADCAST:		// ����������������� ������
					if( pInterface->EventsAllowed & EVENT_SKLP_QUERY_2ALL )
						EventOccured = EVENT_SKLP_QUERY_2ALL;
					break;
				case SKLP_ADDRESS_MYSELF:			// ������ ��������� ����� ������
#ifdef	SKLP_ADDRESS_MYSELF_SECOND
				case SKLP_ADDRESS_MYSELF_SECOND:	// ������ ��������� ����� ������ (������ ���������� �����)
#endif
#ifdef	SKLP_ADDRESS_MYSELF_THIRD
				case SKLP_ADDRESS_MYSELF_THIRD:		// ������ ��������� ����� ������ (������ ���������� �����)
#endif
					if( pInterface->EventsAllowed & EVENT_SKLP_QUERY_2ME )
						EventOccured = EVENT_SKLP_QUERY_2ME;
					break;
				default:							// ������ ��������� ������� ������
#ifdef	SKLP_ADDRESS_CUSTOM
					// ����-��������� ��� ������, ����������� � Run-Time
					extern uint8_t SKLP_CustomAddressGet( void );
					if( SKLP_CustomAddressGet() == pInterface->PacketHeader.Address )
						if( pInterface->EventsAllowed & EVENT_SKLP_QUERY_2ME )
						{
							EventOccured = EVENT_SKLP_QUERY_2ME;
							break;
						}
#endif
					if( pInterface->EventsAllowed & EVENT_SKLP_QUERY_2OTHER )
						EventOccured = EVENT_SKLP_QUERY_2OTHER;
					break;
				}
#ifdef	SKLP_GATEWAY_LIST
				if( pInterface->EventsAllowed & EVENT_SKLP_QUERY_GATEWAY )
				{	// ���� ��������� ����������, ��������� �� ������������ ������ � ������� ������ �� ����������
					const uint8_t aGatewayAddresses[] = { SKLP_GATEWAY_LIST };
					for( int i = 0; i < SIZEOFARRAY( aGatewayAddresses ); i++ )
						if( aGatewayAddresses[i] == pInterface->PacketHeader.Address )
						{	// ��������� ���� �������������� ������ ��������� �������
							EventOccured |= EVENT_SKLP_QUERY_GATEWAY;
							break;
						}
				}
#endif	// SKLP_GATEWAY_LIST
				// ������ ����� �������� �������. ��������� ��� ��� ��� ������������ ���������� ������� ������� (��)
				pInterface->LastIncomingQueryTimeStamp = xTaskGetTickCount( );
				break;
			}
			
			if( EVENT_SKLP_NONE == EventOccured )
			{	// ����� �� �������� ��������. ����� ��������� �����.
				pInterface->State = SKLP_STATE_PacketSkip;
			}
			else
			{	// ����� ��������� � ���. �������� ����� �� ���������� ���������, ��� ����������� ����������.
				pInterface->State = SKLP_STATE_PacketProcess;
			}
			break;
		
		// ��������� SKLP_STATE_PacketReject, SKLP_STATE_PacketSkip � SKLP_STATE_PacketProcess
		// ����� ���������� ������ ������ ����� switch(),
		// � ������ ���� ���������� � ��������� � ��������� switch().
		default:
			assert_param( 0 );
		}
	
		// ��������� ������������� ������ ��������� �� ������ ������
		if( NULL != pInterface->LedRxTimer )
			switch( pInterface->State )
			{
/*			case SKLP_STATE_PacketSkip:
				if( bContinueParsing )
				{			// �������� ��������� ��� ��������� "�������������" ���������, ������� ����������� �� ������
					break;	// � ��������� ������ � ���� ����� do {} while( bContinueParsing ).
				}			// � ���� ������ ��� ������ ��������� ��� �������� ���������, �� ������� ������������� ������ �������� xTimerResetFromISR() ��-�� ��������� ������������ �������.
*/
			case SKLP_STATE_PacketProcess:
				// �� ������ ����������� ������ ������ ��������� � ��������� ������, ������� ��� ����� �������
				assert_param( pdPASS == xTimerResetFromISR( pInterface->LedRxTimer, pdFALSE ) );
			}

		// �������� ��������� ������ ������
		PacketSizeExpected = pInterface->PacketHeader.Size + ( uint16_t ) SKLP_SIZE_TRIM;
		// ���������� ��������� � ����������� �� ���������� ����������� switch()
		switch( pInterface->State )
		{
		case SKLP_STATE_PacketReject:	// � ���������� ����������� ������� ������ ��������� �������� ����� ��-�� ������ �������
			// ���������� ����� � ��������� ������, �� �������� ��������� ������
			assert_param( ByteQueue_RemoveFromTail( pByteQueueRx, NULL, PendingBytesCount ) );
			// ��������� ��������� �����������������
			SKLP_TimerRx_Start( pInterface );
			pInterface->State = SKLP_STATE_WaitSync;
			pInterface->Statistic.PacketsRejectedInFormat++;
			break;
		
		case SKLP_STATE_PacketSkip:		// � ���������� ����������� ������� ������ ��������� �������� ����� (������������� ������ ��� ������� ������� �����)
			SKLP_TimerRx_Stop( pInterface );	// ��� ������������� ������ ������������ ����� ����� ����������� ������ - ��� ����� ���� ������������
			// � ������ ��������� ���������, ���������� �� ���������� �������, ���������� ������ ��������� �����, � ���������� ��������� ���������� ����� ���������
			assert_param( PendingBytesCount >= PacketSizeExpected );
#ifdef	SKLP_SIZE_PACKET_LONG_MAX
			if( PacketSizeExpected < SKLP_SIZE_PACKET_MAX )
			{	// "��������" ����� ��������� � ������� �������
#endif	// SKLP_SIZE_PACKET_LONG_MAX
				assert_param( ByteQueue_RemoveFromTail( pByteQueueRx, NULL, PacketSizeExpected ) );	// ���������� ����� � ��������� ������, �� �������� ��������� ������
				PendingBytesCount -= PacketSizeExpected;
				if( PendingBytesCount > 0 )
				{	// �� ���������� ��������� ������� ������� �����, � ���������� �������� ����� ���� ���������� ������� - (���-�� ������������?)
					bContinueParsing = true;	// ���������� ��������� ����������� ���������
				}
#ifdef	SKLP_SIZE_PACKET_LONG_MAX
			}
			else
			{	// "�������" ����� ��������� ���������, ��� ������� ����������
				assert_param( ByteQueue_RemoveFromTail( pByteQueueRx, NULL, PendingBytesCount ) ); // ���������� ����� � ��������� ������, �� �������� ��������� ������
			}
#endif	// SKLP_SIZE_PACKET_LONG_MAX
			// ������� ��������� �����
			pInterface->State = SKLP_STATE_WaitStart;
			pInterface->Statistic.PacketsSkipped++;
			break;

		case SKLP_STATE_PacketProcess:	// � ���������� ����������� ������� ������ �������� ����� �� ���������� ���������.
			SKLP_TimerRx_Stop( pInterface );	// ��� ������������� ������ ������������ ����� ����� ����������� ������ - ��� ����� ���� ������������
			// ������������ ������������ �������� ����� � �������� ������.
#ifdef	SKLP_SIZE_PACKET_LONG_MAX
			if( PacketSizeExpected < SKLP_SIZE_PACKET_MAX )
			{	// ������ "���������" ������ ��������� � ������� �������
#endif	// SKLP_SIZE_PACKET_LONG_MAX
				if( 0 == ( pInterface->pUART_Hdl->EventBitsMask & EVENT_UART_EXT_MODEM_INIT_COMPLETE ) )
				{	// ������ ��������� ������ ������ ���� ������ ��������������� ���� "������" � ������
					assert_param( PacketSizeExpected == PendingBytesCount );
				}
				else
				{	// � ������ ������, ����������� ��������� ������ � "�������" ������� - ��. ��������� switch() case SKLP_STATE_WaitTail
					assert_param( ( ( PendingBytesCount - PacketSizeExpected ) >= 0 ) && ( ( PendingBytesCount - PacketSizeExpected ) <= 2 ) );
				}
#ifdef	SKLP_SIZE_PACKET_LONG_MAX
			}
			else
			{	// ������ "��������" ������ ������ ���� "�������"
				assert_param( PacketSizeExpected <= PendingBytesCount );
			}
#endif	// SKLP_SIZE_PACKET_LONG_MAX
			// �������� ��������� �������� �� ���������� ��������� � ���������� ������ � ��������� ������ UART.Rx.
			{	// � ��������� ������� "����������" ������ ������. ���� � ����� ��������� ���� ������ �����, ��� ����� ��������� ���� � ByteQueue_RemoveFromTail()
				SKLP_Message_t Message = { pInterface, { pByteQueueRx->iTail, PacketSizeExpected }, EventOccured };
#ifdef	SKLP_SIZE_PACKET_LONG_MAX
				if( PacketSizeExpected >= SKLP_SIZE_PACKET_MAX )
				{	// "�������" ����� ��������� �� ��������� �������, �.�. ��� ��������� ������ �� ��������
					Message.Packet.Size = PendingBytesCount;
				}
#endif	// SKLP_SIZE_PACKET_LONG_MAX
				BaseType_t xHigherPriorityTaskWoken = pdFALSE;
				assert_param( NULL != pInterface->pMessageQueue );
				if( pdPASS == xQueueSendFromISR( pInterface->pMessageQueue, &Message, &xHigherPriorityTaskWoken ) )
				{	// ��������� �������� � ������� ���������
					portYIELD_FROM_ISR( xHigherPriorityTaskWoken ); // ������������ ������������ ������������ ����� ������ �� ����������
				}
				else
				{	// �� ������� �������� ����� � �������, ������ ���������� ������ ��� �� ����������
					pInterface->Statistic.PacketsRejectedInBuzy++;	// �������� ���������, �� �� �� �������, ����� ���������
				}
			}

			// ����� ����� ���������� �� ���������� ������ �� ���� ���������� ��������� ������������� ���������.
			// ���������� ����� � ��������� ������. ���������� � ��������� "����������" ������ ��� ��������� � � ���������� ����� ������������ ��� ��� ����������.
			assert_param( ByteQueue_RemoveFromTail( pByteQueueRx, NULL, PendingBytesCount ) );
			// ������� � �������� ���������� ���������.
			pInterface->State = SKLP_STATE_WaitStart;
			break;

		default:
			// ������ ������ �������� ������. ���� �� ����, ���������� �����, ���� �����
			SKLP_TimerRx_Start( pInterface );	// ���������� � �������� ��������� �����
		}
	} while( bContinueParsing );
}

// ****************************************************************
// ��������� ��������� ������.
// ����������� �� ������, �������������� ���������� ���������, ����� ��������� ��������� �� SKLP_ReceiveFragment().
// ������ SKLP_Task() ������ ����������� � SKLP_MS_TransportLocal.c
// ****************************************************************

// �������� ����� ��� ��������� ����������� ������� � ������������ ������.
// ��������������, ��� SKLP_ProcessPacket() ���������� �� ������ ��������, � ������������� ������������� ������ �� �����.
// ��� �� �����, �������������� ������ � ������, ���� SKLP_ProcessPacket() ����� ������ �� ������ ��������� �����������.
__no_init static uint8_t aQuery[ SKLP_SIZE_PACKET_MAX ];
static uint8_t bQueryBufferAccessTaken = false;

// ����� ������-������� �� ���������� ������ UART � �������� �����, �������� ������� � CRC.
// �������� ���������� �� ������� ���������� �������� �������.
void SKLP_ProcessPacket( SKLP_Interface_t *pInterface, ByteQueueFragment_t PacketToRecive )
{
	assert_param( SKLP_InterfaceValidate( pInterface ) );
	
	// �������� ������ � ��������� ������
	ENTER_CRITICAL_SECTION( );
	assert_param( false == bQueryBufferAccessTaken );
	bQueryBufferAccessTaken = true;
	EXIT_CRITICAL_SECTION( );

	// ���������� � ��������� ������
	pInterface->Statistic.PacketsProcessed++;
	SKLP_CommandResult_t CommandResult = SKLP_COMMAND_RESULT_ERROR_FORMAT;

	// ���������� (���� ��� �������) ������ ������������ � ������ �������� UART.
	// ��������� ������������ �� ������ ���������� ��������� ������� (����., ��� ����� ������ SD)
	// � �� ������ ���������� �������� �������� ������ ����� UART
	bool bFastBaudTimerActive = false;
	taskENTER_CRITICAL( );
	if( NULL != pInterface->FastBaudTimerHandle )
		if( xTimerIsTimerActive( pInterface->FastBaudTimerHandle ) )
		{
			bFastBaudTimerActive = true;
			assert_param( pdPASS == xTimerStop( pInterface->FastBaudTimerHandle, 0 ) );
		}
	taskEXIT_CRITICAL( );

	// ����������� ����� �� ���������� ������ UART � �������� ������� �����, ��������� ������
	bool ReceiveResult = SKLP_ReceivePacket( aQuery, sizeof( aQuery ), pInterface->pUART_Hdl->pByteQueueRx, PacketToRecive );
	// �������� ����� ������� �� ������
	SKLP_Command_t Command = ( SKLP_Command_t ) aQuery[ SKLP_OFFSET_COMMAND ];

	// ���������� ������ � ���������� ������
	if( ReceiveResult )
	{
GPIO_Common_Write( iGPIO_TestPinSKLP_Service, GPIO_PIN_SET );
		// ����� ������, ���� ������� �������� �����.
		// �� ���������� ����-������, ����� pAnswer � ������� ���������� ����� ����������.
		uint8_t *pAnswer = ( void * ) pInterface;
		CommandResult = SKLP_ProcessCommand_Common( aQuery, &pAnswer );
		// ������� �������� ������� � ������� ������� �����
		if( CommandResult >= SKLP_COMMAND_RESULT_RETURN )
		{	// ���������� ������� ������� ������� �����
			// ��������������, ��� ����� ������ ������ ��� ������������������
			assert_param( ( pAnswer != ( void * ) pInterface ) && ( pAnswer != NULL ) );
			if( CommandResult > sizeof( aQuery ) )
				assert_param( pAnswer != aQuery );		// ��� �������, ������� ���������� ����� � ����� �������. ������ ��� ������� ������� ������ �������������� ������ �����
			// ���������� ������ ������������ ��������� ������				
			SKLP_SendPacketFormat_t AnswerPacketFormat;
			uint16_t AnswerPacketSize;
			if( SKLP_COMMAND_MEMORY_READ != Command )
			{	// ������ ������ ��� ������ �� ������� ������
				AnswerPacketFormat = SKLP_SendPacketFormat_Answer;
				AnswerPacketSize = CommandResult + SKLP_SIZE_TRIM;
			}
			else
			{	// ������� ������ ������ ���������� �� ������ ������ �������� ��������� ������
				AnswerPacketFormat = SKLP_SendPacketFormat_AnswerMemRead;
				AnswerPacketSize = CommandResult;
			}
			// ��������� ��������� � CRC ��������� ������, �������� ����� � ���������������� ���������.
			assert_param( SKLP_SendPacket( pAnswer, AnswerPacketSize, AnswerPacketFormat, pInterface->pUART_Hdl ) );
			pInterface->Statistic.PacketsTransmitted++;
		}
		
		// ������� ������������ ������� �� ���������� �������� ������.
		// ������� ����� ���� ��������������� � ����������� �������,
		// ���� ���������� ������� �������������� ��������� ����� ���������� �������� -
		// ��������, ������������ ������������� ������.
		if( NULL != pInterface->xPacketTxCompleteCB )
		{
			pInterface->xPacketTxCompleteCB( pInterface );
			pInterface->xPacketTxCompleteCB = NULL;
		}
		
	}

	// ��������� (���� ����� ��� �������) ������������� ������ ������������ � ������ �������� UART.
	if( bFastBaudTimerActive )
		assert_param( pdPASS == xTimerStart( pInterface->FastBaudTimerHandle, 0 ) );

	// ��������� ��������� ��������� ������������ ������
	if( CommandResult >= SKLP_COMMAND_RESULT_NO_REPLY )
		pInterface->Statistic.PacketsProcessedSuccessful++;
	else
	{	// ��� ������� ���������� ������ ��������� ������. �������� ������ � ���
		static uint8_t FailCount = 10;		// ������� ���������� ������, ����� �� �������� ���
		if( FailCount )
		{
			// �������� ����� ������� � ��� ������
			CommandResult &= 0xFFFF00FF;
			CommandResult |= ( ( uint32_t ) Command ) << 8;
			// �������� ����� UART � ��� ������
			CommandResult &= 0xFFF0FFFF;
			CommandResult |= UART_GET_NUMBER( pInterface->pUART_Hdl->Instance ) << 16;
			// ��������� ��� ������ � ���
			assert_param( MemoryThread_SprintfMutexTake( 1 ) );
			char *pMsg = ( char * ) ResetInfo.aLogMessage;
			int MsgSize = sizeof( ResetInfo.aLogMessage );
			int MsgPos = 0;
			MsgPos += snprintf( pMsg + MsgPos, MsgSize - MsgPos, "[SKLP] Incoming packet processing failure: 0x%08lX", CommandResult );
//			MsgPos += snprintf( pMsg + MsgPos, MsgSize - MsgPos, "(UART=%u Cmd=0x%hh02X, )", ( uint8_t ) Command );
			if( 0 == --FailCount )
				MsgPos += snprintf( pMsg + MsgPos, MsgSize - MsgPos, " <Stop logging this error>" );
			MemoryThread_SprintfMutexGive( );
			assert_param( Logger_WriteRecord( pMsg, LOGGER_FLAGS_APPENDTIMESTAMP | LOGGER_FLAGS_STATICTEXT ) );
		}
	}
		
	// ���������� ������ � ��������� ������
	ATOMIC_WRITE( bQueryBufferAccessTaken, false );
GPIO_Common_Write( iGPIO_TestPinSKLP_Service, GPIO_PIN_RESET );
}

// ������� ����� �� ���������� ������ ������ ���������� � ��������� � ������ ���������
// MessageToGateway		- �����, �������� ����� �������� ���������
// pInterface			- ���������, ���� ����������� �����
void SKLP_ProcessPacketGatewayV2( SKLP_Message_t *pMessageToGateway, SKLP_Interface_t *pInterface )
{
GPIO_Common_Write( iGPIO_TestPinSKLP_Service, GPIO_PIN_SET );
	assert_param( NULL != pMessageToGateway );
	assert_param( SKLP_InterfaceValidate( pInterface ) );
	
	// �������� ������ � ��������� ������
	ENTER_CRITICAL_SECTION( );
	assert_param( false == bQueryBufferAccessTaken );
	bQueryBufferAccessTaken = true;
	EXIT_CRITICAL_SECTION( );

	do
	{
		UART_Ext_HandleTypeDef *pUART_Hdl = pInterface->pUART_Hdl;
		uint16_t PacketSize = pMessageToGateway->Packet.Size;
		if( PacketSize > sizeof( aQuery ) )
			break;		// ������ ������ ��������� ������ ������
		// ����������� ����� �� ���������� ������ UART � �������� ������� �����, �� ��������� ������
		if( !ByteQueue_BufferCopyFragment( pMessageToGateway->pInterface->pUART_Hdl->pByteQueueRx, aQuery, pMessageToGateway->Packet ) )
			break;
		
		// ��������� ����� ����� UART, ��� ���������������
		EventBits_t EventBitsMask = EVENT_UART_EXT_TX_ERROR | EVENT_UART_EXT_TX_COMPLETE;	// ������� ������� � ����� �������� ��� ������
		if( HAL_OK != HAL_UART_Ext_Transmit( pUART_Hdl, aQuery, PacketSize, EventBitsMask ) )
			break;
	
		// ������� ������� � ����� �������� ��� ������
		float TimeToTransmite = HAL_UART_Ext_CalcTimeToTransmite( pUART_Hdl, PacketSize );
		assert_param( TimeToTransmite > 0.0f );
		TickType_t PossibleTimout = 5 + pdMS_TO_TICKS( 1000.0f * 1.2f * TimeToTransmite );
		EventBits_t EventBitsResult = EventBitsMask & xEventGroupWaitBits( pUART_Hdl->EventGroup, EventBitsMask, pdTRUE, pdFALSE, PossibleTimout );
		if( EVENT_UART_EXT_TX_COMPLETE != EventBitsResult )
			break;
	}
	while( 0 );

	// ���������� ������ � ��������� ������
	ATOMIC_WRITE( bQueryBufferAccessTaken, false );
GPIO_Common_Write( iGPIO_TestPinSKLP_Service, GPIO_PIN_RESET );
}

// ����� ������-������� �� ���������� ������ UART � �������� �����, �������� ������� � CRC.
// ���������� ����������
// pInterfaceMaster			���������, ������ ������ ������ � ���� ������� �����
// PacketToGateway			�������� �����
// pGatewayHdl_UART			������� UART, ���� ����������� ������ � ������ ����� �����
// pGatewayHdl_Interface	���������, ���� ����������� ������ � ������ ����� �����
// !!! ��������� ������������ ���� pGatewayHdl_UART, ���� pGatewayHdl_Interface
// WaitAnswerTimeout_ms		������� �� �������� ������, �� ������ ������� �������� ������� (� �������������� ������?)
//							���� 0 - ����� �� �����
void SKLP_ProcessPacketGateway( SKLP_Interface_t *pInterfaceMaster, ByteQueueFragment_t PacketToGateway, UART_Ext_HandleTypeDef *pGatewayHdl_UART, SKLP_Interface_t *pGatewayHdl_Interface, uint16_t WaitAnswerTimeout_ms )
{
	assert_param( SKLP_InterfaceValidate( pInterfaceMaster ) );
	static QueueHandle_t xInterfaceGatewayRxQueue = NULL;
	if( SKLP_InterfaceValidate( pGatewayHdl_Interface ) )
	{	// �������� ����� ��������� SKLP
		pGatewayHdl_UART = pGatewayHdl_Interface->pUART_Hdl;
		if( NULL == xInterfaceGatewayRxQueue )
		{	// ��� ������ ������ �����, ������� ������� ��� ������ ���������
			assert_param ( NULL != ( xInterfaceGatewayRxQueue = xQueueCreate( 1, sizeof( SKLP_Message_t ) ) ) );
		}
	}
	else
		pGatewayHdl_Interface = NULL;
	assert_param( HAL_UART_Ext_ValidateHdl( pGatewayHdl_UART ) );
		
	// �������� ������ � ��������� ������, ������������� � SKLP_ProcessPacket()
	ENTER_CRITICAL_SECTION( );
	assert_param( false == bQueryBufferAccessTaken );
	bQueryBufferAccessTaken = true;
	EXIT_CRITICAL_SECTION( );

	SKLP_CommandResult_t GatewayResult = SKLP_COMMAND_RESULT_ERROR;
	bool bGatewayUART_AccessTaken = false;
	do
	{
		EventBits_t EventBitsMask;
		EventBits_t EventBitsResult;

		// ���������� � ��������� ����������� ������
		pInterfaceMaster->Statistic.PacketsProcessed++;
		// ����������� ����� �� ���������� ������ UART � �������� ������� �����, ��������� ������
		if( !SKLP_ReceivePacket( aQuery, sizeof( aQuery ), pInterfaceMaster->pUART_Hdl->pByteQueueRx, PacketToGateway ) )
			break;
		// ����� ������, ������ � �������

		// ���������� �������� ������ � UART. ���������, ���� ����� ������ ������ ��������.
		EventBitsMask = EVENT_UART_EXT_ACCESS_READY;
		EventBitsResult = EventBitsMask & xEventGroupWaitBits( pGatewayHdl_UART->EventGroup, EventBitsMask, pdTRUE, pdTRUE, pdMS_TO_TICKS( 50 ) );
		if( EventBitsMask != EventBitsResult )
		{	// �� ������� �������� ������ (����������������, �������� �������� �����)
			GatewayResult = SKLP_COMMAND_RESULT_ERROR;
			break;
		}
		bGatewayUART_AccessTaken = true;

		// �������� �������� ���������� ���������� ����� ��������� �������
		HAL_UART_Ext_ReceiveCyclicReset( pGatewayHdl_UART );
		EventBitsMask = 0;
		QueueHandle_t xInterfaceGatewayRxQueueSaved = NULL;
		if( NULL != pGatewayHdl_Interface )
		{	// ��������� � ��������� SKLP
			if( 0 != WaitAnswerTimeout_ms )
			{	// ����������� ������� ����������� ��������� ����� �� ���� �������
				taskENTER_CRITICAL( );
				xInterfaceGatewayRxQueueSaved = pGatewayHdl_Interface->pMessageQueue;
				xQueueReset( xInterfaceGatewayRxQueueSaved );
				pGatewayHdl_Interface->pMessageQueue = xInterfaceGatewayRxQueue;
				taskEXIT_CRITICAL( );
			}
		}
		else
		{	// ��������� � UART
			if( 0 == WaitAnswerTimeout_ms )
				EventBitsMask = EVENT_UART_EXT_TX_COMPLETE;								// ������� ������� � ����� ��������
			else
				EventBitsMask = EVENT_UART_EXT_TX_COMPLETE | EVENT_UART_EXT_RX_IDLE;	// ������� ������� � ����� �������� � ������ ������
		}

		// ���������� �������� ������ � ����, ��� ��������� (����� �� ������������ SKLP_SendPacket())
		if( HAL_OK != HAL_UART_Ext_Transmit( pGatewayHdl_UART, aQuery, PacketToGateway.Size, EventBitsMask ) )
			break;
		// ���������� ����� �������� ���������� ������ � ������ ������������� ������ � �����
		float TimeToTransmite	= HAL_UART_Ext_CalcTimeToTransmite( pGatewayHdl_UART, PacketToGateway.Size );
		float TimeToReceive		= HAL_UART_Ext_CalcTimeToTransmite( pGatewayHdl_UART, SKLP_SIZE_PACKET_MAX );
		assert_param( TimeToTransmite > 0.0f );
		assert_param( TimeToReceive > 0.0f );
		if( 0 == WaitAnswerTimeout_ms )
		{	// ����� ����� �� ���������. ����� ���������� �������� ������, � ��������� ����������
			TickType_t PossibleTimout = pdMS_TO_TICKS( 1000.0f * TimeToTransmite + 2 );		// ���������� ����� �������� �������
			EventBitsResult = EventBitsMask & xEventGroupWaitBits( pGatewayHdl_UART->EventGroup, EventBitsMask, pdTRUE, pdTRUE, PossibleTimout );
			if( EventBitsMask != EventBitsResult )
				GatewayResult = SKLP_COMMAND_RESULT_ERROR;			// �� ������� ��������� �����
			else
				GatewayResult = SKLP_COMMAND_RESULT_NO_REPLY;		// ����� �������
			// ���������� ���������
			break;
		}
		
		// ����� ���������� �������� ������ � ����� �����
		ByteQueueFragment_t PacketToRecive = { 0 };
		ByteQueue_t *pByteQueueRx = pGatewayHdl_UART->pByteQueueRx;
		TickType_t PossibleTimout = pdMS_TO_TICKS( 1000.0f * TimeToTransmite + 2 + 1000.0f * TimeToReceive + WaitAnswerTimeout_ms );	// ���������� ����� �������� ������� � ������������ ������� �� �����.
		if( NULL == pGatewayHdl_Interface )
		{	// ������ ����� UART - ����� ������� ���������� �������� � ������ �������
			EventBits_t EventBitsResult = EventBitsMask & xEventGroupWaitBits( pGatewayHdl_UART->EventGroup, EventBitsMask, pdTRUE, pdTRUE, PossibleTimout );
			if( EventBitsMask != EventBitsResult )
			{	// �� ������� ��������� ��� �������� �����
				GatewayResult = SKLP_COMMAND_RESULT_ERROR_TM;
				break;
			}
			// �������� ����� �������. ������������� �������� ����� � �������� ������
			PacketToRecive = ( ByteQueueFragment_t ) { pByteQueueRx->iTail, ByteQueue_GetSpaceFilled( pByteQueueRx ) };
			// ����� ������������, �������� �����
			ByteQueue_RemoveFromTail( pByteQueueRx, NULL, PacketToRecive.Size );
		}
		else
		{	// ������ ����� ��������� - ����� ��������� ��������� � ������ ��������� ������
			SKLP_Message_t Message;
			if( ( pdTRUE == xQueueReceive( xInterfaceGatewayRxQueue, &Message, PossibleTimout ) ) &&	// ������� ����� SKLP
				( EVENT_SKLP_ANSWER == Message.Event ) )												// ����� � �������
					PacketToRecive = Message.Packet;		// !!! ���� �� ����������� �� ������� EVENT_SKLP_ANSWER, ����� �� ���� ��������?
			else
			{	// �� ������� �������� ���������� �������� �����
				GatewayResult = SKLP_COMMAND_RESULT_ERROR_TM;
				break;
			}
		}

		// �������� ����� ������ � ��������� � �������� ������ UART
		uint8_t *pAnswer = aQuery;		// ������������ ����� ��������� ������� SKLP.Slave ��� �������� ������ �� �����
		if( !SKLP_ReceivePacket( pAnswer, sizeof( aQuery ), pByteQueueRx, PacketToRecive ) ||
			( SKLP_START_ANSWER != pAnswer[SKLP_OFFSET_START] ) )
		{	// ��������� ������� ����������� ������
			GatewayResult = SKLP_COMMAND_RESULT_ERROR_FORMAT;
			break;
		}

		// ��������� ��������� � CRC ��������� ������, �������� ����� � ���������������� ���������.
		assert_param( SKLP_SendPacket( pAnswer, PacketToRecive.Size, SKLP_SendPacketFormat_Answer, pInterfaceMaster->pUART_Hdl ) );
		pInterfaceMaster->Statistic.PacketsTransmitted++;
	} while( 0 );
		
	// ���������� ������ � ��������� ������
	ATOMIC_WRITE( bQueryBufferAccessTaken, false );
	if( bGatewayUART_AccessTaken )
	{	// ���������� ������ � UART
		( void ) xEventGroupSetBits( pGatewayHdl_UART->EventGroup, EVENT_UART_EXT_ACCESS_READY );
	}
}

// ����������� ���������� ����� �� ���������� � �������� �����, ��������� ��������� � ����������� �����
// pPacketDest		- �������� �����, ���� ����������� �����
// PacketSizeMax	- ������ ��������� ������
// pByteQueueRx		- ����� ���������� ������ UART
// PacketToRecive	- ������� � ���������� ����, ������� ���������� �������� �� ���������� ������
// return			- true, ���� ����� ����������, � ��� ������ (START, SIZE, CRC8) �� �������.
bool SKLP_ReceivePacket( uint8_t *pPacketDest, ByteQueueIndex_t PacketSizeMax, ByteQueue_t *pByteQueueRx, ByteQueueFragment_t PacketToRecive )
{
	bool Result = false;

	do
	{	// ��������� ���������
		if( ( NULL == pPacketDest ) || ( NULL == pByteQueueRx ) )
			break;

		// ��������� ������ ������ � �������� � ���������� �������� ��������� ������
		if( PacketToRecive.Size > PacketSizeMax )
			break;

		// ����������� ����� �� ���������� ������ UART � �������� ������� �����, ������ � ����� ������ �� ����������
		if( !ByteQueue_BufferCopyFragment( pByteQueueRx, pPacketDest, PacketToRecive ) )
			break;

		// ��������� ��������� (���� �����)
		uint8_t ByteStart = pPacketDest[SKLP_OFFSET_START];
		if( ( ByteStart != SKLP_START_QUERY ) && ( ByteStart != SKLP_START_ANSWER ) )
			break;

		// ��������� ��������� (���� ������)
		uint8_t ByteSize = pPacketDest[SKLP_OFFSET_SIZE];
#ifdef	SKLP_SIZE_PACKET_LONG_MAX
		if( PacketToRecive.Size < SKLP_SIZE_PACKET_MAX )
		{	// ��� "��������" ������� ��������� ������ � ������� �������
#endif	// SKLP_SIZE_PACKET_LONG_MAX
			if( ( PacketToRecive.Size > SKLP_SIZE_PACKET_MAX ) || ( ( PacketToRecive.Size - SKLP_SIZE_TRIM ) != ByteSize ) )
				break;
#ifdef	SKLP_SIZE_PACKET_LONG_MAX
		}
		else
		{	// ��� "�������" ������� ������ ������ ���� "�������"
			if( SKLP_SIZE_MAX != ByteSize )
				break;
		}
#endif	// SKLP_SIZE_PACKET_LONG_MAX
		
		// ���� �����, �������, ������ - �� �����������

		// ��������� CRC8
		if( CalcCRC8SKLP( pPacketDest, PacketToRecive.Size - 1 ) != pPacketDest[ PacketToRecive.Size - 1 ] )
			break;

		// ����� ������� � ���������� � �������� �����, ��� ��������� �������� ������� ���������
		Result = true;
	}
	while( 0 );

	return Result;
}

// �������� ����� � ����������� ����� ������, ��������� � UART,
// *** � ��������� ���������� ��������. ***
// ����������� �������� � UART ����� DMA �� ����� ���� ������ 65535 ���� ��-�� ����������� DMA.
// ��� ������������� �������� ������� ����� 65535 ���� ���������� ������� HAL_UART_Ext_Transmit()
// pPacket		- ����� ������������� ������ � ���
// PacketSize	- ������ ������������� ������
// Format		- ������ ������������ ������
// pUART_Hdl	- ���������������� ���������, � ������� ��������� �����
bool SKLP_SendPacket( uint8_t *pPacket, uint16_t PacketSize, SKLP_SendPacketFormat_t Format, UART_Ext_HandleTypeDef *pUART_Hdl )
{
	bool ReturnValue = false;
	do
	{
		// ��������� ���������
		if( ( NULL == pPacket ) || ( NULL == pUART_Hdl ) )
			break;

		// �������� � ������� ���� NoWait �� �������
		bool bWaitForTransferFinish = !( Format & SKLP_SendPacketFormat_FlagNoWait );
		bool bPrepareForAnswer		= ( Format & SKLP_SendPacketFormat_FlagPrepForAnswer );
		Format &= SKLP_SendPacketFormat_MaskCommand;

		// ��������� ��������� ���������
		uint8_t Start = 0;
		switch( Format )
		{
		case SKLP_SendPacketFormat_Query:
			Start = SKLP_START_QUERY;
			break;
		case SKLP_SendPacketFormat_Answer:
		case SKLP_SendPacketFormat_AnswerMemRead:
			Start = SKLP_START_ANSWER;
			break;
		}
		if( 0 == Start )
			break;		// �������� �������� Format
		pPacket[SKLP_OFFSET_START] = Start;

		// ��������� ������ � (��������) ����������� �����
		bool bCalcCRCLater = true;
		switch( Format )
		{
		case SKLP_SendPacketFormat_Query:
		case SKLP_SendPacketFormat_Answer:
			assert_param( PacketSize >= SKLP_SIZE_TRIM );
			// ��������� ���� �������
			if( PacketSize <= ( 0xFF + SKLP_SIZE_TRIM ) )
			{	
				pPacket[SKLP_OFFSET_SIZE] = ( uint8_t ) ( PacketSize - SKLP_SIZE_TRIM );
				// ��� ��������� ������� CRC ����� ���������� ����� ��
				pPacket[ PacketSize - 1 ] = CalcCRC8SKLP( pPacket, PacketSize - 1 );
				bCalcCRCLater = false;
			}
			else
			{	// � ������ �������� ������ �������� �������������� ������ CRC. ������ ����� ���������� ��� � �������� �������� ������.
				pPacket[SKLP_OFFSET_SIZE] = 0xFF;
			}
			break;
		case SKLP_SendPacketFormat_AnswerMemRead:
			// ���� ������� � ������ ����������, �� ���������. ����� �������� �������, ����������� ����� ���������� �����
			assert_param( PacketSize > SKLP_MEMSECTORSIZE );
			break;
		default:
			assert_param( 0 );
		}
		
		// ��������� ����� ����� UART
		EventBits_t EventBitsMask = EVENT_UART_EXT_TX_ERROR | EVENT_UART_EXT_TX_COMPLETE;	// ������� ������� � ����� �������� ��� ������
		if( HAL_OK != HAL_UART_Ext_Transmit( pUART_Hdl, pPacket, PacketSize, EventBitsMask | ( bPrepareForAnswer ? EVENT_UART_EXT_RX_IDLE : 0 ) ) )
			break;
		// ��� ������� �������, ���������� CRC ����� �� ����� �������� ������
		if( bCalcCRCLater )
			pPacket[ PacketSize - 1 ] = CalcCRC8SKLP( pPacket, PacketSize - 1 );

		if( bWaitForTransferFinish )
		{	// ������� ������� � ����� �������� ��� ������
			float TimeToTransmite = HAL_UART_Ext_CalcTimeToTransmite( pUART_Hdl, PacketSize );
			assert_param( TimeToTransmite > 0.0f );
			TickType_t PossibleTimout = 5 + pdMS_TO_TICKS( 1000.0f * 1.2f * TimeToTransmite );
			EventBits_t EventBitsResult = EventBitsMask & xEventGroupWaitBits( pUART_Hdl->EventGroup, EventBitsMask, pdTRUE, pdFALSE, PossibleTimout );
			// !!! ���� ������ ������� ��������� � ���. ����������������, ����� ������������ �� ��������� ������ � ����������� �������,
			// !!! ����� ��������� ������, ����� ��������� ����������� ������ � ������ ���� ������������� ��������� ��������-�������������� ����������.
			// !!! ���� ������������� �����, �� �������� �� �� ������� ������, ����� ���������������.
			// assert_param( 0 != EventBitsResult );
			if( EVENT_UART_EXT_TX_COMPLETE != EventBitsResult )
				break;
		}
		
		// ����� ����������� � ��������� ��������� � ���������, ���� ����� ������������ -
		// � ����������� �� SKLP_SendPacketFormat_FlagNoWait
		ReturnValue = true;
	} while( 0 );

	return ReturnValue;
}

// ������ � ��������� � �������� ������
// ���������� ����������:
// - ��� ��������� ������
// - �� ������ �������� �� ��������� ������
// - �� ��������������� ����������� ������
// pInterface			��������� ������
// pQuery				����� �������. ������ ���� ��������� �������� ���������� (� �.�. ������� � �����), � ��������� (�����, ������, CRC) ����� ��������� �����.
// QuerySize			������ ������ ������� (�� ������ �� CRC)
// pAnswer				����� ��� ������ ������. NULL, ���� �� ����� ������.
// pAnswerSize			��� ������ - ������ ������ ��� �����. ��� �������� - ���������� ������ ������
// Timeout				������� �� �������� �������� ��������� ������ (������ ���������� � ������� ���������� �������� �������)
SKLPM_Result_t SKLPM_Query( SKLP_Interface_t	*pInterface, uint8_t *pQuery, uint16_t QuerySize, uint8_t *pAnswer, uint16_t *pAnswerSize, TickType_t Timeout )
{
	SKLPM_Result_t Result = SKLPM_ResultFail;
	do
	{
		SKLP_Message_t *pRxMessage, RxMessage;
		// �������� ����������
		if( NULL == pAnswer )
			pRxMessage = NULL;
		else
		{
			if( ( NULL == pAnswerSize ) || ( *pAnswerSize < SKLP_SIZE_PACKET_MIN_ANSWER ) )
			{
				Result = SKLPM_ResultFail_Parameters;
				break;
			}
			pRxMessage = &RxMessage;
		}

		// ��������� �����. ��� �������������, ��������� ������
		if( SKLPM_ResultOK != ( Result = SKLPM_Query_RetMsg( pInterface, pQuery, QuerySize, pRxMessage, Timeout ) ) )
			break;
		if( NULL == pAnswer )
			break;		// ����� ��������� �� ���������
		
		// ������� �����
		if( pRxMessage->Packet.Size > *pAnswerSize )
		{	// ���������� ����� �� ������ � ���������� �����
			Result = SKLPM_ResultFail_RxFormatPayload;
			break;
		}
		// ����������� ����� �� ���������� ������ UART � �������� ������� �����, ��������� ������
		if( !SKLP_ReceivePacket( pAnswer, *pAnswerSize,  pInterface->pUART_Hdl->pByteQueueRx, pRxMessage->Packet ) )
		{	// �� ������� ������� ����� � �����
			Result = SKLPM_ResultFail_RxFormatSKLP;		// !!! ������ ������ �� CRC, �� ����� ���� ���-�� � ����������� - ������ ���� ������ ������� �� SKLP_ReceivePacket()...
			break;
		}

		// ������ ����� ���������� �������, �� ����������� ���������� ������
		*pAnswerSize = pRxMessage->Packet.Size;
		Result = SKLPM_ResultOK;
	} while( 0 );
	return Result;
}

// ������ � ��������� � �������� ������
// ���������� ����������:
// - ��� ��������� ������
// - �� ������ �������� �� ��������� ������
// - �� ��������������� ����������� ������
// pInterface			��������� ������
// pQuery				����� �������. ������ ���� ��������� �������� ���������� (� �.�. ������� � �����), � ��������� (�����, ������, CRC) ����� ��������� �����.
// QuerySize			������ ������ ������� (�� ������ �� CRC)
// pReturnRxMessage		���� �� NULL -������� �����, � ������� ��������� � ������� (�� ���������)
// Timeout				������� �� �������� �������� ��������� ������ (������ ���������� � ������� ���������� �������� �������)
SKLPM_Result_t SKLPM_Query_RetMsg( SKLP_Interface_t	*pInterface, uint8_t *pQuery, uint16_t QuerySize, SKLP_Message_t *pReturnRxMessage, TickType_t Timeout )
{
	SKLP_InterfaceValidate( pInterface );

	SKLPM_Result_t Result = SKLPM_ResultFail;
	do
	{
		// �������� ����������
		if( ( NULL == pQuery ) || ( QuerySize < SKLP_SIZE_PACKET_MIN_QUERY ) )
		{
			Result = SKLPM_ResultFail_Parameters;
			break;
		}

		// �������� �������� �������
		xQueueReset( pInterface->pMessageQueue );

		// ��������� ����� ����� UART
		TickType_t TimestampStart = xTaskGetTickCount();
		if( !SKLP_SendPacket( pQuery, QuerySize, SKLP_SendPacketFormat_Query, pInterface->pUART_Hdl ) )
		{	// �� ������� ��������� ������
			Result = SKLPM_ResultFail_TxErr;
			break;
		}
		// �������� ������ ���������

		if( NULL == pReturnRxMessage )
		{	// ���� �� ��������� ������� ����� - ���������
			Result = SKLPM_ResultOK;
			break;
		}
			
		// �������� ��������					
		TickType_t TimeElapsed = xTaskGetTickCount() - TimestampStart;
		if( TimeElapsed >= Timeout )
		{	// ����� �������
			Result = SKLPM_ResultFail_RxTimeout;
			break;
		}

		// ������� ������� �� ���������������� �����������
		// ������� ������� �� �������
		SKLP_Message_t Message;
		if( pdFALSE == xQueueReceive( pInterface->pMessageQueue, &Message, Timeout - TimeElapsed ) )
		{	// �������� �������� �� ��������, ��� ��������� ����������� ��� ������� �� ������� ��������
			Result = SKLPM_ResultFail_RxTimeout;
			break;
		}

		// ��������� �������
		if( Message.Event != EVENT_SKLP_ANSWER )
		{	// �� ���� ������ ������?
			Result = SKLPM_ResultFail_RxErr;
			break;
		}

		// ����� ������. ������� ��������� � �������� �������, ��� ���������
		*pReturnRxMessage = Message;
		Result = SKLPM_ResultOK;
	} while( 0 );
	return Result;
}

// ���������� �������, ���������� ���������� �� SKLP_ProcessPacket() ����� ���������� �������� ������.
// ���������� ������ �� ��������-����������� ������, ����� ���������� ����������
// ����� �������� ���������� ����� � ���������������� ����� - ��������,
// ��� ������������ ������, �� �������� ������ ��������.
// !! ����� ������ �������� SKLP_ProcessPacket(), �� ����������!
void SKLP_SetTxCompleteCB( SKLP_Interface_t *pInterface, SKLP_InterfaceCB_t xPacketTxCompleteCB )
{
	assert_param( SKLP_InterfaceValidate( pInterface ) );
	pInterface->xPacketTxCompleteCB = xPacketTxCompleteCB;
}

// ����������� ���������� �� ������ ��������� � ���
void SKLP_PrintStatistics( SKLP_Interface_t *pInterface )
{
	SKLP_Statistic_t Statistic;
	ATOMIC_WRITE( Statistic, pInterface->Statistic );
	assert_param( MemoryThread_SprintfMutexTake( 5 ) );
	snprintf( ( char * ) ResetInfo.aLogMessage, sizeof( ResetInfo.aLogMessage ),
//		"[SKLP] %s channel statistics:\r\nTaskResetCounter = %lu\r\nFragmentsRecieved = %lu\r\nPacketsRejectedInFormat = %lu\r\nPacketsRejectedInBuzy = %lu\r\nPacketsSkipped = %lu\r\nPacketsProcessed = %lu\r\nPacketsProcessedSuccessful = %lu\r\nPacketsTransmitted = %lu\r\n",
//		pInterface->pName, Statistic.TaskResetCounter, Statistic.FragmentsRecieved, Statistic.PacketsRejectedInFormat, Statistic.PacketsRejectedInBuzy, Statistic.PacketsSkipped, Statistic.PacketsProcessed, Statistic.PacketsProcessedSuccessful, Statistic.PacketsTransmitted );
		"[SKLP] %s channel statistics:\r\nFragmentsRecieved = %lu\r\nPacketsRejectedInFormat = %lu\r\nPacketsRejectedInBuzy = %lu\r\nPacketsSkipped = %lu\r\nPacketsProcessed = %lu\r\nPacketsProcessedSuccessful = %lu\r\nPacketsTransmitted = %lu\r\n",
		pInterface->pName, Statistic.FragmentsRecieved, Statistic.PacketsRejectedInFormat, Statistic.PacketsRejectedInBuzy, Statistic.PacketsSkipped, Statistic.PacketsProcessed, Statistic.PacketsProcessedSuccessful, Statistic.PacketsTransmitted );
	MemoryThread_SprintfMutexGive( );
	assert_param( Logger_WriteRecord( ( char * ) ResetInfo.aLogMessage, LOGGER_FLAGS_STATICTEXT ) );
}

