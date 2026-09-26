#ifndef __TELEMETRY_H__
#define __TELEMETRY_H__
#include "core.h"

typedef struct {
    uint8_t  edm_status;
    uint8_t  step_state;
    uint16_t freq_hz;
    uint16_t arc_counter;
    uint16_t tension_g;
    uint16_t feeder_us;
    uint16_t brake_us;
    uint16_t t1;
    uint16_t t0;
    uint16_t checksum;
} tx_msg_t;

typedef struct {
    enum {
        CMD_NONE,
        CMD_MOVE_UP,
        CMD_MOVE_DOWN,
        CMD_MOVE_LEFT,
        CMD_MOVE_RIGHT
    };
    uint8_t  cmd;
    uint8_t  edm_status;
    uint16_t t0;
    uint16_t t1;
    uint16_t checksum;
} rx_msg_t;

extern UART_HandleTypeDef usart2;
extern DMA_HandleTypeDef dma_usart2_tx;

void telemetry_init();
void telemetry_process();
void telemetry_get_rx_msg(rx_msg_t* msg);
tx_msg_t* telemetry_get_tx_msg();
bool telemetry_get_connection_state();


#endif // __TELEMETRY_H__
