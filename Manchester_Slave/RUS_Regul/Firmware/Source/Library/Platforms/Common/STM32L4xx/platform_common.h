// Platform_common.h
// Всякие платформо-зависимые примочки для STM32L4xx_HAL
#ifndef	PLATFORM_COMMON_H
#define	PLATFORM_COMMON_H

#ifndef	STM32L4
#error "Only for STM32L4 family!"
#endif

// Все чипы серии STM32L4, поддерживаемые библиотекой STM32L4xx_HAL
// *	- чипы, которые описывает Reference Manual RM0351
// **	- чипы, которые описывает Description Datasheet STM32L496xx
// +	- чипы, которые описывает Reference Manual RM0394
// ++	- чипы, которые описывает Description Datasheet STM32L433xx
// STM32L412xx	+	!!нет в текущей библиотеке HAL!
// STM32L422xx	+	!!нет в текущей библиотеке HAL!
// STM32L431xx	+
// STM32L432xx	+
// STM32L433xx	++
// STM32L442xx	+
// STM32L443xx	+
// STM32L451xx	+
// STM32L452xx	+
// STM32L462xx	+
// STM32L471xx
// STM32L475xx	*
// STM32L476xx	*
// STM32L485xx	*
// STM32L486xx	*
// STM32L496xx	**
// STM32L4A6xx	*
// STM32L4R5xx
// STM32L4R7xx
// STM32L4R9xx
// STM32L4S5xx	
// STM32L4S7xx
// STM32L4S9xx

// Файл заточен под конкретные чипы. Проверить, что выбранный чип поддерживается.
#if !defined( STM32L496xx ) && !defined( STM32L433xx )
#error	"Select target platform!"
#endif

// UART_CLK_ENABLE()
// Макросы на каждый UART прописаны в stm32l4xx_hal_rcc.h и stm32l4xx_hal_rcc_ex.h, в зависимости от наличия UART на чипе
#if 	defined( STM32L496xx )
#define	UART_CLK_ENABLE( __UART__ )											\
	switch( ( uint32_t ) ( __UART__ ) )										\
	{																		\
	case ( uint32_t ) USART1:	__HAL_RCC_USART1_CLK_ENABLE( );		break;	\
	case ( uint32_t ) USART2:	__HAL_RCC_USART2_CLK_ENABLE( );		break;	\
	case ( uint32_t ) USART3:	__HAL_RCC_USART3_CLK_ENABLE( );		break;	\
	case ( uint32_t ) UART4:	__HAL_RCC_UART4_CLK_ENABLE( );		break;	\
	case ( uint32_t ) UART5:	__HAL_RCC_UART5_CLK_ENABLE( );		break;	\
	case ( uint32_t ) LPUART1:	__HAL_RCC_LPUART1_CLK_ENABLE( );	break;	\
	default:					assert_param( 0 );							\
	}
#elif 	defined( STM32L433xx )
#define	UART_CLK_ENABLE( __UART__ )											\
	switch( ( uint32_t ) ( __UART__ ) ) 									\
	{																		\
	case ( uint32_t ) USART1:	__HAL_RCC_USART1_CLK_ENABLE( );		break;	\
	case ( uint32_t ) USART2:	__HAL_RCC_USART2_CLK_ENABLE( ); 	break;	\
	case ( uint32_t ) USART3:	__HAL_RCC_USART3_CLK_ENABLE( ); 	break;	\
	case ( uint32_t ) LPUART1:	__HAL_RCC_LPUART1_CLK_ENABLE( );	break;	\
	default:					assert_param( 0 );							\
	}
#else
#error	"Select target platform!"
#endif

// Вернуть номер UART
#if		defined( STM32L496xx )
#define	UART_GET_NUMBER( __UART__ )																\
	( ( ( __UART__ ) == USART1 ) ? ( 1 ) : ( ( ( __UART__ ) == USART2 ) ? ( 2 ) :		\
	( ( ( __UART__ ) == USART3 ) ? ( 3 ) :													\
	( ( ( __UART__ ) == UART4 ) ? ( 4 ) :	( ( ( __UART__ ) == UART5 ) ? ( 5 ) :				\
	( ( ( __UART__ ) == LPUART1 ) ? ( 11 ) : 0 ) ) ) ) ) )
