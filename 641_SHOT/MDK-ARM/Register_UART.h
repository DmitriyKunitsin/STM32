#ifndef STM32_L433_UART1_REGISTER_H_
#define STM32_L433_UART1_REGISTER_H_
#include "stdio.h"
#include <stdint.h>

extern uint8_t rx_data ;

#define rUART ((sUART_TypeDef *) USART1_BASE)

typedef struct {
	volatile uint32_t UE		:1; // 0
	volatile uint32_t Reserved1		:1; // 1
	volatile uint32_t RE		:1;	//2
	volatile uint32_t TE		:1; //3
	volatile uint32_t IDLEIE :1; //4
	volatile uint32_t RXNEIE :1; //5
	volatile uint32_t TCIE		:1; // 6
	volatile uint32_t TXEIE		:1; // 7
	volatile uint32_t PEIE		:1; // 8
	volatile uint32_t PS			:1; // 9
	volatile uint32_t PCE			:1; // 10
	volatile uint32_t WAKE		:1; // 11
	volatile uint32_t M0			:1; // 12
	volatile uint32_t MME			:1; // 13
	volatile uint32_t CMIE		:1; // 14
	volatile uint32_t OVER8		:1; // 15
	volatile uint32_t DEDT0		:1; //16
	volatile uint32_t DEDT1		:1; //17
	volatile uint32_t DEDT2		:1; //18
	volatile uint32_t DEDT3		:1; //19
	volatile uint32_t DEDT4		:1; //20
	volatile uint32_t DEAT0		:1; //21
	volatile uint32_t DEAT1		:1;	//22
	volatile uint32_t DEAT2		:1;	//23
	volatile uint32_t DEAT3		:1;	//24
	volatile uint32_t DEAT4		:1;	//25
	volatile uint32_t RTOIE		:1; //26
	volatile uint32_t EOBIE		:1;	//27
	volatile uint32_t M1			:1;	//28
	volatile uint32_t Reserved2	:3; //29-31
} sUSART_CR1;

typedef struct{
	volatile uint32_t Reserved1 :4; // 0 - 3
	volatile uint32_t ADDM7			:1; // 4
	volatile uint32_t LBDL			:1; // 5
	volatile uint32_t LBDIE			:1; // 6
	volatile uint32_t Reserved2	:1; // 7
	volatile uint32_t LBCL			:1; // 8
	volatile uint32_t CPHA			:1; // 9
	volatile uint32_t CPOL			:1; // 10
	volatile uint32_t CLKEN			:1; // 11
	volatile uint32_t STOP 			:2; // 12-13
	volatile uint32_t LINEN			:1; // 14
	volatile uint32_t SWAP			:1; // 15
	volatile uint32_t RXINV			:1;	// 16
	volatile uint32_t TXINV			:1; // 17
	volatile uint32_t DATAINV		:1; // 18
	volatile uint32_t MSBFIRST	:1;	// 19
	volatile uint32_t ABREN			:1; // 20
	volatile uint32_t ABRMOD0		:1; // 21
	volatile uint32_t ABRMOD1		:1; // 22
	volatile uint32_t RTOEN			:1;	// 23
	volatile uint32_t ADD1			:4;	// 24 - 27
	volatile uint32_t ADD2			:4;	// 28 - 31
}	 sUSART_CR2;

typedef struct {
	volatile uint32_t EIE				:1; // 0
	volatile uint32_t IREN			:1; // 1
	volatile uint32_t IRLP			:1; // 2
	volatile uint32_t HDSEL			:1;	// 3
	volatile uint32_t NACK			:1; // 4
	volatile uint32_t	SCEN			:1; // 5
	volatile uint32_t DMAR			:1; // 6
	volatile uint32_t DMAT			:1; // 7
	volatile uint32_t RTSE			:1; // 8
	volatile uint32_t CTSE			:1; // 9
	volatile uint32_t CTSIE			:1; // 10
	volatile uint32_t ONEBIT		:1; // 11
	volatile uint32_t OVRDIS		:1; // 12
	volatile uint32_t DDRE			:1; // 13
	volatile uint32_t DEM				:1; // 14
	volatile uint32_t DEP				:1; // 15
	volatile uint32_t Reserved1	:1;	// 16
	volatile uint32_t SCARCNT		:3;	// 17-19
	volatile uint32_t WUS				:2;	// 20-21
	volatile uint32_t WUFIE			:1;	// 22
	volatile uint32_t UCESM			:1;	// 23
	volatile uint32_t TCBGTIE		:1; // 24
	volatile uint32_t Reserved2 :7;	// 25-31
} sUSART_CR3;

typedef struct {
	volatile uint32_t BRR 			:16; // 0 - 15
	volatile uint32_t Reserved  :16; // 16 - 31
} sUSART_BRR;

