// common_GPIO.c
#include "ProjectConfig.h"		// конфиг платформы, конфиг задачи.
#include "stm32xxxx_hal.h"		// дрова периферии
#include "platform_common.h"	// GPIO_CLK_ENABLE()
#include "common_gpio.h"		// родной

const GPIO_Common_t aGPIO_Common[] = GPIO_Common_defs;

void GPIO_Common_Init( GPIO_CommonIndex_t iPin )
{	// Проверка аргументов
	if( GPIO_Common_isPinSkipped( iPin ) )
		return;
//	assert_param( IS_GPIO_COMMON_INDEX( iPin ) );

	GPIO_Common_t const *pGPIO_Common = &aGPIO_Common[iPin];

	assert_param( IS_GPIO_ALL_INSTANCE( pGPIO_Common->pGPIO ) );
	if( pGPIO_Common->Mode != GPIO_MODE_SKIP )
	{
		// Включить тактирование порта
		GPIO_CLK_ENABLE( pGPIO_Common->pGPIO );
		// Предварительно записать состоЯние пина, которое будет после инициализации
		switch( pGPIO_Common->Pull & GPIO_PULL_INITSTATE_bm )
		{
		case GPIO_PULL_INITSTATE_RESET:
			GPIO_Common_Write( iPin, GPIO_PIN_RESET );
			break;
		case GPIO_PULL_INITSTATE_SET:
			GPIO_Common_Write( iPin, GPIO_PIN_SET );
			break;
		}

		// Инициализировать пин
		GPIO_InitTypeDef GPIO_InitStruct;
		GPIO_InitStruct.Pin 	= pGPIO_Common->Pin;
		GPIO_InitStruct.Pull	= pGPIO_Common->Pull & ~GPIO_PULL_INITSTATE_bm;
		GPIO_InitStruct.Speed	= pGPIO_Common->Speed;
		GPIO_InitStruct.Mode	= pGPIO_Common->Mode;
		HAL_GPIO_Init( pGPIO_Common->pGPIO, &GPIO_InitStruct );
	}
	
}

void GPIO_Common_InitAll( void )
{
	for( uint32_t i = 0; i < SIZEOFARRAY( aGPIO_Common ); i++ )
		GPIO_Common_Init( ( GPIO_CommonIndex_t ) i );
}

// Маскировать/Размаскировать прерывание от выбранной ножки
// !! Соответствующее прерывание EXTI разрешать отдельно!
void GPIO_Common_EnableIRQ( GPIO_CommonIndex_t iPin, ITStatus Status )
{	// Проверка аргументов
//	assert_param( IS_GPIO_COMMON_INDEX( iPin ) );
//	if( iGPIO_NULL == iPin )
//		return;
	if( GPIO_Common_isPinSkipped( iPin ) )
		return;
	
	ENTER_CRITICAL_SECTION( );
#if		defined( STM32F4 ) | defined( STM32F3 )
	if( SET == Status )
	{	// Сбросить уже ожидающий флаг прерывания от пина
		EXTI->PR |= aGPIO_Common[iPin].Pin;
		// Размаскировать вызов прерывания от пина
		EXTI->IMR |= aGPIO_Common[iPin].Pin;
	}
	else
	{	// Замаскировать вызов прерывания от пина
		EXTI->IMR &= ~aGPIO_Common[iPin].Pin;
	}
#elif	defined( STM32L4 )
	if( SET == Status )
	{	// Сбросить уже ожидающий флаг прерывания от пина
		EXTI->PR1 |= aGPIO_Common[iPin].Pin;
		// Размаскировать вызов прерывания от пина
		EXTI->IMR1 |= aGPIO_Common[iPin].Pin;
	}
	else
	{	// Замаскировать вызов прерывания от пина
		EXTI->IMR1 &= ~aGPIO_Common[iPin].Pin;
	}
#else
#error "Select Target Family!"
#endif
	EXIT_CRITICAL_SECTION( );
}