#elif	defined( STM32L433xx )
#define	UART_GET_NUMBER( __UART__ )																\
	( ( ( __UART__ ) == USART1 ) ? ( 1 ) : ( ( ( __UART__ ) == USART2 ) ? ( 2 ) :		\
	( ( ( __UART__ ) == USART3 ) ? ( 3 ) :													\
	( ( ( __UART__ ) == LPUART1 ) ? ( 11 ) : 0 ) ) ) )
#else
#error	"Select target platform!"
#endif

// UART_IRQn()
#if 	defined( STM32L496xx )
#define	UART_IRQn( __UART__ )																					\
	( ( ( __UART__ ) == USART1 ) ? ( USART1_IRQn ) : ( ( ( __UART__ ) == USART2 ) ? ( USART2_IRQn ) :	\
	( ( ( __UART__ ) == USART3 ) ? ( USART3_IRQn ) :															\
	( ( ( __UART__ ) == UART4 ) ? ( UART4_IRQn ) : ( ( ( __UART__ ) == UART5 ) ? ( UART5_IRQn ) :			\
	( ( ( __UART__ ) == LPUART1 ) ? ( LPUART1_IRQn ) :															\
	UsageFault_IRQn ) ) ) ) ) )
#elif 	defined( STM32L433xx )
#define	UART_IRQn( __UART__ )																					\
	( ( ( __UART__ ) == USART1 ) ? ( USART1_IRQn ) : ( ( ( __UART__ ) == USART2 ) ? ( USART2_IRQn ) :	\
	( ( ( __UART__ ) == USART3 ) ? ( USART3_IRQn ) :															\
	( ( ( __UART__ ) == LPUART1 ) ? ( LPUART1_IRQn ) :															\
	UsageFault_IRQn ) ) ) )
#else
#error	"Select target platform!"
#endif

// GPIO_CLK_ENABLE()
#if		defined( STM32L496xx )
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
#elif	defined( STM32L433xx )
#define	GPIO_CLK_ENABLE( __GPIO__ )										\
	switch( ( uint32_t ) ( __GPIO__ ) ) 								\
	{																	\
	case ( uint32_t ) GPIOA:	__HAL_RCC_GPIOA_CLK_ENABLE( );	break;	\
	case ( uint32_t ) GPIOB:	__HAL_RCC_GPIOB_CLK_ENABLE( );	break;	\
	case ( uint32_t ) GPIOC:	__HAL_RCC_GPIOC_CLK_ENABLE( );	break;	\
	case ( uint32_t ) GPIOD:	__HAL_RCC_GPIOD_CLK_ENABLE( );	break;	\
	case ( uint32_t ) GPIOE:	__HAL_RCC_GPIOE_CLK_ENABLE( );	break;	\
	case ( uint32_t ) GPIOH:	__HAL_RCC_GPIOH_CLK_ENABLE( );	break;	\
	default:					assert_param( 0 );						\
	}
#else
#error	"Select target platform!"
#endif

/*/ GPIO_EXTI_IRQn()
#define	GPIO_EXTI_IRQn( __PIN__ )		\
	( ( ( __PIN__ ) == GPIO_PIN_0 ) ? ( EXTI0_IRQn ) : ( ( ( __PIN__ ) == GPIO_PIN_1 ) ? ( EXTI1_IRQn ) :		\
	( ( ( __PIN__ ) == GPIO_PIN_2 ) ? ( EXTI2_IRQn ) : ( ( ( __PIN__ ) == GPIO_PIN_3 ) ? ( EXTI3_IRQn ) :		\
	( ( ( __PIN__ ) == GPIO_PIN_4 ) ? ( EXTI4_IRQn ) :															\
	( ( ( ( __PIN__ ) == GPIO_PIN_5 ) || ( ( __PIN__ ) == GPIO_PIN_6 ) || ( ( __PIN__ ) == GPIO_PIN_7 ) ||		\
	( ( __PIN__ ) == GPIO_PIN_8 ) || ( ( __PIN__ ) == GPIO_PIN_9 ) ) ? ( EXTI9_5_IRQn ) :						\
	( ( ( ( __PIN__ ) == GPIO_PIN_10 ) || ( ( __PIN__ ) == GPIO_PIN_11 ) || ( ( __PIN__ ) == GPIO_PIN_12 ) ||	\
	( ( __PIN__ ) == GPIO_PIN_13 ) || ( ( __PIN__ ) == GPIO_PIN_14 ) || ( ( __PIN__ ) == GPIO_PIN_15 ) ) ? ( EXTI15_10_IRQn ) :	\
	0 ) ) ) ) ) ) )
*/

