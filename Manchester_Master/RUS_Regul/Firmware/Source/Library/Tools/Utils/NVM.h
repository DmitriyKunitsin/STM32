// NVM.h
#ifndef	NVM_H
#define	NVM_H
#include "ProjectConfig.h"		// конфиг платформы, конфиг задачи.
#include "stm32xxxx_hal.h"		// дрова периферии
#include <stdint.h>
#include <stdbool.h>
#if defined(STM32L4)
#include "eeprom_emul.h"
#endif // #if defined(STM32L4)
#if defined(STM32F4)
#include "eeprom.h"
#endif // #if defined(STM32F4)

// –езультат выполнения операций с NVM
typedef enum NVM_Result_enum
{
	NVM_Result_Ok			= 0,	// операция выполнена успешно
	NVM_Result_FailID		= 1,	// недопустимый идентификатор
	NVM_Result_FailData		= 2,	// недопустимый формат данных (размер, CRC, содержимое)
	NVM_Result_FailNVM		= 3,	// ошибка при доступе к EEPROM
} NVM_Result_t;

//  оллбек для верификации данных
struct NVM_Tag_struct;
typedef NVM_Result_t ( *NVM_Callback_t ) ( struct NVM_Tag_struct const *pTag, void *xData );

//  онстантная структура блока NVM, хранящаяся во Flash
typedef struct NVM_Tag_struct
{
	EEPROM_ID_t		ID;				// »дентификатор в драйвере EEPROM
	uint16_t		Size;			// –азмер поля
	void			*pLocation;		// –асположение данных в ќ«”
	void const		*pDefault;		// «начение по-умолчанию
	NVM_Callback_t	xValidationCB;	//  оллбек для верификации, в качестве аргумента - новый тег
	NVM_Callback_t	xResetCB;		//  оллбек перед обновлением, в качестве аргумента - новый тег, если старые данные еще не изменены, или NULL, если старые данные уже изменены
} NVM_Tag_t;

// ќписание функций
NVM_Result_t NVM_TagGetFromRAM( NVM_Tag_t const *pTag, void *pData );						// —читать тэг из RAM в указанный буфер
NVM_Result_t NVM_TagLoadReal( NVM_Tag_t const *pTag, void *pData );							// —читать тэг из NVM во временный буфер
NVM_Result_t NVM_TagLoad( NVM_Tag_t const *pTag );												// «агрузить тэг из NVM или константы
NVM_Result_t NVM_TagCompareRealAndRAM( NVM_Tag_t const *pTag );							// —равнить содержимое RAM и NVM
NVM_Result_t NVM_TagSave( NVM_Tag_t const *pTag, void *pWrittenData );							// —охранить из RAM в NVM
NVM_Result_t NVM_TagSaveNew( NVM_Tag_t const *pTag, void *pNewData, void *pWrittenData );		// ѕринять новое значение тега и сохранить в NVM

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

