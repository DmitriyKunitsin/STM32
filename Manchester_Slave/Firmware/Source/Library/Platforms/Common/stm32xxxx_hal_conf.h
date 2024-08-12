// stm32xxxx_hal_conf.h
// ќбщеплатформенные макросы типа assert(), ENTER_CRITICAL_SECTION()
// ѕеренесено из stm32l4xx_hal_conf.h/stm32f4xx_hal_conf.h, которые вызываются из stm32l4xx_hal.h/stm32f4xx_hal.h
#ifndef	STM32XXXX_HAL_CONF_H
#define	STM32XXXX_HAL_CONF_H

// ќпределение assert_param()
// ѕоскольку вызовов обращений к assert много, и все они приводят к немедленному завершению
// работы, имеет смысл объявлять его как __noreturn, что может на несколько  Ѕ сократить код программы.
// ќднако при этом теряется возможность адекватного просмотра цепочки вызовов при обращении через Call Stack,
// что затрудняет отладку.
// ѕредполагается, что по-умолчанию assert объявлены как __noreturn, а при необходимости отладки
// нужно объявить ASSERT_USE_CALL_STACK для проекта вцелом или для файла со срабатывающим assert.
#ifdef  USE_FULL_ASSERT
	#ifdef	ASSERT_USE_CALL_STACK
	#define	ASSERT_ATTRIBUTE
	#else
	#define	ASSERT_ATTRIBUTE	__noreturn
	#endif	// ASSERT_USE_CALL_STACK
	extern ASSERT_ATTRIBUTE void assert_failed( const char * pFile, uint32_t Line, const char *pFunc );
	#define assert_param( expr ) ( ( expr ) ? ( void ) 0 : assert_failed( __FILE__, __LINE__, __FUNCTION__ ) )
#else
	#define assert_param( expr ) ( ( void ) 0 )
#endif /* USE_FULL_ASSERT */

// —татический assert для проверки на этапе компиляции
// http://www.pixelbeat.org/programming/gcc/static_assert.html
#ifndef	STATIC_ASSERT
#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
#define STATIC_ASSERT(e) enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }
//#define	STATIC_ASSERT(e) ((void)sizeof(char[1 - 2*!(e)]))
#endif	// STATIC_ASSERT

#define	SIZEOFARRAY( Array )	( sizeof( Array ) / sizeof( ( Array )[0] ) )
typedef void ( *FunctionVoid_t ) ( void );

// ∆естка€ критическа€ секци€ - запрет всех прерываний
#define ENTER_CRITICAL_SECTION( )		{	uint32_t volatile SavedPRIMASK = __get_PRIMASK( );	\
											__set_PRIMASK( 1 );
#define EXIT_CRITICAL_SECTION( )		__set_PRIMASK( SavedPRIMASK ); }

// јтомарна€ запись в переменную
#define	ATOMIC_WRITE( LVALUE, RVALUE )		\
	ENTER_CRITICAL_SECTION( );				\
	( LVALUE ) = ( RVALUE );				\
	EXIT_CRITICAL_SECTION( );

// «апись блока битов по маске
#define	WRITE_BIT_MASKED( LVALUE, BITS, MASK )		( ( LVALUE ) = ( ~( MASK ) & ( LVALUE ) ) | ( ( MASK ) & ( BITS ) ) )

// ѕроверка выравнивани€ адреса перед использованием DMA. ¬ыравнивание требуетс€ дл€ 16-и и 32-битного доступа.
#define	VALIDATE_ALIGN( ADDRESS, ALIGN )		( 0 == ( ( uint32_t ) ( ADDRESS ) ) % ( ALIGN ) )

#ifdef	STM32F4
#define	__PLACE_AT_RAM_CCM__		@ "ram_ccm"
#else
#define	__PLACE_AT_RAM_CCM__
#endif
#define	__PLACE_AT_SRAM2__			@ "sram2"


#endif	// STM32XXXX_HAL_CONF_H
