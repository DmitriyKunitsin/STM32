// Platform_common_STM32F4xx.h
// ¬с€кие платформо-зависимые примочки дл€ STM32F4xx_HAL
#ifndef	PLATFORM_COMMON_STM32F4xx_H
#define	PLATFORM_COMMON_STM32F4xx_H

#ifndef	STM32F4
#error "Only for STM32F4 family!"
#endif

// ¬се чипы серии STM32F4, поддерживаемые библиотекой STM32F4xx_HAL
// *	- чипы, которые описывает Reference Manual RM0090
// **	- чипы, которые описывает Description Datasheet STM32F405xx, STM32F407xx
// $	- чипы, которые описывает Reference Manual RM0368
// $$	- чипы, которые описывает Description Datasheet STM32F401xB, STM32F401xC
// STM32F405xx	**
// STM32F415xx	*
// STM32F407xx	**
// STM32F417xx	*
// STM32F427xx	*
// STM32F437xx	*
// STM32F429xx	*
// STM32F439xx	*
// STM32F401xC	$$
// STM32F401xE	$
// STM32F410Tx
// STM32F410Cx
// STM32F410Rx
// STM32F411xE
// STM32F446xx
// STM32F469xx
// STM32F479xx
// STM32F412Cx
// STM32F412Zx	
// STM32F412Rx
// STM32F412Vx
// STM32F413xx
// STM32F423xx

// ‘айл заточен под конкретные чипы. ѕроверить, что выбранный чип поддерживаетс€.
#if !defined(STM32F401xC) && !defined(STM32F405xx) && !defined(STM32F415xx) && !defined(STM32F407xx) && !defined(STM32F417xx) && !defined(STM32F427xx) && !defined(STM32F437xx) && !defined(STM32F429xx) && !defined(STM32F439xx)
#error	"Select target platform!"
#endif

// UART_CLK_ENABLE()
// ћакросы на каждый UART прописаны в stm32f4xx_hal_rcc.h и stm32f4xx_hal_rcc_ex.h, в зависимости от наличи€ UART на чипе
#if defined(STM32F401xC)
#define	UART_CLK_ENABLE( __UART__ )										\
	switch( ( uint32_t ) ( __UART__ ) )									\
	{																	\
	case ( uint32_t ) USART1:	__HAL_RCC_USART1_CLK_ENABLE( );	break;	\
	case ( uint32_t ) USART2:	__HAL_RCC_USART2_CLK_ENABLE( ); break;	\
	case ( uint32_t ) USART6:	__HAL_RCC_USART6_CLK_ENABLE( ); break;	\
	default:					assert_param( 0 );						\
	}
#elif defined(STM32F405xx) || defined(STM32F415xx) || defined(STM32F407xx) || defined(STM32F417xx)
#define	UART_CLK_ENABLE( __UART__ )										\
	switch( ( uint32_t ) ( __UART__ ) )									\
	{																	\
	case ( uint32_t ) USART1:	__HAL_RCC_USART1_CLK_ENABLE( );	break;	\
	case ( uint32_t ) USART2:	__HAL_RCC_USART2_CLK_ENABLE( ); break;	\
	case ( uint32_t ) USART3:	__HAL_RCC_USART3_CLK_ENABLE( ); break;	\
	case ( uint32_t ) UART4:	__HAL_RCC_UART4_CLK_ENABLE( ); break;	\
	case ( uint32_t ) UART5:	__HAL_RCC_UART5_CLK_ENABLE( ); break;	\
	case ( uint32_t ) USART6:	__HAL_RCC_USART6_CLK_ENABLE( ); break;	\
	default:					assert_param( 0 );						\
	}
#elif defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx)
#define	UART_CLK_ENABLE( __UART__ )										\
	switch( ( uint32_t ) ( __UART__ ) )									\
	{																	\
	case ( uint32_t ) USART1:	__HAL_RCC_USART1_CLK_ENABLE( );	break;	\
	case ( uint32_t ) USART2:	__HAL_RCC_USART2_CLK_ENABLE( ); break;	\
	case ( uint32_t ) USART3:	__HAL_RCC_USART3_CLK_ENABLE( ); break;	\
	case ( uint32_t ) UART4:	__HAL_RCC_UART4_CLK_ENABLE( ); break;	\
	case ( uint32_t ) UART5:	__HAL_RCC_UART5_CLK_ENABLE( ); break;	\
	case ( uint32_t ) USART6:	__HAL_RCC_USART6_CLK_ENABLE( ); break;	\
	case ( uint32_t ) UART7:	__HAL_RCC_UART7_CLK_ENABLE( ); break;	\
	case ( uint32_t ) UART8:	__HAL_RCC_UART8_CLK_ENABLE( ); break;	\
	default:					assert_param( 0 );						\
	}
