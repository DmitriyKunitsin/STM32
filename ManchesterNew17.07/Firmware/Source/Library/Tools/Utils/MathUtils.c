#include "ProjectConfig.h"		// конфиг платформы
#include "stm32xxxx_hal.h"		// дрова периферии
#include "MathUtils.h"			// родной
#include <string.h>				// memset()	

/*void DSP_IIR_Init_f32( DSP_IIR_Instace_f32_t *pInstatnce, uint8_t Order, float *pZnum, float *pZden, float *pStates, float InitValue )
{
	assert_param( Order > 0 );
	assert_param( ( NULL != pInstatnce ) || ( NULL != pZnum ) || ( NULL != pZden ) || ( NULL != pStates ) );
	pInstatnce->Order = Order;
	pInstatnce->pZnum = pZnum;
	pInstatnce->pZden = pZden;
	pInstatnce->pStates = pStates;
	for( int i = 0; i < Order; i++ )
	{
		pInstatnce->pStates[i] = InitValue;
	}
}*/

// ƒобавить первое значение в фильтр, с проверкой аргументов
float DSP_IIR_AppendFirst_f32( DSP_IIR_Instace_f32_t const *pInstatnce, float NewValue )
{
	assert_param( NULL != pInstatnce );
	assert_param( ( NULL != pInstatnce->pZnum ) || ( NULL != pInstatnce->pZden ) || ( NULL != pInstatnce->pStates ) );
	assert_param( pInstatnce->Order > 0 );
	float InitState = NewValue * pInstatnce->InitCoeff;
	for( int i = 0; i < pInstatnce->Order; i++ )
		pInstatnce->pStates[i] = InitState;		// последний элемент мусорный и не заполняется, см. DSP_IIR_Append_f32()
	return NewValue;
}

// ƒобавить очередное значение в фильтр
/*float DSP_IIR_Append_f32( DSP_IIR_Instace_f32_t const *pInstatnce, float NewValue )
{
	assert_param( NULL != pInstatnce );
	float SummNum = 0, SummDen = 0;
	for( int i = 0; i < pInstatnce->Order; i++ )
	{
		SummNum += pInstatnce->pStates[i] * pInstatnce->pZnum[i];
		SummDen += pInstatnce->pStates[i] * pInstatnce->pZden[i];
		pInstatnce->pStates[i] = pInstatnce->pStates[ i + 1 ];
	}
	NewValue -= SummDen;
	pInstatnce->pStates[ pInstatnce->Order - 1 ] = NewValue;
	SummNum += pInstatnce->pZnum[ pInstatnce->Order ] * NewValue;
	return SummNum;
}*/

// ƒобавить очередное значение в фильтр
// јргументы не проверяются из-за существенных наклодных расходов при частом вызове.
// ѕредполагается, что сначала вызывается DSP_IIR_AppendFirst_f32(), и там один раз проверяются аргументы.
float DSP_IIR_Append_f32( DSP_IIR_Instace_f32_t const *pInstatnce, float NewValue )
{
	float SummNum = 0, SummDen = 0;
	int Order = pInstatnce->Order;
	float *pStates = pInstatnce->pStates;
	float *pZnum = pInstatnce->pZnum;
	float *pZden = pInstatnce->pZden;
	for( int i = 0; i < Order; i++ )
	{
		float StateCurr = pStates[i];
		SummNum += StateCurr * pZnum[i];
		SummDen += StateCurr * pZden[i];
		pStates[i] = pStates[i+1];			// !!! при последней итерации - выход за пределы массива pStates[Order], по-этому pStates должен быть объявлен размером [Order+1]
	}
	NewValue -= SummDen;
	pStates[ Order - 1 ] = NewValue;
	SummNum += pZnum[ Order ] * NewValue;
	return SummNum;
}

// ****************************************************************
// ‘ильтр с конечной импульсной характеристикой
// по целочисленным значениям uint8_t и суммой uint16_t
void DSP_FIR_UI8_UI16_Setup( DSP_FIR_UI8_UI16_t *pFIR, uint8_t *pBuffer, uint16_t Depth, uint8_t InitValue )
{
	assert_param( NULL != pFIR );
	assert_param( NULL != pBuffer );
	assert_param( 0 != Depth );

	pFIR->Depth		= Depth;
	pFIR->Index		= 0;
	pFIR->pBuffer	= pBuffer;
	uint16_t Summ = 0;
	if( 0 == InitValue )
		memset( pFIR->pBuffer, 0, Depth * sizeof( *pFIR->pBuffer ) );
	else
		for( uint16_t i = 0; i < Depth; i++ )
		{
			pFIR->pBuffer[i] = InitValue;
			Summ += InitValue;
		}
	pFIR->Summ = Summ;
}

uint16_t DSP_FIR_UI8_UI16_Append( DSP_FIR_UI8_UI16_t *pFIR, uint8_t NewValue )
{
	assert_param( NULL != pFIR );
	assert_param( NULL != pFIR->pBuffer );
	assert_param( pFIR->Index < pFIR->Depth );

	uint8_t *pCurrValue = &pFIR->pBuffer[pFIR->Index];
	uint8_t PrevValue = *pCurrValue;
	*pCurrValue = NewValue;
	pFIR->Summ += NewValue - PrevValue;
	pFIR->Index = ( pFIR->Index + 1 ) % pFIR->Depth;

	return pFIR->Summ;
}

