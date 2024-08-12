// Common_GPIO.h
// ���������� ������ � ������� �����-������
#ifndef	COMMON_GPIO_H
#define	COMMON_GPIO_H

#include "stm32xxxx_hal_gpio.h"		// GPIO_TypeDef
#include <stdbool.h>

/**************************************************************
 *	��������� ��� ������������� GPIO_Common_t
 *
 *	������������� PE9 ������ ��������� ��������� �������:
 *		( GPIO_Common_t ) { GPIOE,	GPIO_PIN_9,		9,		GPIO_MODE_OUTPUT_PP,	GPIO_PULLDOWN,	GPIO_SPEED_FREQ_LOW	}
 *	������ ��� ���� ������ ������� PlatformConfig.h:
 *		#define	GPIO_MKP040			GPIOE,	GPIO_PIN_9,		9		// PE9	KT51		Test0
 *		#define	GPIO_TestPin0		GPIO_MKP040
 *	������� ������ ������������� PE9 �������� ��������� �������:
 *		( GPIO_Common_t ) { GPIO_TestPin0,		GPIO_MODE_OUTPUT_PP,	GPIO_PULLDOWN,	GPIO_SPEED_FREQ_LOW	}
 *
 *	GPIO_NULL
 *	���� � ������� ��������������� ������������� ���� GPIO_TestPin0, �� ��� �� ��� �� ���������,
 *	��� ������������� ��� ���� �������������, ����� �������� ������ ����:
 *		#define GPIO_NULL			NULL, 0, 0
 *		#define	GPIO_TestPin0		GPIO_NULL
 *		( GPIO_Common_t ) { GPIO_TestPin0,		GPIO_MODE_OUTPUT_PP,	GPIO_PULLDOWN,	GPIO_SPEED_FREQ_LOW	}
 *	� ���� ������ ����� ���������� ��������� �������� � ����� ��������� ���� GPIO_Common_Init() � GPIO_Common_Write(),
 *	�� �������� �������� ��� ���� ����������� �� �����.
 *
 *	GPIO_MODE_SKIP
 *	����������� ������������� ��� ���� GPIO_Common_t::Mode, ���� ��� ���� ��������� ������ �������������,
 *	�� � ���������� ��������������� ������� ������������� ������� ���� GPIO_Common_Write().
 *	GPIO_Common_Init() �� ����� ��������� ��������.
 *
 *	GPIO_PULL_INITSTATE_bm
 *	����� ��� ���� GPIO_Common_t::Pull. ����� ���� ����� �����������, ����� �������� �������� � ����
 *	��� ������������ GPIO_Common_Init() ����� ���, ��� ���������� ����� ������.
 *	������ ��������� ���� ��� �������������:
 *		( GPIO_Common_t ) { GPIO_TestPin0,		GPIO_MODE_OUTPUT_PP,	( GPIO_PULLDOWN | GPIO_PULL_INITSTATE_SET ),	GPIO_SPEED_FREQ_LOW	}
 **************************************************************/
#define GPIO_NULL					NULL, 0, 0				// ������������� ������ ���� ����� GPIO_Common_t ��� ������������ ����
#define GPIO_MODE_SKIP				0xFFFFFFFF				// ���������� ������������� ��� ���������� GPIO_Common_Init()
#define	GPIO_PULL_INITSTATE_bm		0xC0000000				// ����� ��� GPIO_Common_t::Pull ��� ��������� ���������� �������� ����� ��� �������������:
#define	GPIO_PULL_INITSTATE_SKIP	0						//	- ���������� ��������� ���������� ��������
#define	GPIO_PULL_INITSTATE_RESET	0x80000000				//	- �������� 0
#define	GPIO_PULL_INITSTATE_SET		0xC0000000				//	- �������� 1

typedef uint16_t GPIO_Pin_t;		// ����� ������ ��� ���������� ����� � 16-������ �����

typedef struct GPIO_Common_struct
{
	GPIO_TypeDef	*pGPIO;			// ����
	GPIO_Pin_t		Pin;			// GPIO_PIN_XX		����� ���� (����������� �������� ����� ���������)
	uint8_t			Position;		// 					������� ���� (������ ���� ������ ���� ���)
	uint32_t		Mode;			// GPIO_MODE_XXX		����� ������
	uint32_t		Pull;			// GPIO_PULLXXX		��������
	uint32_t		Speed;			// GPIO_SPEED_FREQ_XXX	��������
//	uint32_t		Alternate;		// �� ������������, �.�. ��������� ������ ��� ������� �������������
} GPIO_Common_t;

