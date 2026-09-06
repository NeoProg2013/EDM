#ifndef _CORE_H_
#define _CORE_H_

#include "stm32f0xx_hal.h"
#include <stdint.h>
#include <math.h>

#define true  (1)
#define false (0)
#define BUILD_UINT16(msb, lsb)   (((uint16_t)(msb) << 8) | (uint8_t)(lsb))

// Pin map
// Target MCU: STM32F030F4P6
// 
// Display ILI9225 (SPI1 interface)
// --------------------------------
// PA9 - LCD_RST   (display reset)
// PB1 - LCD_RS/DC (register select / data-command)
// PA6 - LCD_CS    (chip select)
// PA5 - SPI SCK   (SPI1)
// PA7 - SPI MOSI  (SPI1)
// 
// Telemetry (USART1 + TX DMA)
// ---------
// PA2 - USART1_TX
// PA3 - USART1_RX
// 
// Notes:
// - LCD control pins are hardcoded in lib/ILI9225/ILI9225.cc
// - SPI1 handle is referenced as extern SPI_HandleTypeDef hspi1
// - USART1 and DMA handles are declared in src/telemetry.h
// 

#endif // _CORE_H_