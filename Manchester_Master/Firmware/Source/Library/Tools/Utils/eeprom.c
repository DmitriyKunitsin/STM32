/**
  ******************************************************************************
  * @file    EEPROM_Emulation/src/eeprom.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    10-October-2011
  * @brief   This file provides all the EEPROM emulation firmware functions.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */ 

/** @addtogroup EEPROM_Emulation
  * @{
  */ 

/* Includes ------------------------------------------------------------------*/
//#include "GGLP.h"
//#include "stm32f4xx_hal.h"			// дрова периферии, в т.ч. 
#include "ProjectConfig.h"			// конфиг платформы, конфиг задачи.
#include "stm32xxxx_hal.h"			// дрова периферии, в т.ч. 
#include "Platform_Common.h"		// FLASH_VOLTAGE_RANGE
#include "eeprom.h"
#include <string.h>
#include "FreeRTOS.h"
#include "Semphr.h"

// Перенесено из EEPROM.h
//extern uint8_t eep_MAS[2][0x4000];

/* Exported constants --------------------------------------------------------*/
/* Define the size of the sectors to be used */

// ДлЯ STM32F4xx под EEPROM по-умолчанию используютсЯ 2-Я и 3-Я страницы Flash размером по 16 КБ.
// Первые две страницы по 16 КБ используютсЯ по загрузчик, а последующие страницы большого размера используютсЯ под программу
#define PAGE_SIZE			( ( uint32_t ) 0x4000 )		// 16 КБ

/* Device voltage range supposed to be [2.7V to 3.6V], the operation will 
   be done by word  */
//#define VOLTAGE_RANGE           (uint8_t)VOLTAGE_RANGE_2
#define VOLTAGE_RANGE           FLASH_VOLTAGE_RANGE

/* EEPROM start address in Flash */
/*#ifdef DEBUG_MEM
#define EEPROM_START_ADDRESS  				((uint32_t)&eep_MAS[0])
#else
#define EEPROM_START_ADDRESS  				((uint32_t)0x08004000)
#endif
*/
#define EEPROM_START_ADDRESS  				((uint32_t)0x08004000)
											//		/* EEPROM emulation start address:
                                            //      from sector2 : after 16KByte of used 
                                            //      Flash memory */

/* Pages 0 and 1 base and end addresses */
#define PAGE0_BASE_ADDRESS    ((uint32_t)(EEPROM_START_ADDRESS + 0x0000))
#define PAGE0_END_ADDRESS     ((uint32_t)(PAGE0_BASE_ADDRESS + (PAGE_SIZE - 1)))
#ifdef DEBUG_MEM
#define PAGE0_ID               0
#else
#define PAGE0_ID               FLASH_SECTOR_1
#endif

#define PAGE1_BASE_ADDRESS    ((uint32_t)(EEPROM_START_ADDRESS + PAGE_SIZE))
#define PAGE1_END_ADDRESS     ((uint32_t)(PAGE1_BASE_ADDRESS + (PAGE_SIZE - 1)))
#ifdef DEBUG_MEM
#define PAGE1_ID               1
#else
#define PAGE1_ID               FLASH_SECTOR_2
#endif

/* Used Flash pages for EEPROM emulation */
#define PAGE0                 ((uint16_t)0x0000)
#define PAGE1                 ((uint16_t)0x0001)

/* No valid page define */
#define NO_VALID_PAGE         ((uint16_t)0x00AB)

/* Page status definitions */
#define ERASED                ((uint16_t)0xFFFF)     /* Page is empty */
#define RECEIVE_DATA          ((uint16_t)0xEEEE)     /* Page is marked to receive data */
#define VALID_PAGE            ((uint16_t)0x0000)     /* Page containing valid data */

/* Valid pages in read and write defines */
#define READ_FROM_VALID_PAGE  ((uint8_t)0x00)
#define WRITE_IN_VALID_PAGE   ((uint8_t)0x01)

/* Page full define */
#define PAGE_FULL             ((uint8_t)0x80)

/* Variables' number */
#define NB_OF_VAR             ((uint8_t)0x03)

/* Exported types ------------------------------------------------------------*/
typedef struct{
	uint16_t Start;			// Признак начала блока 0xABCD
	uint16_t Address;		// Виртуальный адрес блока
	uint16_t Size;			// Размер блока данных(в байтах)
}eep_Head_type;

typedef struct{
	uint16_t Start;			// Признак начала блока 0xABCD
	uint16_t Address;		// Виртуальный адрес блока
	uint16_t Size;			// Размер блока данных(в байтах)
	uint16_t Data;
}eep_HeadData_type;


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Global variable used to store variable value in read sequence */
eep_HeadData_type *DataVar;

uint16_t eep_Position = 0;

/*#ifdef	DEBUG_MEM
uint8_t eep_MAS[2][0x4000];
#endif	// DEBUG_MEM
*/

/* Virtual address defined by the user: 0xFFFF value is prohibited */

//uint16_t VirtVarTab[256];
uint16_t VirtVarTab[16];

// Семафор доступа к EEPROM
static SemaphoreHandle_t EEPROM_Lock = NULL;
#define	EEPROM_LOCK_TIMEOUT		( 1 * configTICK_RATE_HZ )


/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
static uint16_t EE_Format(void);
static uint16_t EE_FindValidPage(uint8_t Operation);
static uint16_t EE_VerifyPageFullWriteVariable(uint16_t VirtAddress, void *Data, uint16_t Size);
static uint16_t EE_VerifyPageFullWriteElement(uint16_t VirtAddress, eep_HeadData_type *Data);
static uint16_t EE_PageTransfer(uint16_t VirtAddress, void *Data, uint16_t Size);
uint16_t EE_ReadElement(uint16_t VirtAddress, eep_HeadData_type **Data);
uint16_t EE_ReadNextElement(uint16_t *Block_Pos, uint16_t *VirtAddr);


