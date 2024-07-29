// Common_GPIO.h
// УнификациЯ работы с портами ввода-вывода
#ifndef	COMMON_GPIO_H
#define	COMMON_GPIO_H

#include "stm32xxxx_hal_gpio.h"		// GPIO_TypeDef
#include <stdbool.h>

/**************************************************************
 *	Константы длЯ инициализации GPIO_Common_t
 *
 *	Инициализатор PE9 должен выглЯдеть следующим образом:
 *		( GPIO_Common_t ) { GPIOE,	GPIO_PIN_9,		9,		GPIO_MODE_OUTPUT_PP,	GPIO_PULLDOWN,	GPIO_SPEED_FREQ_LOW	}
 *	Первые три полЯ обычно описаны PlatformConfig.h:
 *		#define	GPIO_MKP040			GPIOE,	GPIO_PIN_9,		9		// PE9	KT51		Test0
 *		#define	GPIO_TestPin0		GPIO_MKP040
 *	Поэтому обычно инициализатор PE9 выглЯдит следующим образом:
 *		( GPIO_Common_t ) { GPIO_TestPin0,		GPIO_MODE_OUTPUT_PP,	GPIO_PULLDOWN,	GPIO_SPEED_FREQ_LOW	}
 *
 *	GPIO_NULL
 *	Если в проекте подразумеваетсЯ использование пина GPIO_TestPin0, но сам он еще не определен,
 *	или использование его надо замаскировать, нужно обнулить первое поле:
 *		#define GPIO_NULL			NULL, 0, 0
 *		#define	GPIO_TestPin0		GPIO_NULL
 *		( GPIO_Common_t ) { GPIO_TestPin0,		GPIO_MODE_OUTPUT_PP,	GPIO_PULLDOWN,	GPIO_SPEED_FREQ_LOW	}
 *	В этом случае можно продолжать нормально работать с пином функциЯми типа GPIO_Common_Init() и GPIO_Common_Write(),
 *	но реальных действий при этом выполнЯтьсЯ не будет.
 *
 *	GPIO_MODE_SKIP
 *	Специальный инициализатор длЯ полЯ GPIO_Common_t::Mode, если длЯ пина требуетсЯ особаЯ инициализациЯ,
 *	но в дальнейшем подразумеваетсЯ обычное использование функций типа GPIO_Common_Write().
 *	GPIO_Common_Init() не будет выполНЯть действий.
 *
 *	GPIO_PULL_INITSTATE_bm
 *	Маска длЯ полЯ GPIO_Common_t::Pull. Через биты маски указываетсЯ, какое значение записать в порт
 *	при иницализации GPIO_Common_Init() перед тем, как установить режим работы.
 *	Пример установки пина при инициализации:
 *		( GPIO_Common_t ) { GPIO_TestPin0,		GPIO_MODE_OUTPUT_PP,	( GPIO_PULLDOWN | GPIO_PULL_INITSTATE_SET ),	GPIO_SPEED_FREQ_LOW	}
 **************************************************************/
#define GPIO_NULL					NULL, 0, 0				// инициализатор первых трех полей GPIO_Common_t длЯ маскированиЯ пина
#define GPIO_MODE_SKIP				0xFFFFFFFF				// пропустить инициализацию при выполнении GPIO_Common_Init()
#define	GPIO_PULL_INITSTATE_bm		0xC0000000				// маска длЯ GPIO_Common_t::Pull длЯ установки начального значениЯ порта при инициализации:
#define	GPIO_PULL_INITSTATE_SKIP	0						//	- пропустить установку начального значениЯ
#define	GPIO_PULL_INITSTATE_RESET	0x80000000				//	- записать 0
#define	GPIO_PULL_INITSTATE_SET		0xC0000000				//	- записать 1

typedef uint16_t GPIO_Pin_t;		// маска одного или нескольких пинов в 16-битном порту

typedef struct GPIO_Common_struct
{
	GPIO_TypeDef	*pGPIO;			// Порт
	GPIO_Pin_t		Pin;			// GPIO_PIN_XX		Маска пина (допускается выбирать сразу несколько)
	uint8_t			Position;		// 					Позиция пина (только если выбран один пин)
	uint32_t		Mode;			// GPIO_MODE_XXX		Режим работы
	uint32_t		Pull;			// GPIO_PULLXXX		Подтяжка
	uint32_t		Speed;			// GPIO_SPEED_FREQ_XXX	Скорость
//	uint32_t		Alternate;		// не используется, т.к. структура только для простой инициализации
} GPIO_Common_t;

// ОбъЯвлениЯ массива со всеми используемыми пинами
extern const GPIO_Common_t aGPIO_Common[];

// Проверка допустимости индекса пина
// ДопускаетсЯ использовать индексы от -1 ( iGPIO_NULL ) до ( iGPIO_Total - 1 ).
// При этом, при обращении по индексу -1 необходимо пропускать выполнение запрошенной операции.
//#define	IS_GPIO_COMMON_INDEX( Index )	( ( Index >= iGPIO_NULL ) && ( Index < iGPIO_Total ) )
inline bool GPIO_Common_isPinSkipped( GPIO_CommonIndex_t iPin )
{
//	assert_param( IS_GPIO_COMMON_INDEX( iPin ) );
	assert_param( ( iPin >= iGPIO_NULL ) && ( iPin < iGPIO_Total ) );
	return ( ( iPin == iGPIO_NULL ) || ( NULL == aGPIO_Common[iPin].pGPIO ) );
}

// ОбъЯвление функций
void GPIO_Common_Init( GPIO_CommonIndex_t iPin );								// ИнициализациЯ выбронного пина
void GPIO_Common_InitAll( void );												// ИнициализациЯ всех пинов из массива
void GPIO_Common_EnableIRQ( GPIO_CommonIndex_t iPin, ITStatus Status );	// Маскирование/размаскирование прерываниЯ от выбранного пина

// Установить/сбросить указанные пины
// ОперациЯ атомарнаЯ, критической секции не требуетсЯ
inline void GPIO_Common_Write( GPIO_CommonIndex_t index, GPIO_PinState State )
{
	if( !GPIO_Common_isPinSkipped( index ) )
	{
//		assert_param( IS_GPIO_COMMON_INDEX( index ) );
		if( GPIO_PIN_RESET != State )
			aGPIO_Common[index].pGPIO->BSRR = aGPIO_Common[index].Pin;
		else
#if		defined( STM32F4 )
			aGPIO_Common[index].pGPIO->BSRR = ( ( uint32_t ) aGPIO_Common[index].Pin ) << 16;		// допускаетсЯ длЯ STM32F4 и STM32L4
#elif	defined( STM32L4 ) | defined( STM32F3 )
			aGPIO_Common[index].pGPIO->BRR = aGPIO_Common[index].Pin;								// допускаетсЯ длЯ STM32L4 и STM32F3
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

// Переключить указанные пины
// ДлЯ обеспечениЯ атомарности требуетсЯ критическаЯ секциЯ
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

// Прочитать состоЯние указанных пинов
// !! Важно - чтение IDR, может не совпадать с ODR!
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

