#include "lcd_static.h"

void segchar(uint8_t seg) {
	switch(seg) {

	case 1:
		SB_SET; SC_SET; SH_SET;
		break;
	case 2:
		SA_SET; SB_SET; SG_SET; SE_SET; SD_SET; SH_SET;
		break;
	case 3:
		SA_SET; SB_SET; SG_SET; SC_SET; SD_SET; SH_SET;
        break;
    case 4:
    	SF_SET; SB_SET; SG_SET; SC_SET; SH_SET;
	    break;
    case 5:
    	SA_SET; SF_SET; SG_SET; SC_SET; SD_SET; SH_SET;
	    break;
	case 6:
		SA_SET; SF_SET; SG_SET; SC_SET; SD_SET; SE_SET; SH_SET;
	    break;
	case 7:
		SB_SET; SC_SET; SA_SET; SH_SET;
	    break;
	case 8:
		SA_SET; SF_SET; SG_SET; SC_SET; SD_SET; SE_SET; SB_SET; SH_SET;
	    break;
	case 9:
	     SA_SET; SB_SET; SG_SET; SF_SET; SC_SET; SD_SET; SH_SET; SH_SET;
	    break;
	case 0:
	    SA_SET; SF_SET; SB_SET; SC_SET;SE_SET; SD_SET; SH_SET;
	     break;
	}
}