const uint8_t aucCRCHi[] = {
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81,
    0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
    0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1,
    0x81, 0x40, 0x01,
    0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
    0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81,
    0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80,
    0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01,
    0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00,
    0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81,
    0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
    0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1,
    0x81, 0x40, 0x01,
    0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01,
    0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81,
    0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
    0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01,
    0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
    0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81,
    0x40
};

const uint8_t aucCRCLo[] = {
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7,
    0x05, 0xC5, 0xC4,
    0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB,
    0x0B, 0xC9, 0x09,
    0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE,
    0xDF, 0x1F, 0xDD,
    0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2,
    0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32,
    0x36, 0xF6, 0xF7,
    0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E,
    0xFE, 0xFA, 0x3A,
    0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B,
    0x2A, 0xEA, 0xEE,
    0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27,
    0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1,
    0x63, 0xA3, 0xA2,
    0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD,
    0x6D, 0xAF, 0x6F,
    0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8,
    0xB9, 0x79, 0xBB,
    0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4,
    0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
    0x50, 0x90, 0x91,
    0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94,
    0x54, 0x9C, 0x5C,
    0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59,
    0x58, 0x98, 0x88,
    0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D,
    0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83,
    0x41, 0x81, 0x80,
    0x40
};
 

uint16_t CRC16( void * pucFrame, uint16_t usLen )
{
    uint8_t           ubCRCHi = 0xFF;
    uint8_t           ubCRCLo = 0xFF;
    uint8_t           ubIndex;
	uint8_t				*p = pucFrame;

    while( usLen-- )
    {
        ubIndex = ubCRCLo ^ *p++;
        ubCRCLo = ubCRCHi ^ ( uint8_t ) aucCRCHi[ubIndex];
        ubCRCHi = ( uint8_t ) aucCRCLo[ubIndex];
    }
    /* Additional casts a for PIC MCC18 compiler to fix a bug when -Oi is not used. 
     * This is required because it does not enforce ANSI c integer promotion
     * rules.
     */
    return ( uint16_t )( ( uint16_t )ubCRCHi << 8 | ( uint16_t )ubCRCLo );
}


uint8_t EE_Check_Erase(uint8_t PageNum)
{
uint32_t PageEnd,*Ptr;

	if(PageNum == 1)
	{
		Ptr = (uint32_t*)0x8004000;
		PageEnd = 0x8007FFF;
	}
	else if(PageNum == 2)
	{
		Ptr = (uint32_t*)0x8008000;
		PageEnd = 0x800BFFF;
	}
	else
		return 0;
	
	for(;(uint32_t)Ptr<PageEnd;Ptr++)
	{
		if(*Ptr != 0xFFFFFFFF)
			return 0;
	}

	return 1;
}

/**
  * @brief  Restore the pages to a known good state in case of page's status
  *   corruption after a power loss.
  * @param  None.
  * @retval - Flash error code: on write Flash error
  *         - HAL_OK: on success
  */

