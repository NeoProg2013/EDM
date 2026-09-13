#ifndef _CORE_H_
#define _CORE_H_

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <string.h>
#include <math.h>

#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))

#define true  (1)
#define false (0)
#define BUILD_UINT16(msb, lsb)   (((uint16_t)(msb) << 8) | (uint8_t)(lsb))

/**
 * ============================================================================
 *             PIN CONFIGURATION MAPPING TABLE (STM32F103C6T6 Blue Pill)
 * ============================================================================
 * 
 *  INTERFACE | PIN FUNCTION | BOARD PIN | NOTES / REMARKS
 * -----------+--------------+-----------+-------------------------------------
 *   CAN1     |  CAN_RX      |    PB8    | Requires AFIO REMAP!
 *  (Remap)   |  CAN_TX      |    PB9    | External transceiver needed
 * -----------+--------------+-----------+-------------------------------------
 *   I2C1     |  I2C1_SCL    |    PB6    | Requires external 4.7 kOhm pull-up
 *            |  I2C1_SDA    |    PB7    | Requires external 4.7 kOhm pull-up
 * -----------+--------------+-----------+-------------------------------------
 *   SPI1     |  SPI1_NSS    |    PA4    | Software CS (or any available GPIO)
 *            |  SPI1_SCK    |    PA5    | Clocked by fast APB2 bus (up to 36MHz)
 *            |  SPI1_MOSI   |    PA7    |
 * -----------+--------------+-----------+-------------------------------------
 *   TIM2     |  TIM2_CH1    |    PA0    | Encoder Mode - Phase A
 *  (Encoder) |  TIM2_CH2    |    PA1    | Encoder Mode - Phase B
 * -----------+--------------+-----------+-------------------------------------
 *   USART1   |  USART1_TX   |    PA9    | Shared with bootloader/UART flashing
 *            |  USART1_RX   |    PA10   | Disconnect during UART flashing
 * -----------+--------------+-----------+-------------------------------------
 */




// Pin map
// Target MCU: STM32F030F4P6
// 
// Display ILI9225 (SPI1 interface)
// --------------------------------
// PA0 - LCD_RST   (display reset)
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

///         PA4 - CS
///         PA5 - SCK  (SPI1)
///         PA7 - MOSI (SPI1)
///         PA6 - RS
///         PA8 - RST

#endif // _CORE_H_