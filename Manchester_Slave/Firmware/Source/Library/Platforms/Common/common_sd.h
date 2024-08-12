// common_sd.h
// јдаптация файла en.stm32cubef4.zip\STM32Cube_FW_F4_V1.15.0\Drivers\BSP\STM324xG_EVAL\stm324xg_eval_sd.c
/**
  ******************************************************************************
  * @file    stm324xg_eval_sd.h
  * @author  MCD Application Team
  * @version V3.0.0
  * @date    27-January-2017
  * @brief   This file contains the common defines and functions prototypes for
  *          the stm324xg_eval_sd.c driver.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM324xG_EVAL_SD_H
#define __STM324xG_EVAL_SD_H

#ifdef __cplusplus
 extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
#include "stm32xxxx_hal.h"

/** @addtogroup BSP
  * @{
  */ 

/** @addtogroup STM324xG_EVAL
  * @{
  */
    
/** @addtogroup STM324xG_EVAL_SD
  * @{
  */    

/** @defgroup STM324xG_EVAL_SD_Exported_Types STM324xG EVAL SD Exported Types
  * @{
  */

/** 
  * @brief SD Card information structure 
  */   
#define BSP_SD_CardInfo HAL_SD_CardInfoTypeDef
/**
  * @}
  */
   

/** @defgroup STM324xG_EVAL_SD_Exported_Constants STM324xG EVAL SD Exported Constants
  * @{
  */
/** 
  * @brief  SD status structure definition  
  */     
#define   MSD_OK                        ((uint8_t)0x00)
#define   MSD_ERROR                     ((uint8_t)0x01)

/** 
  * @brief  SD transfer state definition  
  */     
#define   SD_TRANSFER_OK                ((uint8_t)0x00)
#define   SD_TRANSFER_BUSY              ((uint8_t)0x01)
   
#if	0
// ќбъявлено в ProjectConfig.h
#define SD_DETECT_PIN                    GPIO_PIN_13
#define SD_DETECT_GPIO_PORT              GPIOH
#define __SD_DETECT_GPIO_CLK_ENABLE()    __HAL_RCC_GPIOH_CLK_ENABLE()
#define SD_DETECT_IRQn                   EXTI15_10_IRQn
#endif

//#define SD_DATATIMEOUT           ((uint32_t)100000000)

#define SD_PRESENT               ((uint8_t)0x01)
#define SD_NOT_PRESENT           ((uint8_t)0x00)

#define	SD_RECORD_SIZE			( 32 * 1024 )		// –азмер минимальной стираемой области в SD дл€ выравнивани€ расположени€ области данных FAT при форматировании
   
/* DMA definitions for SD DMA transfer */
#if	0
// ќбъявлено в ProjectConfig.h
#define __DMAx_TxRx_CLK_ENABLE            __HAL_RCC_DMA2_CLK_ENABLE
#define SD_DMAx_Tx_CHANNEL                DMA_CHANNEL_4
#define SD_DMAx_Rx_CHANNEL                DMA_CHANNEL_4
#define SD_DMAx_Tx_STREAM                 DMA2_Stream6  
#define SD_DMAx_Rx_STREAM                 DMA2_Stream3  
#define SD_DMAx_Tx_IRQn                   DMA2_Stream6_IRQn
#define SD_DMAx_Rx_IRQn                   DMA2_Stream3_IRQn
#define BSP_SD_IRQHandler                 SDIO_IRQHandler
#define BSP_SD_DMA_Tx_IRQHandler          DMA2_Stream6_IRQHandler
#define BSP_SD_DMA_Rx_IRQHandler          DMA2_Stream3_IRQHandler
#define SD_DetectIRQHandler()             HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13)
#endif

/**
  * @}
  */
  
/** @defgroup STM324xG_EVAL_SD_Exported_Functions STM324xG EVAL SD Exported Functions
  * @{
  */  
uint8_t BSP_SD_Init(void);
uint8_t BSP_SD_ITConfig(void);
void    BSP_SD_DetectIT(void);
void    BSP_SD_DetectCallback(void);
uint8_t BSP_SD_ReadBlocks(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks, uint32_t Timeout);
uint8_t BSP_SD_WriteBlocks(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks, uint32_t Timeout);
uint8_t BSP_SD_ReadBlocks_DMA(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks);
uint8_t BSP_SD_WriteBlocks_DMA(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks);
uint8_t BSP_SD_Erase(uint32_t StartAddr, uint32_t EndAddr);
uint8_t BSP_SD_Abort( void );
uint8_t BSP_SD_GetCardState(void);
//HAL_SD_CardStateTypeDef BSP_SD_GetCardState(void);
void    BSP_SD_GetCardInfo(HAL_SD_CardInfoTypeDef *CardInfo);
uint8_t BSP_SD_IsDetected(void);

/* These functions can be modified in case the current settings (e.g. DMA stream)
   need to be changed for specific application needs */
void    BSP_SD_MspInit(SD_HandleTypeDef *hsd, void *Params);
void    BSP_SD_Detect_MspInit(SD_HandleTypeDef *hsd, void *Params);
void    BSP_SD_MspDeInit(SD_HandleTypeDef *hsd, void *Params);
void    BSP_SD_AbortCallback(void);
void    BSP_SD_WriteCpltCallback(void);
void    BSP_SD_ReadCpltCallback(void); 

