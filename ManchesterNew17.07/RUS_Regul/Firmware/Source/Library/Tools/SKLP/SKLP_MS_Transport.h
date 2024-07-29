// SKLP_MS_Transport.h
// Протокол последовательной шины СКЛ [Master/Slave], прием и разбор пакетов на шине.
// Описание формата запросов и ответов.
#ifndef	SKLP_MS_TRANSPORT_H
#define	SKLP_MS_TRANSPORT_H

#define	SKLP_RX_BUFFER_SIZE					512		// Размер приемного буфера UART

// Положение полей в пакете СКЛ
#define	SKLP_OFFSET_START					0		// Смещение поля Старта
#define	SKLP_OFFSET_SIZE					1		// Смещение поля Размера
#define	SKLP_OFFSET_ADDRESS					2		// Смещение поля Адреса модуля			только пакет запроса
#define	SKLP_OFFSET_COMMAND					3		// Смещение поля Команды				только пакет запроса
#define	SKLP_OFFSET_DATA_QUERY				4		// Смещение поля Данных					только пакет запроса
#define	SKLP_OFFSET_DATA_ANSWER				2		// Смещение поля Данных					только пакет ответа
#define	SKLP_OFFSET_CRC8( _PACKET_ )		( SKLP_SIZE( _PACKET_ ) + SKLP_SIZE_TRIM - 1 )	// Смещение поля CRC8

// Содержание полей в пакете СКЛ
#define	SKLP_START_QUERY					'@'		// 0x40	Поле Старт - Запрос
#define	SKLP_START_ANSWER					'#'		// 0x23	Поле Старт - Ответ
#define	SKLP_START_QUERY_EXT				'<'		// 0x3C	Поле Старт - Запрос (расширенный протокол)
#define	SKLP_START_ANSWER_EXT				'>'		// 0x3E	Поле Старт - Ответ (расширенный протокол)

// Расчет размера пакета и поля данных в пакете
// _PACKET_ - адрес начала буфера с пакетом
#define	SKLP_SIZE( _PACKET_ )				( ( _PACKET_ )[SKLP_OFFSET_SIZE] )				// Значение поля Размера
#define	SKLP_SIZE_MIN_QUERY					2		// Поле Размера запроса, минимальное - только адрес и команда
#define	SKLP_SIZE_MIN_ANSWER				0		// Поле Размера ответа, минимальное
#define	SKLP_SIZE_MAX						255		// Поле Размера, максимальное
#define	SKLP_SIZE_TRIM						3		// Поле Размера меньше, чем размер всего пакета на величину TRIM (вычитаютсЯ полЯ Старта, Размера, CRC8)
#define	SKLP_SIZE_PACKET( _PACKET_ )		( SKLP_SIZE( _PACKET_ ) + SKLP_SIZE_TRIM )		// Полный размер пакета
#define	SKLP_SIZE_PACKET_MAX				( SKLP_SIZE_MAX + SKLP_SIZE_TRIM )				// Максимально возможный размер пакета
#define	SKLP_SIZE_PACKET_MIN_QUERY			( SKLP_SIZE_MIN_QUERY + SKLP_SIZE_TRIM )		// Минимальный возможный размер пакета (запрос)
#define	SKLP_SIZE_PACKET_MIN_ANSWER			( SKLP_SIZE_MIN_ANSWER + SKLP_SIZE_TRIM )		// Минимальный возможный размер пакета (ответ)
#define	SKLP_SIZE_DATA_MAX_QUERY			( SKLP_SIZE_MAX - SKLP_SIZE_MIN_QUERY )			// Максимально возможный размер поля данных в запросе
#define	SKLP_SIZE_DATA_MAX_ANSWER			( SKLP_SIZE_MAX - SKLP_SIZE_MIN_ANSWER )		// Максимально возможный размер поля данных в ответе
#define	SKLP_SIZE_DATA_QUERY( _PACKET_ )	( SKLP_SIZE( _PACKET_ ) - SKLP_SIZE_MIN_QUERY )	// Размер поля данных в запросе
#define	SKLP_SIZE_DATA_ANSWER( _PACKET_ )	( SKLP_SIZE( _PACKET_ ) - SKLP_SIZE_MIN_ANSWER )// Размер поля данных в ответе

