// NVM.h
#ifndef	NVM_H
#define	NVM_H
#include "ProjectConfig.h"		// ������ ���������, ������ ������.
#include "stm32xxxx_hal.h"		// ����� ���������
#include <stdint.h>
#include <stdbool.h>
#if defined(STM32L4)
#include "eeprom_emul.h"
#endif // #if defined(STM32L4)
#if defined(STM32F4)
#include "eeprom.h"
#endif // #if defined(STM32F4)

// ��������� ���������� �������� � NVM
typedef enum NVM_Result_enum
{
	NVM_Result_Ok			= 0,	// �������� ��������� �������
	NVM_Result_FailID		= 1,	// ������������ �������������
	NVM_Result_FailData		= 2,	// ������������ ������ ������ (������, CRC, ����������)
	NVM_Result_FailNVM		= 3,	// ������ ��� ������� � EEPROM
} NVM_Result_t;

// ������� ��� ����������� ������
struct NVM_Tag_struct;
typedef NVM_Result_t ( *NVM_Callback_t ) ( struct NVM_Tag_struct const *pTag, void *xData );

// ����������� ��������� ����� NVM, ���������� �� Flash
typedef struct NVM_Tag_struct
{
	EEPROM_ID_t		ID;				// ������������� � �������� EEPROM
	uint16_t		Size;			// ������ ����
	void			*pLocation;		// ������������ ������ � ���
	void const		*pDefault;		// �������� ��-���������
	NVM_Callback_t	xValidationCB;	// ������� ��� �����������, � �������� ��������� - ����� ���
	NVM_Callback_t	xResetCB;		// ������� ����� �����������, � �������� ��������� - ����� ���, ���� ������ ������ ��� �� ��������, ��� NULL, ���� ������ ������ ��� ��������
} NVM_Tag_t;

// �������� �������
NVM_Result_t NVM_TagGetFromRAM( NVM_Tag_t const *pTag, void *pData );						// ������� ��� �� RAM � ��������� �����
NVM_Result_t NVM_TagLoadReal( NVM_Tag_t const *pTag, void *pData );							// ������� ��� �� NVM �� ��������� �����
NVM_Result_t NVM_TagLoad( NVM_Tag_t const *pTag );												// ��������� ��� �� NVM ��� ���������
NVM_Result_t NVM_TagCompareRealAndRAM( NVM_Tag_t const *pTag );							// �������� ���������� RAM � NVM
NVM_Result_t NVM_TagSave( NVM_Tag_t const *pTag, void *pWrittenData );							// ��������� �� RAM � NVM
NVM_Result_t NVM_TagSaveNew( NVM_Tag_t const *pTag, void *pNewData, void *pWrittenData );		// ������� ����� �������� ���� � ��������� � NVM

bool EEPROM_WriteCheck( uint16_t EEPROM_Address, void *pBuffer, uint16_t BufferSize, void *pBufferResult );

#if defined (STM32L4)
uint16_t EE_ReadVariable( uint16_t VirtAddress, void *pData, uint16_t Size );
uint16_t EE_WriteVariable( uint16_t VirtAddress, void *pData, uint16_t Size );
#endif // #if defined (STM32L4)


typedef struct NVM_State_struct
{
	uint8_t EEPROM_Ready		:1;
	uint8_t EEPROM_TempLock1	:1;
	uint8_t EEPROM_TempLock2	:1;
	uint8_t SD_Ready			:1;
	uint8_t SD_TempLock1		:1;
	uint8_t SD_TempLock2		:1;
	uint8_t SD_PwrLock1			:1;
	uint8_t SD_PwrLock2			:1;
} NVM_State_t;

extern volatile NVM_State_t NVM_State;

#endif	// NVM_H