#if defined( GPIO_USE_IRQ_EXTI5 ) || defined( GPIO_USE_IRQ_EXTI6 ) || defined( GPIO_USE_IRQ_EXTI7 ) || defined( GPIO_USE_IRQ_EXTI8 ) || defined( GPIO_USE_IRQ_EXTI9 )
#define GPIO_USE_IRQ_EXTI9_5
#endif

#if defined( GPIO_USE_IRQ_EXTI10 ) || defined( GPIO_USE_IRQ_EXTI11 ) || defined( GPIO_USE_IRQ_EXTI12 ) || defined( GPIO_USE_IRQ_EXTI13 ) || defined( GPIO_USE_IRQ_EXTI14 )
#define GPIO_USE_IRQ_EXTI15_10
#endif



#ifdef	GPIO_USE_IRQ_EXTI0
void EXTI0_IRQHandler( void )
{
	HAL_NVIC_ClearPendingIRQ( EXTI0_IRQn );
	HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_0 );
}
#endif

#ifdef	GPIO_USE_IRQ_EXTI1
void EXTI1_IRQHandler( void )
{
	HAL_NVIC_ClearPendingIRQ( EXTI1_IRQn );
	HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_1 );
}
#endif

#ifdef	GPIO_USE_IRQ_EXTI2
void EXTI2_IRQHandler( void )
{
	HAL_NVIC_ClearPendingIRQ( EXTI2_IRQn );
	HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_2 );
}
#endif

#ifdef	GPIO_USE_IRQ_EXTI3
void EXTI3_IRQHandler( void )
{
	HAL_NVIC_ClearPendingIRQ( EXTI3_IRQn );
	HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_3 );
}
#endif

#ifdef	GPIO_USE_IRQ_EXTI4
void EXTI4_IRQHandler( void )
{
	HAL_NVIC_ClearPendingIRQ( EXTI4_IRQn );
	HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_4 );
}
#endif

#ifdef	GPIO_USE_IRQ_EXTI9_5
void EXTI9_5_IRQHandler( void )
{
	HAL_NVIC_ClearPendingIRQ( EXTI9_5_IRQn );
	// Вызвать обработчик для каждого возможного источника
	for( uint32_t PinMask = GPIO_PIN_5; PinMask < GPIO_PIN_9; PinMask <<= 1 )
		HAL_GPIO_EXTI_IRQHandler( ( uint16_t ) PinMask );
}
#endif

#ifdef	GPIO_USE_IRQ_EXTI15_10
void EXTI15_10_IRQHandler( void )
{
	HAL_NVIC_ClearPendingIRQ( EXTI15_10_IRQn );
	// Вызвать обработчик для каждого возможного источника
	for( uint32_t PinMask = GPIO_PIN_10; PinMask < GPIO_PIN_15; PinMask <<= 1 )
		HAL_GPIO_EXTI_IRQHandler( ( uint16_t ) PinMask );
}
#endif


