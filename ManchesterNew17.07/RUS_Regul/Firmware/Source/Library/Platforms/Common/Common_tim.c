// Платформо-зависимый инициализатор драйвера TIM
#include "ProjectConfig.h"			// конфиг платформы, конфиг задачи.
#include "stm32xxxx_hal.h"			// дрова периферии, в т.ч. 
#include "platform_common.h"
#include "Common_tim.h"				// Родной

#if	defined( TIM1_USE )
	TIM_HandleTypeDef TIM1_hdl __PLACE_AT_RAM_CCM__;
#endif	// TIM2_USE

#if	defined( TIM2_USE )
	TIM_HandleTypeDef TIM2_hdl __PLACE_AT_RAM_CCM__;
#endif	// TIM2_USE

#if	defined( TIM3_USE )
	TIM_HandleTypeDef TIM3_hdl __PLACE_AT_RAM_CCM__;
#endif	// TIM3_USE

#if	defined( TIM4_USE )
	TIM_HandleTypeDef TIM4_hdl __PLACE_AT_RAM_CCM__;
#endif	// TIM4_USE

#if	defined( TIM5_USE )
	TIM_HandleTypeDef TIM5_hdl __PLACE_AT_RAM_CCM__;
#endif	// TIM5_USE

#if	defined( TIM6_USE )
	TIM_HandleTypeDef TIM6_hdl __PLACE_AT_RAM_CCM__;
#endif	// TIM6_USE

#if	defined( TIM7_USE )
	TIM_HandleTypeDef TIM7_hdl __PLACE_AT_RAM_CCM__;
#endif	// TIM7_USE

#if	defined( TIM8_USE )
	TIM_HandleTypeDef TIM8_hdl __PLACE_AT_RAM_CCM__;
#endif	// TIM8_USE

#if	defined( TIM9_USE )
	TIM_HandleTypeDef TIM9_hdl __PLACE_AT_RAM_CCM__;
#endif	// TIM9_USE

#if	defined( TIM10_USE )
	TIM_HandleTypeDef TIM10_hdl __PLACE_AT_RAM_CCM__;
#endif	// TIM10_USE

#if	defined( TIM11_USE )
	TIM_HandleTypeDef TIM11_hdl __PLACE_AT_RAM_CCM__;
#endif	// TIM11_USE

#if	defined( TIM12_USE )
	TIM_HandleTypeDef TIM12_hdl __PLACE_AT_RAM_CCM__;
#endif	// TIM12_USE

#if	defined( TIM13_USE )
	TIM_HandleTypeDef TIM13_hdl __PLACE_AT_RAM_CCM__;
#endif	// TIM13_USE

#if	defined( TIM14_USE )
	TIM_HandleTypeDef TIM14_hdl __PLACE_AT_RAM_CCM__;
#endif	// TIM14_USE

#if	defined( TIM15_USE )
	TIM_HandleTypeDef TIM15_hdl __PLACE_AT_RAM_CCM__;
#endif	// TIM15_USE

#if	defined( TIM16_USE )
	TIM_HandleTypeDef TIM16_hdl __PLACE_AT_RAM_CCM__;
#endif	// TIM16_USE

#if	defined( TIM17_USE )
	TIM_HandleTypeDef TIM17_hdl __PLACE_AT_RAM_CCM__;
#endif	// TIM17_USE

#if	defined( TIM18_USE )
	TIM_HandleTypeDef TIM18_hdl __PLACE_AT_RAM_CCM__;
#endif	// TIM18_USE

#if	defined( TIM19_USE )
	TIM_HandleTypeDef TIM19_hdl __PLACE_AT_RAM_CCM__;
#endif	// TIM19_USE


