// stm32xxxx_hal_conf.h
// ����������������� ������� ���� assert(), ENTER_CRITICAL_SECTION()
// ���������� �� stm32l4xx_hal_conf.h/stm32f4xx_hal_conf.h, ������� ���������� �� stm32l4xx_hal.h/stm32f4xx_hal.h
#ifndef	STM32XXXX_HAL_CONF_H
#define	STM32XXXX_HAL_CONF_H

// ����������� assert_param()
// ��������� ������� ��������� � assert �����, � ��� ��� �������� � ������������ ����������
// ������, ����� ����� ��������� ��� ��� __noreturn, ��� ����� �� ��������� �� ��������� ��� ���������.
// ������ ��� ���� �������� ����������� ����������� ��������� ������� ������� ��� ��������� ����� Call Stack,
// ��� ���������� �������.
// ��������������, ��� ��-��������� assert ��������� ��� __noreturn, � ��� ������������� �������
// ����� �������� ASSERT_USE_CALL_STACK ��� ������� ������ ��� ��� ����� �� ������������� assert.
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

// ����������� assert ��� �������� �� ����� ����������
// http://www.pixelbeat.org/programming/gcc/static_assert.html
#ifndef	STATIC_ASSERT
#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
#define STATIC_ASSERT(e) enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }
//#define	STATIC_ASSERT(e) ((void)sizeof(char[1 - 2*!(e)]))
#endif	// STATIC_ASSERT

#define	SIZEOFARRAY( Array )	( sizeof( Array ) / sizeof( ( Array )[0] ) )
typedef void ( *FunctionVoid_t ) ( void );

// ������� ����������� ������ - ������ ���� ����������
#define ENTER_CRITICAL_SECTION( )		{	uint32_t volatile SavedPRIMASK = __get_PRIMASK( );	\
											__set_PRIMASK( 1 );
#define EXIT_CRITICAL_SECTION( )		__set_PRIMASK( SavedPRIMASK ); }

// ��������� ������ � ����������
#define	ATOMIC_WRITE( LVALUE, RVALUE )		\
	ENTER_CRITICAL_SECTION( );				\
	( LVALUE ) = ( RVALUE );				\
	EXIT_CRITICAL_SECTION( );

// ������ ����� ����� �� �����
#define	WRITE_BIT_MASKED( LVALUE, BITS, MASK )		( ( LVALUE ) = ( ~( MASK ) & ( LVALUE ) ) | ( ( MASK ) & ( BITS ) ) )

// �������� ������������ ������ ����� �������������� DMA. ������������ ��������� ��� 16-� � 32-������� �������.
#define	VALIDATE_ALIGN( ADDRESS, ALIGN )		( 0 == ( ( uint32_t ) ( ADDRESS ) ) % ( ALIGN ) )

#ifdef	STM32F4
#define	__PLACE_AT_RAM_CCM__		@ "ram_ccm"
#else
#define	__PLACE_AT_RAM_CCM__
#endif
#define	__PLACE_AT_SRAM2__			@ "sram2"


#endif	// STM32XXXX_HAL_CONF_H
