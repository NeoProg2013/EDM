#ifndef __TELEMETRY_H__
#define __TELEMETRY_H__
#include "core.h"

typedef struct {
    uint8_t  arc_state;
    uint8_t  step_state;
    uint16_t freq_hz;
    uint16_t arc_counter;
    uint16_t tension_g;
    uint16_t feeder_us;
    uint16_t brake_us;
    uint16_t t1;
    uint16_t t0;
} tx_msg_t;

typedef struct {
    uint8_t cmd;
} rx_msg_t;

extern UART_HandleTypeDef usart2;
extern DMA_HandleTypeDef dma_usart2_tx;

void telemetry_init();
void telemetry_tx(tx_msg_t* msg);
void telemetry_get_rx_msg(rx_msg_t* msg);


#endif // __TELEMETRY_H__