typedef struct {
	volatile uint32_t PSC				:8; // 0 - 7
	volatile uint32_t GT				:8; // 8 - 15
	volatile uint32_t Reserved 	:16; // 16 - 31
} sUSART_GTPR;

typedef struct {
	volatile uint32_t RTO				:24; // 0 - 23
	volatile uint32_t BLEN			:8;	 // 24 - 31
} sUSART_RTOR;

typedef struct {
	volatile uint32_t ABRRQ			:1; // 0
	volatile uint32_t SBKRQ			:1; // 1
	volatile uint32_t MMRQ			:1; // 2
	volatile uint32_t RXFRQ			:1; // 3
	volatile uint32_t TXFRQ			:1; // 4
	volatile uint32_t Reserved	:27; // 5 -31
} sUSART_RQR;

typedef struct {
	volatile uint32_t PE				:1; // 0
	volatile uint32_t FE				:1; // 1
	volatile uint32_t NF				:1; // 2
	volatile uint32_t ORE				:1; // 3
	volatile uint32_t IDLE			:1; // 4
	volatile uint32_t RXNE			:1; // 5
	volatile uint32_t TC				:1; // 6
	volatile uint32_t TXE				:1; // 7
	volatile uint32_t LBDF			:1; // 8
	volatile uint32_t CTSIF			:1; // 9
	volatile uint32_t CTS				:1; // 10
	volatile uint32_t RTOF			:1; // 11
	volatile uint32_t EOBF			:1; // 12
	volatile uint32_t Reserved1 	:1; // 13
	volatile uint32_t ABRE			:1; // 14
	volatile uint32_t ABRF			:1; // 15
	volatile uint32_t BUSY			:1; // 16
	volatile uint32_t CMF				:1; // 17
	volatile uint32_t SBKF			:1; // 18
	volatile uint32_t RWU				:1; // 19
	volatile uint32_t WUF				:1; // 20
	volatile uint32_t TEACK			:1; // 21
	volatile uint32_t REACK			:1; // 22
	volatile uint32_t Reserved2 :2; // 23-24
	volatile uint32_t TCBGT			:1; // 25
	volatile uint32_t Reserved3 :6; // 26-31
} sUSART_ISR;

typedef struct {
	volatile uint32_t PECF			:1; // 0
	volatile uint32_t FECF			:1; // 1
	volatile uint32_t NCF				:1; // 2
	volatile uint32_t ORECF			:1; // 3
	volatile uint32_t IDLECF		:1; // 4
	volatile uint32_t Reserved	:1; // 5
	volatile uint32_t TCCF			:1; // 6
	volatile uint32_t TCBGTCF		:1; // 7
	volatile uint32_t CTSCF			:1; // 8
	volatile uint32_t Reserved2 :1; // 9
	volatile uint32_t RTOCF			:1; // 10
	volatile uint32_t EOBCF			:1; // 11
	volatile uint32_t Reserved3 :4; // 12-16
	volatile uint32_t CMCF			:1; // 17
	volatile uint32_t Reserved4 :2; // 18-19
	volatile uint32_t WUCF			:1; // 20
	volatile uint32_t Reserved5 :11; // 21 - 31
}	sUSART_ICR;

typedef struct {
	volatile uint32_t RDR				:9; // 0 - 8
	volatile uint32_t Reserved	:23; // 9 - 31
} sUSART_RDR;

typedef struct {
	volatile uint32_t TDR				:9; // 0 - 8
	volatile uint32_t Reserved 	: 23; // 9 - 31
} sUSART_TDR;

typedef struct {
	volatile sUSART_CR1 CR1; /* Control register     adress offset: 0x00*/
	volatile sUSART_CR2 CR2; /* Control register     adress offset: 0x04*/
	volatile sUSART_CR3 CR3; /* Control register     adress offset: 0x08*/
	volatile sUSART_BRR BRR; /* Baud rate register     adress offset: 0x0C*/
	volatile sUSART_GTPR GTPR; /* Guard time and prescaler register     adress offset: 0x10*/
	volatile sUSART_RTOR RTOR; /* Receiver timeout register     adress offset: 0x14*/
	volatile sUSART_RQR RQR; /* Request  register     adress offset: 0x18*/
	volatile sUSART_ISR ISR; /* Interrupt and status register     adress offset: 0x1C*/
	volatile sUSART_ICR ICR; /* Interrupt flag clear  register     adress offset: 0x20*/
	volatile sUSART_RDR RDR; /* Receive data register     adress offset: 0x24*/
	volatile sUSART_TDR TDR; /* Transmit data  register     adress offset: 0x28*/
} sUART_TypeDef;

#endif /* STM32_F405_UART1_REGISTER_H_ */
