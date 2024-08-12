// ���������-��������� ������������� �������� SPI
#include "ProjectConfig.h"			// ������ ���������, ������ ������.
#include "stm32xxxx_hal.h"			// ����� ���������, � �.�. 
#include "platform_common.h"
#include "Common_spi.h"				// ������
#include "common_gpio.h"			// GPIO_Common_Write


// �������������� ���� �������. ���������� ����� � �� ����
#define	SPI_DECLARE( __SPI_N__ )									\
	extern void SPI##__SPI_N__##_MspInit( void );					\
	extern void SPI##__SPI_N__##_MspDeInit( void );					

SPI_DECLARE( 1 );
SPI_DECLARE( 2 );
SPI_DECLARE( 3 );


// =====================================================================================================
// =============================  interface ������� ������������ ������  ===============================
// =====================================================================================================
// ���������� ����������, ��� ������ SPI �����, � ������� ��������������� �������������
void HAL_SPI_MspInit( SPI_HandleTypeDef* hspi )
{
	if (0)
		;
	#if defined (USE_SPI1)
	else if( hspi == getSPI1Handle() )
	{
		SPI1_MspInit( );
	}
	#endif // #if defined (USE_SPI1)
	#if defined (USE_SPI2)
	else if( hspi == getSPI2Handle() )
	{
		SPI2_MspInit( );
	}
	#endif // #if defined (USE_SPI2)
	#if defined (USE_SPI3)
	else if( hspi == getSPI3Handle() )
	{
		SPI3_MspInit( );
	}
	#endif // #if defined (USE_SPI3)
	else
		assert_param( 0 );
}

void HAL_SPI_MspDeInit( SPI_HandleTypeDef* hspi )
{
	if (0)
		;
	#if defined (USE_SPI1)
	else if( hspi == getSPI1Handle() )
	{
		SPI1_MspDeInit( );
	}
	#endif // #if defined (USE_SPI1)
	#if defined (USE_SPI2)
	else if( hspi == getSPI2Handle() )
	{
		SPI2_MspDeInit( );
	}
	#endif // #if defined (USE_SPI2)
	#if defined (USE_SPI3)
	else if( hspi == getSPI3Handle() )
	{
		SPI3_MspDeInit( );
	}
	#endif // #if defined (USE_SPI3)
	else
		assert_param( 0 );
}

// ������� ���������� HAL 
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *phspi)
{
	if (0)
		;
	#if defined (USE_SPI1)
	else if( phspi == getSPI1Handle( ))
	{
		SPI1_TxRxCpltCallback(phspi);
	}
	#endif
	#if defined (USE_SPI2)
	else if( phspi == getSPI2Handle( ) )
	{
		SPI2_TxRxCpltCallback(phspi);
	}
	#endif
	#if defined (USE_SPI3)
	else if( phspi == getSPI3Handle( ) )
	{
		SPI3_TxRxCpltCallback(phspi);
	}
	#endif
}

// ������� ���������� HAL 
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *phspi)
{
	if (0)
		;
	#if defined (USE_SPI1)
	else if( phspi == getSPI1Handle( ))
	{
		assert_param(0);
	}
	#endif
	#if defined (USE_SPI2)
	else if( phspi == getSPI2Handle( ) )
	{
		assert_param(0);
	}
	#endif
	#if defined (USE_SPI3)
	else if( phspi == getSPI3Handle( ) )
	{
		assert_param(0);
	}
	#endif
}