// Получить входную периферийную частоту таймера
uint32_t TIM_Common_GetFrq( TIM_TypeDef *pTIM_Instance )
{
	uint32_t APBxT_Frq = 0; 	// тактовая частота шины
	do
	{
		if( NULL == pTIM_Instance )
			break;
		switch( ( uint32_t ) pTIM_Instance )
		{
		#ifdef	TIM1
			case ( uint32_t ) TIM1:
		#endif
		#ifdef	TIM8
			case ( uint32_t ) TIM8:
		#endif
		#ifdef	TIM9
			case ( uint32_t ) TIM9:
		#endif
		#ifdef	TIM10
			case ( uint32_t ) TIM10:
		#endif
		#ifdef	TIM11
			case ( uint32_t ) TIM11:
		#endif
		#ifdef	TIM15
			case ( uint32_t ) TIM15:
		#endif
		#ifdef	TIM16
			case ( uint32_t ) TIM16:
		#endif
		#ifdef	TIM17
			case ( uint32_t ) TIM17:
		#endif
		#ifdef	TIM19
			case ( uint32_t ) TIM19:
		#endif
				// Эти таймеры тактированы от PCLK2
				APBxT_Frq = HAL_RCC_GetPCLK2Freq( );	// частота шины, от которой тактирован таймер
				if( RCC_CFGR_PPRE2_DIV1 != ( RCC->CFGR & RCC_CFGR_PPRE2 ) )
					APBxT_Frq *= 2; 					// частота таймеров вдвое больше частоты шины, если частота шины поделена относительно системной
				break;
		
		#ifdef	TIM2
			case ( uint32_t ) TIM2:		// 32-bit
		#endif
		#ifdef	TIM3
			case ( uint32_t ) TIM3:
		#endif
		#ifdef	TIM4
			case ( uint32_t ) TIM4:
		#endif
		#ifdef	TIM5
			case ( uint32_t ) TIM5:		// 32-bit
		#endif
		#ifdef	TIM6
			case ( uint32_t ) TIM6:
		#endif
		#ifdef	TIM7
			case ( uint32_t ) TIM7:
		#endif
		#ifdef	TIM12
			case ( uint32_t ) TIM12:
		#endif
		#ifdef	TIM13
			case ( uint32_t ) TIM13:
		#endif
		#ifdef	TIM14
			case ( uint32_t ) TIM14:
		#endif
		#ifdef	TIM18
			case ( uint32_t ) TIM18:
		#endif
				// Эти таймеры тактированы от PCLK1
				APBxT_Frq = HAL_RCC_GetPCLK1Freq( );	// частота шины, от которой тактирован таймер
				if( RCC_CFGR_PPRE1_DIV1 != ( RCC->CFGR & RCC_CFGR_PPRE1 ) )
					APBxT_Frq *= 2; 					// частота таймеров вдвое больше частоты шины, если частота шины поделена относительно системной
				break;
				
			default:
				// Если задействован неучтеный таймер, не удалось определить частоту
				APBxT_Frq = 0;
		}
	} while( 0 );
	
	return APBxT_Frq;
}


// Расчет предделителей таймера
// !!! Тест!
// !!! Проверено на шестнадцатибитных таймерах!!!
bool TIM_Common_SetupPrescalers( TIM_HandleTypeDef *pTIM, uint32_t Period_us, uint32_t ClockDivision )
{
	bool Result = false;
	do
	{
		// Проверка аргументов
		if( ( NULL == pTIM ) || ( 0 == Period_us ) || !IS_TIM_CLOCKDIVISION_DIV( ClockDivision ) )
			break;

		if( 0
#ifdef	TIM2
			|| ( pTIM->Instance == TIM2 )
#endif
#ifdef	TIM5
			|| ( pTIM->Instance == TIM5 )
#endif
			)
//		if( ( pTIM->Instance == TIM2 ) || ( pTIM->Instance == TIM5 ) )
			break;		// это 32-битные таймеры, расчет предделителей на них не проверен

		// Получить входную периферийную частоту таймера
		uint32_t APBxT_Frq = TIM_Common_GetFrq( pTIM->Instance );
		if( 0 == APBxT_Frq )
			break;		// запрашиваемый таймер не найден
		
		// Подобрать предделитель и период таймера по требуемой длительности срабатывания
		uint32_t Divisor = ( uint32_t ) ( APBxT_Frq / TIM_CLOCKDIVISION( ClockDivision ) / ( 1000000 / Period_us ) );		// Divisor == Prescaler * Period
		uint32_t Prescaler = 1;
		uint32_t Period;
		while( 1 )
		{
			Period = Divisor / Prescaler;
			if( Period <= __UNSIGNED_SHORT_MAX__ )
			{
				Result = true;
				break;
			}
			if( Prescaler <= __UNSIGNED_SHORT_MAX__ / 2 )
				Prescaler *= 2;
			else if( Prescaler != __UNSIGNED_SHORT_MAX__ )
				Prescaler = __UNSIGNED_SHORT_MAX__;
			else
				break;		// ПодходЯщий предделитель не найден
		}
		if( !Result )
			break;
		
		// Подобраны подходЯщий период и предделитель
		pTIM->Init.ClockDivision	= ClockDivision;
		pTIM->Init.Prescaler		= ( uint16_t ) ( Prescaler - 1 );
		pTIM->Init.Period			= ( uint16_t ) Period;
	} while( 0 );

	return Result;
}