// DMA_CLK_ENABLE()
#define	DMA_CLK_ENABLE( __DMAx_CHANNELy__ )			\
	switch( ( uint32_t ) ( __DMAx_CHANNELy__ ) )	\
	{												\
	case ( uint32_t ) DMA1_Channel1:				\
	case ( uint32_t ) DMA1_Channel2: 				\
	case ( uint32_t ) DMA1_Channel3: 				\
	case ( uint32_t ) DMA1_Channel4: 				\
	case ( uint32_t ) DMA1_Channel5: 				\
	case ( uint32_t ) DMA1_Channel6: 				\
	case ( uint32_t ) DMA1_Channel7: 				\
		__HAL_RCC_DMA1_CLK_ENABLE( );	break;		\
	case ( uint32_t ) DMA2_Channel1: 				\
	case ( uint32_t ) DMA2_Channel2: 				\
	case ( uint32_t ) DMA2_Channel3: 				\
	case ( uint32_t ) DMA2_Channel4: 				\
	case ( uint32_t ) DMA2_Channel5: 				\
	case ( uint32_t ) DMA2_Channel6: 				\
	case ( uint32_t ) DMA2_Channel7: 				\
		__HAL_RCC_DMA2_CLK_ENABLE( );	break;		\
	default: assert_param( 0 );						\
}


// DMA_IRQn()
#define	DMA_IRQn( __DMAx_CHANNELy__ )																												\
	( ( ( __DMAx_CHANNELy__ ) == DMA1_Channel1 ) ? ( DMA1_Channel1_IRQn ) : ( ( ( __DMAx_CHANNELy__ ) == DMA1_Channel2 ) ? ( DMA1_Channel2_IRQn ) :	\
	( ( ( __DMAx_CHANNELy__ ) == DMA1_Channel3 ) ? ( DMA1_Channel3_IRQn ) : ( ( ( __DMAx_CHANNELy__ ) == DMA1_Channel4 ) ? ( DMA1_Channel4_IRQn ) :	\
	( ( ( __DMAx_CHANNELy__ ) == DMA1_Channel5 ) ? ( DMA1_Channel5_IRQn ) : ( ( ( __DMAx_CHANNELy__ ) == DMA1_Channel6 ) ? ( DMA1_Channel6_IRQn ) :	\
	( ( ( __DMAx_CHANNELy__ ) == DMA1_Channel7 ) ? ( DMA1_Channel7_IRQn ) :																			\
	( ( ( __DMAx_CHANNELy__ ) == DMA2_Channel1 ) ? ( DMA2_Channel1_IRQn ) : ( ( ( __DMAx_CHANNELy__ ) == DMA2_Channel2 ) ? ( DMA2_Channel2_IRQn ) :	\
	( ( ( __DMAx_CHANNELy__ ) == DMA2_Channel3 ) ? ( DMA2_Channel3_IRQn ) : ( ( ( __DMAx_CHANNELy__ ) == DMA2_Channel4 ) ? ( DMA2_Channel4_IRQn ) :	\
	( ( ( __DMAx_CHANNELy__ ) == DMA2_Channel5 ) ? ( DMA2_Channel5_IRQn ) : ( ( ( __DMAx_CHANNELy__ ) == DMA2_Channel6 ) ? ( DMA2_Channel6_IRQn ) :	\
	( ( ( __DMAx_CHANNELy__ ) == DMA2_Channel7 ) ? ( DMA2_Channel7_IRQn ) :																			\
 	UsageFault_IRQn ) ) ) ) ) ) ) ) ) ) ) ) ) )


