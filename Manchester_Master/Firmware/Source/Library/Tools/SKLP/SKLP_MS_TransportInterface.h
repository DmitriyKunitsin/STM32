// SKLP_MS_TransportInterface.h
// �������� ���������������� ���� ��� [Master/Slave], �������� ����������������� ���������� � ������������ �������.
#ifndef	SKLP_MS_TRANSPORT_INTERFACE_H
#define	SKLP_MS_TRANSPORT_INTERFACE_H

#include "ProjectConfig.h"		// GPIO_CommonIndex_t
#include "common_uart.h"		// UART_Ext_HandleTypeDef
#include "common_tim.h"			// TIM_HandleTypeDef
#include "FreeRTOS.h"
#include "Queue.h"

// ************************* ���� ������ *************************
// ��������� �������� ������ ������
typedef enum SKLP_State_enum
{
	SKLP_STATE_WaitSync = 0,	// �������� ������������� (����� �� ����). ������ �������� �������� ����� ��������, �� ���������� �������������
	SKLP_STATE_WaitStart,		// �������� ���������� �����
	SKLP_STATE_WaitSize,		// �������� ������� ������
	SKLP_STATE_WaitTail,		// �������� ����� ������
	SKLP_STATE_PacketProcess,	// ����� ��������� � ����� ������, � ������ ���� ������� �� ���������� ���������
	SKLP_STATE_PacketReject,	// ����� ������ ���� �������� (������ �������, ����������� �����������������)
	SKLP_STATE_PacketSkip,		// ����� ������ ���� �������� (������ ����������, �� ��������� �� � ����� ������ - ����������������� �� ���������)
} SKLP_State_t;

// ���������� ������ � ��������� ������� �� ����������������� ����������
typedef struct SKLP_Statistic_struct
{
	uint32_t FragmentsRecieved;				// ���������� �������� ���������� �� UART.Rx.Idle (���� ������ �� ����������� � �� �������������, ������������� ���������� �������� �������)
	uint32_t PacketsRejectedInFormat;		// ���������� ����������� ������� (��������� ������� ������)
	uint32_t PacketsRejectedInBuzy;			// ���������� ����������� ������� (�� ��������� ��������� ����������� ������)
	uint32_t PacketsSkipped;				// ���������� ����������� ������� (������ ��� CRC ����������, �� ���������� �� � ����� ������)
	uint32_t PacketsProcessed;				// ���������� �������, ���������� �� ��������� (������ ��� CRC ����������, ���������� � ����� ������)
	uint32_t PacketsProcessedSuccessful;	// ���������� ������� ������������ ������� (���������� ������ ������ � �������)
	uint32_t PacketsTransmitted;			// ���������� ���������� �������
	uint32_t HeadersRecieved;				// ���������� �������� ���������� (START+SIZE) �� UART.Rx.Idle
} SKLP_Statistic_t;

// ��������� ������������ ������
#pragma pack( 1 )
typedef struct SKLP_PacketHeader_struct
{
	uint8_t Start;
	uint8_t Size;
	uint8_t Address;
} SKLP_PacketHeader_t;
#pragma pack( )

// ��� ��� ������������� �������� �� SKLP_ProcessPacket() �� ���������� �������� ������
struct SKLP_Interface_struct;
typedef void ( *SKLP_InterfaceCB_t )( struct SKLP_Interface_struct *pInterface );

// �������, ������������ � ������� ����������� �� ���������� �������
typedef uint8_t SKLP_InterfaceEvent_t;
#define	EVENT_SKLP_QUERY_2ME		( ( SKLP_InterfaceEvent_t ) ( 1 << 0 ) )	// ������ ������ �� ��� �����
#define	EVENT_SKLP_QUERY_2ALL		( ( SKLP_InterfaceEvent_t ) ( 1 << 1 ) )	// ������ ����������������� ������
#define	EVENT_SKLP_QUERY_2OTHER		( ( SKLP_InterfaceEvent_t ) ( 1 << 2 ) )	// ������ ������ �� ����� �����
#define	EVENT_SKLP_QUERY_GATEWAY	( ( SKLP_InterfaceEvent_t ) ( 1 << 3 ) )	// ������ ������ �� �����, ���������� � ������ ���������� (��������� _2OTHER �, ��������, _2ALL)
#define	EVENT_SKLP_ANSWER			( ( SKLP_InterfaceEvent_t ) ( 1 << 4 ) )	// ������ ����� �� ������
#define	EVENT_SKLP_NONE				( ( SKLP_InterfaceEvent_t ) 0 )				// ��� �������
#define	EVENT_SKLP_ALL				( EVENT_SKLP_QUERY_2ME | EVENT_SKLP_QUERY_2ALL | EVENT_SKLP_QUERY_2OTHER | EVENT_SKLP_QUERY_GATEWAY | EVENT_SKLP_ANSWER )

