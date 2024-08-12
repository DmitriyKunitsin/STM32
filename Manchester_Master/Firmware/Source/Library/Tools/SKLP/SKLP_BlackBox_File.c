// SKLP_BlackBox_File.c
// »нтерфейс доступа к черному €щику по стандарту — Ћ
// –еализаци€ через файл в FatFS
#include "ProjectConfig.h"		// конфиг платформы, конфиг задачи.
#include "stm32xxxx_hal.h"		// дрова периферии
#include "LogFile.h"
#include "SKLP_BlackBox.h"
#include "SKLP_Service.h"
#include "SKLP_Memory.h"
#include "stdio.h"
#include "string.h"
#include "Logger.h"

typedef struct SKLP_BlackBoxMessage_struct
{
	MemoryThreadMessageHeader_t	Header;
	SKLP_Time_t					TimeStampSKLP;
	char						aText[BLACKBOX_MESSAGE_MAXLENGHT];
} SKLP_BlackBoxMessage_t;

typedef struct SKLP_BlackBoxMessageExt_struct
{
	MemoryThreadMessageHeader_t	Header;
	SKLP_Time_t					TimeStampSKLP;
	union
	{
		struct
		{
			SKLP_BlackBoxCallback_t	xCallback;
			uint32_t				CallbackArgument;
			char					aSrcTxt[ BLACKBOX_MESSAGE_MAXLENGHT - sizeof( xCallback ) - sizeof( CallbackArgument ) ];
		};
		char					aText[BLACKBOX_MESSAGE_MAXLENGHT];
	};
} SKLP_BlackBoxMessageExt_t;

static FIL BlackBoxFile;
/*static const char aBlackBoxFileName[] = "BlackBox.txt";
static int BlackBoxErrorCount = 0;
const LogFileHdl_t LogFileHdl_BlackBox = { &BlackBoxFile, aBlackBoxFileName, &BlackBoxErrorCount };
*/
LogFileHdl_t LogFileHdl_BlackBox = { &BlackBoxFile, "BlackBox.txt", 0 };

//  оллбек из задачи MemoryThread_Task() для сохранения записи в „я
static void SKLP_BlackBox_WriteRecordCB( MemoryThreadMessage_t *pMessage )
{
	assert_param( NULL != pMessage );
	assert_param( SKLP_BlackBox_WriteRecordCB == pMessage->Header.xCallback );
	SKLP_BlackBoxMessage_t *pBlackBoxMessage = ( SKLP_BlackBoxMessage_t * ) pMessage;

	static char aRecord[ BLACKBOX_MESSAGE_MAXLENGHT + sizeof( "&DDMMYYHHMMSS" ) ];
	// ‘ормат записи:
	// "&DDMMYYHHMMSSMessage"
	assert_param( MemoryThread_SprintfMutexTake( 5 ) );
	int RecordSize = snprintf(			// snprintf() возвращает количество выведенных символов, без учета '\0'
		aRecord, sizeof( aRecord ),
		"&%02hhu%02hhu%02hhu%02hhu%02hhu%02hhu%s",
		pBlackBoxMessage->TimeStampSKLP.Day,
		pBlackBoxMessage->TimeStampSKLP.Month,
		pBlackBoxMessage->TimeStampSKLP.YearFrom2000,
		pBlackBoxMessage->TimeStampSKLP.Hour,
		pBlackBoxMessage->TimeStampSKLP.Minute,
		pBlackBoxMessage->TimeStampSKLP.Second,
		pBlackBoxMessage->aText );
	MemoryThread_SprintfMutexGive( );
	if( RecordSize > 0 )
		LogFile_WriteRecord( &LogFileHdl_BlackBox, aRecord, RecordSize );

	char aLoggerRecord[50];
	snprintf( aLoggerRecord, sizeof( aLoggerRecord ), "Record to Black Box: %s", aRecord );
	assert_param( Logger_WriteRecord( aLoggerRecord, LOGGER_FLAGS_APPENDTIMESTAMP ) );
}

// ƒобавление записи в черный €щик
// «десь выполняется минимум операций для подготовки сообщения в очередь задачи работы с памятью.
// ¬ремязатратные операции (sprintf(), запись в файл) будут выполняться в низкоприоритетной задаче MemoryThread_Task()
/*bool SKLP_BlackBox_WriteRecord( char const *pText )
{
	// ѕроверки перед записью
	assert_param( NULL != pText );
	if( !SKLP_FlagsModule.DataSaving )
		return true;									// в неавтономном режиме запрещена запись в „я
		
	// —оставить сообщение для задачи MemoryThread_Task()
	SKLP_BlackBoxMessage_t Message;
	assert_param( sizeof( Message ) <= sizeof( MemoryThreadMessage_t ) );
	ATOMIC_WRITE( Message.TimeStampSKLP, SKLP_Time );
	strncpy( Message.aText, pText, sizeof( Message.aText ) );
	Message.Header.xCallback = SKLP_BlackBox_WriteRecordCB;
	// “олько для записи в „я сообщений о собственной перезагрузке: необходимо дождаться инициализации фйловой системы, чтобы сообщение не потреялось.
	int32_t Flags = 0;
	if( ( SKLP_BLACKBOX_TAG_RESET_UNKNOWN[0] == Message.aText[0] ) &&
		( SKLP_BLACKBOX_TAG_RESET_UNKNOWN[1] == Message.aText[1] ) )
		Flags |= MEMORYTHREAD_FLAGS_WAITFORFS;
	// ќтправить сообщение в очередь задачи MemoryThread_Task()
	return MemoryThread_AppendMessage( ( MemoryThreadMessage_t * ) &Message, Flags );
}
*/

