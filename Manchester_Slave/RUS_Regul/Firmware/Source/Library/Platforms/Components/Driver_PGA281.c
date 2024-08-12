// Driver_PGA281.c
// ������� PGA281, ����������������� ��������� � ��������������� ��������� 0.125-128(176)
#include "ProjectConfig.h"		// ������ ���������
#include "stm32xxxx_hal.h"		// ����� ���������
#include "Driver_PGA281.h"		// ������
#include "Common_gpio.h"

// ��� �������� PGA281
//static const float aPGA281_Gains[] = { 0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 32.0f, 64.0f, 128.0f };
static const float aPGA281_Gains[] =
//	G4=0	G4=1
{
	0.125f,	0.172f,		// 0	1
	0.25f,	0.344f,		// 2	3
	0.5f,	0.688f,		// 4	5
	1.0f,	1.375f,		// 6	7
	2.0f,	2.75f,		// 8	9
	4.0f,	5.5f,		// 10	11
	8.0f,	11.0f,		// 12	13
	16.0f,	22.0f,		// 14	15
	32.0f,	44.0f,		// 16	17
	64.0f,	88.0f,		// 18	19
	128.0f,	176.0f,		//	20	21
};

// ������������� - �������� ����� ������������� ����������. iG � ����������� � ������������ �� PGA281
bool PGA281_Init( PGA281_Hdl_t *pPGA_Hdl, GPIO_CommonIndex_t iG0, GPIO_CommonIndex_t iG1, GPIO_CommonIndex_t iG2, GPIO_CommonIndex_t iG3, GPIO_CommonIndex_t iG4 )
{
	assert_param( NULL != pPGA_Hdl );
	pPGA_Hdl->aiGPIO_G[0] = iG4;
	pPGA_Hdl->aiGPIO_G[1] = iG0;
	pPGA_Hdl->aiGPIO_G[2] = iG1;
	pPGA_Hdl->aiGPIO_G[3] = iG2;
	pPGA_Hdl->aiGPIO_G[4] = iG3;
	PGA281_Command( pPGA_Hdl, PGA_Command_SetMin );
	return true;
}

// ���������� ������� ������
PGA_Gain_t PGA281_Command( PGA281_Hdl_t *pPGA_Hdl, PGA_Command_t Command )
{
	assert_param( NULL != pPGA_Hdl );
	int iGainInc;
	int iGainMax;
	if( iGPIO_NULL == pPGA_Hdl->aiGPIO_G[0] )
	{
		iGainInc = 2;
		iGainMax = SIZEOFARRAY( aPGA281_Gains ) - 2;
	}
	else
	{
		iGainInc = 1;
		iGainMax = SIZEOFARRAY( aPGA281_Gains ) - 1;
	}
	int iNewGain;
	switch( Command )
	{
	// ���������� ����� ��������
	case PGA_Command_Set:			// ���������� ������� ��������
		if( ( pPGA_Hdl->iGain < 0 ) || ( pPGA_Hdl->iGain > iGainMax ) )
			iNewGain = 0;
		else
			iNewGain = pPGA_Hdl->iGain;
		break;
	case PGA_Command_SetMin:		// ���������� ����������� ��������
		iNewGain = 0;
		break;
	case PGA_Command_SetMax:		// ���������� ������������ ��������
		iNewGain = iGainMax;
		break;
	case PGA_Command_Inc:			// ��������� �������� �� ���� �����
		iNewGain = pPGA_Hdl->iGain;
		if( iNewGain < iGainMax )
			iNewGain += iGainInc;
		break;
	case PGA_Command_Dec:			// ��������� �������� �� ���� �����
		iNewGain = pPGA_Hdl->iGain;
		if( iNewGain > 0 )
			iNewGain -= iGainInc;
		break;

	// ������� �������� ��������
	case PGA_Command_GetCurrent:	// ������� ������� ��������
		return aPGA281_Gains[pPGA_Hdl->iGain];
	case PGA_Command_GetMin:		// ������� ����������� ��������
		return aPGA281_Gains[0];
	case PGA_Command_GetMax:		// ������� ������������ ��������
		return aPGA281_Gains[iGainMax];
		
	default:
		assert_param( 0 );
	}
	// ������������� ���������
	assert_param( ( iNewGain >= 0 ) && ( iNewGain <= iGainMax ) );
	if( 2 == iGainInc )
		assert_param( 0 == ( iNewGain % 2 ) );
	pPGA_Hdl->iGain = iNewGain;

	// ���������� ����� ��������
	ENTER_CRITICAL_SECTION( );
	for( int i = 0; i < SIZEOFARRAY( pPGA_Hdl->aiGPIO_G ); i++, iNewGain >>= 1 )
		GPIO_Common_Write( pPGA_Hdl->aiGPIO_G[i], ( ( iNewGain & 1 ) ? GPIO_PIN_SET : GPIO_PIN_RESET ) );
	EXIT_CRITICAL_SECTION( );

	// ������� ������� ��������
	return aPGA281_Gains[pPGA_Hdl->iGain];
}