// ��������� ����������������� ���������� ��� ��������� ���
typedef struct SKLP_Interface_struct
{
	char					*pName;					// ��������
	UART_Ext_HandleTypeDef	*pUART_Hdl;				// ������� UART
	TIM_HandleTypeDef		*pTimerRxHdl;			// ������� ������� ������������ ����� ����� ������� � ������
	SKLP_Statistic_t		Statistic;				// ���������� ������ � ��������� �������

	// ������� FreeRTOS
	QueueHandle_t			pMessageQueue;			// ������� ��� �������� ������� �� ������������ ���������� UART
	SKLP_InterfaceEvent_t	EventsAllowed;			// ������ ��������� �������
	TimerHandle_t			FastBaudTimerHandle;	// ������ ��� ��������������� �������� � ������ �������� UART ��� ���������� ��������
	TickType_t				LastIncomingQueryTimeStamp;	// ������� ������� ����������� ���������� ��������� �������
	TimerHandle_t			LedRxTimer;				// ������ �������� ���������� �� ������ ������
	GPIO_CommonIndex_t		LedRxPin;				// ����� ���������� �����������

	// ��������� �������� ������� �������
	SKLP_State_t			State;					// ��������� �������� ������ ������
	SKLP_PacketHeader_t		PacketHeader;			// ����� ��������� ��������������� ������
	uint32_t				RxBuffer_SavedState;	// ����������� ��������� �������� Rx.DMA.NDTR, ������������ ��� �������� ���������� ������ ����� DMA, ����� ���������� UART.Rx.Idle ��� �� ��������
	SKLP_InterfaceCB_t		xPacketTxCompleteCB;	// ������������ ������� �� SKLP_ProcessPacket() �� ���������� �������� ������
} SKLP_Interface_t;

// ���������, ������������ � ������� ����������� �� ���������� �������
typedef struct SKLP_Message_struct
{
	SKLP_Interface_t		*pInterface;	// ���������-����������� (����������, ���� ��������� ����������� �������� �� ���� �������)
	ByteQueueFragment_t		Packet;			// ������������ ��������� ������ � ��������� ������ UART
	SKLP_InterfaceEvent_t	Event;		// ��� ���������� �������
	uint8_t mTxEventManchester;
	uint8_t mRxEventManchester;
} SKLP_Message_t;

// ������ ������������ ������ ��� �������� � ���������������� ���������
typedef enum SKLP_SendPacketFormat_enum
{
	SKLP_SendPacketFormat_Query				= 0x01,	// ������, ��������� ��������� SKLP_START_QUERY
	SKLP_SendPacketFormat_Answer			= 0x02,	// �����, ��������� ��������� SKLP_START_ANSWER
	SKLP_SendPacketFormat_AnswerMemRead		= 0x03,	// �����, ��������� ��������� SKLP_START_ANSWER, ����������� ���� [Size]
	SKLP_SendPacketFormat_MaskCommand		= 0x03,	// ����� �������
	SKLP_SendPacketFormat_FlagPrepForAnswer	= 0x40,	// ����: ����� ���������� �������� ������, ����������������� UART ��� ��������� ������� �� ��������� ������, �� ����� �� �����
	SKLP_SendPacketFormat_FlagNoWait		= 0x80,	// ����: �� ������� ���������� �������� ������
} SKLP_SendPacketFormat_t;