uint16_t EE_Init(void)
{
  uint16_t PageStatus0 = 6, PageStatus1 = 6;
  uint16_t VarIdx = 0;
  uint16_t EepromStatus = 0, ReadStatus = 0;
  uint16_t i,BlockPos=0;
  uint16_t  FlashStatus,VirtAddress;
  uint8_t flag;

  assert_param( NULL == EEPROM_Lock );

  HAL_FLASH_Unlock();
  HAL_NVIC_SetPriority(FLASH_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(FLASH_IRQn);
  __HAL_FLASH_ENABLE_IT(FLASH_IT_ERR);
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                  FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);

  /* Get Page0 status */
  PageStatus0 = (*(__IO uint16_t*)PAGE0_BASE_ADDRESS);
  /* Get Page1 status */
  PageStatus1 = (*(__IO uint16_t*)PAGE1_BASE_ADDRESS);
  
  eep_Position = 2;
  //Ищем конец активного блока
  while(EE_ReadNextElement((uint16_t*)&BlockPos,&VirtAddress) == 1);
  eep_Position = BlockPos + 2;

  
  
  /* Check for invalid header states and repair if necessary */
  switch (PageStatus0)
  {
    case ERASED:
      if (PageStatus1 == VALID_PAGE) /* Page0 erased, Page1 valid */
      {
		  /* Erase Page0 */
		  if(EE_Check_Erase(PAGE0_ID) == 0)
		  {
			  FLASH_Erase_Sector(PAGE0_ID,VOLTAGE_RANGE);
			  FLASH_WaitForLastOperation((uint32_t)50);
		  }
      }
      else if (PageStatus1 == RECEIVE_DATA) /* Page0 erased, Page1 receive */
      {
        /* Erase Page0 */
		if(EE_Check_Erase(PAGE0_ID) == 0)
		{
			FLASH_Erase_Sector(PAGE0_ID,VOLTAGE_RANGE);
        	FLASH_WaitForLastOperation((uint32_t)50);
		}
        /* Mark Page1 as valid */
        FlashStatus = HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,PAGE1_BASE_ADDRESS, VALID_PAGE);
        /* If program operation was failed, a Flash error code is returned */
        if (FlashStatus != HAL_OK)
        {
          return FlashStatus;
        }
      }
      else /* First EEPROM access (Page0&1 are erased) or invalid state -> format EEPROM */
      {
        /* Erase both Page0 and Page1 and set Page0 as valid page */
        FlashStatus = EE_Format();
        /* If erase/program operation was failed, a Flash error code is returned */
        if (FlashStatus != HAL_OK)
        {
          return FlashStatus;
        }
      }
      break;

    case RECEIVE_DATA:
      if (PageStatus1 == VALID_PAGE) /* Page0 receive, Page1 valid */
      {
		  BlockPos = 0;
		  VarIdx = 0;
		  flag = 1;
		  /* Transfer process: transfer variables from old to the new active page */
		  while(EE_ReadNextElement((uint16_t*)&BlockPos,&VirtAddress) == 1)
		  {
			for(i=0;i<VarIdx;i++)
				if(VirtVarTab[i] != VirtAddress)
				  flag = 1;
				else
				{
				   flag = 0;
				   break;
				}
			
			if(flag == 1)			//нашли новый адрес
			{
			  flag = 0;
			  VirtVarTab[VarIdx] = VirtAddress;
			  /* Read the other last variable updates */
			  ReadStatus = EE_ReadElement(VirtVarTab[VarIdx], &DataVar);
			  /* In case variable corresponding to the virtual address was found */
			  if(ReadStatus == 1)
			  {
				/* Transfer the variable to the new active page */
				EepromStatus = EE_VerifyPageFullWriteElement(VirtVarTab[VarIdx], DataVar);
				/* If program operation was failed, a Flash error code is returned */
				if (EepromStatus != HAL_OK)
				{
				  return EepromStatus;
				}
			  }
			  VarIdx++;
			 
			}
		  }
        /* Mark Page0 as valid */
        FlashStatus = HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,PAGE0_BASE_ADDRESS, VALID_PAGE);
        /* If program operation was failed, a Flash error code is returned */
        if (FlashStatus != HAL_OK)
        {
          return FlashStatus;
        }
		
		if(EE_Check_Erase(PAGE1_ID) == 0)
		{
			/* Erase Page1 */
			FLASH_Erase_Sector(PAGE1_ID,VOLTAGE_RANGE);
        	FLASH_WaitForLastOperation((uint32_t)50);
		}
      }
      else if (PageStatus1 == ERASED) /* Page0 receive, Page1 erased */
      {
		  if(EE_Check_Erase(PAGE1_ID) == 0)
		  {
			  /* Erase Page1 */
			  FLASH_Erase_Sector(PAGE1_ID,VOLTAGE_RANGE);
			  FLASH_WaitForLastOperation((uint32_t)50);
		  }
	  
        /* Mark Page0 as valid */
        FlashStatus = HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,PAGE0_BASE_ADDRESS, VALID_PAGE);
        /* If program operation was failed, a Flash error code is returned */
        if (FlashStatus != HAL_OK)
        {
          return FlashStatus;
        }
      }
      else /* Invalid state -> format eeprom */
      {
        /* Erase both Page0 and Page1 and set Page0 as valid page */
        FlashStatus = EE_Format();
        /* If erase/program operation was failed, a Flash error code is returned */
        if (FlashStatus != HAL_OK)
        {
          return FlashStatus;
        }
      }
      break;

    case VALID_PAGE:
      if (PageStatus1 == VALID_PAGE) /* Invalid state -> format eeprom */
      {
        /* Erase both Page0 and Page1 and set Page0 as valid page */
        FlashStatus = EE_Format();
        /* If erase/program operation was failed, a Flash error code is returned */
        if (FlashStatus != HAL_OK)
        {
          return FlashStatus;
        }
      }
      else if (PageStatus1 == ERASED) /* Page0 valid, Page1 erased */
      {
  /*    FLASH_EraseInitTypeDef EraseInitStruct;
	  uint32_t SectorError = 0;
        // Erase Page1 
		  EraseInitStruct.TypeErase = TYPEERASE_SECTORS;
		  EraseInitStruct.VoltageRange = VOLTAGE_RANGE_3;
		  EraseInitStruct.Sector = PAGE1_ID;
		  EraseInitStruct.NbSectors = 1;		
		  if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK)
		  { 

			Error_Handler();
		  }*/
		if(EE_Check_Erase(PAGE1_ID) == 0)
		{
			FLASH_Erase_Sector(PAGE1_ID,VOLTAGE_RANGE);
        	FLASH_WaitForLastOperation((uint32_t)50);
		}
      }
      else /* Page0 valid, Page1 receive */
      {
		  BlockPos = 0;
		  VarIdx = 0;
		  flag = 1;
		  /* Transfer process: transfer variables from old to the new active page */
		  while(EE_ReadNextElement((uint16_t*)&BlockPos,&VirtAddress) == 1)
		  {
			for(i=0;i<VarIdx;i++)
				if(VirtVarTab[i] != VirtAddress)
				  flag = 1;
				else
				{
				   flag = 0;
				   break;
				}
			
			if(flag == 1)			//нашли новый адрес
			{
			  flag = 0;
			  VirtVarTab[VarIdx] = VirtAddress;
			  /* Read the other last variable updates */
			  ReadStatus = EE_ReadElement(VirtVarTab[VarIdx], &DataVar);
			  /* In case variable corresponding to the virtual address was found */
			  if(ReadStatus == 1)
			  {
				/* Transfer the variable to the new active page */
				EepromStatus = EE_VerifyPageFullWriteElement(VirtVarTab[VarIdx], DataVar);
				/* If program operation was failed, a Flash error code is returned */
				if (EepromStatus != HAL_OK)
				{
				  return EepromStatus;
				}
			  }
			  VarIdx++;
			 
			}
		  }
        /* Mark Page1 as valid */
        FlashStatus = HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,PAGE1_BASE_ADDRESS, VALID_PAGE);
        /* If program operation was failed, a Flash error code is returned */
        if (FlashStatus != HAL_OK)
        {
          return FlashStatus;
        }
        /* Erase Page0 */
		if(EE_Check_Erase(PAGE0_ID) == 0)
		{
			FLASH_Erase_Sector(PAGE0_ID,VOLTAGE_RANGE);
        	FLASH_WaitForLastOperation((uint32_t)50);
		}
      }
      break;

    default:  /* Any other state -> format eeprom */
      /* Erase both Page0 and Page1 and set Page0 as valid page */
      FlashStatus = EE_Format();
      /* If erase/program operation was failed, a Flash error code is returned */
      if (FlashStatus != HAL_OK)
      {
        return FlashStatus;
      }
      break;
  }

  EEPROM_Lock = xSemaphoreCreateBinary( );
  assert_param( NULL != EEPROM_Lock );
  assert_param( pdTRUE == xSemaphoreGive( EEPROM_Lock ) );

  return HAL_OK;
}

