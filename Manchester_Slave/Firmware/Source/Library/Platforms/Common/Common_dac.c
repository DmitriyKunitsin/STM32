// ���������-��������� ������������� �������� DAC
#include "ProjectConfig.h"			// ������ ���������, ������ ������.
#include "stm32xxxx_hal.h"			// ����� ���������, � �.�. 
#include "platform_common.h"
#include "Common_dac.h"				// ������

// ����������� ��� ����������� � ������������ ������ ���������� ����������� �������
// ��� ������������� ��������� ��������� ����������
//	�������� ����� ������� ��������, ������ ������� ������� ��������� �� ����������,
//	� ��� ���������� ��������� ����� ������ common_dac
#if 1
#if	defined( USE_DAC_CHANNEL_1 )
	DAC_HandleTypeDef DAC1_hdl __PLACE_AT_RAM_CCM__;
#endif	// USE_DAC_CHANNEL_1
#if	defined( USE_DAC_CHANNEL_2 )
	DAC_HandleTypeDef DAC2_hdl __PLACE_AT_RAM_CCM__;
#endif	// USE_DAC_CHANNEL_2
#endif // #if 0

// �������������� ���� �������. ���������� ����� � �� ����
#ifdef	USE_DAC_CHANNEL_1
extern void DAC1_MspInit( void );					
extern void DAC1_MspDeInit( void );					
#endif	// USE_DAC_CHANNEL_1
#ifdef	USE_DAC_CHANNEL_2
extern void DAC2_MspInit( void );					
extern void DAC2_MspDeInit( void );					
#endif	// USE_DAC_CHANNEL_2

// ������������� �������������� ����������� ������� �� ���� �������� �������

void HAL_DAC_MspInit( DAC_HandleTypeDef *hdac )
{
	if( 0 )
		;
#ifdef	USE_DAC_CHANNEL_1
	else if( hdac == &DAC1_hdl )
		DAC1_MspInit( );
#endif	// USE_DAC_CHANNEL_1
		
#ifdef	USE_DAC_CHANNEL_2
	else if( hdac == &DAC2_hdl )
		DAC2_MspInit( );
#endif	// USE_DAC_CHANNEL_2
}

void HAL_DAC_MspDeInit( DAC_HandleTypeDef *hdac )
{
	if( 0 )
		;
#ifdef	USE_DAC_CHANNEL_1
	else if( hdac == &DAC1_hdl )
		DAC1_MspDeInit( );
#endif	// USE_DAC_CHANNEL_1
	
#ifdef	USE_DAC_CHANNEL_2
	else if( hdac == &DAC2_hdl )
		DAC2_MspDeInit( );
#endif	// USE_DAC_CHANNEL_2
}


#ifdef	USE_DAC_CHANNEL_1_IRQ
	void DAC1_IRQHandler( void )	 	{	HAL_DAC_IRQHandler( &DAC1_hdl ); }
#endif	// USE_DAC_CHANNEL_1_IRQ

#ifdef	USE_DAC_CHANNEL_2_IRQ
	void DAC2_IRQHandler( void )	 	{	HAL_DAC_IRQHandler( &DAC2_hdl ); }
#endif	// USE_DAC_CHANNEL_2_IRQ


