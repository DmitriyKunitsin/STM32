// Конфигуратор платформы, включается в stm32xxxx_hal_conf.h.
// Выбор правильного файла-конфигуратора, в зависимости от настройки препроцессора IAR EWARM.

#if		defined( USE_PLATFORM_LOOCH_638_00_02_00_O1 ) || defined( USE_PLATFORM_LOOCH_638_00_02_00_O3 )
	#define	USE_PLATFORM_LOOCH_638_00_02_00				// Платформы 638_02 разных литер почти приведены к единому, но есть нюансы!
	#include "LOOCH.638.00.02.00\PlatformConfig.h"		// Контроллер спектрометрического АЦП ЛУЧ.638.00.02.00 в составе модулЯ ГГК-ЛП ЛУЧ.638.00.00.00
#elif	defined( USE_PLATFORM_LOOCH_641_00_05_00 )
	#include "LOOCH.641.00.05.00\PlatformConfig.h"		// Контроллер спектрометрических АЦП ЛУЧ.641.00.05.00 и ЛУЧ.641.00.06.00 в составе модулЯ ННКТ-ГГКЛП-АКП ЛУЧ.641.00.00.00
#elif	defined( USE_PLATFORM_OKP_423_03 )
	#include "OKP.423.03\PlatformConfig.h"				// Контроллер ГГК-ЛП ОКР.423.03 (Опытный макет)
#elif   defined( USE_PLATFORM_OKR_354_10 )
        #include "OKR.354.00.10.00\PlatformConfig.h"
#else
	#error "Select Platform!"
#endif
