#ifndef _SPI1_H_
#define	_SPI1_H_
#include <stdint.h>

void spi1_init();
void spi1_write(uint8_t* data, uint16_t size);

#endif // _SPI1_H_