// Декларирование рЯда функций. Реализации может и не быть
#define	TIM_DECLARE( __TIM_N__ )									\
	extern void TIM##__TIM_N__##_MspInit( void );					\
	extern void TIM##__TIM_N__##_MspDeInit( void );					\
	extern void TIM##__TIM_N__##_PWM_MspInit( void );				\
	extern void TIM##__TIM_N__##_PWM_MspDeInit( void );				\
	extern void TIM##__TIM_N__##_PeriodElapsedCallback( void );		\
	extern void TIM##__TIM_N__##_OC_DelayElapsedCallback( void );

TIM_DECLARE( 1 );
TIM_DECLARE( 2 );
TIM_DECLARE( 3 );
TIM_DECLARE( 4 );
TIM_DECLARE( 5 );
TIM_DECLARE( 6 );
TIM_DECLARE( 7 );
TIM_DECLARE( 8 );
TIM_DECLARE( 9 );
TIM_DECLARE( 10 );
TIM_DECLARE( 11 );
TIM_DECLARE( 12 );
TIM_DECLARE( 13 );
TIM_DECLARE( 14 );
TIM_DECLARE( 15 );
TIM_DECLARE( 16 );
TIM_DECLARE( 17 );
TIM_DECLARE( 18 );
TIM_DECLARE( 19 );

// Общепроектные низкоуровневые обработчики вызовов от всех таймеров системы

void HAL_TIM_Base_MspInit( TIM_HandleTypeDef *htim )
{
	if( 0 )
		;
#ifdef	TIM1_USE
	else if( htim == &TIM1_hdl )	TIM1_MspInit( );
#endif	// TIM1_USE
		
#ifdef	TIM2_USE
	else if( htim == &TIM2_hdl )	TIM2_MspInit( );
#endif	// TIM2_USE
	
#ifdef	TIM3_USE
	else if( htim == &TIM3_hdl )	TIM3_MspInit( );
#endif	// TIM3_USE
	
#ifdef	TIM4_USE
	else if( htim == &TIM4_hdl )	TIM4_MspInit( );
#endif	// TIM4_USE

#ifdef	TIM5_USE
	else if( htim == &TIM5_hdl )	TIM5_MspInit( );
#endif	// TIM5_USE

#ifdef	TIM6_USE
	else if( htim == &TIM6_hdl )	TIM6_MspInit( );
#endif	// TIM6_USE

#ifdef	TIM7_USE
	else if( htim == &TIM7_hdl )	TIM7_MspInit( );
#endif	// TIM7_USE

#ifdef	TIM8_USE
	else if( htim == &TIM8_hdl )	TIM8_MspInit( );
#endif	// TIM8_USE

#ifdef	TIM9_USE
	else if( htim == &TIM9_hdl )	TIM9_MspInit( );
#endif	// TIM9_USE

#ifdef	TIM10_USE
	else if( htim == &TIM10_hdl )	TIM10_MspInit( );
#endif	// TIM10_USE

#ifdef	TIM11_USE
	else if( htim == &TIM11_hdl )	TIM11_MspInit( );
#endif	// TIM11_USE

#ifdef	TIM12_USE
	else if( htim == &TIM12_hdl )	TIM12_MspInit( );
#endif	// TIM12_USE

#ifdef	TIM13_USE
	else if( htim == &TIM13_hdl )	TIM13_MspInit( );
#endif	// TIM13_USE

#ifdef	TIM14_USE
	else if( htim == &TIM14_hdl )	TIM14_MspInit( );
#endif	// TIM14_USE

#ifdef	TIM15_USE
	else if( htim == &TIM15_hdl )	TIM15_MspInit( );
#endif	// TIM15_USE

#ifdef	TIM16_USE
	else if( htim == &TIM16_hdl )	TIM16_MspInit( );
#endif	// TIM16_USE

#ifdef	TIM17_USE
	else if( htim == &TIM17_hdl )	TIM17_MspInit( );
#endif	// TIM17_USE

#ifdef	TIM18_USE
	else if( htim == &TIM18_hdl )	TIM18_MspInit( );
#endif	// TIM18_USE

#ifdef	TIM19_USE
	else if( htim == &TIM19_hdl )	TIM19_MspInit( );
#endif	// TIM19_USE
}

