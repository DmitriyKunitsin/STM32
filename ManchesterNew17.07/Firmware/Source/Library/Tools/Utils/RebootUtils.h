// RebootUtils.h
// �������, ��������� � �������� � �������������
#ifndef	REBOOT_UTILS_H
#define	REBOOT_UTILS_H

#include <stdint.h>
#include <stdbool.h>
#include "SKLP_Time.h"		// TimeSubs_t

// ���������
void WatchdogReset( void );									// ���������� ����������� �������
void SystemHardwareInit( void );								// ������������� ���������� ����������� � ����������� �������, ��������� ��������� ������
bool RebootDelayed( const float Delay_s, const char *pNote );	// ������� Reboot() ����� �������
__noreturn void Reboot( const char *pNote );						// ���������� ������������, ��������� ����� ��������� � ��� ��� ����������� �������
__noreturn void ResetToApplication( uint32_t xApplication );		// ���������� ������������ � ������� �� ��������� ����������
__noreturn void ResetToFunction( FunctionVoid_t xFunction );	// ���������� ������������ � ������� �� �������� �������
extern void FaultCallback( void );								// ������� �� ����������� assert() � Hard Fault - ����� ���������� ����������� �������� (������� ������) � ������ ������
///*static*/ /*__noreturn*/ void JumpToApplication( uint32_t ApplicationAddress );

// ���, ������������ ������� ������ ������������
typedef enum ResetInfoTag_enum
{
	ResetInfoTag_Hardware	= 0x00000000ul,			// ������� ���������� ��������
	ResetInfoTag_Clear		= 0x01234567ul,			// ����� ��������� �������� ��� � ��� ���������. �������� ������ �� ����.
	ResetInfoTag_Assertion	= 0x12345678ul,			// ������������ ����� Assert, ������� ���������� Assertion Info
	ResetInfoTag_HardFault	= 0x23456789ul,			// ������������ ����� Hard Fault, ������� ���������� Hard Fault Info
	ResetInfoTag_Reboot		= 0x34567890ul,			// ������ ������������, ������� ���������� � �������
	ResetInfoTag_JumpToApp	= 0x45678901ul,			// ������ ������������ ��� ������ ���� ��������� ����� ��������� �� ������ ����������
	ResetInfoTag_JumpToFunc	= 0x56789012ul, 		// ������ ������������ ��� ������ ���� ��������� ����� ��������� �� ��������� �������
	// ������ ���� �������� ��������������� �� ���������� ������������
} ResetInfoTag_t;

// ����������� ��������� ������������ - ���������� ��� �����������
typedef enum ResetSource_enum
{
	ResetSource_Undefined = 0,		// �� ������� ���������� (�� ������ ����������)
	ResetSource_Power,				// ����� �� ���������� ������� ��� ����������� �������
	ResetSource_Watchdog,			// ����� �� ����������� ������� (������������ ��� ��������)
	ResetSource_Pin,				// ����� �� ����� Reset ��� ���������� �������
	ResetSource_SoftUndefined,		// ����������� ������������, �� ������� ���������� �������
	ResetSource_SoftReboot,			// ������������ �� ������� ����� (��������, ����� ��������� �������������������� �������� ������� ����� ��������������)
	ResetSource_SoftAssertion,		// ������������ �� Assert
	ResetSource_SoftFault,			// ������������ �� Hard Fault
} ResetSource_t;

// ������ ��������
typedef enum ResetStage_enum
{
	ResetStage_PowerUp = 0,					// ����� ������ ������� ��� ��������
	ResetStage_SystemInitStart,				// ���� � SystemInit()->SystemInitCallback()
	ResetStage_SystemInitFinish,			// ����� �� SystemInit()->SystemInitCallback()
	// ���-�� ����� ���������������� ����������� ����������
	ResetStage_SystemHardwareInitStart,		// ���� � main()->SystemHardwareInit()
	ResetStage_SystemHardwareInitFinish,	// ����� �� main()->SystemHardwareInit()
	ResetStage_InitComplete,				// ������������� ���������
	ResetStage_SoftReset,					// ���������� ������������ ������
} ResetStage_t;

// ���������, � ������� ����������� ���������� ����� ����������� ������ �������������
typedef struct ResetInfo_struct
{
	ResetInfoTag_t	Tag;							// ���, �� �������� ����� ���������� �������� ������� ������ (��������������� ����� ������ �������������, ������������ ����� ��������� ��� ������)
	ResetInfoTag_t	TagSaved;						// ����������� ���, ������� � ��������� �� ����� ������
	ResetStage_t	ResetStage;
	ResetSource_t	ResetSource;					// �������� ��������� ������������ - ���������� ��� �����������
	uint32_t		ResetCounter;					// ������ ��������� ����� ������������
	uint32_t		xResetToApplication;			// �����, �� ������� ���������� ������� ��� �������, ���� ��� ����� ResetInfoTag_JumpToApp
	char			aRebootMessage[256];			// �����, ����������� ����� �������������� �������������. ������ ���� ������� � ��� ����� ����������� ���������
	char			aLogMessage[256+128];			// �����, ����������� � ����������� Reset (������ main) ��� ������������ ������ � ��� ����� ������� �������� ������� � ���������� ������������
	TimeSubs_t		RTC_SubsecondCorrection;		// �������� ��� ��������� ������� RTC, �.�. ������������ ��� ������������� �� ����, � ��������� ������ ���� ��������� ��� ������������. ����� �� ������ ��� ������������
} ResetInfo_t;

extern volatile ResetInfo_t ResetInfo;


// ���������� ����� �� ��������������� ����� BuildInfo.c - ���������?
extern const char aBuildInfo_Date[];
extern const char aBuildInfo_Time[];
extern const char aBuildInfo_ProjName[];
extern const char aBuildInfo_ConfigName[];
extern const char aBuildInfo_ComputerName[];
extern const char aBuildInfo_SVN_Revision[];

#endif	// REBOOT_UTILS_H
