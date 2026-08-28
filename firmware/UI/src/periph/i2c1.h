#ifndef _I2C1_H_
#define	_I2C1_H_
#include <stdint.h>


void i2c1_init();
void i2c1_write8(uint8_t addr, uint8_t reg, uint8_t v);
uint8_t i2c1_read8(uint8_t addr, uint8_t reg);

#endif // _I2C1_H_