// ADC_CLK_ENABLE()
#define	ADC_CLK_ENABLE( __ADC__ )	__HAL_RCC_ADC_CLK_ENABLE( )

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
	case ( uint32_t ) DAC1:		__HAL_RCC_DAC1_CLK_ENABLE( );	break;	\
	default:					assert_param( 0 );						\
	} } while( 0 )
#endif	// DACx


// TIM_CLOCKDIVISION()
#define	TIM_CLOCKDIVISION( _DIV_ )	( ( ( _DIV_ ) == TIM_CLOCKDIVISION_DIV1 ) ? 1 :			\
									( ( ( _DIV_ ) == TIM_CLOCKDIVISION_DIV2 ) ? 2 :			\
									( ( ( _DIV_ ) == TIM_CLOCKDIVISION_DIV4 ) ? 4 : 0 ) ) )

// TIM_CLK_ENABLE()
#if		defined( STM32L496xx )
#define	TIM_CLK_ENABLE( _TIM_ )											\
	switch( ( uint32_t ) ( _TIM_ ) )									\
	{																	\
	case ( uint32_t ) TIM1: 	__HAL_RCC_TIM1_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM2: 	__HAL_RCC_TIM2_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM3: 	__HAL_RCC_TIM3_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM4: 	__HAL_RCC_TIM4_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM5: 	__HAL_RCC_TIM5_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM6: 	__HAL_RCC_TIM6_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM7: 	__HAL_RCC_TIM7_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM8: 	__HAL_RCC_TIM8_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM15:	__HAL_RCC_TIM15_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM16:	__HAL_RCC_TIM16_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM17: 	__HAL_RCC_TIM17_CLK_ENABLE( );	break;	\
	case ( uint32_t ) LPTIM1:	__HAL_RCC_LPTIM1_CLK_ENABLE( ); break;	\
	case ( uint32_t ) LPTIM2:	__HAL_RCC_LPTIM2_CLK_ENABLE( ); break;	\
	default:					assert_param( 0 );						\
	}