void HAL_TIM_Base_MspDeInit( TIM_HandleTypeDef *htim )
{
	if( 0 )
		;
#ifdef	TIM1_USE
	else if( htim == &TIM1_hdl )	TIM1_MspDeInit( );
#endif	// TIM1_USE
		
#ifdef	TIM2_USE
	else if( htim == &TIM2_hdl )	TIM2_MspDeInit( );
#endif	// TIM2_USE
	
#ifdef	TIM3_USE
	else if( htim == &TIM3_hdl )	TIM3_MspDeInit( );
#endif	// TIM3_USE
	
#ifdef	TIM4_USE
	else if( htim == &TIM4_hdl )	TIM4_MspDeInit( );
#endif	// TIM4_USE

#ifdef	TIM5_USE
	else if( htim == &TIM5_hdl )	TIM5_MspDeInit( );
#endif	// TIM5_USE

#ifdef	TIM6_USE
	else if( htim == &TIM6_hdl )	TIM6_MspDeInit( );
#endif	// TIM6_USE

#ifdef	TIM7_USE
	else if( htim == &TIM7_hdl )	TIM7_MspDeInit( );
#endif	// TIM7_USE

#ifdef	TIM8_USE
	else if( htim == &TIM8_hdl )	TIM8_MspDeInit( );
#endif	// TIM8_USE

#ifdef	TIM9_USE
	else if( htim == &TIM9_hdl )	TIM9_MspDeInit( );
#endif	// TIM9_USE

#ifdef	TIM10_USE
	else if( htim == &TIM10_hdl )	TIM10_MspDeInit( );
#endif	// TIM10_USE

#ifdef	TIM11_USE
	else if( htim == &TIM11_hdl )	TIM11_MspDeInit( );
#endif	// TIM11_USE

#ifdef	TIM12_USE
	else if( htim == &TIM12_hdl )	TIM12_MspDeInit( );
#endif	// TIM12_USE

#ifdef	TIM13_USE
	else if( htim == &TIM13_hdl )	TIM13_MspDeInit( );
#endif	// TIM13_USE

#ifdef	TIM14_USE
	else if( htim == &TIM14_hdl )	TIM14_MspDeInit( );
#endif	// TIM14_USE

#ifdef	TIM15_USE
	else if( htim == &TIM15_hdl )	TIM15_MspDeInit( );
#endif	// TIM15_USE

#ifdef	TIM16_USE
	else if( htim == &TIM16_hdl )	TIM16_MspDeInit( );
#endif	// TIM16_USE

#ifdef	TIM17_USE
	else if( htim == &TIM17_hdl )	TIM17_MspDeInit( );
#endif	// TIM17_USE

#ifdef	TIM18_USE
	else if( htim == &TIM18_hdl )	TIM18_MspDeInit( );
#endif	// TIM18_USE

#ifdef	TIM19_USE
	else if( htim == &TIM19_hdl )	TIM19_MspDeInit( );
#endif	// TIM19_USE
}


void HAL_TIM_IC_MspInit( TIM_HandleTypeDef *htim )
{
	HAL_TIM_Base_MspInit( htim );
}

void HAL_TIM_IC_MspDeInit( TIM_HandleTypeDef *htim )
{
	HAL_TIM_Base_MspDeInit( htim );
}

void HAL_TIM_OC_MspInit( TIM_HandleTypeDef *htim )
{
	HAL_TIM_Base_MspInit( htim );
}

void HAL_TIM_OC_MspDeInit( TIM_HandleTypeDef *htim )
{
	HAL_TIM_Base_MspDeInit( htim );
}

void HAL_TIM_PWM_MspInit( TIM_HandleTypeDef *htim )
{
	HAL_TIM_Base_MspInit( htim );
}

