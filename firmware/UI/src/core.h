#ifndef _CORE_H_
#define _CORE_H_

#include "stm32f0xx_hal.h"
#include <stdint.h>
#include <math.h>

#define true  (1)
#define false (0)
#define BUILD_UINT16(msb, lsb)   (((uint16_t)(msb) << 8) | (uint8_t)(lsb))

#endif // _CORE_H_