// ƒобавление записи в черный €щик, присвоить указанное время
// «десь выполняется минимум операций для подготовки сообщения в очередь задачи работы с памятью.
// ¬ремязатратные операции (sprintf(), запись в файл) будут выполняться в низкоприоритетной задаче MemoryThread_Task()
bool SKLP_BlackBox_WriteRecordTimestamp( char const *pText, SKLP_Time_t Timestamp )
{
	// ѕроверки перед записью
	assert_param( NULL != pText );
	if( !SKLP_FlagsModule.DataSaving )
		return true;									// в неавтономном режиме запрещена запись в „я
		
	// —оставить сообщение для задачи MemoryThread_Task()
	SKLP_BlackBoxMessage_t Message;
	assert_param( sizeof( Message ) <= sizeof( MemoryThreadMessage_t ) );
	Message.TimeStampSKLP = Timestamp;
	strncpy( Message.aText, pText, sizeof( Message.aText ) );
	Message.Header.xCallback = SKLP_BlackBox_WriteRecordCB;
	// “олько для записи в „я сообщений о собственной перезагрузке: необходимо дождаться инициализации фйловой системы, чтобы сообщение не потреялось.
	int32_t Flags = 0;
	if( ( SKLP_BLACKBOX_TAG_RESET_UNKNOWN[0] == Message.aText[0] ) &&
		( SKLP_BLACKBOX_TAG_RESET_UNKNOWN[1] == Message.aText[1] ) )
		Flags |= MEMORYTHREAD_FLAGS_WAITFORFS;
	// ќтправить сообщение в очередь задачи MemoryThread_Task()
	return MemoryThread_AppendMessage( ( MemoryThreadMessage_t * ) &Message, Flags );
}


// ƒобавление записи в черный €щик. ¬ качестве метки используется текущее системное время.
bool SKLP_BlackBox_WriteRecord( char const *pText )
{
	SKLP_Time_t Timestamp;
	ATOMIC_WRITE( Timestamp, SKLP_Time );
	return SKLP_BlackBox_WriteRecordTimestamp( pText, Timestamp );
}

/*
static void SKLP_BlackBox_WriteRecordExtCB( MemoryThreadMessage_t *pMessage )
{
	assert_param( NULL != pMessage );
	assert_param( SKLP_BlackBox_WriteRecordCB == pMessage->Header.xCallback );
	SKLP_BlackBoxMessageExt_t *pBlackBoxMessage = ( SKLP_BlackBoxMessageExt_t * ) pMessage;
	assert_param( NULL != pBlackBoxMessage->xCallback );
	pBlackBoxMessage->xCallback( pBlackBoxMessage->aSrcTxt, pBlackBoxMessage->CallbackArgument, pBlackBoxMessage->aText );
	SKLP_BlackBox_WriteRecordCB( pMessage );
}


bool SKLP_BlackBox_WriteRecordExt( char const *pText, SKLP_BlackBoxCallback_t xCallback, uint32_t CallbackArgument )
{
	if( !SKLP_FlagsModule.DataSaving )
		return true;
	SKLP_BlackBoxMessageExt_t Message;
	assert_param( sizeof( Message ) <= sizeof( MemoryThreadMessage_t ) );
	ATOMIC_WRITE( Message.TimeStampSKLP, SKLP_Time );
	Message.xCallback = xCallback;
	Message.CallbackArgument = CallbackArgument;
	strncpy( Message.aSrcTxt, pText, sizeof( Message.aSrcTxt ) );
	Message.Header.xCallback = SKLP_BlackBox_WriteRecordExtCB;
	return MemoryThread_AppendMessage( ( MemoryThreadMessage_t * ) &Message, 0 );
}
*/

/*bool SKLP_BlackBox_Clear( void )
{
	LogFileMessage_t LogFileMessage = { { LogFile_ClearCB, 0 }, &LogFileHdl_BlackBox };
	return MemoryThread_AppendMessage( ( MemoryThreadMessage_t * ) &LogFileMessage, false );
}*/