void HAL_TIM_PWM_MspDeInit( TIM_HandleTypeDef *htim )
{
	HAL_TIM_Base_MspDeInit( htim );
}


void HAL_TIM_PeriodElapsedCallback( TIM_HandleTypeDef *htim )
{
	if( 0 )
		;
#ifdef	TIM1_USE_IRQ
	else if( htim == &TIM1_hdl )	{ TIM1_PeriodElapsedCallback( );	return; }
#endif	// TIM1_USE_IRQ
		
#ifdef	TIM2_USE_IRQ
	else if( htim == &TIM2_hdl )	{ TIM2_PeriodElapsedCallback( );	return; }
#endif	// TIM2_USE_IRQ
	
#ifdef	TIM3_USE_IRQ
	else if( htim == &TIM3_hdl )	{ TIM3_PeriodElapsedCallback( );	return; }
#endif	// TIM3_USE_IRQ
	
#ifdef	TIM4_USE_IRQ
	else if( htim == &TIM4_hdl )	{ TIM4_PeriodElapsedCallback( );	return; }
#endif	// TIM4_USE_IRQ

#ifdef	TIM5_USE_IRQ
	else if( htim == &TIM5_hdl )	{ TIM5_PeriodElapsedCallback( );	return; }
#endif	// TIM5_USE_IRQ

#ifdef	TIM6_USE_IRQ
	else if( htim == &TIM6_hdl )	{ TIM6_PeriodElapsedCallback( );	return; }
#endif	// TIM6_USE_IRQ

#ifdef	TIM7_USE_IRQ
	else if( htim == &TIM7_hdl )	{ TIM7_PeriodElapsedCallback( );	return; }
#endif	// TIM7_USE_IRQ

#ifdef	TIM8_USE_IRQ
	else if( htim == &TIM8_hdl )	{ TIM8_PeriodElapsedCallback( );	return; }
#endif	// TIM8_USE_IRQ

#ifdef	TIM9_USE_IRQ
	else if( htim == &TIM9_hdl )	{ TIM9_PeriodElapsedCallback( );	return; }
#endif	// TIM9_USE_IRQ

#ifdef	TIM10_USE_IRQ
	else if( htim == &TIM10_hdl )	{ TIM10_PeriodElapsedCallback( );	return; }
#endif	// TIM10_USE_IRQ

#ifdef	TIM11_USE_IRQ
	else if( htim == &TIM11_hdl )	{ TIM11_PeriodElapsedCallback( );	return; }
#endif	// TIM11_USE_IRQ

#ifdef	TIM12_USE_IRQ
	else if( htim == &TIM12_hdl )	{ TIM12_PeriodElapsedCallback( );	return; }
#endif	// TIM12_USE_IRQ

#ifdef	TIM13_USE_IRQ
	else if( htim == &TIM13_hdl )	{ TIM13_PeriodElapsedCallback( );	return; }
#endif	// TIM13_USE_IRQ

#ifdef	TIM14_USE_IRQ
	else if( htim == &TIM14_hdl )	{ TIM14_PeriodElapsedCallback( );	return; }
#endif	// TIM14_USE_IRQ

#ifdef	TIM15_USE_IRQ
	else if( htim == &TIM15_hdl )	{ TIM15_PeriodElapsedCallback( );	return; }
#endif	// TIM15_USE_IRQ

#ifdef	TIM16_USE_IRQ
	else if( htim == &TIM16_hdl )	{ TIM16_PeriodElapsedCallback( );	return; }
#endif	// TIM16_USE_IRQ

#ifdef	TIM17_USE_IRQ
	else if( htim == &TIM17_hdl )	{ TIM17_PeriodElapsedCallback( );	return; }
#endif	// TIM17_USE_IRQ

#ifdef	TIM18_USE_IRQ
	else if( htim == &TIM18_hdl )	{ TIM18_PeriodElapsedCallback( );	return; }
#endif	// TIM18_USE_IRQ

#ifdef	TIM19_USE_IRQ
	else if( htim == &TIM19_hdl )	{ TIM19_PeriodElapsedCallback( );	return; }
#endif	// TIM19_USE_IRQ
}