// ��������� ���������� ������� (�������) ��� �������
typedef enum SKLPM_Result_enum
{
	SKLPM_ResultOK = 0,				// ������� ��������� �������
	SKLPM_ResultOK_Skip,			// ����� ��� �������� � ����� � ������� ������ �������� ������
	SKLPM_ResultOK_Restart,			// ���������� ������������� ���� ������
	SKLPM_ResultOK_Finish,			// ���������� ��������� ���� ������
	SKLPM_ResultFail,				// ������ (��������������������)
	SKLPM_ResultFail_Internal,		// ������ ���������� (???)
	SKLPM_ResultFail_Parameters, 	// ������ ���������� �������
	SKLPM_ResultFail_ChannelBuzy,	// �� ������� ������������ � ����������������� ������
	SKLPM_ResultFail_TxErr,			// �������� ������ ��� �������� ������� � UART
	SKLPM_ResultFail_TxTimeout, 	// ����� ������� ��� �������� ����� �������� �������
	SKLPM_ResultFail_RxTimeout,		// ����� ������� �� �������� ������ (�� ���������� ����� �� ������ ����� � �������)
	SKLPM_ResultFail_RxIdleTimeout,	// ����� ������� �� �������� ������ (�� ���������� ����� �� ���� ���������� � ������ ������)
	SKLPM_ResultFail_RxErr, 		// �������� ������ ��� ������ ������ �� UART
	SKLPM_ResultFail_RxOwr,			// ������������ ������ ���������
	SKLPM_ResultFail_RxEcho,		// � ����� �� ������ ������� ���
//	SKLPM_ResultFail_RxFormat, 		// ������ ������� ��������� ������ (�����, ������, CRC)
	SKLPM_ResultFail_RxFormatSKLP,	// ������ ������� ��������� ������ �� �������� ��������� (�����, ������, CRC)
	SKLPM_ResultFail_RxFormatPayload,	// ������ ������� ����������� ��������� ������ (������, ��������� � �.�.)
} SKLPM_Result_t;

// �������� �� ������������ ���������� ������� � UART
void SKLP_TimerElapsed( SKLP_Interface_t *pInterface );		// ������� �� TIM.Ovf. ����� �� ���� - ������� ����� ��� ��������������������
void SKLP_ReceiveFragment( SKLP_Interface_t *pInterface );	// ������� �� USART.RxIdle. ������� �������� ������ �� ������ UART. ��� ��������� ������� ������ ��������� �� ���������� ���������

// ����������� ���������� ����� �� ���������� � �������� �����, ��������� ��������� � ����������� �����
bool SKLP_ReceivePacket( uint8_t *pPacket, ByteQueueIndex_t PacketSizeMax, ByteQueue_t *pByteQueueRx, ByteQueueFragment_t PacketToRecive );
// ������� ����� �� ������ UART � ��������� ��� ���������
void SKLP_ProcessPacket( SKLP_Interface_t *pInterface, ByteQueueFragment_t PacketToRecive );
// ������� ����� �� ������ UART � ��������� ����������
void SKLP_ProcessPacketGateway( SKLP_Interface_t *pInterfaceMaster, ByteQueueFragment_t PacketToGateway, UART_Ext_HandleTypeDef *pGatewayHdl_UART, SKLP_Interface_t *pGatewayHdl_Interface, uint16_t WaitAnswerTimeout_ms );
void SKLP_ProcessPacketGatewayV2( SKLP_Message_t *pMessageToGateway, SKLP_Interface_t *pInterface );
// �������� ����� � ����������� ����� ������, � ��������� � UART
bool SKLP_SendPacket( uint8_t *pPacket, uint16_t PacketSize, SKLP_SendPacketFormat_t Format, UART_Ext_HandleTypeDef *pUART_Hdl );
// ������ � ��������� � �������� ������
SKLPM_Result_t SKLPM_Query( SKLP_Interface_t	*pInterface, uint8_t *pQuery, uint16_t QuerySize, uint8_t *pAnswer, uint16_t *pAnswerSize, TickType_t Timeout );
SKLPM_Result_t SKLPM_Query_RetMsg( SKLP_Interface_t	*pInterface, uint8_t *pQuery, uint16_t QuerySize, SKLP_Message_t *pReturnRxMessage, TickType_t Timeout );
// ����������� ���������� �� ������ ����������������� ������ � ���
void SKLP_PrintStatistics( SKLP_Interface_t *pInterface );
// ���������� �������, ���������� ���������� �� SKLP_ProcessPacket() ����� ���������� �������� ������
void SKLP_SetTxCompleteCB( SKLP_Interface_t *pInterface, SKLP_InterfaceCB_t xPacketTxCompleteCB );

#endif	// SKLP_MS_TRANSPORT_INTERFACE_H