#else
#error	"Select target platform!"
#endif

// UART_IRQn()
// ¬ернуть номер UART
#if defined(STM32F401xC)
#define	UART_IRQn( __UART__ )																			\
	( ( ( __UART__ ) == USART1 ) ? ( USART1_IRQn ) : ( ( ( __UART__ ) == USART2 ) ? ( USART2_IRQn ) :	\
	( ( ( __UART__ ) == USART6 ) ? ( USART6_IRQn ) : UsageFault_IRQn ) ) )
#define	UART_GET_NUMBER( __UART__ )																		\
	( ( ( __UART__ ) == USART1 ) ? ( 1 ) : ( ( ( __UART__ ) == USART2 ) ? ( 2 ) :						\
	( ( ( __UART__ ) == USART6 ) ? ( 5 ) : 0 ) ) )

#elif defined(STM32F405xx) || defined(STM32F415xx) || defined(STM32F407xx) || defined(STM32F417xx)
#define	UART_IRQn( __UART__ )																			\
	( ( ( __UART__ ) == USART1 ) ? ( USART1_IRQn ) : ( ( ( __UART__ ) == USART2 ) ? ( USART2_IRQn ) :	\
	( ( ( __UART__ ) == USART3 ) ? ( USART3_IRQn ) : ( ( ( __UART__ ) == USART6 ) ? ( USART6_IRQn ) :	\
	( ( ( __UART__ ) == UART4 ) ? ( UART4_IRQn ) : ( ( ( __UART__ ) == UART5 ) ? ( UART5_IRQn ) :		\
	UsageFault_IRQn ) ) ) ) ) )
#define	UART_GET_NUMBER( __UART__ )																		\
	( ( ( __UART__ ) == USART1 ) ? ( 1 ) : ( ( ( __UART__ ) == USART2 ) ? ( 2 ) :						\
	( ( ( __UART__ ) == USART3 ) ? ( 3 ) : ( ( ( __UART__ ) == USART6 ) ? ( 6 ) :						\
	( ( ( __UART__ ) == UART4 ) ? ( 4 ) : ( ( ( __UART__ ) == UART5 ) ? ( 5 ) : 0 ) ) ) ) ) )

#elif defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx)
#define	UART_IRQn( __UART__ )																			\
	( ( ( __UART__ ) == USART1 ) ? ( USART1_IRQn ) : ( ( ( __UART__ ) == USART2 ) ? ( USART2_IRQn ) :	\
	( ( ( __UART__ ) == USART3 ) ? ( USART3_IRQn ) : ( ( ( __UART__ ) == USART6 ) ? ( USART6_IRQn ) :	\
	( ( ( __UART__ ) == UART4 ) ? ( UART4_IRQn ) : ( ( ( __UART__ ) == UART5 ) ? ( UART5_IRQn ) :		\
	( ( ( __UART__ ) == UART7 ) ? ( UART7_IRQn ) : ( ( ( __UART__ ) == UART8 ) ? ( UART8_IRQn ) :		\
	UsageFault_IRQn ) ) ) ) ) ) ) )
#define	UART_GET_NUMBER( __UART__ )																		\
	( ( ( __UART__ ) == USART1 ) ? ( 1 ) : ( ( ( __UART__ ) == USART2 ) ? ( 2 ) :						\
	( ( ( __UART__ ) == USART3 ) ? ( 3 ) : ( ( ( __UART__ ) == USART6 ) ? ( 6 ) :						\
	( ( ( __UART__ ) == UART4 ) ? ( 4 ) : ( ( ( __UART__ ) == UART5 ) ? ( 5 ) :							\
	( ( ( __UART__ ) == UART7 ) ? ( 7 ) : ( ( ( __UART__ ) == UART8 ) ? ( 8 ) : 0 ) ) ) ) ) ) ) )
#else
#error	"Select target platform!"
#endif

#if defined(STM32F401xC)
#define	GPIO_CLK_ENABLE( __GPIO__ )										\
	switch( ( uint32_t ) ( __GPIO__ ) )									\
	{																	\
	case ( uint32_t ) GPIOA:	__HAL_RCC_GPIOA_CLK_ENABLE( );	break;	\
	case ( uint32_t ) GPIOB:	__HAL_RCC_GPIOB_CLK_ENABLE( );	break;	\
	case ( uint32_t ) GPIOC:	__HAL_RCC_GPIOC_CLK_ENABLE( );	break;	\
	case ( uint32_t ) GPIOD:	__HAL_RCC_GPIOD_CLK_ENABLE( );	break;	\
	case ( uint32_t ) GPIOE:	__HAL_RCC_GPIOE_CLK_ENABLE( );	break;	\
	case ( uint32_t ) GPIOH:	__HAL_RCC_GPIOH_CLK_ENABLE( );	break;	\
	default:					assert_param( 0 );						\
	}