typedef enum EE_Action_enum
{
	EE_ActionRead,
	EE_ActionCompare,
} EE_Action_t;

static uint16_t EE_ReadCompare(uint16_t VirtAddress, void *pData, uint16_t Size, EE_Action_t Action )
{
	static eep_HeadData_type *pHeader;
	assert_param( NULL != pData );
	assert_param( NULL != EEPROM_Lock );
	assert_param( pdTRUE == xSemaphoreTake( EEPROM_Lock, EEPROM_LOCK_TIMEOUT ) );
	uint16_t Status = EEPROM_READ_FAIL;
	do
	{
		if( 1 != EE_ReadElement( VirtAddress, &pHeader) )
			break;
		Status = EEPROM_READ_FAIL_SIZE;
		if( Size != pHeader->Size )
			break;
		switch( Action )
		{
		case EE_ActionRead:
			memcpy( pData, &pHeader->Data, pHeader->Size );
			Status = EEPROM_READ_OK;
			break;
		case EE_ActionCompare:
			Status = ( 0 == memcmp( pData, &pHeader->Data, pHeader->Size ) ) ? EEPROM_COMPARE_OK : EEPROM_COMPARE_FAIL;
			break;
		default:
			assert_param( 0 );
		}
	} while( 0 );
	xSemaphoreGive( EEPROM_Lock );
	return Status;
}

uint16_t EE_ReadVariable( uint16_t VirtAddress, void *pData, uint16_t Size )
{
	return EE_ReadCompare( VirtAddress, pData, Size, EE_ActionRead );
}

uint16_t EE_CompareVariable( uint16_t VirtAddress, void *pData, uint16_t Size )
{
	return EE_ReadCompare( VirtAddress, pData, Size, EE_ActionCompare );
}

/*
uint16_t EE_ReadVariable(uint16_t VirtAddress, void *Data, uint16_t Size)
{
 static eep_HeadData_type *tmpData;
 uint16_t ReadStatus;
 assert_param( NULL != EEPROM_Lock );
 assert_param( pdTRUE == xSemaphoreTake( EEPROM_Lock, EEPROM_LOCK_TIMEOUT ) );
	ReadStatus = EEPROM_READ_FAIL;
	if(EE_ReadElement(VirtAddress,&tmpData) == 1)
		if( tmpData->Size != Size )
			ReadStatus = EEPROM_READ_FAIL_SIZE;
		else
		{
			memcpy(Data,&tmpData->Data,tmpData->Size);
			ReadStatus = EEPROM_READ_OK;
		}

	xSemaphoreGive( EEPROM_Lock );
	return ReadStatus;
}

uint16_t EE_CompareVariable(uint16_t VirtAddress, void *Data, uint16_t Size)
{
 static eep_HeadData_type *tmpData;
 uint16_t Status;
 assert_param( NULL != EEPROM_Lock );
 assert_param( pdTRUE == xSemaphoreTake( EEPROM_Lock, EEPROM_LOCK_TIMEOUT ) );
	Status = EEPROM_COMPARE_FAIL;
	if(EE_ReadElement(VirtAddress,&tmpData) == 1)
		if( tmpData->Size == Size )
			if( 0 == memcmp(Data,&tmpData->Data,tmpData->Size) )
				Status = EEPROM_COMPARE_OK;

	xSemaphoreGive( EEPROM_Lock );
	return Status;
}*/