HAL_StatusTypeDef SD_DMAConfigRx(SD_HandleTypeDef *hsd);
HAL_StatusTypeDef SD_DMAConfigTx(SD_HandleTypeDef *hsd);


// —труктура SD state, альтернатива HAL_SD_CardStatusTypeDef, но на битовых пол€х
#pragma pack(1)
typedef struct BSP_SD_CardStatus_struct
{	// все пол€ объ€влены uint16_t, т.к. при совмещении uint16_t и uint8_t увеличивалс€ размер структуры на 1 байт
	uint16_t	Reserved5[19];						/*!< Reserved 					  								*/
	uint16_t	Reserved4					:8;		/*!< Reserved 					  								*/
	uint16_t	Reserved3[5];						/*!< Reserved 					  								*/
	uint16_t	UHS_AU_GRADE				:4;		/*!< Size of AU for UHS mode									*/
	uint16_t	UHS_SPEED_GRADE				:4;		/*!< Speed Grade for UHS mode									*/
	uint16_t	ERASE_OFFSET				:2;		/*!< Carries information about the erase offset					*/
	uint16_t	ERASE_TIMEOUT				:6;		/*!< Determines the timeout for any number of AU erase			*/
	uint16_t	ERASE_SIZE					:16;	/*!< Determines the number of AUs to be erased in one operation	*/
	uint16_t	Reserved2					:4;		/*!< Reserved 					  								*/
	uint16_t	AU_SIZE						:4;		/*!< Carries information about the card's allocation unit size	*/
	uint16_t	PERFORMANCE_MOVE			:8;		/*!< Carries information about the card's performance move		*/
	uint16_t	SPEED_CLASS					:8;		/*!< Carries information about the speed class of the card		*/
	uint32_t	SIZE_OF_PROTECTED_AREA		:32;	/*!< Carries information about the capacity of protected area	*/
	uint16_t	SD_CARD_TYPE				:16;	/*!< Carries information about card type 					  	*/
	uint16_t	Reserved1					:6;		/*!< Reserved 					  								*/
	uint16_t	Reserved0					:7;		/*!< Reserved for Security function					   			*/
	uint16_t	SECURED_MODE				:1;		/*!< Card is in secured mode of operation					   	*/
	uint16_t	DAT_BUS_WIDTH				:2;		/*!< Shows the currently defined data bus width                	*/
} BSP_SD_CardStatus_t;
#pragma pack()

// —труктура SD.CID, интерпретация SD_HandleTypeDef::CID, альтернатива HAL_MMC_CardCIDTypeDef, но на битовых пол€х
#pragma pack(1)
typedef struct BSP_SD_CID_struct
{
	uint32_t	Reserved1					:1;		//
	uint32_t	CRC7						:7;		// CRC
	uint32_t	ManufacturingDateMonth		:4;		// MDT.M	4-bit Month from 1
	uint32_t	ManufacturingDateYear		:8;		// MDT.Y	8-bit Year from 2000
	uint32_t	Reserved2					:4;		//
	uint32_t	ProductSerialNumber			:32;	// PSN		32-bit binary number
	uint32_t	ProductVersionM				:4;		// PRV		16-bit BCD, N.M
	uint32_t	ProductVersionN				:4;		//
	uint32_t	ProductName1				:32;	// PNM		40-bit ASCII string
	uint32_t	ProductName2				:8;		//
	uint32_t	OEM_ApplicationID			:16;	// OID		16-bit ASCII string
	uint32_t	ManufacturerID				:8;		// MID		8-bit binary number
} BSP_SD_CID_t;
#pragma pack()

// ѕолучение информации о SD-карте
uint8_t BSP_SD_GetCardStatus( BSP_SD_CardStatus_t *pCardStatus );
//void BSP_SD_GetCardCID( BSP_SD_CID_t *pCID, char *pCID_ASCII, uint32_t CID_ASCII_BufferSize );
void BSP_SD_GetCardCID( void );

// ѕолучение информации об операциях с SD-картой. –еализация в SD_diskio.c
typedef enum SD_Fail_enum
{
	eSD_Fail_Empty = 0,
	eSD_Fail_Read,
	eSD_Fail_ReadCompare,
	eSD_Fail_Write,
	eSD_Fail_WriteCompare,
	eSD_Fail_Init,
	eSD_Fail_GetStatus,
	eSD_FailsTotal
} SD_Fail_t;

typedef struct SD_Stat_struct
{
	uint32_t	OpCount;				// количество операций чтения/записи секторов. используется как индекс в aSD_Operations[]
	uint8_t		FailCount;				// количество возникших ошибок. используется как индекс в aSD_FailStat[]
	float		TimeoutMax_ms;			// максимальный таймаут с момента предыдущего чтения статистики
	float		TimeoutAvg_ms;			// усредненный таймаут
	uint32_t	WritesCount;			// количество запросов на запись
	uint32_t	ReadsCount;				// количество запросов на чтение
	uint8_t		aFails[eSD_FailsTotal];	// количество ошибок по типам
} SD_Stat_t;

extern void SD_GetStat( SD_Stat_t *pStat );
extern BSP_SD_CID_t SD_CID;
extern char aSD_CID_ASCII[70];

/**
  * @}
  */ 

/**
  * @}
  */ 
  
/**
  * @}
  */ 
  
/**
  * @}
  */ 

#ifdef __cplusplus
}
#endif

#endif /* __STM324xG_EVAL_SD_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
