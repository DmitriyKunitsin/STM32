// SKLP_MS_Transport.h
// �������� ���������������� ���� ��� [Master/Slave], ����� � ������ ������� �� ����.
// �������� ������� �������� � �������.
#ifndef	SKLP_MS_TRANSPORT_H
#define	SKLP_MS_TRANSPORT_H

#define	SKLP_RX_BUFFER_SIZE					512		// ������ ��������� ������ UART

// ��������� ����� � ������ ���
#define	SKLP_OFFSET_START					0		// �������� ���� ������
#define	SKLP_OFFSET_SIZE					1		// �������� ���� �������
#define	SKLP_OFFSET_ADDRESS					2		// �������� ���� ������ ������			������ ����� �������
#define	SKLP_OFFSET_COMMAND					3		// �������� ���� �������				������ ����� �������
#define	SKLP_OFFSET_DATA_QUERY				4		// �������� ���� ������					������ ����� �������
#define	SKLP_OFFSET_DATA_ANSWER				2		// �������� ���� ������					������ ����� ������
#define	SKLP_OFFSET_CRC8( _PACKET_ )		( SKLP_SIZE( _PACKET_ ) + SKLP_SIZE_TRIM - 1 )	// �������� ���� CRC8

// ���������� ����� � ������ ���
#define	SKLP_START_QUERY					'@'		// 0x40	���� ����� - ������
#define	SKLP_START_ANSWER					'#'		// 0x23	���� ����� - �����
#define	SKLP_START_QUERY_EXT				'<'		// 0x3C	���� ����� - ������ (����������� ��������)
#define	SKLP_START_ANSWER_EXT				'>'		// 0x3E	���� ����� - ����� (����������� ��������)

// ������ ������� ������ � ���� ������ � ������
// _PACKET_ - ����� ������ ������ � �������
#define	SKLP_SIZE( _PACKET_ )				( ( _PACKET_ )[SKLP_OFFSET_SIZE] )				// �������� ���� �������
#define	SKLP_SIZE_MIN_QUERY					2		// ���� ������� �������, ����������� - ������ ����� � �������
#define	SKLP_SIZE_MIN_ANSWER				0		// ���� ������� ������, �����������
#define	SKLP_SIZE_MAX						255		// ���� �������, ������������
#define	SKLP_SIZE_TRIM						3		// ���� ������� ������, ��� ������ ����� ������ �� �������� TRIM (���������� ���� ������, �������, CRC8)
#define	SKLP_SIZE_PACKET( _PACKET_ )		( SKLP_SIZE( _PACKET_ ) + SKLP_SIZE_TRIM )		// ������ ������ ������
#define	SKLP_SIZE_PACKET_MAX				( SKLP_SIZE_MAX + SKLP_SIZE_TRIM )				// ����������� ��������� ������ ������
#define	SKLP_SIZE_PACKET_MIN_QUERY			( SKLP_SIZE_MIN_QUERY + SKLP_SIZE_TRIM )		// ����������� ��������� ������ ������ (������)
#define	SKLP_SIZE_PACKET_MIN_ANSWER			( SKLP_SIZE_MIN_ANSWER + SKLP_SIZE_TRIM )		// ����������� ��������� ������ ������ (�����)
#define	SKLP_SIZE_DATA_MAX_QUERY			( SKLP_SIZE_MAX - SKLP_SIZE_MIN_QUERY )			// ����������� ��������� ������ ���� ������ � �������
#define	SKLP_SIZE_DATA_MAX_ANSWER			( SKLP_SIZE_MAX - SKLP_SIZE_MIN_ANSWER )		// ����������� ��������� ������ ���� ������ � ������
#define	SKLP_SIZE_DATA_QUERY( _PACKET_ )	( SKLP_SIZE( _PACKET_ ) - SKLP_SIZE_MIN_QUERY )	// ������ ���� ������ � �������
#define	SKLP_SIZE_DATA_ANSWER( _PACKET_ )	( SKLP_SIZE( _PACKET_ ) - SKLP_SIZE_MIN_ANSWER )// ������ ���� ������ � ������

// ��������� ���������
#define	SKLP_SIGNATURE_COMMAND_WORKTIME		'T'			// ������������ � �������� ������/������ �������� ������������� �������
#define	SKLP_SIGNATURE_COMMAND_NVM_SET		"notice"	// ������������ � �������� ������ ������������� �������������
#define	SKLP_SIGNATURE_SAVING_START			"START"		// ��������� ������, ����������� � ������ �������
#define	SKLP_SIGNATURE_SAVING_STOP			"STOP"		// ��������� ����� ���������� �����, ������������ � ������

// ������ ��������� �������
#define	SKLP_ADDRESS_BROADCAST				0x00	// �����������������
#define	SKLP_ADDRESS_DEPTHMETER				0x01	// ����������
#define	SKLP_ADDRESS_BKZ					0x10	// ���
#define	SKLP_ADDRESS_MPI_500				0x10	// ���	������ ������ � ��������� ��� ��������� ���.500. !!������������ � ���, �� ���������� ��������������� DeviceType
#define	SKLP_ADDRESS_RUS_Tele				0x1A	// ���, ���������� ����������
#define	SKLP_ADDRESS_RUS_Pump				0x1B	// ���, ���������� �������������� (�����������������)
#define	SKLP_ADDRESS_RUS_Pump0				0x1C	// ���, ���������� �������������� ������ 0�
#define	SKLP_ADDRESS_RUS_Pump1				0x1D	// ���, ���������� �������������� ������ 120�
#define	SKLP_ADDRESS_RUS_Pump2				0x1E	// ���, ���������� �������������� ������ 240�
#define	SKLP_ADDRESS_RUS_Incl				SKLP_ADDRESS_INCL
#define	SKLP_ADDRESS_RUS_GK					SKLP_ADDRESS_VIKPB_GK
#define	SKLP_ADDRESS_BK2					0x20	// �K		������� �������
#define	SKLP_ADDRESS_BK						0x22	// �K		������� �������
#define	SKLP_ADDRESS_BKS					0x24	// �K�		������� ������� (�����������)
#define	SKLP_ADDRESS_MRGK_REZ				0x30	// ����	����� ��������������
#define	SKLP_ADDRESS_IK						0x40	// ��
#define	SKLP_ADDRESS_VMKZ					0x50	// �����
#define	SKLP_ADDRESS_VIKPB					0x55	// �����
#define	SKLP_ADDRESS_VIKPB_GK				0x56	// ����� ��
#define	SKLP_ADDRESS_MUP					0x62	// ���		������ ���������� ������������
#define	SKLP_ADDRESS_NNKT					0x71	// ����	�������-���������� �������
#define	SKLP_ADDRESS_MRGK_GK				0x72	// ����	����� ��
#define	SKLP_ADDRESS_NNKT2					0x73	// ����2	�������-���������� �������, �� ������
#define	SKLP_ADDRESS_GGKP					0x80	// ���-�	�����-�����-�����������
#define	SKLP_ADDRESS_GGLP					0x81	// ����	�����-�����-����-�����������
#define	SKLP_ADDRESS_GGKP_2					0x82	// ���	!!! ��������, ��� ���� ���!
#define	SKLP_ADDRESS_GGLP_SADC				0x88	// ���-��-���	������������������ ��� ��� ���-��
#define	SKLP_ADDRESS_MI						0x90	// ��
#define	SKLP_ADDRESS_MI_GK					0x91	// ��		����� ��
#define	SKLP_ADDRESS_INCL					0x99	// ����	����������� (������� ��������)
#define	SKLP_ADDRESS_AK						0xA0	// ��		������ ������������� ��������
#define	SKLP_ADDRESS_AKD					0xAF	// ���		���������� ������ ��� (������������� �������� �� ������� ������)
#define	SKLP_ADDRESS_ACP					0xB0	// ���		�������� ���.435.00.01.00 ������ ��� ���.679.00.00.00 (�������������� � ������� �������)
#define	SKLP_ADDRESS_ACPB_623				0xB1	// ���(�)	���������� ����������� ���.623.00.02.00 ������ ��� ���.623.00.00.00 (��������������� ����� �������� ��������)
#define	SKLP_ADDRESS_ACPB_619				0xB2	// ���(�)	���������� ����������� ���.619.00.04.00 ������ ��� ���.619.00.00.00 (������������)
#define	SKLP_ADDRESS_ACP_GENHV				0xBF	// ���		�������������� ��������� ���.435.00.03.00 ������������� �����������-����������� ���.679.00.00.00
#define	SKLP_ADDRESS_INGK					0xC0	// ����	���������� ������ ����
#define	SKLP_ADDRESS_INK					0xC1	// ���		���������� ���
#define	SKLP_ADDRESS_INGK_FPGA				0xCF	// ����	��������� ��������� ����
#define	SKLP_ADDRESS_MP						0xD0	// ��		������ �������
#define	SKLP_ADDRESS_MP_18V_BROADCAST		0xD0	// ��18	������ ������� 18 �	����� �������
#define	SKLP_ADDRESS_MP_18V					0xD1	// ��18	������ ������� 18 �	������ � �������������� ����������
#define	SKLP_ADDRESS_MP_36V_BROADCAST		0xD2	// ��36	������ ������� 36 �	����� �������
#define	SKLP_ADDRESS_MP_36V					0xD3	// ��36	������ ������� 36 �	������ � �������������� ����������
#define	SKLP_ADDRESS_NDR					0xDA	// ���		����������� �����������
#define	SKLP_ADDRESS_NDR_CALIBR_TORQUE		0xDB	// ���		���������� ������� ���
#define	SKLP_ADDRESS_NDMt					0xDB	// �����	������-���������� ������ ������������
#define	SKLP_ADDRESS_NDR_CALIBR_AXIS		0xDC	// ���		���������� ������ �������� ���
#define	SKLP_ADDRESS_INCL_ZENI				0xDD	// ����	���������� ��������� ����
#define	SKLP_ADDRESS_NDMt2					0xDE	// �����	������-���������� ������ ������������ (����)
#define	SKLP_ADDRESS_NDMp					0xDF	// ����	���������� ������ ������������
#define	SKLP_ADDRESS_MPP_160				0xE0	// ���-160	������ ������ � �������
#define	SKLP_ADDRESS_ER						0xE2	// ��
#define	SKLP_ADDRESS_MPI_600				0xEA	// ���		������ ������ � ��������� ��� ��������� ���.600
#define	SKLP_ADDRESS_TEST_SD				0xEB	// ����������� SD-���� �� ������ ��� ���.600 ��� ����.������
#define	SKLP_ADDRESS_RDI					0xEC	// ����������� ������ ������������ �� ������ ���
#define	SKLP_ADDRESS_MPP					0xF0	// ���		������ ������ � ������� (�������)
#define	SKLP_ADDRESS_NP						0xF0	// ��
#define	SKLP_ADDRESS_MPP_1					0xF1	// ���_1	������ ������ � ������� (�������-1)
#define	SKLP_ADDRESS_MPP_2					0xF2	// ���_2	������ ������ � ������� (�������-2)
#define	SKLP_ADDRESS_MPP_3					0xF3	// ���_3	������ ������ � ������� (�������-3)