// Функция поиска валидной записи с указаного начального адреса
// uint16_t *Block_Pos - Начальный адрес(смещение с начала блока)  поиска
// uint16_t *VirtAddr - Виртуальный адрес найденого блока
uint16_t EE_ReadNextElement(uint16_t *Block_Pos, uint16_t *VirtAddr)
{
  uint16_t ValidPage = PAGE0;
  uint16_t AddressValue = 0x5555, ReadStatus = 0;
  uint32_t Address = EEPROM_START_ADDRESS, PageEndAddress = EEPROM_START_ADDRESS;
  eep_HeadData_type *eep_Head;
  
  /* Get active Page for read operation */
  ValidPage = EE_FindValidPage(READ_FROM_VALID_PAGE);

  /* Check if there is no valid page */
  if (ValidPage == NO_VALID_PAGE)
  {
    return  NO_VALID_PAGE;
  }

  /* Get the valid Page start Address */
  Address = (uint32_t)(EEPROM_START_ADDRESS + (uint32_t)(ValidPage * PAGE_SIZE) + 2 + *Block_Pos);

  /* Get the valid Page end Address */
  PageEndAddress = (uint32_t)((EEPROM_START_ADDRESS - sizeof(eep_HeadData_type))
  													 + (uint32_t)((1 + ValidPage) * PAGE_SIZE));

  /* Check each active page address starting from end */
  while (Address < (PageEndAddress))
  {
    /* Get the current location content to be compared with virtual address */
    AddressValue = (*(__IO uint16_t*)Address);
	// Находим признак начала записи
	if(AddressValue == 0xABCD)
	{
		eep_Head = (eep_HeadData_type *)Address;
		// 
	    if (CRC16(eep_Head,eep_Head->Size+sizeof(eep_Head_type) == *(__IO uint16_t*)(Address+sizeof(eep_Head_type)+eep_Head->Size)))
	    {
			*VirtAddr = eep_Head->Address;
		     /* In case variable value is read, reset ReadStatus flag */
		     ReadStatus = 1;
			 /* Next address location (%2 учёт выравнивания 2байта)*/
			 *Block_Pos += eep_Head->Size + eep_Head->Size%2 + sizeof(eep_HeadData_type);
			 break;
	    }
	}
	else
	{
		if((*(__IO uint32_t*)Address) == 0xFFFFFFFF)
        {
        	// !!! зачем это?
            *VirtAddr = Address - EEPROM_START_ADDRESS + ValidPage * PAGE_SIZE;
			// !!! 19.04.2017 добавил после сбоЯ, когда в конце области попалсЯ битый блок, который блочил дальнейшую работу
			*Block_Pos = Address - EEPROM_START_ADDRESS - ValidPage * PAGE_SIZE;
			// !!!
			break;
        }
		else
			Address += 2;
	}
  }
  /* Return ReadStatus value: (0: variable exist, 1: variable doesn't exist) */
  return ReadStatus;
}

/**
  * @brief  Returns the last stored variable data, if found, which correspond to
  *   the passed virtual address
  * @param  VirtAddress: Variable virtual address
  * @param  Data: Global variable contains the read variable value
  * @retval Success or error status:
  *           - 0: if variable was found
  *           - 1: if the variable was not found
  *           - NO_VALID_PAGE: if no valid page was found.
  */

// Чтение элемента расположеного по вирт. адресу
// Функция просматривает все записи на странице с указаным адресом и возвращает
// указатель на последний элемент.
//
// uint16_t VirtAddress - виртуальный адрес элемента
// eep_HeadData_type **Data - Возвращаемый указатель на начало найденого элемента
uint16_t EE_ReadElement(uint16_t VirtAddress, eep_HeadData_type **Data)
{
  uint16_t ValidPage = PAGE0;
  uint16_t AddressValue = 0x5555, ReadStatus = 0;
  uint32_t Address = EEPROM_START_ADDRESS, PageEndAddress = EEPROM_START_ADDRESS;
  eep_HeadData_type *eep_Head;
  
  *Data = (eep_HeadData_type *)0;

  /* Get active Page for read operation */
  ValidPage = EE_FindValidPage(READ_FROM_VALID_PAGE);

  /* Check if there is no valid page */
  if (ValidPage == NO_VALID_PAGE)
  {
    return  NO_VALID_PAGE;
  }

  /* Get the valid Page start Address */
  Address = (uint32_t)(EEPROM_START_ADDRESS + (uint32_t)(ValidPage * PAGE_SIZE) + 2);

  /* Get the valid Page end Address */
  PageEndAddress = (uint32_t)((EEPROM_START_ADDRESS - sizeof(eep_HeadData_type))
  													 + (uint32_t)((1 + ValidPage) * PAGE_SIZE));

  /* Check each active page address starting from end */
  while (Address < (PageEndAddress))
  {
    /* Get the current location content to be compared with virtual address */
    AddressValue = (*(__IO uint16_t*)Address);
	if(AddressValue == 0xABCD)
	{
		eep_Head = (eep_HeadData_type *)Address;
	    /* Compare the read address with the virtual address */
	    if ((eep_Head->Address == VirtAddress) &&
			( CRC16(eep_Head,eep_Head->Size+sizeof(eep_Head_type) == *(__IO uint16_t*)(Address+sizeof(eep_Head_type)+eep_Head->Size))))
	    {
		      *Data = (eep_HeadData_type*)Address;

		      /* In case variable value is read, reset ReadStatus flag */
		      ReadStatus = 1;
	    }
	    /* Next address location */
	    Address += eep_Head->Size + eep_Head->Size%2 + sizeof(eep_HeadData_type);
	}
	else
	{
		if((*(__IO uint32_t*)Address) == 0xFFFFFFFF)
			break;
		else
			Address += 2;
	}
  }
  /* Return ReadStatus value: (0: variable exist, 1: variable doesn't exist) */
  return ReadStatus;
}

/**
  * @brief  Writes/upadtes variable data in EEPROM.
  * @param  VirtAddress: Variable virtual address
  * @param  Data: 16 bit data to be written
  * @retval Success or error status:
  *           - HAL_OK: on success
  *           - PAGE_FULL: if valid page is full
  *           - NO_VALID_PAGE: if no valid page was found
  *           - Flash error code: on write Flash error
  */
