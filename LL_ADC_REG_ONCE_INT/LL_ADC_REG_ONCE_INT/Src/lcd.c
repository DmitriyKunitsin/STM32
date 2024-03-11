#include "lcd.h"
#include "i2c_user.h"
//------------------------------------------------
char str1[100];
uint8_t portlcd;
//------------------------------------------------
__STATIC_INLINE void DelayMicro(__IO uint32_t micros)
{
	micros *=(SystemCoreClock / 1000000) / 9;
	while (micros--);
}
//------------------------------------------------
void LCD_WriteByteI2CLCD(uint8_t bt)
{
  I2C_SendByteByADDR(I2C1, bt,0x4E);
}
//------------------------------------------------
__STATIC_INLINE void DelayNano(__IO uint32_t nanos)
{
	nanos = nanos * (SystemCoreClock / 1000000) / 9000;
	while (nanos--);
}
//------------------------------------------------
void sendhalfbyte(uint8_t c)
{
  c<<=4;
  LCD_WriteByteI2CLCD(portlcd|c);
  LCD_WriteByteI2CLCD((portlcd|=0x04)|c);
  DelayNano(200);
  LCD_WriteByteI2CLCD((portlcd&=~0x04)|c);
}
//------------------------------------------------
void sendbyte(uint8_t c, uint8_t mode)
{
	if(mode==0) rs_reset();
	else rs_set();
	uint8_t hc=0;
	hc=c>>4;
  DelayNano(100);
	sendhalfbyte(hc);sendhalfbyte(c);
}
//------------------------------------------------
void LCD_Clear(void)
{
	sendbyte(0x01,0);
	LL_mDelay(2);
}
//------------------------------------------------
void LCD_SendChar(char ch)
{
	sendbyte(ch,1);
}
//------------------------------------------------
void LCD_String(char* st)
{
	uint8_t i=0;
	while(st[i]!=0)
	{
		sendbyte(st[i],1);
		i++;
	}
}
//------------------------------------------------
void LCD_SetPos(uint8_t x, uint8_t y)
{
	switch(y)
	{
		case 0:
			sendbyte(x|0x80,0);
			break;
		case 1:
			sendbyte((0x40+x)|0x80,0);
			break;
		case 2:
			sendbyte((0x14+x)|0x80,0);
			break;
		case 3:
			sendbyte((0x54+x)|0x80,0);
			break;
	}
}
//------------------------------------------------
void LCD_ini(void)
{
  LL_mDelay(50);
  LCD_WriteByteI2CLCD(0);
  setwrite();//запись
  LL_mDelay(100);
  sendhalfbyte(0x03);
  DelayMicro(4500);
  sendhalfbyte(0x03);
  DelayMicro(4500);
  sendhalfbyte(0x03);
  DelayMicro(4500);
  sendhalfbyte(0x02);
  sendbyte(0x08,0);//дисплей пока выключаем
  sendbyte(0x0C,0);//дисплей включаем (D=1), курсоры никакие не нужны
  LL_mDelay(1);
  sendbyte(0x01,0);// уберем мусор
  LL_mDelay(2);
  sendbyte(0x06,0);// пишем влево
  LL_mDelay(1);
  sendbyte(0x0C,0);//дисплей включаем (D=1), курсоры никакие не нужны
  sendbyte(0x02,0);//курсор на место
  LL_mDelay(2);
  setled();//подсветка
  setwrite();//запись
}