// ������������������ ������� ��������� ���
typedef enum SKLP_Command_enum /*: uint8_t // only in C++11 */
{
	SKLP_COMMAND_RESET 				= 0x00,
	SKLP_COMMAND_STOP_LOGGING 		= 0x00,		//	�� ��������
	SKLP_COMMAND_ID_GET				= 0x01,
	SKLP_COMMAND_ID_SET				= 0x02,
	SKLP_COMMAND_DATA_ACQ_GET_LONG	= 0x03,		// ������ �������� ����� �������� ������
	SKLP_COMMAND_NVM_GET			= 0x04,
	SKLP_COMMAND_NVM_SET			= 0x05,
	SKLP_COMMAND_WORKTIME_GET		= 0x07,
	SKLP_COMMAND_WORKTIME_INC		= 0x08,
	SKLP_COMMAND_WORKTIME_SET		= 0x09,
	SKLP_COMMAND_DATA_ACQ_GET		= 0x13,		// ������ ����� �������� ������
	SKLP_COMMAND_DATA_TECH_GET		= 0x14, 	// ������ ����� ��������������� ������
	SKLP_COMMAND_MEMORY_ERASE		= 0x20,
	SKLP_COMMAND_MEMORY_STATE_GET	= 0x21,
	SKLP_COMMAND_MEMORY_READ		= 0x22,
	SKLP_COMMAND_EVENTS_READ		= 0x28,
	SKLP_COMMAND_EVENTS_CLEAR		= 0x29,
	SKLP_COMMAND_TIME_SYNC_MASTER	= 0x31,		// ��������� �������� ������� ��� ���, ���, ��. ��������� ������ ������ ����������
	SKLP_COMMAND_TIME_LOG_START_SET	= 0x32,		// ���������� ����� ���������
	SKLP_COMMAND_TIME_FREEZE		= 0x33,		// ���������� ������� �����
	SKLP_COMMAND_TIME_FROZEN_GET	= 0x34,		// ������� ����� ������������ �����
	SKLP_COMMAND_TIME_LOG_START_GET	= 0x35,		// ��������� ����� ���������
	SKLP_COMMAND_TIME_LOG_STOP_SET 	= 0x36,		// ���������� ����� ����������
	SKLP_COMMAND_TIME_SYNC			= 0x37,
	SKLP_COMMAND_TIME_LOG_STOP_GET 	= 0x38,		// ��������� ����� ����������
	SKLP_COMMAND_TIME_SYNC2			= 0x3F,		// ������ #37 ��� LWD, !!!�������� ������ ������ �������!!!
	SKLP_COMMAND_MEMORY_INFO_GET	= 0x41,
	SKLP_COMMAND_BAUD_SET			= 0x46,
	SKLP_COMMAND_GOTO_BOOTLOADER	= 0xB1,		// !! ����! ������� �� BootLoader
	SKLP_COMMAND_DATA_ACQ_START		= 0xFF,		// ���������� � ����������

	SKLP_COMMAND_AKP_CUSTOM			= 0x60,
	SKLP_COMMAND_FREE				= 0xFE		// ����������������� ������, �������������� ������������ ������� - ������������ ��� ��������� ��������� ������� � ������� ���������� ���������
} SKLP_Command_t;

//#ifdef	STATIC_ASSERT
//STATIC_ASSERT( sizeof( SKLP_Command_t ) == sizeof( uint8_t ) );
//#endif

// �������� UART ��-���������
#ifndef	SKLP_BAUD_DEFAULT
#define	SKLP_BAUD_DEFAULT				115200	// [���]	RS-485
#endif
#define	SKLP_BAUD_SIG60_DEFAULT			57600	// [���]	����� SIG60

// ��������
#define	SKLP_BAUD_FAST_COOLDOWN_s		1.5f	// [�]	������� ��������������� ������ �������� UART ����� ��������� �������� �������
#define	SKLP_TIMERRX_PAUSE_BYTES		6.0f	// [����]	������������ ����� �� ���� (� ������), ����� ������� ����������� ����� �� ���������. +1 ���� ��� ���������� ������ ���������� �����
#define	SKLP_LEDRX_FLASH_DURATION_s		15e-3f	// [�]	������������ ������� ���������� ��� ������ ����������� ������ (������ ��� ������)
#define	SKLP_TIMEOUT_READMEMANSWER_s	1.1f	// [�]	���������� ������� �� ������ �������� ������ �� [0x22] - ������� ������ ������

#endif	// SKLP_MS_TRANSPORT_H

