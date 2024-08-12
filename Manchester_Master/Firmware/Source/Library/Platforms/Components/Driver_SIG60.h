// Driver_SIG60.h
// ƒрайвер модема SIG60
#ifndef DRIVER_SIG60_H
#define DRIVER_SIG60_H

#include <stdint.h>
#include <stdbool.h>

// »нтерфейс
// HDI	- вход UART
// HDO	- выход UART
// HDC	- режим работа (High) / управление (Low)

/*/ ƒопускамые частоты
typedef enum SIG60_BitRates_enum
{
	SIG60_BitRate_9600,
	SIG60_BitRate_19200,
	SIG60_BitRate_38400,
	SIG60_BitRate_57600,
	SIG60_BitRate_115200,
} SIG60_BitRates_t;*/

//  оманды
#define	SIG60_CMD_bm				( ( uint8_t ) ( 0x0F << 0 ) )
#define	SIG60_CMD_WRITEREG			( ( uint8_t ) ( 0x05 << 0 ) )
#define	SIG60_CMD_READREG			( ( uint8_t ) ( 0x0D << 0 ) )

// –егистры управлени€
#define	SIG16_ADDRESS_CR0			( ( uint8_t ) ( 0x00 << 4 ) )
#define	SIG60_CR0_WritableBits_bm	0x7F	// Ѕиты, доступные дл€ записи
#define	SIG60_CR0_F1nF0_bm			0x01	// F1nF0 -
#define	SIG60_CR0_F1nF0_bp			0		//   configure the SIG60 to operate at F0 ("0") or F1 ("1")
#define	SIG60_CR0_RemLoopback_bm	0x02	// Remote Loopback -
#define	SIG60_CR0_RemLoopback_bp	1		//   When "1" the SIG60 will transmit back the last received byte
#define	SIG60_CR0_InterfHop_bm		0x04	// Interference hoping -
#define	SIG60_CR0_InterfHop_bp		2		//   When "1" the SIG60 will switch to the second frequency upon detecting interference on the line.
#define	SIG60_CR0_LongWUM_bm		0x08	// Long WUM -
#define	SIG60_CR0_LongWUM_bp		3		//   Determines the length of the Wake Up Message (WUM) that is transmitted when the SIG60 exits the Sleep mode (initiated by its host). У1Ф - ~150mSec, У0Ф - ~75mS
#define	SIG60_CR0_nAutoSleep_bm		0x10	// nAutoSleep -
#define	SIG60_CR0_nAutoSleep_bp		4		//   "0" puts the SIG60 into Sleep mode after 8 seconds of inactivity
#define	SIG60_CR0_nAutoWUM_bm		0x20	// nAuto WUM -
#define	SIG60_CR0_nAutoWUM_bp		5		//   "1" disables the SIG60 from sending a Wake Up Message when wakened by its host
#define	SIG60_CR0_nIntLoopback_bm	0x40	// nLoopBack -
#define	SIG60_CR0_nIntLoopback_bp	6		//   "1" disables loopback of HDI to HDO
#define	SIG60_CR0_Interf_bm			0x80	// Interference -
#define	SIG60_CR0_Interf_bp			7		//   "1" when interference signal is detected on the Powerline (Read Only)

#define	SIG16_ADDRESS_CR1			( ( uint8_t ) ( 0x01 << 4 ) )
#define	SIG60_CR1_WritableBits_bm	0x3F	// Ѕиты, доступные дл€ записи
#define	SIG60_CR1_Freq_bm			0x0F	// Frequencies
#define	SIG60_CR1_Freq_bp			0		//	
#define	SIG60_CR1_BitRate_bm		0x30	// Bit Rate
#define	SIG60_CR1_BitRate_bp		4		//