// ���������� ������� �� ����� ������������� ������
extern const GPIO_Common_t aGPIO_Common[];

// �������� ������������ ������� ����
// ����������� ������������ ������� �� -1 ( iGPIO_NULL ) �� ( iGPIO_Total - 1 ).
// ��� ����, ��� ��������� �� ������� -1 ���������� ���������� ���������� ����������� ��������.
//#define	IS_GPIO_COMMON_INDEX( Index )	( ( Index >= iGPIO_NULL ) && ( Index < iGPIO_Total ) )
inline bool GPIO_Common_isPinSkipped( GPIO_CommonIndex_t iPin )
{
//	assert_param( IS_GPIO_COMMON_INDEX( iPin ) );
	assert_param( ( iPin >= iGPIO_NULL ) && ( iPin < iGPIO_Total ) );
	return ( ( iPin == iGPIO_NULL ) || ( NULL == aGPIO_Common[iPin].pGPIO ) );
}

// ���������� �������
void GPIO_Common_Init( GPIO_CommonIndex_t iPin );								// ������������� ���������� ����
void GPIO_Common_InitAll( void );												// ������������� ���� ����� �� �������
void GPIO_Common_EnableIRQ( GPIO_CommonIndex_t iPin, ITStatus Status );	// ������������/��������������� ���������� �� ���������� ����

// ����������/�������� ��������� ����
// �������� ���������, ����������� ������ �� ���������
inline void GPIO_Common_Write( GPIO_CommonIndex_t index, GPIO_PinState State )
{
	if( !GPIO_Common_isPinSkipped( index ) )
	{
//		assert_param( IS_GPIO_COMMON_INDEX( index ) );
		if( GPIO_PIN_RESET != State )
			aGPIO_Common[index].pGPIO->BSRR = aGPIO_Common[index].Pin;
		else
#if		defined( STM32F4 )
			aGPIO_Common[index].pGPIO->BSRR = ( ( uint32_t ) aGPIO_Common[index].Pin ) << 16;		// ����������� ��� STM32F4 � STM32L4
#elif	defined( STM32L4 ) | defined( STM32F3 )
			aGPIO_Common[index].pGPIO->BRR = aGPIO_Common[index].Pin;								// ����������� ��� STM32L4 � STM32F3
#else
#error	"Select Target Family!"
#endif
	}
}

/*inline void GPIO_Common_WriteMasked( GPIO_CommonIndex_t index, uint16_t Value )
{
	if( !GPIO_Common_isPinSkipped( index ) )
	{
		assert_param( IS_GPIO_COMMON_INDEX( index ) );
		ENTER_CRITICAL_SECTION( );
		aGPIO_Common[index].pGPIO->ODR = ( aGPIO_Common[index].pGPIO->ODR & ~aGPIO_Common[index].Pin ) | Value;
		EXIT_CRITICAL_SECTION( );
	}
}*/

// ����������� ��������� ����
// ��� ����������� ����������� ��������� ����������� ������
inline void GPIO_Common_Toggle( GPIO_CommonIndex_t index )
{
	if( !GPIO_Common_isPinSkipped( index ) )
	{
//		assert_param( IS_GPIO_COMMON_INDEX( index ) );
		ENTER_CRITICAL_SECTION( );
		aGPIO_Common[index].pGPIO->ODR ^= aGPIO_Common[index].Pin;
		EXIT_CRITICAL_SECTION( );
	}
}

// ��������� ��������� ��������� �����
// !! ����� - ������ IDR, ����� �� ��������� � ODR!
inline GPIO_Pin_t GPIO_Common_Read( GPIO_CommonIndex_t index )
{
	if( !GPIO_Common_isPinSkipped( index ) )
	{
//		assert_param( IS_GPIO_COMMON_INDEX( index ) );
		return aGPIO_Common[index].pGPIO->IDR & aGPIO_Common[index].Pin;
	}
	else
		return 0;
}



#endif	// COMMON_GPIO_H