// Некоторые сигнатуры
#define	SKLP_SIGNATURE_COMMAND_WORKTIME		'T'			// используетсЯ в командах чтениЯ/записи счетчика отработанного времени
#define	SKLP_SIGNATURE_COMMAND_NVM_SET		"notice"	// используетсЯ в командах записи калибровочных коэффициентов
#define	SKLP_SIGNATURE_SAVING_START			"START"		// заголовок блоков, сохранЯемых в памЯть модулей
#define	SKLP_SIGNATURE_SAVING_STOP			"STOP"		// аппендикс после последнего блока, сохраненного в памЯть

// Адреса различных модулей
#define	SKLP_ADDRESS_BROADCAST				0x00	// Широковещательный
#define	SKLP_ADDRESS_DEPTHMETER				0x01	// Глубиномер
#define	SKLP_ADDRESS_BKZ					0x10	// БКЗ
#define	SKLP_ADDRESS_MPI_500				0x10	// МПИ	Модуль памяти и измерений длЯ комплекса ЛУЧ.500. !!Пересекается с БКЗ, но отличается идентификатором DeviceType
#define	SKLP_ADDRESS_RUS_Tele				0x1A	// РУС, контроллер телеметрии
#define	SKLP_ADDRESS_RUS_Pump				0x1B	// РУС, контроллер электропривода (широковещательный)
#define	SKLP_ADDRESS_RUS_Pump0				0x1C	// РУС, контроллер электропривода насоса 0°
#define	SKLP_ADDRESS_RUS_Pump1				0x1D	// РУС, контроллер электропривода насоса 120°
#define	SKLP_ADDRESS_RUS_Pump2				0x1E	// РУС, контроллер электропривода насоса 240°
#define	SKLP_ADDRESS_RUS_Incl				SKLP_ADDRESS_INCL
#define	SKLP_ADDRESS_RUS_GK					SKLP_ADDRESS_VIKPB_GK
#define	SKLP_ADDRESS_BK2					0x20	// БK		Боковой каротаж
#define	SKLP_ADDRESS_BK						0x22	// БK		Боковой каротаж
#define	SKLP_ADDRESS_BKS					0x24	// БKС		Боковой каротаж (сканирующий)
#define	SKLP_ADDRESS_MRGK_REZ				0x30	// МРГК	Канал резистивиметра
#define	SKLP_ADDRESS_IK						0x40	// ИК
#define	SKLP_ADDRESS_VMKZ					0x50	// ВЭМКЗ
#define	SKLP_ADDRESS_VIKPB					0x55	// ВИКПБ
#define	SKLP_ADDRESS_VIKPB_GK				0x56	// ВИКПБ ГК
#define	SKLP_ADDRESS_MUP					0x62	// МУП		Модуль управлениЯ пульстатором
#define	SKLP_ADDRESS_NNKT					0x71	// ННКт	Нейтрон-нейтронный каротаж
#define	SKLP_ADDRESS_MRGK_GK				0x72	// МРГК	Канал ГК
#define	SKLP_ADDRESS_NNKT2					0x73	// ННКт2	Нейтрон-нейтронный каротаж, на модеме
#define	SKLP_ADDRESS_GGKP					0x80	// ГГК-П	Гамма-гамма-плотностной
#define	SKLP_ADDRESS_GGLP					0x81	// ГГЛП	Гамма-гамма-лито-плотностной
#define	SKLP_ADDRESS_GGKP_2					0x82	// ГГП	!!! уточнить, кто есть кто!
#define	SKLP_ADDRESS_GGLP_SADC				0x88	// ГГК-ЛП-АЦП	спектрометрический АЦП длЯ ГГК-ЛП
#define	SKLP_ADDRESS_MI						0x90	// МИ
#define	SKLP_ADDRESS_MI_GK					0x91	// МИ		Канал ГК
#define	SKLP_ADDRESS_INCL					0x99	// ИНКЛ	Инклинометр (Василий Литвинов)
#define	SKLP_ADDRESS_AK						0xA0	// АК		Модуль акустического каротажа
#define	SKLP_ADDRESS_AKD					0xAF	// АКД		Контроллер памяти АКД (акустического каротажа по эмиссии долота)
#define	SKLP_ADDRESS_ACP					0xB0	// АКП		Приемник ЛУЧ.435.00.01.00 модулЯ АКП ЛУЧ.679.00.00.00 (восьмизондовый с опорным каналом)
#define	SKLP_ADDRESS_ACPB_623				0xB1	// АКП(б)	Контроллер каверномера ЛУЧ.623.00.02.00 модулЯ ГГП ЛУЧ.623.00.00.00 (вспомогательный канал контролЯ прижатиЯ)
#define	SKLP_ADDRESS_ACPB_619				0xB2	// АКП(б)	Контроллер каверномера ЛУЧ.619.00.04.00 модулЯ АКП ЛУЧ.619.00.00.00 (трехзондовый)
#define	SKLP_ADDRESS_ACP_GENHV				0xBF	// АКП		Высоковольтный генератор ЛУЧ.435.00.03.00 акустического каверномера-профилемера ЛУЧ.679.00.00.00
#define	SKLP_ADDRESS_INGK					0xC0	// ИНГК	Контроллер памяти ИНГК
#define	SKLP_ADDRESS_INK					0xC1	// ИНК		Контроллер ИНК
#define	SKLP_ADDRESS_INGK_FPGA				0xCF	// ИНГК	Генератор нейтронов ИНГК
#define	SKLP_ADDRESS_MP						0xD0	// МП		Модуль питания
#define	SKLP_ADDRESS_MP_18V_BROADCAST		0xD0	// МП18	Модуль питания 18 В	поиск модулей
#define	SKLP_ADDRESS_MP_18V					0xD1	// МП18	Модуль питания 18 В	только с дополнительной адресацией
#define	SKLP_ADDRESS_MP_36V_BROADCAST		0xD2	// МП36	Модуль питания 36 В	поиск модулей
#define	SKLP_ADDRESS_MP_36V					0xD3	// МП36	Модуль питания 36 В	только с дополнительной адресацией
#define	SKLP_ADDRESS_NDR					0xDA	// НДР		Регистратор наддолотный
#define	SKLP_ADDRESS_NDR_CALIBR_TORQUE		0xDB	// НДР		Калибратор момента НДР
#define	SKLP_ADDRESS_NDMt					0xDB	// ППНДМ	Приемо-передатчик модулЯ наддолотного
#define	SKLP_ADDRESS_NDR_CALIBR_AXIS		0xDC	// НДР		Калибратор осевой нагрузки НДР
#define	SKLP_ADDRESS_INCL_ZENI				0xDD	// ИНКЛ	Измеритель зенитного угла
#define	SKLP_ADDRESS_NDMt2					0xDE	// ППНДМ	Приемо-передатчик модулЯ наддолотного (план)
#define	SKLP_ADDRESS_NDMp					0xDF	// ИНДМ	Измеритель модулЯ наддолотного
#define	SKLP_ADDRESS_MPP_160				0xE0	// МПП-160	Модуль памяти и питания
#define	SKLP_ADDRESS_ER						0xE2	// ЭР
#define	SKLP_ADDRESS_MPI_600				0xEA	// МПИ		Модуль памяти и измерений длЯ комплекса ЛУЧ.600
#define	SKLP_ADDRESS_TEST_SD				0xEB	// Тестировщик SD-карт на основе МПИ ЛУЧ.600 или спец.сетнда
#define	SKLP_ADDRESS_RDI					0xEC	// Регистратор данных инклинометра на основе МПИ
#define	SKLP_ADDRESS_MPP					0xF0	// МПП		Модуль памяти и питания (ведущий)
#define	SKLP_ADDRESS_NP						0xF0	// НП
#define	SKLP_ADDRESS_MPP_1					0xF1	// МПП_1	Модуль памяти и питания (ведомый-1)
#define	SKLP_ADDRESS_MPP_2					0xF2	// МПП_2	Модуль памяти и питания (ведомый-2)
#define	SKLP_ADDRESS_MPP_3					0xF3	// МПП_3	Модуль памяти и питания (ведомый-3)