typedef enum SIG60_CR1_Freq_enum
{																	//	F0			F1
	SIG60_CR1_Freq_1M75_4M5_gc	= ( 0x00 << SIG60_CR1_Freq_bp ),	// 1.75 Mhz	4.5 MHz
	SIG60_CR1_Freq_1M75_5M5_gc	= ( 0x01 << SIG60_CR1_Freq_bp ),	// 1.75 Mhz	5.5 MHz
	SIG60_CR1_Freq_1M75_6M0_gc	= ( 0x02 << SIG60_CR1_Freq_bp ),	// 1.75 Mhz	6.0 MHz
	SIG60_CR1_Freq_1M75_6M5_gc	= ( 0x03 << SIG60_CR1_Freq_bp ),	// 1.75 Mhz	6.5 MHz
	SIG60_CR1_Freq_4M5_5M5_gc	= ( 0x04 << SIG60_CR1_Freq_bp ),	// 4.5 Mhz	5.5 MHz
	SIG60_CR1_Freq_4M5_6M0_gc	= ( 0x05 << SIG60_CR1_Freq_bp ),	// 4.5 Mhz	6.0 MHz
	SIG60_CR1_Freq_4M5_6M5_gc	= ( 0x06 << SIG60_CR1_Freq_bp ),	// 4.5 Mhz	6.5 MHz
	SIG60_CR1_Freq_4M5_10M5_gc	= ( 0x07 << SIG60_CR1_Freq_bp ),	// 4.5 Mhz	10.5 MHz
	SIG60_CR1_Freq_10M5_13M0_gc	= ( 0x08 << SIG60_CR1_Freq_bp ),	// 10.5 MHz	13.0 MHz
	SIG60_CR1_Freq_5M5_10M5_gc	= ( 0x09 << SIG60_CR1_Freq_bp ),	// 5.5 Mhz	10.5 MHz
	SIG60_CR1_Freq_5M5_13M0_gc	= ( 0x0A << SIG60_CR1_Freq_bp ),	// 5.5 Mhz	13.0 MHz
	SIG60_CR1_Freq_6M0_10M5_gc	= ( 0x0B << SIG60_CR1_Freq_bp ),	// 6.0 Mhz	10.5 MHz
	SIG60_CR1_Freq_6M0_13M0_gc	= ( 0x0C << SIG60_CR1_Freq_bp ),	// 6.0 Mhz	13.0 MHz
	SIG60_CR1_Freq_6M5_10M5_gc	= ( 0x0D << SIG60_CR1_Freq_bp ),	// 6.5 Mhz	10.5 MHz
	SIG60_CR1_Freq_6M5_13M0_gc	= ( 0x0E << SIG60_CR1_Freq_bp ),	// 6.5 Mhz	13.0 MHz
	SIG60_CR1_Freq_5M5_6M5_gc	= ( 0x0F << SIG60_CR1_Freq_bp ),	// 5.5 Mhz	6.5 MHz
} SIG60_CR1_Freq_t;

typedef enum SIG60_CR1_BitRate_enum
{
	SIG60_CR1_BitRate_9600_115200_gc	= ( 0x02 << SIG60_CR1_BitRate_bp ), // 9600 @ 1.75..6.5 MHz / 115200 @ 10.5..13 MHz 
	SIG60_CR1_BitRate_19200_gc			= ( 0x03 << SIG60_CR1_BitRate_bp ), // 1.75..13 MHz
	SIG60_CR1_BitRate_38400_gc			= ( 0x00 << SIG60_CR1_BitRate_bp ),	// 4.5..13 MHz
	SIG60_CR1_BitRate_57600_gc			= ( 0x01 << SIG60_CR1_BitRate_bp ), // 4.5..13 MHz
} SIG60_CR1_BitRate_;

//#define	SIG60_CR0_DEFAULT			0x00
#define	SIG60_CR1_DEFAULT			0x3F	// 5.5 MHz, 6.5 MHz, 19200
#define	SIG60_CR0_DEFAULT_VITALY	0x70	// F0, noRemLoopback, noInterfHopp, noLongWUM, nAutoSleep, nIntLoopback
#define	SIG60_CR1_DEFAULT_VITALY	0xD9	// 5.5 MHz, 10.5 MHz, 57600
#define	SIG60_BITRATE_DEFAULT		19200	// —корость по-умолчанию после сброса
#define	SIG60_BITRATE_SKLP			57600	// —корость, на которую заточены модемы — Ћ

// »нициализация занимает до 25 мс, с учетом сброса питания и ожиданием ответа от модема
bool SIG60_Init( uint8_t CR0, uint8_t CR1 );
inline bool SIG60_InitDefault( void ) {	return SIG60_Init( SIG60_CR0_DEFAULT_VITALY, SIG60_CR1_DEFAULT_VITALY );	}
// ќтключить SIG60 путем подачи Reset
void SIG60_PowerOff( void );

#endif	// DRIVER_SIG60_H

