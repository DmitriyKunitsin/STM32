// Platform_common.h
// ¬с€кие платформо-зависимые примочки дл€ STM32F3xx_HAL
#ifndef	PLATFORM_COMMON_H
#define	PLATFORM_COMMON_H

#ifndef	STM32F3
#error "Only for STM32F3 family!"
#endif

// ‘айл заточен под конкретные чипы. ѕроверить, что выбранный чип поддерживаетс€.
#if !defined(STM32F373xC)
#error	"Select target platform!"
#endif

// UART_CLK_ENABLE()
// ћакросы на каждый UART прописаны в stm32l4xx_hal_rcc.h и stm32l4xx_hal_rcc_ex.h, в зависимости от наличи€ UART на чипе
#if defined(STM32F373xC)
#define	UART_CLK_ENABLE( __UART__ )										\
	switch( ( uint32_t ) ( __UART__ ) )									\
	{																	\
	case ( uint32_t ) USART1:	__HAL_RCC_USART1_CLK_ENABLE( );	break;	\
	case ( uint32_t ) USART2:	__HAL_RCC_USART2_CLK_ENABLE( );	break;	\
	case ( uint32_t ) USART3:	__HAL_RCC_USART3_CLK_ENABLE( );	break;	\
	default:					assert_param( 0 );						\
	}
#else
#error	"Select target platform!"
#endif

// ¬ернуть номер UART
#if defined(STM32F373xC)
#define	UART_GET_NUMBER( __UART__ )													\
	( ( ( __UART__ ) == USART1 ) ? ( 1 ) : ( ( ( __UART__ ) == USART2 ) ? ( 2 ) :	\
	( ( ( __UART__ ) == USART3 ) ? ( 3 ) : 0 ) ) )
#else
#error	"Select target platform!"
#endif

// UART_IRQn()
#if defined(STM32F373xC)
#define	UART_IRQn( __UART__ )																			\
	( ( ( __UART__ ) == USART1 ) ? ( USART1_IRQn ) : ( ( ( __UART__ ) == USART2 ) ? ( USART2_IRQn ) :	\
	( ( ( __UART__ ) == USART3 ) ? ( USART3_IRQn ) : UsageFault_IRQn ) ) )
#else
#error	"Select target platform!"
#endif

