// Очередь (кольцевой буфер) с элементом "байт"
// В первую очередь предназначена для взаимодействия с зацикленным DMA,
// поэтому размер органичен 64K - ограничение DMA STM32F
#ifndef	BYTE_QUEUE_H
#define	BYTE_QUEUE_H

#include <stdint.h>		// int types
#include <stdbool.h>	// bool
#include <string.h>		// memcpy(), NULL

// Тип для индексации очереди, а также для размера буфера
typedef uint16_t ByteQueueIndex_t;

// Структура по управлениею очередью
typedef struct ByteQueue_struct
{	// условие (iHead == iTail) означает, что очередь пуста, либо переполнена.
	uint8_t *pBuffer;			// адрес кольцевого буфера
	ByteQueueIndex_t Size;		// размер кольцевого буфера
	ByteQueueIndex_t iHead;		// голова - индекс первого свободного элемента после последнего добавленного элемента
	ByteQueueIndex_t iTail;		// хвост - индекс самого последнего добавленного и невычитанного элемента
} ByteQueue_t;

// Фрагмент в кольцевом буфере
typedef struct ByteQueueFragment_struct
{
	ByteQueueIndex_t iStart;	// Начало фрагмента
	ByteQueueIndex_t Size;		// Размер фрагмента
} ByteQueueFragment_t;

// Макрос для создания буфера и очереди
//#define BYTE_QUEUE_CREATE( __QUEUE_NAME__, __QUEUE_BUFFER_SIZE__ )								\
//	uint8_t a##__QUEUE_NAME__##_Buffer[__QUEUE_BUFFER_SIZE__];									\
//	ByteQueue_t __QUEUE_NAME__ = { a##__QUEUE_NAME__##_Buffer, __QUEUE_BUFFER_SIZE__, 0, 0 };
#define BYTE_QUEUE_CREATE( __QUEUE_NAME__, __QUEUE_BUFFER_SIZE__ )								\
	static __no_init uint8_t a##__QUEUE_NAME__##_Buffer[__QUEUE_BUFFER_SIZE__];					\
	ByteQueue_t __QUEUE_NAME__ = { a##__QUEUE_NAME__##_Buffer, __QUEUE_BUFFER_SIZE__, 0, 0 };

// Проверка допустимости использования pByteQueue
inline bool ByteQueue_Validate( ByteQueue_t *pByteQueue )	{ return ( pByteQueue != NULL ) && ( pByteQueue->pBuffer != NULL ) && ( pByteQueue->Size > 0 ); }

// Очистка буфера
inline void ByteQueue_Clear( ByteQueue_t *pByteQueue )	{ pByteQueue->iHead = pByteQueue->iTail = 0; }

// Добавить байт в голову очереди
inline void ByteQueue_AppendByte( ByteQueue_t *pByteQueue, uint8_t Byte )
{
	pByteQueue->pBuffer[pByteQueue->iHead] = Byte;
	pByteQueue->iHead = ( pByteQueue->iHead + 1 ) % pByteQueue->Size;
}

// Прочитать байт из очереди по индексу с хвоста, не передвигая хвост
inline uint8_t ByteQueue_Peek( ByteQueue_t *pByteQueue, ByteQueueIndex_t Index )
{
/*	ByteQueueIndex_t NewIndex = Index + pByteQueue->iTail;
	if( NewIndex >= pByteQueue->Size )
		NewIndex -= pByteQueue->Size;
	return pByteQueue->pBuffer[ NewIndex ];
*/
	return pByteQueue->pBuffer[ ( Index + pByteQueue->iTail ) % pByteQueue->Size ];
}

// Вернуть размер заполненной области
inline ByteQueueIndex_t ByteQueue_GetSpaceFilled( ByteQueue_t *pByteQueue )
{
	if( pByteQueue->iHead > pByteQueue->iTail )
		return pByteQueue->iHead - pByteQueue->iTail;
	else
		return pByteQueue->Size - pByteQueue->iTail + pByteQueue->iHead;
}

// Вернуть размер свободной области
inline ByteQueueIndex_t ByteQueue_GetSpaceEmpty( ByteQueue_t *pByteQueue )
{
	return pByteQueue->Size - ByteQueue_GetSpaceFilled( pByteQueue );
}

// Передвинуть индекс (хвоста либо головы) на заданную величину
inline void ByteQueue_AdvanceIndexBy( ByteQueue_t *pByteQueue, ByteQueueIndex_t *pIndex, ByteQueueIndex_t Increment )
{
/*	ByteQueueIndex_t NewIndex = *pIndex + Increment;
	if( NewIndex >= pByteQueue->Size )
		NewIndex -= pByteQueue->Size;
	*pIndex = NewIndex;
*/
	*pIndex = ( *pIndex + Increment ) % pByteQueue->Size;
}

// Установить индекс (хвоста либо головы) в заданное значение
inline ByteQueueIndex_t ByteQueue_AdvanceIndexTo( ByteQueue_t *pByteQueue, ByteQueueIndex_t *pIndex, ByteQueueIndex_t NewIndex )
{
	ByteQueueIndex_t Increment;
	if( NewIndex >= *pIndex )
		Increment = NewIndex - *pIndex;
	else
		Increment = pByteQueue->Size + NewIndex - *pIndex;
	*pIndex = NewIndex;
	return Increment;
}

inline void ByteQueue_AdvanceHeadBy( ByteQueue_t *pByteQueue, ByteQueueIndex_t Increment )				{ ByteQueue_AdvanceIndexBy( pByteQueue, &pByteQueue->iHead, Increment ); }
inline void ByteQueue_AdvanceTailBy( ByteQueue_t *pByteQueue, ByteQueueIndex_t Increment )				{ ByteQueue_AdvanceIndexBy( pByteQueue, &pByteQueue->iTail, Increment ); }
inline ByteQueueIndex_t ByteQueue_AdvanceHeadTo( ByteQueue_t *pByteQueue, ByteQueueIndex_t NewIndex )	{ return ByteQueue_AdvanceIndexTo( pByteQueue, &pByteQueue->iHead, NewIndex ); }
inline ByteQueueIndex_t ByteQueue_AdvanceTailTo( ByteQueue_t *pByteQueue, ByteQueueIndex_t NewIndex )	{ return ByteQueue_AdvanceIndexTo( pByteQueue, &pByteQueue->iTail, NewIndex ); }

// Считать произвольный фрагмент кольцевого буфера в линейный буфер, не обращая внимание на голову и хвост
bool ByteQueue_BufferCopyFragment( ByteQueue_t *pByteQueue, uint8_t *pDestination, ByteQueueFragment_t FragmentToCopy );
// Считать часть данных из хвоста кольцевого буфера в линейный буфер, не перемещая хвост
bool ByteQueue_BufferCopyFromTail( ByteQueue_t *pByteQueue, uint8_t *pDestination, ByteQueueIndex_t SizeToCopy );
// Считать часть данных из хвоста кольцевого буфера в линейный буфер и переместить хвост
bool ByteQueue_RemoveFromTail( ByteQueue_t *pByteQueue, uint8_t *pDestination, ByteQueueIndex_t SizeToCopy );

//bool ByteQueue_BufferAppendToHeadl( ByteQueue_t *pByteQueue, uint8_t *pSource, ByteQueueIndex_t Size );
// Добавление производиться черех DMA, ручное добавление пока не востребовано

#endif	// BYTE_QUEUE_H