uint16_t EE_WriteVariable(uint16_t VirtAddress, void *Data, uint16_t Size)
{
  uint16_t Status = 0;
//  vTracePrintF(xTraceOpenLabel("EEPROM"), "WR: Adr=%d Size=%d", VirtAddress,Size);

  assert_param( NULL != EEPROM_Lock );
  assert_param( pdTRUE == xSemaphoreTake( EEPROM_Lock, EEPROM_LOCK_TIMEOUT ) );

  /* Write the variable virtual address and value in the EEPROM */
  Status = EE_VerifyPageFullWriteVariable(VirtAddress, Data,Size);

  /* In case the EEPROM active page is full */
  if (Status == PAGE_FULL)
  {
    /* Perform Page transfer */
    Status = EE_PageTransfer(VirtAddress, Data, Size);
//	vTracePrintF(xTraceOpenLabel("EEPROM"), "PageFull", VirtAddress,Size);
//	AddLogMessage("EEP_PAGE_FULL");
  }

  xSemaphoreGive( EEPROM_Lock );
  /* Return last operation status */
  return Status;
}




/**
  * @brief  Erases PAGE and PAGE1 and writes VALID_PAGE header to PAGE
  * @param  None
  * @retval Status of the last operation (Flash write or erase) done during
  *         EEPROM formating
  */
static uint16_t EE_Format(void)
{
  uint16_t FlashStatus = HAL_OK;
  /* Erase Page0 */
  FLASH_Erase_Sector(PAGE0_ID,VOLTAGE_RANGE);

  /* Set Page0 as valid page: Write VALID_PAGE at Page0 base address */
  FlashStatus = HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,PAGE0_BASE_ADDRESS, VALID_PAGE);

  /* If program operation was failed, a Flash error code is returned */
  if (FlashStatus != HAL_OK)
  {
    return FlashStatus;
  }

  /* Erase Page1 */
  FLASH_Erase_Sector(PAGE1_ID,VOLTAGE_RANGE);

  /* Return Page1 erase operation status */
  return FlashStatus;
}

/**
  * @brief  Find valid Page for write or read operation
  * @param  Operation: operation to achieve on the valid page.
  *   This parameter can be one of the following values:
  *     @arg READ_FROM_VALID_PAGE: read operation from valid page
  *     @arg WRITE_IN_VALID_PAGE: write operation from valid page
  * @retval Valid page number (PAGE or PAGE1) or NO_VALID_PAGE in case
  *   of no valid page was found
  */
static uint16_t EE_FindValidPage(uint8_t Operation)
{
  uint16_t PageStatus0 = 6, PageStatus1 = 6;

  /* Get Page0 actual status */
  PageStatus0 = (*(__IO uint16_t*)PAGE0_BASE_ADDRESS);

  /* Get Page1 actual status */
  PageStatus1 = (*(__IO uint16_t*)PAGE1_BASE_ADDRESS);

  /* Write or read operation */
  switch (Operation)
  {
    case WRITE_IN_VALID_PAGE:   /* ---- Write operation ---- */
      if (PageStatus1 == VALID_PAGE)
      {
        /* Page0 receiving data */
        if (PageStatus0 == RECEIVE_DATA)
        {
          return PAGE0;         /* Page0 valid */
        }
        else
        {
          return PAGE1;         /* Page1 valid */
        }
      }
      else if (PageStatus0 == VALID_PAGE)
      {
        /* Page1 receiving data */
        if (PageStatus1 == RECEIVE_DATA)
        {
          return PAGE1;         /* Page1 valid */
        }
        else
        {
          return PAGE0;         /* Page0 valid */
        }
      }
      else
      {
        return NO_VALID_PAGE;   /* No valid Page */
      }

    case READ_FROM_VALID_PAGE:  /* ---- Read operation ---- */
      if (PageStatus0 == VALID_PAGE)
      {
        return PAGE0;           /* Page0 valid */
      }
      else if (PageStatus1 == VALID_PAGE)
      {
        return PAGE1;           /* Page1 valid */
      }
      else
      {
        return NO_VALID_PAGE ;  /* No valid Page */
      }

    default:
      return PAGE0;             /* Page0 valid */
  }
}

/**
  * @brief  Verify if active page is full and Writes variable in EEPROM.
  * @param  VirtAddress: 16 bit virtual address of the variable
  * @param  Data: 16 bit data to be written as variable value
  * @retval Success or error status:
  *           - HAL_OK: on success
  *           - PAGE_FULL: if valid page is full
  *           - NO_VALID_PAGE: if no valid page was found
  *           - Flash error code: on write Flash error
  */
static uint16_t EE_VerifyPageFullWriteVariable(uint16_t VirtAddress, void *Data, uint16_t Size)
{
  uint16_t FlashStatus = HAL_OK;
  uint16_t ValidPage = PAGE0;
  uint32_t Address = EEPROM_START_ADDRESS,AddressTmp;
  eep_Head_type eep_Head;
  	
	eep_Head.Start = 0xABCD;
	eep_Head.Address = VirtAddress;
	eep_Head.Size = Size;
	
  /* Get valid Page for write operation */
  ValidPage = EE_FindValidPage(WRITE_IN_VALID_PAGE);

  /* Check if there is no valid page */
  if (ValidPage == NO_VALID_PAGE)
  {
    return  NO_VALID_PAGE;
  }

  /* Get the valid Page start Address */
  Address = (uint32_t)(EEPROM_START_ADDRESS + (uint32_t)(ValidPage * PAGE_SIZE) + eep_Position);
  AddressTmp = Address;


	if((eep_Position + Size + sizeof(eep_HeadData_type)) < PAGE_SIZE)
	{
		for(uint8_t i=0; i < sizeof(eep_Head_type)/2; Address+=2,eep_Position+=2, i++)
		{
			/* Set variable data */
			FlashStatus = HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,Address, ((uint16_t*)&eep_Head)[i]);
			
			/* If program operation was failed, a Flash error code is returned */
			if (FlashStatus != HAL_OK)
			{
			  return FlashStatus;
			}
		}

		for(uint16_t i=0; i < (Size+1)/2; Address+=2,eep_Position+=2, i++)
		{
			/* Set variable data */
			FlashStatus = HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,Address, ((uint16_t*)Data)[i]);
//			(*(uint16_t*)Data)++;
			
			/* If program operation was failed, a Flash error code is returned */
			if (FlashStatus != HAL_OK)
			{
			  return FlashStatus;
			}
		}
		
		FlashStatus = HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,Address, CRC16((void*)AddressTmp,Size+sizeof(eep_Head_type)));
		eep_Position+=2;
		
		return FlashStatus;
	}
	else
		return PAGE_FULL;
