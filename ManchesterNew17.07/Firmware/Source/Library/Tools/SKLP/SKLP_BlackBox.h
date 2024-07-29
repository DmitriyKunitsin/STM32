// SKLP_BlackBox.h
// Интерфейс доступа к черному ящику по стандарту СКЛ
#ifndef	SKLP_BLACKBOX_H
#define	SKLP_BLACKBOX_H

#include <stdbool.h>
#include "SKLP_Time.h"

#define	BLACKBOX_MESSAGE_MAXLENGHT	15

typedef void ( * SKLP_BlackBoxCallback_t )( char *pSrcTxt, uint32_t Argument, char *pDest );
//bool SKLP_BlackBox_Init( void );								// Инициализация
bool SKLP_BlackBox_WriteRecord( char const *pText );	// Добавление записи в черный ящик
//bool SKLP_BlackBox_WriteRecordExt( char const *pText, SKLP_BlackBoxCallback_t xCallback, uint32_t CallbackArgument );	// Добавление записи в черный ящик с вызовом пользовательского коллбека 
//bool SKLP_BlackBox_Clear( void );							// Очистка черного ящика при входе в автономный режим
bool SKLP_BlackBox_WriteRecordTimestamp( char const *pText, SKLP_Time_t Timestamp );	// Добавление записи в черный ящик, использовать указанное времЯ

// Объявление имени идентификатора, связанного с регионом -
// чтобы можно было централизованно инициализировать соответствующий регион памяти
//extern const char aBlackBox_Name[];

// Стандартные сохраняемые в черный ящик теги			'*' - "нестандартные" теги, 'A' - за тегом следует алиас подчиненного модулЯ, 'S' - за тегом следует серийник подчиненного модулЯ
#define	SKLP_BLACKBOX_TAG_GOTOAUTO				"M1"	// переход в режим ожидания (автономный режим)
#define	SKLP_BLACKBOX_TAG_LOGSTART				"M2"	// ? переход в режим регистрации (автономный режим)
#define	SKLP_BLACKBOX_TAG_EXITAUTO_CMD			"M3U"	// выход из автономного режима (неавтономный режим) по команде
#define	SKLP_BLACKBOX_TAG_EXITAUTO_FAIL			"M3F"	// выход из автономного режима (неавтономный режим) по переполнению Flash
#define	SKLP_BLACKBOX_TAG_AUTO_IDLE_SKLP		"M4I"	// * прекращение регистрации, ожидание активации (автономный режим) - нет запросов по СКЛ
#define	SKLP_BLACKBOX_TAG_AUTO_IDLE_TIME		"M4T"	// * прекращение регистрации, ожидание активации (автономный режим) - выход за пределы временного окна
#define	SKLP_BLACKBOX_TAG_AUTO_ACTIVE_SKLP		"M5I"	// * активация (восстановление регистрации) - восстановление запросов по СКЛ
#define	SKLP_BLACKBOX_TAG_AUTO_ACTIVE_TIME		"M5T"	// * активация (восстановление регистрации) - попадание в пределы временного окна
//#define	SKLP_BLACKBOX_TAG_MPI_GOTOAUTO_CMD		SKLP_BLACKBOX_TAG_EXITAUTO_CMD	// переход в режим ожидания (автономный режим) по команде от опрератора (через RS-485)
#define	SKLP_BLACKBOX_TAG_MPI_GOTODRILL_CMD		"M6U"	// * переход МПИ в режим бурениЯ по команде от опрератора (через RS-485)
#define	SKLP_BLACKBOX_TAG_MPI_GOTODRILL_MUP		"M6M"	// * переход МПИ в режим бурениЯ по команде от МУП (через гидроканал) 
#define	SKLP_BLACKBOX_TAG_MPI_GOTOLOGGING_CMD	"M7U"	// * переход МПИ в режим каротажа по команде от опрератора (через RS-485)
#define	SKLP_BLACKBOX_TAG_MPI_GOTOLOGGING_MUP	"M7M"	// * переход МПИ в режим каротажа по команде от МУП (через гидроканал) 
#define	SKLP_BLACKBOX_TAG_RESET_PWR				"R1UR0"		// перезагрузка по пропаданию питания
#define	SKLP_BLACKBOX_TAG_RESET_WDT				"R1WR0"		// перезагрузка по WatchDog
#define	SKLP_BLACKBOX_TAG_RESET_SOFT_REBOOT		"R1S1R0"	// *	перезагрузка по решению софта (Reboot)
#define	SKLP_BLACKBOX_TAG_RESET_SOFT_ASSERT		"R1S2R0"	// *	перезагрузка по решению софта (Assert)
#define	SKLP_BLACKBOX_TAG_RESET_SOFT_FAULT		"R1S3R0"	// *	перезагрузка по решению софта (Hard Fault)
#define	SKLP_BLACKBOX_TAG_RESET_UNKNOWN			"R1NR0"		// *	перезагрузка по по невыясненной причине
// Вместо R0 должно быть R0, R1, R6, R7 - по номеру режима, в котором очнулись.

#define	SKLP_BLACKBOX_TAG_RESET_SLAVESTART		"R21"	// перезагрузка подчиненного модуля (запуск)
#define	SKLP_BLACKBOX_TAG_RESET_SLAVEFINISH		"R22"	// перезагрузка подчиненного модуля (завершение)
#define	SKLP_BLACKBOX_TAG_FLASH_RESTORE			"F01"	// нормальная работа flash восстановлена
#define	SKLP_BLACKBOX_TAG_FLASH_ERR_WRITE		"F21"	// ошибка записи данных в память
#define	SKLP_BLACKBOX_TAG_FLASH_ERR_READ		"F31"	// ошибка чтения данных из памяти
#define	SKLP_BLACKBOX_TAG_FLASH_FORMAT			"F41"	// *	произведено форматирование
#define	SKLP_BLACKBOX_TAG_SLAVE_FAIL_ANSWER		"E1"	// A		Обмен с подчиненным модулем потерян (нет ответа)
#define	SKLP_BLACKBOX_TAG_SLAVE_FAIL_CRC		"E2"	// A		Обмен с подчиненным модулем потерян (ошибка CRC на большом блоке данных)
#define	SKLP_BLACKBOX_TAG_SLAVE_RESTORE			"E3"	// A		Обмен с подчиненным модулем восстановлен
#define	SKLP_BLACKBOX_TAG_SLAVE_APPEAR			"E4"	// *AS	Обмен с подчиненным модулем восстановлен, но зафиксирован новый серийный номер
#define	SKLP_BLACKBOX_TAG_SLAVE_FAIL_SYNC		"E5"	// *		Обмен с подчиненным модулем разсинхронизирован

#define	SKLP_BLACKBOX_TAG_VOLTAGE				"V1"	// 		??отказ аккумулЯторов в МПП??
#define	SKLP_BLACKBOX_TAG_VPLC_FAULT			"V5"	// *		отказ модемной линии MPI.+VPLC
#define	SKLP_BLACKBOX_TAG_VPLC_RESTORE			"V6"	// *		восстановление модемной линии MPI.+VPLC

#endif	// SKLP_BLACKBOX_H

