#ifndef _CORE_H_
#define _CORE_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))

// SYSCLK = 168 MHz 
// HCLK (AHB) = 168 MHz 
// PCLK1 (APB1) = 42 MHz
// PCLK2 (APB2) = 84 MHz


// Pin map
// USART2 - to display board
//     PA2 -> TX
//     PA3 -> RX

#endif // _CORE_H_