/*
  // Check each active page address starting from begining *
  while (Address < PageEndAddress)
  {
    // Verify if Address and Address+2 contents are 0xFFFFFFFF *
    if ((*(__IO uint32_t*)Address) == 0xFFFFFFFF)
    {
      // Set variable data *
      FlashStatus = HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,Address, Data);
      // If program operation was failed, a Flash error code is returned *
      if (FlashStatus != HAL_OK)
      {
        return FlashStatus;
      }
      // Set variable virtual address *
      FlashStatus = HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,Address + 2, VirtAddress);
      // Return program operation status *
      return FlashStatus;
    }
    else
    {
      // Next address location *
      Address = Address + 4;
    }
  }

  // Return PAGE_FULL in case the valid page is full *
  return PAGE_FULL;*/
}

// Запись элемента с указанным адресом в конец активной страницы
//
// uint16_t VirtAddress - виртуальный адрес элемента
// eep_HeadData_type *Data - указатель на начало записываемого элемента
static uint16_t EE_VerifyPageFullWriteElement(uint16_t VirtAddress, eep_HeadData_type *Data)
{
  uint16_t FlashStatus = HAL_OK;
  uint16_t ValidPage = PAGE0,Size;
  uint32_t Address = EEPROM_START_ADDRESS;
  
	Size = Data->Size + sizeof(eep_HeadData_type);
  /* Get valid Page for write operation */
  ValidPage = EE_FindValidPage(WRITE_IN_VALID_PAGE);

  /* Check if there is no valid page */
  if (ValidPage == NO_VALID_PAGE)
  {
    return  NO_VALID_PAGE;
  }

  /* Get the valid Page start Address */
  Address = (uint32_t)(EEPROM_START_ADDRESS + (uint32_t)(ValidPage * PAGE_SIZE) + eep_Position);


	if((eep_Position + Size) < PAGE_SIZE)
	{
		for(uint8_t i=0; i < Size/2; Address+=2,eep_Position+=2, i++)
		{
			/* Set variable data */
			FlashStatus = HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,Address, ((uint16_t*)Data)[i]);
			
			/* If program operation was failed, a Flash error code is returned */
			if (FlashStatus != HAL_OK)
			{
			  return FlashStatus;
			}
		}
		
	
		return FlashStatus;
	}
	else
		return PAGE_FULL;
}

//Принудительное обновление страницы EEPROM
uint16_t EE_Refresh( void )
{
	uint16_t  VirtAddress;
	uint16_t FlashStatus = HAL_OK;
	uint32_t NewPageAddress = EEPROM_START_ADDRESS;
	uint16_t OldPageId=0,BlockPos,i;
	uint16_t ValidPage = PAGE0, VarIdx = 0;
	uint16_t EepromStatus = 0, ReadStatus = 0;
	uint8_t flag;
	
	assert_param( NULL != EEPROM_Lock );
	assert_param( pdTRUE == xSemaphoreTake( EEPROM_Lock, EEPROM_LOCK_TIMEOUT ) );

	if(eep_Position < 300)
	{
		return 1;
	}

	eep_Position = 2;
	ValidPage = EE_FindValidPage(READ_FROM_VALID_PAGE);
	
	if (ValidPage == PAGE1) 	  /* Page1 valid */
	{
	  /* New page address where variable will be moved to */
	  NewPageAddress = PAGE0_BASE_ADDRESS;
	
	  /* Old page ID where variable will be taken from */
	  OldPageId = PAGE1_ID;
	}
	else if (ValidPage == PAGE0)  /* Page0 valid */
	{
	  /* New page address  where variable will be moved to */
	  NewPageAddress = PAGE1_BASE_ADDRESS;
	
	  /* Old page ID where variable will be taken from */
	  OldPageId = PAGE0_ID;
	}
	else
	{
	  return NO_VALID_PAGE; 	  /* No valid Page */
	}
	
	/* Set the new Page status to RECEIVE_DATA status */
	FlashStatus = HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,NewPageAddress, RECEIVE_DATA);
	/* If program operation was failed, a Flash error code is returned */
	if (FlashStatus != HAL_OK)
	{
	  return FlashStatus;
	}


	VarIdx = 0;
	flag = 1;
	BlockPos = 0;
	/* Transfer process: transfer variables from old to the new active page */
	while(EE_ReadNextElement((uint16_t*)&BlockPos,&VirtAddress) == 1)
	{
	  for(i=0;i < VarIdx;i++)
	   if(VirtVarTab[i] != VirtAddress)
		 flag = 1;
	   else
	   {
		 flag = 0;
		 break;
	   }
	   
	  if(flag == 1)
	  {
        flag = 0;
		VirtVarTab[VarIdx] = VirtAddress;
		/* Read the other last variable updates */
		ReadStatus = EE_ReadElement(VirtVarTab[VarIdx], &DataVar);
		/* In case variable corresponding to the virtual address was found */
		if(ReadStatus == 1)
		{
		  /* Transfer the variable to the new active page */
		  EepromStatus = EE_VerifyPageFullWriteElement(VirtVarTab[VarIdx], DataVar);
		  /* If program operation was failed, a Flash error code is returned */
		  if (EepromStatus != HAL_OK)
		  {
			return EepromStatus;
		  }
		}
		VarIdx++;
	   
	  }
	}
	
	/* Erase the old Page: Set old Page status to ERASED status */
	FLASH_Erase_Sector(OldPageId, VOLTAGE_RANGE);
	
	/* Set new Page status to VALID_PAGE status */
	FlashStatus = HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,NewPageAddress, VALID_PAGE);
	/* If program operation was failed, a Flash error code is returned */
	if (FlashStatus != HAL_OK)
	{
	  return FlashStatus;
	}
	
	xSemaphoreGive( EEPROM_Lock );
	/* Return last operation flash status */
	return FlashStatus;
}