void HAL_TIM_OC_DelayElapsedCallback( TIM_HandleTypeDef *htim )
{
	if( 0 )
		;
#ifdef	TIM1_USE_IRQ_OC
	else if( htim == &TIM1_hdl )	{ TIM1_OC_DelayElapsedCallback( );	return; }
#endif	// TIM1_USE_IRQ_OC
		
#ifdef	TIM2_USE_IRQ_OC
	else if( htim == &TIM2_hdl )	{ TIM2_OC_DelayElapsedCallback( );	return; }
#endif	// TIM2_USE_IRQ_OC
	
#ifdef	TIM3_USE_IRQ_OC
	else if( htim == &TIM3_hdl )	{ TIM3_OC_DelayElapsedCallback( );	return; }
#endif	// TIM3_USE_IRQ_OC
	
#ifdef	TIM4_USE_IRQ_OC
	else if( htim == &TIM4_hdl )	{ TIM4_OC_DelayElapsedCallback( );	return; }
#endif	// TIM4_USE_IRQ_OC

#ifdef	TIM5_USE_IRQ_OC
	else if( htim == &TIM5_hdl )	{ TIM5_OC_DelayElapsedCallback( );	return; }
#endif	// TIM5_USE_IRQ_OC

#ifdef	TIM6_USE_IRQ_OC
	else if( htim == &TIM6_hdl )	{ TIM6_OC_DelayElapsedCallback( );	return; }
#endif	// TIM6_USE_IRQ_OC

#ifdef	TIM7_USE_IRQ_OC
	else if( htim == &TIM7_hdl )	{ TIM7_OC_DelayElapsedCallback( );	return; }
#endif	// TIM7_USE_IRQ_OC

#ifdef	TIM8_USE_IRQ_OC
	else if( htim == &TIM8_hdl )	{ TIM8_OC_DelayElapsedCallback( );	return; }
#endif	// TIM8_USE_IRQ_OC

#ifdef	TIM9_USE_IRQ_OC
	else if( htim == &TIM9_hdl )	{ TIM9_OC_DelayElapsedCallback( );	return; }
#endif	// TIM9_USE_IRQ_OC

#ifdef	TIM10_USE_IRQ_OC
	else if( htim == &TIM10_hdl )	{ TIM10_OC_DelayElapsedCallback( );	return; }
#endif	// TIM10_USE_IRQ_OC

#ifdef	TIM11_USE_IRQ_OC
	else if( htim == &TIM11_hdl )	{ TIM11_OC_DelayElapsedCallback( );	return; }
#endif	// TIM11_USE_IRQ_OC

#ifdef	TIM12_USE_IRQ_OC
	else if( htim == &TIM12_hdl )	{ TIM12_OC_DelayElapsedCallback( );	return; }
#endif	// TIM12_USE_IRQ_OC

#ifdef	TIM13_USE_IRQ_OC
	else if( htim == &TIM13_hdl )	{ TIM13_OC_DelayElapsedCallback( );	return; }
#endif	// TIM13_USE_IRQ_OC

#ifdef	TIM14_USE_IRQ_OC
	else if( htim == &TIM14_hdl )	{ TIM14_OC_DelayElapsedCallback( );	return; }
#endif	// TIM14_USE_IRQ_OC

#ifdef	TIM15_USE_IRQ_OC
	else if( htim == &TIM15_hdl )	{ TIM15_OC_DelayElapsedCallback( );	return; }
#endif	// TIM15_USE_IRQ_OC

#ifdef	TIM16_USE_IRQ_OC
	else if( htim == &TIM16_hdl )	{ TIM16_OC_DelayElapsedCallback( );	return; }
#endif	// TIM16_USE_IRQ_OC

#ifdef	TIM17_USE_IRQ_OC
	else if( htim == &TIM17_hdl )	{ TIM17_OC_DelayElapsedCallback( );	return; }
#endif	// TIM17_USE_IRQ_OC

#ifdef	TIM18_USE_IRQ_OC
	else if( htim == &TIM18_hdl )	{ TIM18_OC_DelayElapsedCallback( );	return; }
#endif	// TIM18_USE_IRQ_OC

#ifdef	TIM19_USE_IRQ_OC
	else if( htim == &TIM19_hdl )	{ TIM19_OC_DelayElapsedCallback( );	return; }
#endif	// TIM19_USE_IRQ_OC
}