#elif	defined( STM32L433xx )
#define	TIM_CLK_ENABLE( _TIM_ )											\
	switch( ( uint32_t ) ( _TIM_ ) )									\
	{																	\
	case ( uint32_t ) TIM1: 	__HAL_RCC_TIM1_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM2: 	__HAL_RCC_TIM2_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM6: 	__HAL_RCC_TIM6_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM7: 	__HAL_RCC_TIM7_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM15:	__HAL_RCC_TIM15_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM16:	__HAL_RCC_TIM16_CLK_ENABLE( );	break;	\
	case ( uint32_t ) LPTIM1:	__HAL_RCC_LPTIM1_CLK_ENABLE( ); break;	\
	case ( uint32_t ) LPTIM2:	__HAL_RCC_LPTIM2_CLK_ENABLE( ); break;	\
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

// I2C_CLK_ENABLE()
#if		defined( STM32L496xx )
#define	I2C_CLK_ENABLE( __I2C__ )										\
	switch( ( uint32_t ) ( __I2C__ ) )									\
	{																	\
	case ( uint32_t ) I2C1:		__HAL_RCC_I2C1_CLK_ENABLE( );	break;	\
	case ( uint32_t ) I2C2:		__HAL_RCC_I2C2_CLK_ENABLE( );	break;	\
	case ( uint32_t ) I2C3:		__HAL_RCC_I2C3_CLK_ENABLE( );	break;	\
	case ( uint32_t ) I2C4: 	__HAL_RCC_I2C4_CLK_ENABLE( );	break;	\
	default:					assert_param( 0 );						\
	}
#elif	defined( STM32L433xx )
#define	I2C_CLK_ENABLE( __I2C__ )										\
	switch( ( uint32_t ) ( __I2C__ ) )									\
	{																	\
	case ( uint32_t ) I2C1: 	__HAL_RCC_I2C1_CLK_ENABLE( );	break;	\
	case ( uint32_t ) I2C2: 	__HAL_RCC_I2C2_CLK_ENABLE( );	break;	\
	case ( uint32_t ) I2C3: 	__HAL_RCC_I2C3_CLK_ENABLE( );	break;	\
	default:					assert_param( 0 );						\
	}
#else
#error	"Select target platform!"
#endif
	
// I2C_SCL_SDA_AF
// I2C #1, #2, #3 - всегда AF4
// I2C #4
// - PB6, PB7	AF5
// - PB10, PB11	AF3
// - PC0, PC1	AF2
// - PD12, PD13	AF4
// - PF14, PF15	AF4
#define	I2C_123_SCL_SDA_AF( __I2C__ )						\
	( ( ( __I2C__ ) == I2C1 ) ? ( GPIO_AF4_I2C1 ) :		\
	( ( ( __I2C__ ) == I2C2 ) ? ( GPIO_AF4_I2C2 ) :		\
	( ( ( __I2C__ ) == I2C3 ) ? ( GPIO_AF4_I2C3 ) :		\
	0 ) ) )

/*/ RCC_PERIPHCLK_I2Cx
#define	RCC_PERIPHCLK_I2Cx( __I2C__ )							\
	( ( ( __I2C__ ) == I2C1 ) ? ( RCC_PERIPHCLK_I2C1 ) :		\
	( ( ( __I2C__ ) == I2C2 ) ? ( RCC_PERIPHCLK_I2C2 ) :		\
	( ( ( __I2C__ ) == I2C3 ) ? ( RCC_PERIPHCLK_I2C3 ) :		\
	0 ) ) )

// RCC_I2CxCLKSOURCE
// __I2C__		I2C1, I2C2, I2C3
// __SOURCE__	PCLK, SYSCLK, HSI
#define	__RCC_I2Cx_CLKSOURCE( __SOURCE__ )		( __SOURCE__ )
#define	RCC_I2Cx_CLKSOURCE( __I2C__, __SOURCE__ )																					\
	( ( ( __I2C__ ) == I2C1 ) ? ( RCC_I2C1CLKSOURCE##__RCC_I2Cx_CLKSOURCE( __SOURCE__ ) ) :	\
	( ( ( __I2C__ ) == I2C2 ) ? ( RCC_I2C2CLKSOURCE_SYSCLK ) :	\
	( ( ( __I2C__ ) == I2C3 ) ? ( RCC_I2C3CLKSOURCE_SYSCLK ) :	\
	0 ) ) )*/

// Reference manual - Device Electronic Signature, Unique Device ID
#define	DEVSIGN_UID_BASE			( ( uint32_t * ) UID_BASE )					// Unique Device ID register, 96 bit
#define	DEVSIGN_UID_XY				DEVSIGN_UID_BASE[0]							// X and Y coordinates on the wafer
#define	DEVSIGN_UID_WAF_NUM			( DEVSIGN_UID_BASE[1] & 0xFF )				// Wafer number (8-bit unsigned number)
#define	DEVSIGN_UID_LOT_NUM			( ( ( char * ) DEVSIGN_UID_BASE ) + 5 )	// Lot number (ASCII encoded), 7 bytes

// Reference manual - Device Electronic Signature, Flash Size & Package
#define	DEVSIGN_FLASHSIZE			( *( uint16_t * ) FLASHSIZE_BASE )			// Device Flash Size, [KB]
#define	DEVSIGN_PACKAGE				( *( uint16_t * ) PACKAGE_BASE )			// Package type
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
#endif	// STM32L475xx STM32L476xx STM32L485xx STM32L486xx STM32L496xx STM32L4A6xx

#if		defined(STM32L412xx) || defined(STM32L422xx) || defined(STM32L431xx) || defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L442xx) || defined(STM32L443xx) || defined(STM32L451xx) || defined(STM32L452xx) || defined(STM32L462xx)
#define	DEVSIGN_PACKAGE_LQFP64		0x0000
#define	DEVSIGN_PACKAGE_WLCSP64		0x0001
#define	DEVSIGN_PACKAGE_LQFP100		0x0002
#define	DEVSIGN_PACKAGE_WLCSP36		0x0005
#define	DEVSIGN_PACKAGE_UFQFPN36	0x0008
#define	DEVSIGN_PACKAGE_LQFP32		0x0009
#define	DEVSIGN_PACKAGE_UFQFPN48	0x000A
#define	DEVSIGN_PACKAGE_LQFP48		0x000B
#define	DEVSIGN_PACKAGE_WLCSP49		0x000C
#define	DEVSIGN_PACKAGE_UFBGA64		0x000D
#define	DEVSIGN_PACKAGE_UFBGA100	0x000E
#define	DEVSIGN_PACKAGE_WLCSP36_ES	0x000F
#define	DEVSIGN_PACKAGE_LQFP64_ES	0x0016
#endif	// STM32L412xx STM32L422xx STM32L431xx STM32L432xx STM32L433xx STM32L442xx STM32L443xx STM32L451xx STM32L452xx STM32L462xx


// Reference manual - MCU device ID code
#define	DBGMCU_IDCODE_DEVID			( DBGMCU->IDCODE & 0xFFF )
#define	DBGMCU_IDCODE_REVID			( DBGMCU->IDCODE >> 16 )
#if		defined(STM32L496xx) || defined(STM32L4A6xx) 
#define	DEVSIGN_DEVID				0x461
#define	DEVSIGN_REVID_A				0x1000
#define	DEVSIGN_REVID_B				0x2000
#elif	defined(STM32L475xx) || defined(STM32L476xx) || defined(STM32L486xx) 
#define	DEVSIGN_DEVID				0x415
#define	DEVSIGN_REVID_1				0x1000
#define	DEVSIGN_REVID_2				0x1001
#define	DEVSIGN_REVID_3				0x1003
#define	DEVSIGN_REVID_4				0x1007

#elif	defined(STM32L412xx) || defined(STM32L422xx) || defined(STM32L431xx) || defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L442xx) || defined(STM32L443xx) || defined(STM32L451xx) || defined(STM32L452xx) || defined(STM32L462xx)
#define	DEVSIGN_REVID_A				0x1000
#define	DEVSIGN_REVID_Z				0x1001
#define	DEVSIGN_REVID_Y				0x2001
#if		defined(STM32L412xx) || defined(STM32L422xx)
#define	DEVSIGN_DEVID				0x464
#elif	defined(STM32L431xx) || defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L442xx) || defined(STM32L443xx)
#define	DEVSIGN_DEVID				0x435
#elif	defined(STM32L451xx) || defined(STM32L452xx) || defined(STM32L462xx)
#define	DEVSIGN_DEVID				0x462
#endif	// STM32L412xx STM32L422xx STM32L431xx STM32L432xx STM32L433xx STM32L442xx STM32L443xx STM32L451xx STM32L452xx STM32L462xx

#else
#error	"Select target platform!"
#endif

// SRAM
#define	SRAM_MAIN_BASE		SRAM1_BASE
#if		defined( STM32L496xx ) || defined( STM32L433xx )
#define	SRAM_MAIN_END		( SRAM1_BASE + SRAM1_SIZE_MAX + SRAM2_SIZE - 1 )
#else
#error	"Select target platform!"
#endif

#define	IS_SRAM_MAIN( __ADDRESS__ )		( ( ( ( uint32_t ) ( __ADDRESS__ ) ) >= SRAM_MAIN_BASE ) && ( ( ( uint32_t ) ( __ADDRESS__ ) ) <= SRAM_MAIN_END ) )
#define	IS_SRAM( __ADDRESS__ ) 			( IS_SRAM_MAIN( __ADDRESS__ ) )
//#undef	FLASH_END
#define	FLASH_END						( FLASH_BASE + DEVSIGN_FLASHSIZE * 1024ul - 1 )
#define	IS_FLASH( __ADDRESS__ )			( ( ( ( uint32_t ) ( __ADDRESS__ ) ) >= FLASH_BASE ) && ( ( ( uint32_t ) ( __ADDRESS__ ) ) <= FLASH_END ) )

// Проверка, что адрес является допустимым для записи через DMA (принадлежит основному пространству ОЗУ)
// Адреса регионов SRAM1_BASE (112K) и смежного SRAM2_BASE (16K) указаны в стандартном конфиге, но без указания размера.
//#define	IS_DMA_WRITABLE_MEMORY( __ADDRESS__ )	( ( ( ( uint32_t ) ( __ADDRESS__ ) ) >= SRAM_MAIN_BASE ) && ( ( ( uint32_t ) ( __ADDRESS__ ) ) <= SRAM_MAIN_END ) )
#define	IS_DMA_WRITABLE_MEMORY( __ADDRESS__ )		IS_SRAM_MAIN( __ADDRESS__ )

/*/ Проверить выравнивание адреса буфера длЯ приема-передачи и настройки размера элементарной посылки модулЯ DMA
// По факту, размер посылки настроены следующим образом:
// USART	- 1 байт, самом собой
// SDIO		- 1 байт. В большинстве случаев, можно было бы использовать 4 байта, т.к. обращение к SD все равно через буфер 512 байт.
// Но в частных случаЯх (подтверждено, но уточнить и описать), FatFS может подсовывать и произвольные адреса, что приводит к неправильной работе.
// FatFS перепиливать неохота, проще слегка замедлить DMA (в свЯзи с чем допилен BSP_SD_MspInit()),
// т.к. скорость обращениЯ к SD определЯетсЯ скоростью самой SD.
// ADC		- 2 или 4 байта
// Пока, длЯ тестированиЯ, тело поместил в Common_sd.c. А вообще надо бы в Platform_common.c, но он в проектах не используетсЯ.
HAL_StatusTypeDef DMA_ValidateBufferAlign( DMA_HandleTypeDef *pDMA, uint8_t *pBuffer );
*/

// Расчет выходных частот PLL по делителям
#define COMMON_RCC_CALC_PLL( Input, M, N, PQR )			( ( Input ) / ( M ) * ( N ) / ( PQR ) )

// Частота на выходе PLL, котораЯ подаетсЯ через мультиплексор на SYSCLK
#define	COMMON_RCC_PLLCLK_DEFAULT			COMMON_RCC_CALC_PLL( HSE_VALUE, PROJECTCONFIG_CLOCK_RCC_PLL_M, PROJECTCONFIG_CLOCK_RCC_PLL_N, PROJECTCONFIG_CLOCK_RCC_PLL_R )
#define	COMMON_RCC_SYSCLK_DEFAULT			COMMON_RCC_PLLCLK_DEFAULT

// Расчет требуемой задержки при обращении к Flash по частоте и напряжению питания
#define KK	1000000
#if defined( STM32L496xx ) || defined( STM32L433xx )
#if		( PWR_REGULATOR_VOLTAGE_SCALE == PWR_REGULATOR_VOLTAGE_SCALE1 )
#define COMMON_RCC_PICK_FLASH_LATENCY( HCLK )	( ( ( HCLK ) <= 16*KK )	? ( FLASH_LATENCY_0 ) : ( ( ( HCLK ) <= 32*KK )		? ( FLASH_LATENCY_1 ) :	\
												( ( ( HCLK ) <= 48*KK )	? ( FLASH_LATENCY_2 ) : ( ( ( HCLK ) <= 64*KK )		? ( FLASH_LATENCY_3 ) :	\
												( ( ( HCLK ) <= 80*KK )	? ( FLASH_LATENCY_4 ) :	0xFFFFFFFF ) ) ) ) )