/**
  * @brief  Transfers last updated variables data from the full Page to
  *   an empty one.
  * @param  VirtAddress: 16 bit virtual address of the variable
  * @param  Data: 16 bit data to be written as variable value
  * @retval Success or error status:
  *           - HAL_OK: on success
  *           - PAGE_FULL: if valid page is full
  *           - NO_VALID_PAGE: if no valid page was found
  *           - Flash error code: on write Flash error
  */
static uint16_t EE_PageTransfer(uint16_t VirtAddress, void *Data, uint16_t Size)
{
  uint16_t FlashStatus = HAL_OK;
  uint32_t NewPageAddress = EEPROM_START_ADDRESS;
  uint16_t OldPageId=0,BlockPos,i;
  uint16_t ValidPage = PAGE0, VarIdx = 0;
  uint16_t EepromStatus = 0, ReadStatus = 0;
  uint8_t flag;
  eep_Position = 2;

  /* Get active Page for read operation */
  ValidPage = EE_FindValidPage(READ_FROM_VALID_PAGE);

  if (ValidPage == PAGE1)       /* Page1 valid */
  {
    /* New page address where variable will be moved to */
    NewPageAddress = PAGE0_BASE_ADDRESS;

    /* Old page ID where variable will be taken from */
    OldPageId = PAGE1_ID;
  }
  else if (ValidPage == PAGE0)  /* Page0 valid */
  {
    /* New page address  where variable will be moved to */
    NewPageAddress = PAGE1_BASE_ADDRESS;

    /* Old page ID where variable will be taken from */
    OldPageId = PAGE0_ID;
  }
  else
  {
    return NO_VALID_PAGE;       /* No valid Page */
  }

  /* Set the new Page status to RECEIVE_DATA status */
  FlashStatus = HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,NewPageAddress, RECEIVE_DATA);
  /* If program operation was failed, a Flash error code is returned */
  if (FlashStatus != HAL_OK)
  {
    return FlashStatus;
  }

  /* Write the variable passed as parameter in the new active page */
  EepromStatus = EE_VerifyPageFullWriteVariable(VirtAddress, Data, Size);
  VarIdx = 1;
  VirtVarTab[0] = VirtAddress;
  /* If program operation was failed, a Flash error code is returned */
  if (EepromStatus != HAL_OK)
  {
    return EepromStatus;
  }
  
  BlockPos = 0;
  /* Transfer process: transfer variables from old to the new active page */
  while(EE_ReadNextElement((uint16_t*)&BlockPos,&VirtAddress) == 1)
  {
  	for(flag=0,i=0;i<VarIdx;i++)
	 if(VirtVarTab[i] != VirtAddress)
       flag = 1;
     else
     {
       flag = 0;
       break;
     }
     
    if(flag == 1)
    {
	  VirtVarTab[VarIdx] = VirtAddress;
      /* Read the other last variable updates */
      ReadStatus = EE_ReadElement(VirtVarTab[VarIdx], &DataVar);
      /* In case variable corresponding to the virtual address was found */
      if(ReadStatus == 1)
      {
        /* Transfer the variable to the new active page */
        EepromStatus = EE_VerifyPageFullWriteElement(VirtVarTab[VarIdx], DataVar);
        /* If program operation was failed, a Flash error code is returned */
        if (EepromStatus != HAL_OK)
        {
          return EepromStatus;
        }
      }
	  VarIdx++;
     
    }
  }

  /* Erase the old Page: Set old Page status to ERASED status */
  FLASH_Erase_Sector(OldPageId, VOLTAGE_RANGE);

  /* Set new Page status to VALID_PAGE status */
  FlashStatus = HAL_FLASH_Program(TYPEPROGRAM_HALFWORD,NewPageAddress, VALID_PAGE);
  /* If program operation was failed, a Flash error code is returned */
  if (FlashStatus != HAL_OK)
  {
    return FlashStatus;
  }

  /* Return last operation flash status */
  return FlashStatus;
}

#if 0
uint16_t EE_Search_END_Block( void )
{
 static uint16_t BlockPos = 0,VirtAddress;
 eep_HeadData_type *eep_Head;
 assert_param( NULL != EEPROM_Lock );
 assert_param( pdTRUE == xSemaphoreTake( EEPROM_Lock, EEPROM_LOCK_TIMEOUT ) );

	/* Transfer process: transfer variables from old to the new active page */
	while(EE_ReadNextElement((uint16_t*)&BlockPos,&VirtAddress) == 1);
	eep_Head = (eep_HeadData_type *)BlockPos;
	BlockPos += eep_Head->Size+sizeof(eep_Head_type);
	xSemaphoreGive( EEPROM_Lock );
	return BlockPos;
}
#endif

/**
  * @}
  */ 

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
