// ������������ ���������, ���������� � stm32xxxx_hal_conf.h.
// ����� ����������� �����-�������������, � ����������� �� ��������� ������������� IAR EWARM.

#if   defined( USE_PLATFORM_OKR_354_10 )
        #include "OKR.354.00.10.00\PlatformConfig.h"
#elif   defined( USE_PLATFORM_LOOCH_601_03 )
        #include "../LOOCH.601.00.03.00\PlatformConfig.h"
#else
	#error "Select Platform!"
#endif