#elif defined(STM32F405xx) || defined(STM32F415xx) || defined(STM32F407xx) || defined(STM32F417xx) || defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx)
#define	GPIO_CLK_ENABLE( __GPIO__ )										\
	switch( ( uint32_t ) ( __GPIO__ ) )									\
	{																	\
	case ( uint32_t ) GPIOA:	__HAL_RCC_GPIOA_CLK_ENABLE( );	break;	\
	case ( uint32_t ) GPIOB:	__HAL_RCC_GPIOB_CLK_ENABLE( );	break;	\
	case ( uint32_t ) GPIOC:	__HAL_RCC_GPIOC_CLK_ENABLE( );	break;	\
	case ( uint32_t ) GPIOD:	__HAL_RCC_GPIOD_CLK_ENABLE( );	break;	\
	case ( uint32_t ) GPIOE:	__HAL_RCC_GPIOE_CLK_ENABLE( );	break;	\
	case ( uint32_t ) GPIOF:	__HAL_RCC_GPIOF_CLK_ENABLE( );	break;	\
	case ( uint32_t ) GPIOG:	__HAL_RCC_GPIOG_CLK_ENABLE( );	break;	\
	case ( uint32_t ) GPIOH:	__HAL_RCC_GPIOH_CLK_ENABLE( );	break;	\
	case ( uint32_t ) GPIOI:	__HAL_RCC_GPIOI_CLK_ENABLE( );	break;	\
	default:					assert_param( 0 );						\
	}
#else
#error	"Select target platform!"
#endif

// GPIO_EXTI_IRQn()
#define	GPIO_EXTI_IRQn( __PIN__ )		\
	( ( ( __PIN__ ) == GPIO_PIN_0 ) ? ( EXTI0_IRQn ) : ( ( ( __PIN__ ) == GPIO_PIN_1 ) ? ( EXTI1_IRQn ) :		\
	( ( ( __PIN__ ) == GPIO_PIN_2 ) ? ( EXTI2_IRQn ) : ( ( ( __PIN__ ) == GPIO_PIN_3 ) ? ( EXTI3_IRQn ) :		\
	( ( ( __PIN__ ) == GPIO_PIN_4 ) ? ( EXTI4_IRQn ) :															\
	( ( ( ( __PIN__ ) == GPIO_PIN_5 ) || ( ( __PIN__ ) == GPIO_PIN_6 ) || ( ( __PIN__ ) == GPIO_PIN_7 ) ||		\
	( ( __PIN__ ) == GPIO_PIN_8 ) || ( ( __PIN__ ) == GPIO_PIN_9 ) ) ? ( EXTI9_5_IRQn ) :						\
	( ( ( ( __PIN__ ) == GPIO_PIN_10 ) || ( ( __PIN__ ) == GPIO_PIN_11 ) || ( ( __PIN__ ) == GPIO_PIN_12 ) ||	\
	( ( __PIN__ ) == GPIO_PIN_13 ) || ( ( __PIN__ ) == GPIO_PIN_14 ) || ( ( __PIN__ ) == GPIO_PIN_15 ) ) ? ( EXTI15_10_IRQn ) :	\
	0 ) ) ) ) ) ) )


// DMA_CLK_ENABLE()
#define	DMA_CLK_ENABLE( __DMAx_STREAMy__ )			\
	switch( ( uint32_t ) ( __DMAx_STREAMy__ ) )		\
	{												\
	case ( uint32_t ) DMA1_Stream0:					\
	case ( uint32_t ) DMA1_Stream1:					\
	case ( uint32_t ) DMA1_Stream2: 				\
	case ( uint32_t ) DMA1_Stream3: 				\
	case ( uint32_t ) DMA1_Stream4: 				\
	case ( uint32_t ) DMA1_Stream5: 				\
	case ( uint32_t ) DMA1_Stream6: 				\
	case ( uint32_t ) DMA1_Stream7: 				\
		__HAL_RCC_DMA1_CLK_ENABLE( );	break;		\
	case ( uint32_t ) DMA2_Stream0: 				\
	case ( uint32_t ) DMA2_Stream1: 				\
	case ( uint32_t ) DMA2_Stream2: 				\
	case ( uint32_t ) DMA2_Stream3: 				\
	case ( uint32_t ) DMA2_Stream4: 				\
	case ( uint32_t ) DMA2_Stream5: 				\
	case ( uint32_t ) DMA2_Stream6: 				\
	case ( uint32_t ) DMA2_Stream7: 				\
		__HAL_RCC_DMA2_CLK_ENABLE( );	break;		\
	default: assert_param( 0 );						\
}