#elif	( PWR_REGULATOR_VOLTAGE_SCALE == PWR_REGULATOR_VOLTAGE_SCALE2 )
#define COMMON_RCC_PICK_FLASH_LATENCY( HCLK )	( ( ( HCLK ) <= 6*KK )	? ( FLASH_LATENCY_0 ) : ( ( ( HCLK ) <= 12*KK )		? ( FLASH_LATENCY_1 ) :	\
												( ( ( HCLK ) <= 18*KK ) ? ( FLASH_LATENCY_2 ) : ( ( ( HCLK ) <= 26*KK ) 	? ( FLASH_LATENCY_3 ) : \
												( ( ( HCLK ) <= 26*KK ) ? ( FLASH_LATENCY_4 ) : 0xFFFFFFFF ) ) ) ) )
#else
#error "Select PWR_REGULATOR_VOLTAGE_SCALE!"
#endif	// VDD_VALUE
#else
#error "Select target!"
#endif	// Target

// РазрЯдность АЦП
#define	ADC_RESOLUTION( __RES_BITS__ )																									\
	( ( 12 == ( __RES_BITS__ ) ) ? ( ADC_RESOLUTION_12B ) : ( ( 10 == ( __RES_BITS__ ) ) ? ( ADC_RESOLUTION_10B ) :				\
	( ( 8 == ( __RES_BITS__ ) ) ? ( ADC_RESOLUTION_8B ) : ( ( 6 == ( __RES_BITS__ ) ) ? ( ADC_RESOLUTION_6B ) : 0xFFFFFFFF ))) )

