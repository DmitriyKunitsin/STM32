// Очередь (кольцевой буфер) с элементом "байт"
#include "ByteQueue.h"		// родной

/*/ Считать часть данных из хвоста кольцевого буфера в линейный буфер, не перемещая хвост
bool ByteQueue_BufferCopyFromTail( ByteQueue_t *pByteQueue, uint8_t *pDestination, ByteQueueIndex_t SizeToCopy )
{
	// Проверка аргументов
	if( ( NULL == pDestination ) || !ByteQueue_Validate( pByteQueue ) )
		return false;
	if( SizeToCopy > ByteQueue_GetSpaceFilled( pByteQueue ) )
		return false;

	if( ( SizeToCopy + pByteQueue->iTail ) < pByteQueue->Size )
	{	// при копировании из хвоста не происходит переход через границу кольцевого буфера
		memcpy( pDestination, &pByteQueue->pBuffer[ pByteQueue->iTail ], SizeToCopy );		// скопировать все за раз
	}
	else
	{	// при копировании из хвоста происходит переход через границу кольцевого буфера
		ByteQueueIndex_t SizeToCopy_1 = pByteQueue->Size - pByteQueue->iTail;
		ByteQueueIndex_t SizeToCopy_2 = SizeToCopy - SizeToCopy_1;
		memcpy( pDestination, &pByteQueue->pBuffer[ pByteQueue->iTail ], SizeToCopy_1 );	// скопировать первую часть - от хвоста до конца кольцевого буфера
		memcpy( pDestination + SizeToCopy_1, &pByteQueue->pBuffer[ 0 ], SizeToCopy_2 );		// скопировать вторую часть - от начала кольцевого буфера
	}

	return true;
}
*/
	
// Считать произвольный фрагмент кольцевого буфера в линейный буфер, не обращая внимание на голову и хвост
bool ByteQueue_BufferCopyFragment( ByteQueue_t *pByteQueue, uint8_t *pDestination, ByteQueueFragment_t FragmentToCopy )
{
	// Проверка аргументов
	if( ( NULL == pDestination ) || !ByteQueue_Validate( pByteQueue ) ||
		( FragmentToCopy.iStart >= pByteQueue->Size ) || ( FragmentToCopy.Size > pByteQueue->Size ) )
		return false;
	if( FragmentToCopy.Size == 0 )
		return true;

	if( ( ( uint32_t ) FragmentToCopy.Size + FragmentToCopy.iStart - 1 ) < pByteQueue->Size )
	{	// при копировании с указанного места не происходит переход через границу кольцевого буфера
		memcpy( pDestination, &pByteQueue->pBuffer[FragmentToCopy.iStart], FragmentToCopy.Size );	// скопировать все за раз
	}
	else
	{	// при копировании с указанного места происходит переход через границу кольцевого буфера
		ByteQueueIndex_t SizeToCopy_1 = pByteQueue->Size - FragmentToCopy.iStart;
		ByteQueueIndex_t SizeToCopy_2 = FragmentToCopy.Size - SizeToCopy_1;
/*		if( !IS_SRAM_MAIN( pDestination ) || !IS_SRAM_MAIN( pDestination + SizeToCopy_1 - 1 ) || !IS_SRAM_MAIN( pDestination + ( SizeToCopy_1 + SizeToCopy_2 ) - 1 ) ||
			 !IS_SRAM_MAIN( &pByteQueue->pBuffer[ FragmentToCopy.iStart ] ) || !IS_SRAM_MAIN( &pByteQueue->pBuffer[ FragmentToCopy.iStart + SizeToCopy_1 - 1 ] ) )
			assert_param( 0 );
*/
		memcpy( pDestination, &pByteQueue->pBuffer[ FragmentToCopy.iStart ], SizeToCopy_1 );	// скопировать первую часть - от с указанного места до конца кольцевого буфера
		memcpy( pDestination + SizeToCopy_1, pByteQueue->pBuffer, SizeToCopy_2 );				// скопировать вторую часть - от начала кольцевого буфера
	}

	return true;
}

// Считать часть данных из хвоста кольцевого буфера в линейный буфер, не перемещая хвост
bool ByteQueue_BufferCopyFromTail( ByteQueue_t *pByteQueue, uint8_t *pDestination, ByteQueueIndex_t SizeToCopy )
{
	// Проверка аргументов
	if( ( NULL == pDestination ) || !ByteQueue_Validate( pByteQueue ) )
		return false;
	if( SizeToCopy > ByteQueue_GetSpaceFilled( pByteQueue ) )
		return false;
	ByteQueueFragment_t FragmentToCopy = { pByteQueue->iTail, SizeToCopy };
	return ByteQueue_BufferCopyFragment( pByteQueue, pDestination, FragmentToCopy );
}

// Считать часть данных из хвоста кольцевого буфера в линейный буфер и переместить хвост
bool ByteQueue_RemoveFromTail( ByteQueue_t *pByteQueue, uint8_t *pDestination, ByteQueueIndex_t SizeToCopy )
{
	if( NULL != pDestination )
	{	// Если поле pDestination инициализировано, необходимо предварительно скопировать данные
		if( !ByteQueue_BufferCopyFromTail( pByteQueue, pDestination, SizeToCopy ) )
			return false;
	}
	// Переместить хвост буфера
	ByteQueue_AdvanceTailBy( pByteQueue, SizeToCopy );
	return true;
}

