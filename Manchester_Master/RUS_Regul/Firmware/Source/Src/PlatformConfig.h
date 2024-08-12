// ������������ ���������, ���������� � stm32xxxx_hal_conf.h.
// ����� ����������� �����-�������������, � ����������� �� ��������� ������������� IAR EWARM.

#if		defined( USE_PLATFORM_LOOCH_638_00_02_00_O1 ) || defined( USE_PLATFORM_LOOCH_638_00_02_00_O3 )
	#define	USE_PLATFORM_LOOCH_638_00_02_00				// ��������� 638_02 ������ ����� ����� ��������� � �������, �� ���� ������!
	#include "LOOCH.638.00.02.00\PlatformConfig.h"		// ���������� ������������������� ��� ���.638.00.02.00 � ������� ������ ���-�� ���.638.00.00.00
#elif	defined( USE_PLATFORM_LOOCH_641_00_05_00 )
	#include "LOOCH.641.00.05.00\PlatformConfig.h"		// ���������� ������������������ ��� ���.641.00.05.00 � ���.641.00.06.00 � ������� ������ ����-�����-��� ���.641.00.00.00
#elif	defined( USE_PLATFORM_OKP_423_03 )
	#include "OKP.423.03\PlatformConfig.h"				// ���������� ���-�� ���.423.03 (������� �����)
#elif   defined( USE_PLATFORM_OKR_354_10 )
        #include "OKR.354.00.10.00\PlatformConfig.h"
#else
	#error "Select Platform!"
#endif
