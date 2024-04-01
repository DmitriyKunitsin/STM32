#include "float_to_char.h"

/**
 * Функция перевода из флоат в строку (выходная строка, число типа флоат, количество знаков после запятой)
 */
void float_to_char(uint8_t *out, float x, int decimalPoin)
{
	int decimalPoint = decimalPoin + 1;
    uint16_t absval = fabs(x);
    uint16_t absvalcopy = absval;

    int decimalcount = 0;

    while (absvalcopy != 0)
    {
        absvalcopy /= 10;
        decimalcount++;
    }

    uint8_t *absbuffer = malloc(sizeof(uint8_t) * (decimalcount + decimalPoint + 3)); // Увеличиваем размер на 2 для дополнительных символов (точка и два знака после запятой)
    int absbufferindex = 0;
    absvalcopy = absval;
    uint8_t temp;

    int i = 0;
    if (x < 1 && x > -1) // Проверяем, меньше ли число 1
    {
        *(absbuffer + absbufferindex) = '0'; // Добавляем ведущий ноль перед точкой
        absbufferindex++;
    }

    for (i = decimalcount; i > 0; i--)
    {
        uint16_t frst1 = fabs((absvalcopy / pow(10.0, i - 1)));
        temp = (frst1 % 10) + 0x30;
        *(absbuffer + absbufferindex) = temp;
        absbufferindex++;
    }

    if (decimalPoint > 0)
    {
        *(absbuffer + absbufferindex) = '.';
        absbufferindex++;

        //------------------- Decimal Extractor ---------------------//
        for (i = 1; i < decimalPoint + 1; i++)
        {
            uint32_t valueFloat = (x - (float)absval) * pow(10, i);
            *(absbuffer + absbufferindex) = ((valueFloat) % 10) + 0x30;
            absbufferindex++;
        }
    }

    for (i = 0; i < (decimalcount + decimalPoint + 1); i++)
    {
        *(out + i) = *(absbuffer + i);
    }

    i = 0;
    if (decimalPoint > 0)
        i = 1;
    *(out + decimalcount + decimalPoint + i) = 0;
}





