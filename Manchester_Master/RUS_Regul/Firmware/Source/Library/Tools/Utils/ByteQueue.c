// ������� (��������� �����) � ��������� "����"
#include "ByteQueue.h"		// ������

/*/ ������� ����� ������ �� ������ ���������� ������ � �������� �����, �� ��������� �����
bool ByteQueue_BufferCopyFromTail( ByteQueue_t *pByteQueue, uint8_t *pDestination, ByteQueueIndex_t SizeToCopy )
{
	// �������� ����������
	if( ( NULL == pDestination ) || !ByteQueue_Validate( pByteQueue ) )
		return false;
	if( SizeToCopy > ByteQueue_GetSpaceFilled( pByteQueue ) )
		return false;

	if( ( SizeToCopy + pByteQueue->iTail ) < pByteQueue->Size )
	{	// ��� ����������� �� ������ �� ���������� ������� ����� ������� ���������� ������
		memcpy( pDestination, &pByteQueue->pBuffer[ pByteQueue->iTail ], SizeToCopy );		// ����������� ��� �� ���
	}
	else
	{	// ��� ����������� �� ������ ���������� ������� ����� ������� ���������� ������
		ByteQueueIndex_t SizeToCopy_1 = pByteQueue->Size - pByteQueue->iTail;
		ByteQueueIndex_t SizeToCopy_2 = SizeToCopy - SizeToCopy_1;
		memcpy( pDestination, &pByteQueue->pBuffer[ pByteQueue->iTail ], SizeToCopy_1 );	// ����������� ������ ����� - �� ������ �� ����� ���������� ������
		memcpy( pDestination + SizeToCopy_1, &pByteQueue->pBuffer[ 0 ], SizeToCopy_2 );		// ����������� ������ ����� - �� ������ ���������� ������
	}

	return true;
}
*/
	
// ������� ������������ �������� ���������� ������ � �������� �����, �� ������� �������� �� ������ � �����
bool ByteQueue_BufferCopyFragment( ByteQueue_t *pByteQueue, uint8_t *pDestination, ByteQueueFragment_t FragmentToCopy )
{
	// �������� ����������
	if( ( NULL == pDestination ) || !ByteQueue_Validate( pByteQueue ) ||
		( FragmentToCopy.iStart >= pByteQueue->Size ) || ( FragmentToCopy.Size > pByteQueue->Size ) )
		return false;
	if( FragmentToCopy.Size == 0 )
		return true;

	if( ( ( uint32_t ) FragmentToCopy.Size + FragmentToCopy.iStart - 1 ) < pByteQueue->Size )
	{	// ��� ����������� � ���������� ����� �� ���������� ������� ����� ������� ���������� ������
		memcpy( pDestination, &pByteQueue->pBuffer[FragmentToCopy.iStart], FragmentToCopy.Size );	// ����������� ��� �� ���
	}
	else
	{	// ��� ����������� � ���������� ����� ���������� ������� ����� ������� ���������� ������
		ByteQueueIndex_t SizeToCopy_1 = pByteQueue->Size - FragmentToCopy.iStart;
		ByteQueueIndex_t SizeToCopy_2 = FragmentToCopy.Size - SizeToCopy_1;
/*		if( !IS_SRAM_MAIN( pDestination ) || !IS_SRAM_MAIN( pDestination + SizeToCopy_1 - 1 ) || !IS_SRAM_MAIN( pDestination + ( SizeToCopy_1 + SizeToCopy_2 ) - 1 ) ||
			 !IS_SRAM_MAIN( &pByteQueue->pBuffer[ FragmentToCopy.iStart ] ) || !IS_SRAM_MAIN( &pByteQueue->pBuffer[ FragmentToCopy.iStart + SizeToCopy_1 - 1 ] ) )
			assert_param( 0 );
*/
		memcpy( pDestination, &pByteQueue->pBuffer[ FragmentToCopy.iStart ], SizeToCopy_1 );	// ����������� ������ ����� - �� � ���������� ����� �� ����� ���������� ������
		memcpy( pDestination + SizeToCopy_1, pByteQueue->pBuffer, SizeToCopy_2 );				// ����������� ������ ����� - �� ������ ���������� ������
	}

	return true;
}

// ������� ����� ������ �� ������ ���������� ������ � �������� �����, �� ��������� �����
bool ByteQueue_BufferCopyFromTail( ByteQueue_t *pByteQueue, uint8_t *pDestination, ByteQueueIndex_t SizeToCopy )
{
	// �������� ����������
	if( ( NULL == pDestination ) || !ByteQueue_Validate( pByteQueue ) )
		return false;
	if( SizeToCopy > ByteQueue_GetSpaceFilled( pByteQueue ) )
		return false;
	ByteQueueFragment_t FragmentToCopy = { pByteQueue->iTail, SizeToCopy };
	return ByteQueue_BufferCopyFragment( pByteQueue, pDestination, FragmentToCopy );
}

// ������� ����� ������ �� ������ ���������� ������ � �������� ����� � ����������� �����
bool ByteQueue_RemoveFromTail( ByteQueue_t *pByteQueue, uint8_t *pDestination, ByteQueueIndex_t SizeToCopy )
{
	if( NULL != pDestination )
	{	// ���� ���� pDestination ����������������, ���������� �������������� ����������� ������
		if( !ByteQueue_BufferCopyFromTail( pByteQueue, pDestination, SizeToCopy ) )
			return false;
	}
	// ����������� ����� ������
	ByteQueue_AdvanceTailBy( pByteQueue, SizeToCopy );
	return true;
}

