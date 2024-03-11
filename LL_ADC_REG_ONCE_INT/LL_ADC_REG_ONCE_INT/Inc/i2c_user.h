#ifndef I2C_USER_H_
#define I2C_USER_H_
//------------------------------------------------
#include "stm32f1xx.h"
#include "stm32f1xx_ll_i2c.h"
//------------------------------------------------
void I2C_SendByteByADDR(I2C_TypeDef * i2c, uint8_t c,uint8_t addr);
//------------------------------------------------
//------------------------------------------------
#endif /* I2C_USER_H_ */
