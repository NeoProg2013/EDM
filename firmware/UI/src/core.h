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

// ============================================================================
//             PIN CONFIGURATION MAPPING TABLE (STM32F103C6T6 Blue Pill)
// ============================================================================
// 
//  INTERFACE | PIN FUNCTION | BOARD PIN | NOTES / REMARKS
// -----------+--------------+-----------+-------------------------------------
//   CAN1     | CAN_RX       |    PB8    | Requires AFIO REMAP!
//  (Remap)   | CAN_TX       |    PB9    | External transceiver 
// -----------+--------------+-----------+-------------------------------------
//   I2C1     | I2C1_SCL     |    PB6    | 
//            | I2C1_SDA     |    PB7    | 
// -----------+--------------+-----------+-------------------------------------
//   SPI1     | SPI1_CS      |    PA4    | Clocked by fast APB2 bus (up to 36MHz)
//  ILI9225   | SPI1_SCK     |    PA5    | 
//            | SPI1_MOSI    |    PA7    |
//            | ILI_9225_LED |    PA2    |
//            | ILI_9225_RST |    PA3    |
//            | ILI_9225_RS  |    PA6    |
// -----------+--------------+-----------+-------------------------------------
//   TIM2     | TIM2_CH1     |    PA0    | Encoder Mode - Phase A
//  (Encoder) | TIM2_CH2     |    PA1    | Encoder Mode - Phase B
// -----------+--------------+-----------+-------------------------------------
//   USART1   | USART1_TX    |    PA9    | 
//            | USART1_RX    |    PA10   | 
// -----------+--------------+-----------+-------------------------------------

#endif // _CORE_H_