// ****************************************************************
// ‘ильтр с конечной импульсной характеристикой
// по целочисленным значениям uint16_t и суммой uint32_t
void DSP_FIR_UI16_UI32_Setup( DSP_FIR_UI16_UI32_t *pFIR, uint16_t *pBuffer, uint16_t Depth, uint16_t InitValue )
{
	assert_param( NULL != pFIR );
	assert_param( NULL != pBuffer );
	assert_param( 0 != Depth );

	pFIR->Depth		= Depth;
	pFIR->Index		= 0;
	pFIR->pBuffer	= pBuffer;
	uint32_t Summ = 0;
	if( 0 == InitValue )
		memset( pFIR->pBuffer, 0, Depth * sizeof( *pFIR->pBuffer ) );
	else
		for( uint16_t i = 0; i < Depth; i++ )
		{
			pFIR->pBuffer[i] = InitValue;
			Summ += InitValue;
		}
	pFIR->Summ = Summ;
}

uint32_t DSP_FIR_UI16_UI32_Append( DSP_FIR_UI16_UI32_t *pFIR, uint16_t NewValue )
{
	assert_param( NULL != pFIR );
	assert_param( NULL != pFIR->pBuffer );
	assert_param( pFIR->Index < pFIR->Depth );

	uint16_t *pCurrValue = &pFIR->pBuffer[pFIR->Index];
	uint16_t PrevValue = *pCurrValue;
	*pCurrValue = NewValue;
	pFIR->Summ += NewValue - PrevValue;
	pFIR->Index = ( pFIR->Index + 1 ) % pFIR->Depth;

	return pFIR->Summ;
}

// ****************************************************************
// ‘ильтр с конечной импульсной характеристикой
// по целочисленным значениям int16_t и суммой int32_t
void DSP_FIR_SI16_SI32_Setup( DSP_FIR_SI16_SI32_t *pFIR, int16_t *pBuffer, uint16_t Depth, int16_t InitValue )
{
	assert_param( NULL != pFIR );
	assert_param( NULL != pBuffer );
	assert_param( 0 != Depth );

	pFIR->Depth		= Depth;
	pFIR->Index		= 0;
	pFIR->pBuffer	= pBuffer;
	int32_t Summ = 0;
	if( 0 == InitValue )
		memset( pFIR->pBuffer, 0, Depth * sizeof( *pFIR->pBuffer ) );
	else
		for( uint16_t i = 0; i < Depth; i++ )
		{
			pFIR->pBuffer[i] = InitValue;
			Summ += InitValue;
		}
	pFIR->Summ = Summ;
}

int32_t DSP_FIR_SI16_SI32_Append( DSP_FIR_SI16_SI32_t *pFIR, int16_t NewValue )
{
	assert_param( NULL != pFIR );
	assert_param( NULL != pFIR->pBuffer );
	assert_param( pFIR->Index < pFIR->Depth );

	int16_t *pCurrValue = &pFIR->pBuffer[pFIR->Index];
	int16_t PrevValue = *pCurrValue;
	*pCurrValue = NewValue;
	pFIR->Summ += NewValue - PrevValue;
	pFIR->Index = ( pFIR->Index + 1 ) % pFIR->Depth;

	return pFIR->Summ;
}

// ****************************************************************
// ‘ильтр с конечной импульсной характеристикой
// по целочисленным значениям int32_t и суммой int32_t
void DSP_FIR_SI32_SI32_Setup( DSP_FIR_SI32_SI32_t *pFIR, int32_t *pBuffer, uint16_t Depth, int32_t InitValue )
{
	assert_param( NULL != pFIR );
	assert_param( NULL != pBuffer );
	assert_param( 0 != Depth );

	pFIR->Depth		= Depth;
	pFIR->Index		= 0;
	pFIR->pBuffer	= pBuffer;
	int32_t Summ = 0;
	if( 0 == InitValue )
		memset( pFIR->pBuffer, 0, Depth * sizeof( *pFIR->pBuffer ) );
	else
		for( uint16_t i = 0; i < Depth; i++ )
		{
			pFIR->pBuffer[i] = InitValue;
			Summ += InitValue;
		}
	pFIR->Summ = Summ;
}

int32_t DSP_FIR_SI32_SI32_Append( DSP_FIR_SI32_SI32_t *pFIR, int32_t NewValue )
{
	assert_param( NULL != pFIR );
	assert_param( NULL != pFIR->pBuffer );
	assert_param( pFIR->Index < pFIR->Depth );

	int32_t *pCurrValue = &pFIR->pBuffer[pFIR->Index];
	int32_t PrevValue = *pCurrValue;
	*pCurrValue = NewValue;
	pFIR->Summ += NewValue - PrevValue;
	pFIR->Index = ( pFIR->Index + 1 ) % pFIR->Depth;

	return pFIR->Summ;
}

