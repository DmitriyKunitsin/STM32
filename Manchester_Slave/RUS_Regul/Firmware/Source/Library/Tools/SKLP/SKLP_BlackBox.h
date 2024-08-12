// SKLP_BlackBox.h
// ��������� ������� � ������� ����� �� ��������� ���
#ifndef	SKLP_BLACKBOX_H
#define	SKLP_BLACKBOX_H

#include <stdbool.h>
#include "SKLP_Time.h"

#define	BLACKBOX_MESSAGE_MAXLENGHT	15

typedef void ( * SKLP_BlackBoxCallback_t )( char *pSrcTxt, uint32_t Argument, char *pDest );
//bool SKLP_BlackBox_Init( void );								// �������������
bool SKLP_BlackBox_WriteRecord( char const *pText );	// ���������� ������ � ������ ����
//bool SKLP_BlackBox_WriteRecordExt( char const *pText, SKLP_BlackBoxCallback_t xCallback, uint32_t CallbackArgument );	// ���������� ������ � ������ ���� � ������� ����������������� �������� 
//bool SKLP_BlackBox_Clear( void );							// ������� ������� ����� ��� ����� � ���������� �����
bool SKLP_BlackBox_WriteRecordTimestamp( char const *pText, SKLP_Time_t Timestamp );	// ���������� ������ � ������ ����, ������������ ��������� �����

// ���������� ����� ��������������, ���������� � �������� -
// ����� ����� ���� ��������������� ���������������� ��������������� ������ ������
//extern const char aBlackBox_Name[];

// ����������� ����������� � ������ ���� ����			'*' - "�������������" ����, 'A' - �� ����� ������� ����� ������������ ������, 'S' - �� ����� ������� �������� ������������ ������
#define	SKLP_BLACKBOX_TAG_GOTOAUTO				"M1"	// ������� � ����� �������� (���������� �����)
#define	SKLP_BLACKBOX_TAG_LOGSTART				"M2"	// ? ������� � ����� ����������� (���������� �����)
#define	SKLP_BLACKBOX_TAG_EXITAUTO_CMD			"M3U"	// ����� �� ����������� ������ (������������ �����) �� �������
#define	SKLP_BLACKBOX_TAG_EXITAUTO_FAIL			"M3F"	// ����� �� ����������� ������ (������������ �����) �� ������������ Flash
#define	SKLP_BLACKBOX_TAG_AUTO_IDLE_SKLP		"M4I"	// * ����������� �����������, �������� ��������� (���������� �����) - ��� �������� �� ���
#define	SKLP_BLACKBOX_TAG_AUTO_IDLE_TIME		"M4T"	// * ����������� �����������, �������� ��������� (���������� �����) - ����� �� ������� ���������� ����
#define	SKLP_BLACKBOX_TAG_AUTO_ACTIVE_SKLP		"M5I"	// * ��������� (�������������� �����������) - �������������� �������� �� ���
#define	SKLP_BLACKBOX_TAG_AUTO_ACTIVE_TIME		"M5T"	// * ��������� (�������������� �����������) - ��������� � ������� ���������� ����
//#define	SKLP_BLACKBOX_TAG_MPI_GOTOAUTO_CMD		SKLP_BLACKBOX_TAG_EXITAUTO_CMD	// ������� � ����� �������� (���������� �����) �� ������� �� ���������� (����� RS-485)
#define	SKLP_BLACKBOX_TAG_MPI_GOTODRILL_CMD		"M6U"	// * ������� ��� � ����� ������� �� ������� �� ���������� (����� RS-485)
#define	SKLP_BLACKBOX_TAG_MPI_GOTODRILL_MUP		"M6M"	// * ������� ��� � ����� ������� �� ������� �� ��� (����� ����������) 
#define	SKLP_BLACKBOX_TAG_MPI_GOTOLOGGING_CMD	"M7U"	// * ������� ��� � ����� �������� �� ������� �� ���������� (����� RS-485)
#define	SKLP_BLACKBOX_TAG_MPI_GOTOLOGGING_MUP	"M7M"	// * ������� ��� � ����� �������� �� ������� �� ��� (����� ����������) 
#define	SKLP_BLACKBOX_TAG_RESET_PWR				"R1UR0"		// ������������ �� ���������� �������
#define	SKLP_BLACKBOX_TAG_RESET_WDT				"R1WR0"		// ������������ �� WatchDog
#define	SKLP_BLACKBOX_TAG_RESET_SOFT_REBOOT		"R1S1R0"	// *	������������ �� ������� ����� (Reboot)
#define	SKLP_BLACKBOX_TAG_RESET_SOFT_ASSERT		"R1S2R0"	// *	������������ �� ������� ����� (Assert)
#define	SKLP_BLACKBOX_TAG_RESET_SOFT_FAULT		"R1S3R0"	// *	������������ �� ������� ����� (Hard Fault)
#define	SKLP_BLACKBOX_TAG_RESET_UNKNOWN			"R1NR0"		// *	������������ �� �� ������������ �������
// ������ R0 ������ ���� R0, R1, R6, R7 - �� ������ ������, � ������� ��������.

#define	SKLP_BLACKBOX_TAG_RESET_SLAVESTART		"R21"	// ������������ ������������ ������ (������)
#define	SKLP_BLACKBOX_TAG_RESET_SLAVEFINISH		"R22"	// ������������ ������������ ������ (����������)
#define	SKLP_BLACKBOX_TAG_FLASH_RESTORE			"F01"	// ���������� ������ flash �������������
#define	SKLP_BLACKBOX_TAG_FLASH_ERR_WRITE		"F21"	// ������ ������ ������ � ������
#define	SKLP_BLACKBOX_TAG_FLASH_ERR_READ		"F31"	// ������ ������ ������ �� ������
#define	SKLP_BLACKBOX_TAG_FLASH_FORMAT			"F41"	// *	����������� ��������������
#define	SKLP_BLACKBOX_TAG_SLAVE_FAIL_ANSWER		"E1"	// A		����� � ����������� ������� ������� (��� ������)
#define	SKLP_BLACKBOX_TAG_SLAVE_FAIL_CRC		"E2"	// A		����� � ����������� ������� ������� (������ CRC �� ������� ����� ������)
#define	SKLP_BLACKBOX_TAG_SLAVE_RESTORE			"E3"	// A		����� � ����������� ������� ������������
#define	SKLP_BLACKBOX_TAG_SLAVE_APPEAR			"E4"	// *AS	����� � ����������� ������� ������������, �� ������������ ����� �������� �����
#define	SKLP_BLACKBOX_TAG_SLAVE_FAIL_SYNC		"E5"	// *		����� � ����������� ������� ������������������

#define	SKLP_BLACKBOX_TAG_VOLTAGE				"V1"	// 		??����� ������������� � ���??
#define	SKLP_BLACKBOX_TAG_VPLC_FAULT			"V5"	// *		����� �������� ����� MPI.+VPLC
#define	SKLP_BLACKBOX_TAG_VPLC_RESTORE			"V6"	// *		�������������� �������� ����� MPI.+VPLC

#endif	// SKLP_BLACKBOX_H

