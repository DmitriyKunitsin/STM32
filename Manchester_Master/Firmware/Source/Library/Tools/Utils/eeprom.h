//#error "!!!"
/**
  ******************************************************************************
  * @file    EEPROM_Emulation/inc/eeprom.h 
  * @author  MCD Application Team
   * @version V1.0.0
  * @date    10-October-2011
  * @brief   This file contains all the functions prototypes for the EEPROM 
  *          emulation firmware library.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __EEPROM_H
#define __EEPROM_H

/* Includes ------------------------------------------------------------------*/
//#include "stm32f4xx.h"
//#include "stm32f4xx_hal_flash.h"
//#include <string.h>
//#include "GGLP.h"
#include <stdint.h>

#if 0
extern uint8_t eep_MAS[2][0x4000];

/* Exported constants --------------------------------------------------------*/
/* Define the size of the sectors to be used */
#define PAGE_SIZE               (uint32_t)0x4000  /* Page size = 16KByte */

/* Device voltage range supposed to be [2.7V to 3.6V], the operation will 
   be done by word  */
//#define VOLTAGE_RANGE           (uint8_t)VOLTAGE_RANGE_2
#define VOLTAGE_RANGE           FLASH_VOLTAGE_RANGE

/* EEPROM start address in Flash */
#ifdef DEBUG_MEM
#define EEPROM_START_ADDRESS  				((uint32_t)&eep_MAS[0])
#else
#define EEPROM_START_ADDRESS  				((uint32_t)0x08004000)
#endif
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
#endif


#define	EEPROM_WRITE_OK				HAL_OK
#define	EEPROM_READ_OK				1
#define	EEPROM_READ_FAIL			0
#define	EEPROM_READ_FAIL_SIZE		-1
#define	EEPROM_COMPARE_OK			EEPROM_READ_OK
#define	EEPROM_COMPARE_FAIL			EEPROM_READ_FAIL

#define	EE_TAG( __ADDRESS__, __DATA__ )		( __ADDRESS__ ), &( __DATA__ ), sizeof( __DATA__ )

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
uint16_t EE_Init(void);
uint16_t EE_ReadVariable(uint16_t VirtAddress, void *Data, uint16_t Size);
uint16_t EE_CompareVariable(uint16_t VirtAddress, void *Data, uint16_t Size);
uint16_t EE_WriteVariable(uint16_t VirtAddress, void *Data, uint16_t Size);
uint16_t EE_Search_END_Block( void );
uint16_t EE_Refresh( void );


#endif /* __EEPROM_H */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