// Коллбек из обработчика прерываний от всех пинов
void HAL_GPIO_EXTI_Callback( uint16_t GPIO_Pin )
{
#ifdef	GPIO_USE_IRQ_EXTI0
	if( GPIO_Pin == GPIO_PIN_0 )		{	extern void GPIO_PIN_0_Callback( void );	GPIO_PIN_0_Callback( );		return; }
#endif	// GPIO_USE_IRQ_EXTI0

#ifdef	GPIO_USE_IRQ_EXTI1
	if( GPIO_Pin == GPIO_PIN_1 )		{	extern void GPIO_PIN_1_Callback( void );	GPIO_PIN_1_Callback( );		return; }
#endif	// GPIO_USE_IRQ_EXTI1

#ifdef	GPIO_USE_IRQ_EXTI2
	if( GPIO_Pin == GPIO_PIN_2 )		{	extern void GPIO_PIN_2_Callback( void );	GPIO_PIN_2_Callback( );		return; }
#endif	// GPIO_USE_IRQ_EXTI2

#ifdef	GPIO_USE_IRQ_EXTI3
	if( GPIO_Pin == GPIO_PIN_3 )		{	extern void GPIO_PIN_3_Callback( void );	GPIO_PIN_3_Callback( );		return; }
#endif	// GPIO_USE_IRQ_EXTI3

#ifdef	GPIO_USE_IRQ_EXTI4
	if( GPIO_Pin == GPIO_PIN_4 )		{	extern void GPIO_PIN_4_Callback( void );	GPIO_PIN_4_Callback( );		return; }
#endif	// GPIO_USE_IRQ_EXTI4

#ifdef	GPIO_USE_IRQ_EXTI5
	if( GPIO_Pin == GPIO_PIN_5 )		{	extern void GPIO_PIN_5_Callback( void );	GPIO_PIN_5_Callback( );		return; }
#endif	// GPIO_USE_IRQ_EXTI5

#ifdef	GPIO_USE_IRQ_EXTI6
	if( GPIO_Pin == GPIO_PIN_6 )		{	extern void GPIO_PIN_6_Callback( void );	GPIO_PIN_6_Callback( );		return; }
#endif	// GPIO_USE_IRQ_EXTI6

#ifdef	GPIO_USE_IRQ_EXTI7
	if( GPIO_Pin == GPIO_PIN_7 )		{	extern void GPIO_PIN_7_Callback( void );	GPIO_PIN_7_Callback( );		return; }
#endif	// GPIO_USE_IRQ_EXTI7

#ifdef	GPIO_USE_IRQ_EXTI8
	if( GPIO_Pin == GPIO_PIN_8 )		{	extern void GPIO_PIN_8_Callback( void );	GPIO_PIN_8_Callback( );		return; }
#endif	// GPIO_USE_IRQ_EXTI8

#ifdef	GPIO_USE_IRQ_EXTI9
	if( GPIO_Pin == GPIO_PIN_9 )		{	extern void GPIO_PIN_9_Callback( void );	GPIO_PIN_9_Callback( );		return; }
#endif	// GPIO_USE_IRQ_EXTI9

#ifdef	GPIO_USE_IRQ_EXTI10
	if( GPIO_Pin == GPIO_PIN_10 )		{	extern void GPIO_PIN_10_Callback( void );	GPIO_PIN_10_Callback( );	return; }
#endif	// GPIO_USE_IRQ_EXTI10

#ifdef	GPIO_USE_IRQ_EXTI11
		if( GPIO_Pin == GPIO_PIN_11 )	{	extern void GPIO_PIN_11_Callback( void );	GPIO_PIN_11_Callback( );	return; }
#endif	// GPIO_USE_IRQ_EXTI11

#ifdef	GPIO_USE_IRQ_EXTI12
		if( GPIO_Pin == GPIO_PIN_12 )	{	extern void GPIO_PIN_12_Callback( void );	GPIO_PIN_12_Callback( );	return; }
#endif	// GPIO_USE_IRQ_EXTI12
	
#ifdef	GPIO_USE_IRQ_EXTI13
		if( GPIO_Pin == GPIO_PIN_13 )	{	extern void GPIO_PIN_13_Callback( void );	GPIO_PIN_13_Callback( );	return; }
#endif	// GPIO_USE_IRQ_EXTI13
	
#ifdef	GPIO_USE_IRQ_EXTI14
		if( GPIO_Pin == GPIO_PIN_14 )	{	extern void GPIO_PIN_14_Callback( void );	GPIO_PIN_14_Callback( );	return; }
#endif	// GPIO_USE_IRQ_EXTI14
	
#ifdef	GPIO_USE_IRQ_EXTI15
		if( GPIO_Pin == GPIO_PIN_15 )	{	extern void GPIO_PIN_15_Callback( void );	GPIO_PIN_15_Callback( );	return; }
#endif	// GPIO_USE_IRQ_EXTI15
	
	assert_param( 0 );
}