#if	defined( TIM1_USE_IRQ ) || defined( TIM1_USE_IRQ_OC )
	void TIM1_IRQHandler( void )	 	{	HAL_TIM_IRQHandler( &TIM1_hdl ); }
#endif	// TIM1_USE_IRQ

#if	defined( TIM2_USE_IRQ ) || defined( TIM2_USE_IRQ_OC )
	void TIM2_IRQHandler( void )	 	{	HAL_TIM_IRQHandler( &TIM2_hdl ); }
#endif	// TIM2_USE_IRQ

#if	defined( TIM3_USE_IRQ ) || defined( TIM3_USE_IRQ_OC )
	void TIM3_IRQHandler( void )	 	{	HAL_TIM_IRQHandler( &TIM3_hdl ); }
#endif	// TIM3_USE_IRQ

#if	defined( TIM4_USE_IRQ ) || defined( TIM4_USE_IRQ_OC )
	void TIM4_IRQHandler( void )	 	{	HAL_TIM_IRQHandler( &TIM4_hdl ); }
#endif	// TIM4_USE_IRQ

#ifdef	TIM5_USE_IRQ
	void TIM5_IRQHandler( void )	 	{	HAL_TIM_IRQHandler( &TIM5_hdl ); }
#endif	// TIM5_USE_IRQ

#ifdef	TIM6_USE_IRQ
	void TIM6_IRQHandler( void )	 	{	HAL_TIM_IRQHandler( &TIM6_hdl ); }
#endif	// TIM6_USE_IRQ

#ifdef	TIM7_USE_IRQ
	void TIM7_IRQHandler( void ) 		{	HAL_TIM_IRQHandler( &TIM7_hdl ); }
#endif	// TIM7_USE_IRQ

#ifdef	TIM8_USE_IRQ
	void TIM8_IRQHandler( void ) 		{	HAL_TIM_IRQHandler( &TIM8_hdl ); }
#endif	// TIM7_USE_IRQ

#ifdef	TIM9_USE_IRQ
	void TIM9_IRQHandler( void )	 	{	HAL_TIM_IRQHandler( &TIM9_hdl ); }
#endif	// TIM9_USE_IRQ

#ifdef	TIM10_USE_IRQ
	void TIM10_IRQHandler( void ) 	{	HAL_TIM_IRQHandler( &TIM10_hdl ); }
#endif	// TIM7_USE_IRQ

#ifdef	TIM11_USE_IRQ
	void TIM11_IRQHandler( void ) 	{	HAL_TIM_IRQHandler( &TIM11_hdl ); }
#endif	// TIM7_USE_IRQ

#ifdef	TIM12_USE_IRQ
	void TIM12_IRQHandler( void )	{	HAL_TIM_IRQHandler( &TIM12_hdl ); }
#endif	// TIM12_USE_IRQ

#ifdef	TIM13_USE_IRQ
	void TIM13_IRQHandler( void )	{	HAL_TIM_IRQHandler( &TIM13_hdl ); }
#endif	// TIM13_USE_IRQ

#ifdef	TIM14_USE_IRQ
	void TIM14_IRQHandler( void )	 {	HAL_TIM_IRQHandler( &TIM14_hdl ); }
#endif	// TIM14_USE_IRQ

#ifdef	TIM15_USE_IRQ
	void TIM15_IRQHandler( void ) 	{	HAL_TIM_IRQHandler( &TIM15_hdl ); }
#endif	// TIM7_USE_IRQ

#ifdef	TIM16_USE_IRQ
	void TIM16_IRQHandler( void )	{	HAL_TIM_IRQHandler( &TIM16_hdl ); }
#endif	// TIM16_USE_IRQ

#ifdef	TIM17_USE_IRQ
	void TIM17_IRQHandler( void )	{	HAL_TIM_IRQHandler( &TIM17_hdl ); }
#endif	// TIM17_USE_IRQ

#ifdef	TIM18_USE_IRQ
	void TIM18_IRQHandler( void )	{	HAL_TIM_IRQHandler( &TIM18_hdl ); }
#endif	// TIM18_USE_IRQ

#ifdef	TIM19_USE_IRQ
	void TIM19_IRQHandler( void )	{	HAL_TIM_IRQHandler( &TIM19_hdl ); }
#endif	// TIM19_USE_IRQ


