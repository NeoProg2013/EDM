#ifndef _CORE_H_
#define _CORE_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#define BUILD_UINT16(msb, lsb)   (((uint16_t)(msb) << 8) | (uint8_t)(lsb))

extern uint64_t HAL_GetTickUs();

// SYSCLK = 168 MHz 
// HCLK (AHB) = 168 MHz 
// PCLK1 (APB1) = 42 MHz
// PCLK2 (APB2) = 84 MHz


// Pin map
//
// +------+----------------------+---------------------------+-------------------------------------------+
// | Pin  | Signal               | Peripheral                | Notes                                     |
// +------+----------------------+---------------------------+-------------------------------------------+
// | PA0  | MOSFET_GATE_GND      | GPIO output               | Spark module, gate GND control            |
// | PA1  | MOSFET_GATE_CTRL     | TIM2_CH2                  | Spark PWM output, AF1                     |
// | PA2  | USART2_TX            | USART2                    | Telemetry TX, AF7                         |
// | PA3  | USART2_RX            | USART2                    | Telemetry RX, AF7                         |
// | PA11 | X_STEP               | GPIO output               | X axis step pin in current code           |
// | PA12 | X_DIR                | GPIO output               | X axis direction                          |
// | PB11 | HEAD_HX711_VCC       | GPIO output               | HX711 software-powered VCC                |
// | PB12 | HEAD_HX711_GND       | GPIO output               | HX711 software-driven GND                 |
// | PB13 | HEAD_HX711_DOUT      | GPIO input                | HX711 data output                         |
// | PB14 | HEAD_HX711_SCK       | GPIO output               | HX711 serial clock                        |
// | PC6  | FEEDBACK             | TIM8 input capture        | Feedback pulse measurement, AF3           |
// | PA9  | Y_STEP               | GPIO output               | Y axis step pin                           |
// | PA10 | Y_DIR                | GPIO output               | Y axis direction                          |
// | PA8  | HEAD_BRAKE_EN        | GPIO output               | Brake driver enable                       |
// | PC9  | HEAD_BRAKE_STEP      | TIM3_CH4                  | Brake PWM output, AF2                     |
// | PD11 | HEAD_FEEDER_EN       | GPIO output               | Feeder driver enable                      |
// | PD12 | HEAD_FEEDER_STEP     | TIM4_CH1                  | Feeder PWM output, AF2                    |
// +------+----------------------+---------------------------+-------------------------------------------+
//
// Peripheral summary:
// - USART2: PA2 TX, PA3 RX
// - TIM2_CH2: PA1 spark PWM
// - TIM3_CH4: PC9 brake PWM
// - TIM4_CH1: PD12 feeder PWM
// - TIM8 input capture: PC6 feedback
// - DMA1_Stream6: USART2 TX DMA
// - IRQs: USART2_IRQn, DMA1_Stream6_IRQn
//
// Notes:
// - X axis old documentation mentions PA11 as TIM1_CH4 PWM step output,
//   but the current implementation uses plain GPIO stepping via stepper_driver_t.
// - Y axis mapping in motion_controller currently conflicts with brake mapping on PC9.


#endif // _CORE_H_