// GPIO_CLK_ENABLE()
#if defined(STM32F373xC)
#define	GPIO_CLK_ENABLE( __GPIO__ )										\
	switch( ( uint32_t ) ( __GPIO__ ) )									\
	{																	\
	case ( uint32_t ) GPIOA:	__HAL_RCC_GPIOA_CLK_ENABLE( );	break;	\
	case ( uint32_t ) GPIOB:	__HAL_RCC_GPIOB_CLK_ENABLE( );	break;	\
	case ( uint32_t ) GPIOC:	__HAL_RCC_GPIOC_CLK_ENABLE( );	break;	\
	case ( uint32_t ) GPIOD:	__HAL_RCC_GPIOD_CLK_ENABLE( );	break;	\
	case ( uint32_t ) GPIOE:	__HAL_RCC_GPIOE_CLK_ENABLE( );	break;	\
	case ( uint32_t ) GPIOF:	__HAL_RCC_GPIOF_CLK_ENABLE( );	break;	\
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
	( ( ( __DMAx_CHANNELy__ ) == DMA2_Channel5 ) ? ( DMA2_Channel5_IRQn ) : 																		\
 	UsageFault_IRQn ) ) ) ) ) ) ) ) ) ) ) )

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
#if defined(STM32F373xC)
#define	TIM_CLK_ENABLE( _TIM_ )											\
	switch( ( uint32_t ) ( _TIM_ ) )									\
	{																	\
	case ( uint32_t ) TIM2: 	__HAL_RCC_TIM2_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM3: 	__HAL_RCC_TIM3_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM4: 	__HAL_RCC_TIM4_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM5: 	__HAL_RCC_TIM5_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM6: 	__HAL_RCC_TIM6_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM7: 	__HAL_RCC_TIM7_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM12:	__HAL_RCC_TIM12_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM13:	__HAL_RCC_TIM13_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM14:	__HAL_RCC_TIM14_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM15:	__HAL_RCC_TIM15_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM16:	__HAL_RCC_TIM16_CLK_ENABLE( );	break;	\
	case ( uint32_t ) TIM17: 	__HAL_RCC_TIM17_CLK_ENABLE( );	break;	\
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

// Reference manual - Device Electronic Signature
#define	DEVSIGN_UID_BASE	( ( uint32_t * ) UID_BASE )				// 		Unique Device ID register, 96 bit
#define	DEVSIGN_FLASHSIZE	( *( uint16_t * ) FLASHSIZE_BASE )		// [KB]	Device Flash Size
//#define	DEVSIGN_PACKAGE		( *( uint16_t * ) PACKAGE_BASE )		//		Package type

// Reference manual - MCU device ID code
#define	DBGMCU_IDCODE_DEVID		( DBGMCU->IDCODE & 0xFFF )
#define	DBGMCU_IDCODE_REVID		( DBGMCU->IDCODE >> 16 )
#if		defined(STM32F373xC)
#define	DEVSIGN_DEVID			0x432
#define	DEVSIGN_REVID_A			0x1000
#define	DEVSIGN_REVID_B			0x2000
#else
#error	"Select target platform!"
#endif

// SRAM
#if		defined ( STM32F373CBT7 )
#define	SRAM_SIZE	0x6000ul		// 24 KB
#elif	defined ( STM32F373RCT6 ) || defined ( STM32F373CCT6 )
#define	SRAM_SIZE	0x8000ul		// 32 KB
#else
#error "Select Chip Type!"
#endif
#define	SRAM_MAIN_BASE		SRAM_BASE
#define	SRAM_MAIN_SIZE		SRAM_SIZE
#define	SRAM_MAIN_END		( SRAM_BASE + SRAM_SIZE - 1 )

#define	IS_SRAM_MAIN( __ADDRESS__ )		( ( ( ( uint32_t ) ( __ADDRESS__ ) ) >= SRAM_MAIN_BASE ) && ( ( ( uint32_t ) ( __ADDRESS__ ) ) <= SRAM_MAIN_END ) )
#define	IS_SRAM( __ADDRESS__ ) 			( IS_SRAM_MAIN( __ADDRESS__ ) )
//#undef	FLASH_END
#define	FLASH_END						( FLASH_BASE + DEVSIGN_FLASHSIZE * 1024ul - 1 )
#define	IS_FLASH( __ADDRESS__ )			( ( ( ( uint32_t ) ( __ADDRESS__ ) ) >= FLASH_BASE ) && ( ( ( uint32_t ) ( __ADDRESS__ ) ) <= FLASH_END ) )

// ѕроверка, что адрес €вл€етс€ допустимым дл€ записи через DMA (принадлежит основному пространству ќ«”)
// јдреса регионов SRAM1_BASE (112K) и смежного SRAM2_BASE (16K) указаны в стандартном конфиге, но без указани€ размера.
//#define	IS_DMA_WRITABLE_MEMORY( __ADDRESS__ )	( ( ( ( uint32_t ) ( __ADDRESS__ ) ) >= SRAM_MAIN_BASE ) && ( ( ( uint32_t ) ( __ADDRESS__ ) ) <= SRAM_MAIN_END ) )
#define	IS_DMA_WRITABLE_MEMORY( __ADDRESS__ )		IS_SRAM_MAIN( __ADDRESS__ )

/*/ ѕроверить выравнивание адреса буфера для приема-передачи и настройки размера элементарной посылки модуля DMA
// ѕо факту, размер посылки настроены следующим образом:
// USART	- 1 байт, самом собой
// SDIO		- 1 байт. ¬ большинстве случаев, можно было бы использовать 4 байта, т.к. обращение к SD все равно через буфер 512 байт.
// Ќо в частных случаях (подтверждено, но уточнить и описать), FatFS может подсовывать и произвольные адреса, что приводит к неправильной работе.
// FatFS перепиливать неохота, проще слегка замедлить DMA (в связи с чем допилен BSP_SD_MspInit()),
// т.к. скорость обращения к SD определяется скоростью самой SD.
// ADC		- 2 или 4 байта
// ѕока, для тестирования, тело поместил в Common_sd.c. ј вообще надо бы в Platform_common.c, но он в проектах не используется.
HAL_StatusTypeDef DMA_ValidateBufferAlign( DMA_HandleTypeDef *pDMA, uint8_t *pBuffer );
*/

// –асчет выходных частот PLL по делител€м
#define COMMON_RCC_CALC_PLL( Input, M, N, PQR )			( ( Input ) / ( M ) * ( N ) / ( PQR ) )

// „астота на выходе PLL, которая подается через мультиплексор на SYSCLK
#define	COMMON_RCC_PLLCLK_DEFAULT			COMMON_RCC_CALC_PLL( HSE_VALUE, PROJECTCONFIG_CLOCK_RCC_PLL_M, PROJECTCONFIG_CLOCK_RCC_PLL_N, PROJECTCONFIG_CLOCK_RCC_PLL_R )
#define	COMMON_RCC_SYSCLK_DEFAULT			COMMON_RCC_PLLCLK_DEFAULT

// –асчет требуемой задержки при обращении к Flash по частоте и напр€жению питани€
#define KK	1000000
#if defined(STM32F373xC)
#define COMMON_RCC_PICK_FLASH_LATENCY( HCLK )	( ( ( HCLK ) <= 24*KK )	? ( FLASH_LATENCY_0 ) : ( ( ( HCLK ) <= 48*KK )		? ( FLASH_LATENCY_1 ) :	\
												( ( ( HCLK ) <= 72*KK )	? ( FLASH_LATENCY_2 ) :	0xFFFFFFFF ) ) )
#else
#error "Select target!"
#endif	// Target

// –егистры калибровочных данных ј÷ѕ
#if defined(STM32F373xC)
#define	ADC_REFIN_CAL			( * ( uint16_t * ) 0x1FFFF7BA )			// »змеренный внутренний Vref при температуре 30 degC и питании VDDA 3.3 V
#define	ADC_REFIN_CAL_VDDA		( ( float ) 3.3 )
#define	ADC_REFIN_CAL_TEMP		( ( float ) 30 )
#define	ADC_TS_CAL1				( * ( uint16_t * ) 0x1FFFF7B8 )			// »змеренный внутренний температурный датчик при температуре 30 degC и питании VDDA 3.3 V
#define	ADC_TS_TEMP1			( ( float ) 30. )
#define	ADC_TS_CAL2				( * ( uint16_t * ) 0x1FFFF7C2 )			// »змеренный внутренний температурный датчик при температуре 110 degC и питании VDDA 3.3 V
#define	ADC_TS_TEMP2			( ( float ) 110. )
#endif


#endif	// PLATFORM_COMMON_H