// DMA_IRQn()
#define	DMA_IRQn( __DMAx_STREAMy__ )																											\
	( ( ( __DMAx_STREAMy__ ) == DMA1_Stream0 ) ? ( DMA1_Stream0_IRQn ) : ( ( ( __DMAx_STREAMy__ ) == DMA1_Stream1 ) ? ( DMA1_Stream1_IRQn ) :	\
	( ( ( __DMAx_STREAMy__ ) == DMA1_Stream2 ) ? ( DMA1_Stream2_IRQn ) : ( ( ( __DMAx_STREAMy__ ) == DMA1_Stream3 ) ? ( DMA1_Stream3_IRQn ) :	\
	( ( ( __DMAx_STREAMy__ ) == DMA1_Stream4 ) ? ( DMA1_Stream4_IRQn ) : ( ( ( __DMAx_STREAMy__ ) == DMA1_Stream5 ) ? ( DMA1_Stream5_IRQn ) :	\
	( ( ( __DMAx_STREAMy__ ) == DMA1_Stream6 ) ? ( DMA1_Stream6_IRQn ) : ( ( ( __DMAx_STREAMy__ ) == DMA1_Stream7 ) ? ( DMA1_Stream7_IRQn ) :	\
	( ( ( __DMAx_STREAMy__ ) == DMA2_Stream0 ) ? ( DMA2_Stream0_IRQn ) : ( ( ( __DMAx_STREAMy__ ) == DMA2_Stream1 ) ? ( DMA2_Stream1_IRQn ) :	\
	( ( ( __DMAx_STREAMy__ ) == DMA2_Stream2 ) ? ( DMA2_Stream2_IRQn ) : ( ( ( __DMAx_STREAMy__ ) == DMA2_Stream3 ) ? ( DMA2_Stream3_IRQn ) :	\
	( ( ( __DMAx_STREAMy__ ) == DMA2_Stream4 ) ? ( DMA2_Stream4_IRQn ) : ( ( ( __DMAx_STREAMy__ ) == DMA2_Stream5 ) ? ( DMA2_Stream5_IRQn ) :	\
	( ( ( __DMAx_STREAMy__ ) == DMA2_Stream6 ) ? ( DMA2_Stream6_IRQn ) : ( ( ( __DMAx_STREAMy__ ) == DMA2_Stream7 ) ? ( DMA2_Stream7_IRQn ) :	\
 	UsageFault_IRQn ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) )


// ADC_CLK_ENABLE()
#if		defined( ADC1 ) && defined( ADC2 ) && defined( ADC3 )
#define	ADC_CLK_ENABLE( __ADC__ )	do {								\
	switch( ( uint32_t ) ( __ADC__ ) )									\
	{																	\
	case ( uint32_t ) ADC1:		__HAL_RCC_ADC1_CLK_ENABLE( );	break;	\
	case ( uint32_t ) ADC2:		__HAL_RCC_ADC2_CLK_ENABLE( );	break;	\
	case ( uint32_t ) ADC3:		__HAL_RCC_ADC3_CLK_ENABLE( );	break;	\
	default:					assert_param( 0 );						\
	} } while( 0 )
#elif	defined( ADC1 ) && defined( ADC2 )
#define	ADC_CLK_ENABLE( __ADC__ )	do {								\
	switch( ( uint32_t ) ( __ADC__ ) )									\
	{																	\
	case ( uint32_t ) ADC1:		__HAL_RCC_ADC1_CLK_ENABLE( );	break;	\
	case ( uint32_t ) ADC2:		__HAL_RCC_ADC2_CLK_ENABLE( );	break;	\
	default:					assert_param( 0 );						\
	} } while( 0 )
#elif	defined( ADC1 )
#define	ADC_CLK_ENABLE( __ADC__ )	do {								\
	switch( ( uint32_t ) ( __ADC__ ) )									\
	{																	\
	case ( uint32_t ) ADC1:		__HAL_RCC_ADC1_CLK_ENABLE( );	break;	\
	default:					assert_param( 0 );						\
	} } while( 0 )
#endif	// ADCx

// DAC_CLK_ENABLE()
#if	defined( DAC1 ) && defined( DAC2 )
#define	DAC_CLK_ENABLE( __DAC__ )	do {								\
	switch( ( uint32_t ) ( __DAC__ ) )									\
	{																	\
	case ( uint32_t ) DAC1:		__HAL_RCC_DAC1_CLK_ENABLE( );	break;	\
	case ( uint32_t ) DAC2:		__HAL_RCC_DAC2_CLK_ENABLE( );	break;	\
	default:					assert_param( 0 );						\
	} } while( 0 )
#elif	defined( DAC1 )
#define	DAC_CLK_ENABLE( __DAC__ )	do {								\
	switch( ( uint32_t ) ( __DAC__ ) )									\
	{																	\
	case ( uint32_t ) DAC1:		__HAL_RCC_DAC_CLK_ENABLE( );	break;	\
	default:					assert_param( 0 );						\
	} } while( 0 )
#endif	// DACx


// TIM_CLOCKDIVISION()
#define	TIM_CLOCKDIVISION( _DIV_ )	( ( ( _DIV_ ) == TIM_CLOCKDIVISION_DIV1 ) ? 1 :			\
									( ( ( _DIV_ ) == TIM_CLOCKDIVISION_DIV2 ) ? 2 :			\
									( ( ( _DIV_ ) == TIM_CLOCKDIVISION_DIV4 ) ? 4 : 0 ) ) )

// TIM_CLK_ENABLE()
#if defined(STM32F401xC)
#define	TIM_CLK_ENABLE( _TIM_ )											\
	switch( ( uint32_t ) ( _TIM_ ) )									\
	{																	\
	case ( uint32_t ) TIM1:		__HAL_RCC_TIM1_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM2:		__HAL_RCC_TIM2_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM3:		__HAL_RCC_TIM3_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM4:		__HAL_RCC_TIM4_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM5:		__HAL_RCC_TIM5_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM9:		__HAL_RCC_TIM9_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM10:	__HAL_RCC_TIM10_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM11: 	__HAL_RCC_TIM11_CLK_ENABLE( );	break;	\
	default:					assert_param( 0 );						\
	}
#elif defined(STM32F405xx) || defined(STM32F415xx) || defined(STM32F407xx) || defined(STM32F417xx) || defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx)
#define	TIM_CLK_ENABLE( _TIM_ )											\
	switch( ( uint32_t ) ( _TIM_ ) )									\
	{																	\
	case ( uint32_t ) TIM1:		__HAL_RCC_TIM1_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM2:		__HAL_RCC_TIM2_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM3:		__HAL_RCC_TIM3_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM4:		__HAL_RCC_TIM4_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM5:		__HAL_RCC_TIM5_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM6:		__HAL_RCC_TIM6_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM7:		__HAL_RCC_TIM7_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM8:		__HAL_RCC_TIM8_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM9:		__HAL_RCC_TIM9_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM10:	__HAL_RCC_TIM10_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM11: 	__HAL_RCC_TIM11_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM12: 	__HAL_RCC_TIM12_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM13: 	__HAL_RCC_TIM13_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM14:	__HAL_RCC_TIM14_CLK_ENABLE( );	break;	\
	default:					assert_param( 0 );						\
	}
#else
#error	"Select target platform!"
#endif


// SPI_CLK_ENABLE()
#define	SPI_CLK_ENABLE( __SPI__ )										\
	switch( ( uint32_t ) ( __SPI__ ) )									\
	{																	\
	case ( uint32_t ) SPI1:		__HAL_RCC_SPI1_CLK_ENABLE( );	break;	\
	case ( uint32_t ) SPI2:		__HAL_RCC_SPI2_CLK_ENABLE( );	break;	\
	case ( uint32_t ) SPI3:		__HAL_RCC_SPI3_CLK_ENABLE( );	break;	\
	default:					assert_param( 0 );						\
	}

// SPI_FORCE_RESET()
#define	SPI_FORCE_RESET( __SPI__ )																									\
	( ( ( __SPI__ ) == SPI1 ) ? ( __HAL_RCC_SPI1_FORCE_RESET( ) ) : ( ( ( __SPI__ ) == SPI2 ) ? ( __HAL_RCC_SPI2_FORCE_RESET( ) ) :	\
	( ( ( __SPI__ ) == SPI3 ) ? ( __HAL_RCC_SPI3_FORCE_RESET( ) ) : 	0 ) ) )

// SPI_RELEASE_RESET()
#define	SPI_RELEASE_RESET( __SPI__ )																									\
	( ( ( __SPI__ ) == SPI1 ) ? ( __HAL_RCC_SPI1_RELEASE_RESET( ) ) : ( ( ( __SPI__ ) == SPI2 ) ? ( __HAL_RCC_SPI2_RELEASE_RESET( ) ) :	\
	( ( ( __SPI__ ) == SPI3 ) ? ( __HAL_RCC_SPI3_RELEASE_RESET( ) ) : 	0 ) ) )

// FLASH_VOLTAGE_RANGE
#if		( VDD_VALUE > 2700 ) && defined( VPP_PRESENT )
#define	FLASH_VOLTAGE_RANGE		FLASH_VOLTAGE_RANGE_4
#elif	( VDD_VALUE > 2700 )
#define	FLASH_VOLTAGE_RANGE		FLASH_VOLTAGE_RANGE_3
#elif	( VDD_VALUE > 2100 )
#define	FLASH_VOLTAGE_RANGE		FLASH_VOLTAGE_RANGE_2
#elif	( VDD_VALUE > 1800 )
#define	FLASH_VOLTAGE_RANGE		FLASH_VOLTAGE_RANGE_1
#else
#error "Select VDD Voltage!"
#endif	// VDD_VALUE

// Reference manual - Device Electronic Signature, Unique Device ID
#define	DEVSIGN_UID_BASE			( ( uint32_t * ) UID_BASE )				// Unique Device ID register, 96 bit
#define	DEVSIGN_UID_XY				DEVSIGN_UID_BASE[0]						// X and Y coordinates on the wafer
#define	DEVSIGN_UID_WAF_NUM			( DEVSIGN_UID_BASE[1] & 0xFF )			// Wafer number (8-bit unsigned number)
#define	DEVSIGN_UID_LOT_NUM			( ( ( char * ) DEVSIGN_UID_BASE ) + 5 )	// Lot number (ASCII encoded), 7 bytes

// Reference manual - Device Electronic Signature, Flash Size & Package
#define	DEVSIGN_FLASHSIZE			( *( uint16_t * ) FLASHSIZE_BASE )		// Device Flash Size, [KB]
#define	DEVSIGN_PACKAGE				( *( uint16_t * ) PACKAGE_BASE )		// Package type
/* »нформации по корпусам не нашел
#if		defined(STM32L475xx) || defined(STM32L476xx) || defined(STM32L485xx) || defined(STM32L486xx) || defined(STM32L496xx) || defined(STM32L4A6xx)
#define	DEVSIGN_PACKAGE_LQFP64		0x0000
#define	DEVSIGN_PACKAGE_LQFP100		0x0002
#define	DEVSIGN_PACKAGE_UFBGA132	0x0003
#define	DEVSIGN_PACKAGE_LQFP144		0x0004
#if		defined(STM32L496xx) || defined(STM32L4A6xx)
#define	DEVSIGN_PACKAGE_UFBGA169	0x0010
#define	DEVSIGN_PACKAGE_WLCSP100	0x0011
#endif
#if		defined(STM32L475xx) || defined(STM32L476xx) || defined(STM32L486xx)
#define	DEVSIGN_PACKAGE_WLCSP72		0x0004
#define	DEVSIGN_PACKAGE_WLCSP81		0x0004
#endif
#endif*/

// Reference manual - MCU device ID code
#define	DBGMCU_IDCODE_DEVID		( DBGMCU->IDCODE & 0xFFF )
#define	DBGMCU_IDCODE_REVID		( DBGMCU->IDCODE >> 16 )
#if		defined(STM32F401xC)
#define	DEVSIGN_DEVID			0x423		// STM32F401xB/C
#define	DEVSIGN_REVID_Z			0x1000
#define	DEVSIGN_REVID_A			0x1001
#elif	defined(STM32F401xE)
#define	DEVSIGN_DEVID			0x433		// STM32F401xD/E
#define	DEVSIGN_REVID_A			0x1000
#define	DEVSIGN_REVID_Z			0x1001
#elif	defined(STM32F405xx) || defined(STM32F407xx) || defined(STM32F415xx) || defined(STM32F417xx)
#define	DEVSIGN_DEVID			0x413
#define	DEVSIGN_REVID_A			0x1000
#define	DEVSIGN_REVID_Z			0x1001
#define	DEVSIGN_REVID_1			0x1003
#define	DEVSIGN_REVID_2			0x1007
#define	DEVSIGN_REVID_4			0x100F
#define	DEVSIGN_REVID_Y			0x100F
#elif	defined(STM32F423xx) || defined(STM32F427xx) || defined(STM32F429xx)	// STM32F42xxx & STM32F43xxx
#define	DEVSIGN_DEVID			0x419
#define	DEVSIGN_REVID_A			0x1000
#define	DEVSIGN_REVID_Y			0x1003
#define	DEVSIGN_REVID_1			0x1007
#define	DEVSIGN_REVID_3			0x2001
#define	DEVSIGN_REVID_4			0x2007
#define	DEVSIGN_REVID_5			0x2007
#define	DEVSIGN_REVID_B			0x2007
#else
#error	"Select target platform!"
#endif

// SRAM
#define	SRAM_MAIN_BASE		SRAM1_BASE
#if		defined(STM32F401xC)
#define	SRAM_MAIN_END		( SRAM1_BASE + 0x0000FFFF )
#elif		defined(STM32F405xx) || defined(STM32F415xx) || defined(STM32F407xx) || defined(STM32F417xx)
#define	SRAM_MAIN_END		( SRAM1_BASE + 0x0001FFFF )
#elif	defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx)
#define	SRAM_MAIN_END		( SRAM1_BASE + 0x0002FFFF )
#else
#error	"Select target platform!"
#endif

#if		defined(STM32F405xx) || defined(STM32F415xx) || defined(STM32F407xx) || defined(STM32F417xx) || defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx)
#define	SRAM_CCM_BASE		CCMDATARAM_BASE
#define	SRAM_CCM_END		CCMDATARAM_END
#define	IS_SRAM_CCM( __ADDRESS__ )		( ( ( ( uint32_t ) ( __ADDRESS__ ) ) >= SRAM_CCM_BASE ) && ( ( ( uint32_t ) ( __ADDRESS__ ) ) <= SRAM_CCM_END ) )
#endif

#define	IS_SRAM_MAIN( __ADDRESS__ )		( ( ( ( uint32_t ) ( __ADDRESS__ ) ) >= SRAM_MAIN_BASE ) && ( ( ( uint32_t ) ( __ADDRESS__ ) ) <= SRAM_MAIN_END ) )
#undef	FLASH_END
#define	FLASH_END						( FLASH_BASE + DEVSIGN_FLASHSIZE * 1024ul - 1 )
#define	IS_FLASH( __ADDRESS__ )			( ( ( ( uint32_t ) ( __ADDRESS__ ) ) >= FLASH_BASE ) && ( ( ( uint32_t ) ( __ADDRESS__ ) ) <= FLASH_END ) )

#ifdef	SRAM_CCM_BASE
#define	IS_SRAM( __ADDRESS__ ) 			( IS_SRAM_MAIN( __ADDRESS__ ) || IS_SRAM_CCM( __ADDRESS__ ) )
#else
#define	IS_SRAM( __ADDRESS__ ) 			( IS_SRAM_MAIN( __ADDRESS__ ) )
#endif

// ѕроверка, что адрес €вл€етс€ допустимым дл€ записи через DMA (принадлежит основному пространству ќ«”)
// јдреса регионов SRAM1_BASE (112K) и смежного SRAM2_BASE (16K) указаны в стандартном конфиге, но без указани€ размера.
//#define	IS_DMA_WRITABLE_MEMORY( __ADDRESS__ )	( ( ( ( uint32_t ) ( __ADDRESS__ ) ) >= SRAM_MAIN_BASE ) && ( ( ( uint32_t ) ( __ADDRESS__ ) ) <= SRAM_MAIN_END ) )
#define	IS_DMA_WRITABLE_MEMORY( __ADDRESS__ )		IS_SRAM_MAIN( __ADDRESS__ )

// ѕроверить выравнивание адреса буфера для приема-передачи и настройки размера элементарной посылки модуля DMA
// ѕо факту, размер посылки настроены следующим образом:
// USART	- 1 байт, самом собой
// SDIO		- 1 байт. ¬ большинстве случаев, можно было бы использовать 4 байта, т.к. обращение к SD все равно через буфер 512 байт.
// Ќо в частных случаях (подтверждено, но уточнить и описать), FatFS может подсовывать и произвольные адреса, что приводит к неправильной работе.
// FatFS перепиливать неохота, проще слегка замедлить DMA (в связи с чем допилен BSP_SD_MspInit()),
// т.к. скорость обращения к SD определяется скоростью самой SD.
// ADC		- 2 или 4 байта
// ѕока, для тестирования, тело поместил в Common_sd.c. ј вообще надо бы в Platform_common.c, но он в проектах не используется.
HAL_StatusTypeDef DMA_ValidateBufferAlign( DMA_HandleTypeDef *pDMA, uint8_t *pBuffer );

/*
// —татический assert для проверки на этапе компиляции
// http://www.pixelbeat.org/programming/gcc/static_assert.html
#ifndef	STATIC_ASSERT
#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
#define STATIC_ASSERT(e) enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }
//#define	STATIC_ASSERT(e) ((void)sizeof(char[1 - 2*!(e)]))
#endif	// STATIC_ASSERT
*/

// –асчет выходных частот PLL по делител€м
#define COMMON_RCC_CALC_PLL( Input, M, N, PQR )			( ( Input ) / ( M ) * ( N ) / ( PQR ) )

// „астота на выходе PLL, которая подается через мультиплексор на SYSCLK
#define	COMMON_RCC_PLLCLK_DEFAULT			COMMON_RCC_CALC_PLL( HSE_VALUE, PROJECTCONFIG_CLOCK_RCC_PLL_M, PROJECTCONFIG_CLOCK_RCC_PLL_N, PROJECTCONFIG_CLOCK_RCC_PLL_P )
#define	COMMON_RCC_SYSCLK_DEFAULT			COMMON_RCC_PLLCLK_DEFAULT

// –асчет требуемой задержки при обращении к Flash по частоте и напр€жению питани€
#define KK	1000000
#if defined(STM32F405xx) || defined(STM32F415xx) || defined(STM32F407xx) || defined(STM32F417xx) || \
    defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F411xE)
#if ( ( VDD_VALUE ) > 2700 )
#define COMMON_RCC_PICK_FLASH_LATENCY( HCLK )	( ( ( HCLK ) < 30*KK )		? ( FLASH_LATENCY_0 ) : ( ( ( HCLK ) <= 60*KK )		? ( FLASH_LATENCY_1 ) :	\
												( ( ( HCLK ) <= 90*KK )		? ( FLASH_LATENCY_2 ) : ( ( ( HCLK ) <= 120*KK )	? ( FLASH_LATENCY_3 ) :	\
												( ( ( HCLK ) <= 150*KK )	? ( FLASH_LATENCY_4 ) : ( ( ( HCLK ) <= 168*KK )	? ( FLASH_LATENCY_5 ) :	\
												0xFFFFFFFF ) ) ) ) ) )
#elif ( ( VDD_VALUE) > 2400 )
#define COMMON_RCC_PICK_FLASH_LATENCY( HCLK )	( ( ( HCLK ) < 24*KK )		? ( FLASH_LATENCY_0 ) : ( ( ( HCLK ) <= 48*KK )		? ( FLASH_LATENCY_1 ) :	\
												( ( ( HCLK ) <= 72*KK )		? ( FLASH_LATENCY_2 ) : ( ( ( HCLK ) <= 96*KK )		? ( FLASH_LATENCY_3 ) :	\
												( ( ( HCLK ) <= 120*KK )	? ( FLASH_LATENCY_4 ) : ( ( ( HCLK ) <= 144*KK )	? ( FLASH_LATENCY_5 ) :	\
												( ( ( HCLK ) <= 168*KK )	? ( FLASH_LATENCY_6 ) :	0xFFFFFFFF ) ) ) ) ) ) )
#elif ( ( VDD_VALUE) > 2100 )
#define COMMON_RCC_PICK_FLASH_LATENCY( HCLK )	( ( ( HCLK ) < 22*KK )		? ( FLASH_LATENCY_0 ) : ( ( ( HCLK ) <= 44*KK )		? ( FLASH_LATENCY_1 ) :	\
												( ( ( HCLK ) <= 66*KK )		? ( FLASH_LATENCY_2 ) : ( ( ( HCLK ) <= 88*KK )		? ( FLASH_LATENCY_3 ) :	\
												( ( ( HCLK ) <= 110*KK )	? ( FLASH_LATENCY_4 ) : ( ( ( HCLK ) <= 132*KK )	? ( FLASH_LATENCY_5 ) :	\
												( ( ( HCLK ) <= 154*KK )	? ( FLASH_LATENCY_6 ) :	( ( ( HCLK ) <= 168*KK )	? ( FLASH_LATENCY_7 ) :	\
												0xFFFFFFFF ) ) ) ) ) ) ) )
#elif ( ( VDD_VALUE) > 1800 )
#define COMMON_RCC_PICK_FLASH_LATENCY( HCLK )	( ( ( HCLK ) < 20*KK )		? ( FLASH_LATENCY_0 ) : ( ( ( HCLK ) <= 40*KK )		? ( FLASH_LATENCY_1 ) :	\
												( ( ( HCLK ) <= 60*KK )		? ( FLASH_LATENCY_2 ) : ( ( ( HCLK ) <= 80*KK )		? ( FLASH_LATENCY_3 ) :	\
												( ( ( HCLK ) <= 100*KK )	? ( FLASH_LATENCY_4 ) : ( ( ( HCLK ) <= 120*KK )	? ( FLASH_LATENCY_5 ) :	\
												( ( ( HCLK ) <= 140*KK )	? ( FLASH_LATENCY_6 ) :	( ( ( HCLK ) <= 160*KK )	? ( FLASH_LATENCY_7 ) :	\
												0xFFFFFFFF ) ) ) ) ) ) ) )
#else
#error "Select VDD_VALUE!"
#endif	// VDD_VALUE
#else
#error "Select target!"
#endif	// Target

#endif	// PLATFORM_COMMON_STM32F4xx_H

