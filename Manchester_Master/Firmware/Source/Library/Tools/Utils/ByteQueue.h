// ������� (��������� �����) � ��������� "����"
// � ������ ������� ������������� ��� �������������� � ����������� DMA,
// ������� ������ ��������� 64K - ����������� DMA STM32F
#ifndef	BYTE_QUEUE_H
#define	BYTE_QUEUE_H

#include <stdint.h>		// int types
#include <stdbool.h>	// bool
#include <string.h>		// memcpy(), NULL

// ��� ��� ���������� �������, � ����� ��� ������� ������
typedef uint16_t ByteQueueIndex_t;

// ��������� �� ����������� ��������
typedef struct ByteQueue_struct
{	// ������� (iHead == iTail) ��������, ��� ������� �����, ���� �����������.
	uint8_t *pBuffer;			// ����� ���������� ������
	ByteQueueIndex_t Size;		// ������ ���������� ������
	ByteQueueIndex_t iHead;		// ������ - ������ ������� ���������� �������� ����� ���������� ������������ ��������
	ByteQueueIndex_t iTail;		// ����� - ������ ������ ���������� ������������ � ������������� ��������
} ByteQueue_t;

// �������� � ��������� ������
typedef struct ByteQueueFragment_struct
{
	ByteQueueIndex_t iStart;	// ������ ���������
	ByteQueueIndex_t Size;		// ������ ���������
} ByteQueueFragment_t;

// ������ ��� �������� ������ � �������
//#define BYTE_QUEUE_CREATE( __QUEUE_NAME__, __QUEUE_BUFFER_SIZE__ )								\
//	uint8_t a##__QUEUE_NAME__##_Buffer[__QUEUE_BUFFER_SIZE__];									\
//	ByteQueue_t __QUEUE_NAME__ = { a##__QUEUE_NAME__##_Buffer, __QUEUE_BUFFER_SIZE__, 0, 0 };
#define BYTE_QUEUE_CREATE( __QUEUE_NAME__, __QUEUE_BUFFER_SIZE__ )								\
	static __no_init uint8_t a##__QUEUE_NAME__##_Buffer[__QUEUE_BUFFER_SIZE__];					\
	ByteQueue_t __QUEUE_NAME__ = { a##__QUEUE_NAME__##_Buffer, __QUEUE_BUFFER_SIZE__, 0, 0 };

// �������� ������������ ������������� pByteQueue
inline bool ByteQueue_Validate( ByteQueue_t *pByteQueue )	{ return ( pByteQueue != NULL ) && ( pByteQueue->pBuffer != NULL ) && ( pByteQueue->Size > 0 ); }

// ������� ������
inline void ByteQueue_Clear( ByteQueue_t *pByteQueue )	{ pByteQueue->iHead = pByteQueue->iTail = 0; }

// �������� ���� � ������ �������
inline void ByteQueue_AppendByte( ByteQueue_t *pByteQueue, uint8_t Byte )
{
	pByteQueue->pBuffer[pByteQueue->iHead] = Byte;
	pByteQueue->iHead = ( pByteQueue->iHead + 1 ) % pByteQueue->Size;
}

// ��������� ���� �� ������� �� ������� � ������, �� ���������� �����
inline uint8_t ByteQueue_Peek( ByteQueue_t *pByteQueue, ByteQueueIndex_t Index )
{
/*	ByteQueueIndex_t NewIndex = Index + pByteQueue->iTail;
	if( NewIndex >= pByteQueue->Size )
		NewIndex -= pByteQueue->Size;
	return pByteQueue->pBuffer[ NewIndex ];
*/
	return pByteQueue->pBuffer[ ( Index + pByteQueue->iTail ) % pByteQueue->Size ];
}

// ������� ������ ����������� �������
inline ByteQueueIndex_t ByteQueue_GetSpaceFilled( ByteQueue_t *pByteQueue )
{
	if( pByteQueue->iHead > pByteQueue->iTail )
		return pByteQueue->iHead - pByteQueue->iTail;
	else
		return pByteQueue->Size - pByteQueue->iTail + pByteQueue->iHead;
}

// ������� ������ ��������� �������
inline ByteQueueIndex_t ByteQueue_GetSpaceEmpty( ByteQueue_t *pByteQueue )
{
	return pByteQueue->Size - ByteQueue_GetSpaceFilled( pByteQueue );
}

// ����������� ������ (������ ���� ������) �� �������� ��������
inline void ByteQueue_AdvanceIndexBy( ByteQueue_t *pByteQueue, ByteQueueIndex_t *pIndex, ByteQueueIndex_t Increment )
{
/*	ByteQueueIndex_t NewIndex = *pIndex + Increment;
	if( NewIndex >= pByteQueue->Size )
		NewIndex -= pByteQueue->Size;
	*pIndex = NewIndex;
*/
	*pIndex = ( *pIndex + Increment ) % pByteQueue->Size;
}

// ���������� ������ (������ ���� ������) � �������� ��������
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

// ������� ������������ �������� ���������� ������ � �������� �����, �� ������� �������� �� ������ � �����
bool ByteQueue_BufferCopyFragment( ByteQueue_t *pByteQueue, uint8_t *pDestination, ByteQueueFragment_t FragmentToCopy );
// ������� ����� ������ �� ������ ���������� ������ � �������� �����, �� ��������� �����
bool ByteQueue_BufferCopyFromTail( ByteQueue_t *pByteQueue, uint8_t *pDestination, ByteQueueIndex_t SizeToCopy );
// ������� ����� ������ �� ������ ���������� ������ � �������� ����� � ����������� �����
bool ByteQueue_RemoveFromTail( ByteQueue_t *pByteQueue, uint8_t *pDestination, ByteQueueIndex_t SizeToCopy );

//bool ByteQueue_BufferAppendToHeadl( ByteQueue_t *pByteQueue, uint8_t *pSource, ByteQueueIndex_t Size );
// ���������� ������������� ����� DMA, ������ ���������� ���� �� ������������

#endif	// BYTE_QUEUE_H