// Широкоиспользуемые команды протокола СКЛ
typedef enum SKLP_Command_enum /*: uint8_t // only in C++11 */
{
	SKLP_COMMAND_RESET 				= 0x00,
	SKLP_COMMAND_STOP_LOGGING 		= 0x00,		//	не отвечать
	SKLP_COMMAND_ID_GET				= 0x01,
	SKLP_COMMAND_ID_SET				= 0x02,
	SKLP_COMMAND_DATA_ACQ_GET_LONG	= 0x03,		// запрос большого блока основных данных
	SKLP_COMMAND_NVM_GET			= 0x04,
	SKLP_COMMAND_NVM_SET			= 0x05,
	SKLP_COMMAND_WORKTIME_GET		= 0x07,
	SKLP_COMMAND_WORKTIME_INC		= 0x08,
	SKLP_COMMAND_WORKTIME_SET		= 0x09,
	SKLP_COMMAND_DATA_ACQ_GET		= 0x13,		// запрос блока основных данных
	SKLP_COMMAND_DATA_TECH_GET		= 0x14, 	// запрос блока технологических данных
	SKLP_COMMAND_MEMORY_ERASE		= 0x20,
	SKLP_COMMAND_MEMORY_STATE_GET	= 0x21,
	SKLP_COMMAND_MEMORY_READ		= 0x22,
	SKLP_COMMAND_EVENTS_READ		= 0x28,
	SKLP_COMMAND_EVENTS_CLEAR		= 0x29,
	SKLP_COMMAND_TIME_SYNC_MASTER	= 0x31,		// Установка текущего времени для МПИ, МПП, БИ. Остальные модули обычно игнорируют
	SKLP_COMMAND_TIME_LOG_START_SET	= 0x32,		// установить время включения
	SKLP_COMMAND_TIME_FREEZE		= 0x33,		// заморозить текущее время
	SKLP_COMMAND_TIME_FROZEN_GET	= 0x34,		// вернуть ранее замороженное время
	SKLP_COMMAND_TIME_LOG_START_GET	= 0x35,		// прочитать время включения
	SKLP_COMMAND_TIME_LOG_STOP_SET 	= 0x36,		// установить время выключения
	SKLP_COMMAND_TIME_SYNC			= 0x37,
	SKLP_COMMAND_TIME_LOG_STOP_GET 	= 0x38,		// прочитать время выключения
	SKLP_COMMAND_TIME_SYNC2			= 0x3F,		// аналог #37 для LWD, !!!возможен другой формат команды!!!
	SKLP_COMMAND_MEMORY_INFO_GET	= 0x41,
	SKLP_COMMAND_BAUD_SET			= 0x46,
	SKLP_COMMAND_GOTO_BOOTLOADER	= 0xB1,		// !! тест! перейти на BootLoader
	SKLP_COMMAND_DATA_ACQ_START		= 0xFF,		// приступить к измерениЯм

	SKLP_COMMAND_AKP_CUSTOM			= 0x60,
	SKLP_COMMAND_FREE				= 0xFE		// Зарезервированный индекс, соответсвующий недопустимой команде - используется для индикации свободной позиции в таблице ассоциаций коллбеков
} SKLP_Command_t;

//#ifdef	STATIC_ASSERT
//STATIC_ASSERT( sizeof( SKLP_Command_t ) == sizeof( uint8_t ) );
//#endif

// Скорость UART по-умолчанию
#ifndef	SKLP_BAUD_DEFAULT
#define	SKLP_BAUD_DEFAULT				115200	// [бод]	RS-485
#endif
#define	SKLP_BAUD_SIG60_DEFAULT			57600	// [бод]	Модем SIG60

// Тайминги
#define	SKLP_BAUD_FAST_COOLDOWN_s		1.5f	// [с]	таймаут автоматического сброса скорости UART после последней принятой команды
#define	SKLP_TIMERRX_PAUSE_BYTES		6.0f	// [байт]	максимальная пауза на шине (в байтах), после которой разорванный пакет не сшивается. +1 байт длЯ завершение приема очередного байта
#define	SKLP_LEDRX_FLASH_DURATION_s		15e-3f	// [с]	длительность горения светодиода при приеме нормального пакета (своего или чужого)
#define	SKLP_TIMEOUT_READMEMANSWER_s	1.1f	// [с]	допустимый таймаут на начало передачи ответа на [0x22] - команду чтениЯ памЯти

#endif	// SKLP_MS_TRANSPORT_H

