#include "i2c_user.h"
//------------------------------------------------
#define I2C_REQUEST_WRITE                       0x00
#define I2C_REQUEST_READ                        0x01
//------------------------------------------------
void I2C_SendByteByADDR(I2C_TypeDef * i2c, uint8_t c,uint8_t addr)
{
  //Disable Pos
  LL_I2C_DisableBitPOS(I2C1);
  LL_I2C_AcknowledgeNextData(i2c, LL_I2C_ACK);
  LL_I2C_GenerateStartCondition(i2c);
  while(!LL_I2C_IsActiveFlag_SB(i2c)){};
  (void) i2c->SR1;
  LL_I2C_TransmitData8(i2c, addr | I2C_REQUEST_WRITE);
  while(!LL_I2C_IsActiveFlag_ADDR(i2c)){};
  LL_I2C_ClearFlag_ADDR(i2c);
  LL_I2C_TransmitData8(i2c, c);
  while(!LL_I2C_IsActiveFlag_TXE(i2c)){};
  LL_I2C_GenerateStopCondition(i2c);
}
//------------------------------------------------