// Минимум, максимум и размах значений АЦП
#define	ADC_LIMIT_MIN( __RES_BITS__ )	( 0 )
#define	ADC_LIMIT_MAX( __RES_BITS__ )	( ( 1 << ( __RES_BITS__ ) ) - 1 )
#define	ADC_SPAN( __RES_BITS__ )	( ADC_LIMIT_MAX( __RES_BITS__ ) -  ADC_LIMIT_MIN( __RES_BITS__ ) + 1 )

// Регистры калибровочных данных АЦП
// Device Description, Temperature sensor
#if	defined( STM32L496xx ) || defined( STM32L433xx )
#define	ADC_REFIN_CAL				( * ( uint16_t * ) 0x1FFF75AA )		// Измеренный внутренний Vref при температуре 30 degC и питании VDDA 3.0 V
#define	ADC_REFIN_CAL_VDDA			3.0f								// +-0.01 V
#define	ADC_REFIN_CAL_TEMP			30.0f								// +-5 degC
#define	ADC_TS_CAL1					( * ( uint16_t * ) 0x1FFF75A8 )		// Измеренный внутренний температурный датчик при температуре 30 degC и питании VDDA 3.0 V
#define	ADC_TS_TEMP1				30.0f								// +-5 degC
#define	ADC_TS_CAL2					( * ( uint16_t * ) 0x1FFF75CA )		// Измеренный внутренний температурный датчик при температуре 130 degC и питании VDDA 3.0 V
#define	ADC_TS_TEMP2				130.0f								// +-5 degC
#endif

// Расчет напрЯжениЯ VDDA по измеренному каналу ADC.VREF					[В]
#define ADC_CALC_VDDA( __ADC_VREF__ )							( ( ADC_REFIN_CAL_VDDA * ADC_REFIN_CAL ) / ( __ADC_VREF__ ) )
// Расчет напрЯжениЯ на канале АЦП по измеренному коду и напрЯжению VDDA	[В]
#define ADC_CALC_AIN( __ADC_AIN__, __VDDA__, __ADC_SPAN__ )		( ( ( __VDDA__ ) * ( __ADC_AIN__ ) ) / ( __ADC_SPAN__ ) )
// Расчет темпретуры чипа по измеренному коду термодатчика					[грЦ]
#define	ADC_CALC_TS_TEMP( __ADC_TS__ )							( ( ( __ADC_TS__ ) - ADC_TS_CAL1 ) * ( ( ADC_TS_TEMP2 - ADC_TS_TEMP1 ) / ( ADC_TS_CAL2 - ADC_TS_CAL1 ) ) + ADC_TS_TEMP1 )

#endif	// PLATFORM_COMMON_H

