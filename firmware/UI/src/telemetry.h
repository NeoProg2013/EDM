#ifndef __TELEMETRY_H__
#define __TELEMETRY_H__
#include "core.h"

struct rx_msg_t {
    uint8_t  arc_state;
    uint8_t  step_state;
    uint16_t freq_hz;
    uint16_t arc_counter;
    uint16_t tension_g;
    uint16_t feeder_us;
    uint16_t brake_us;
    uint16_t t1;
    uint16_t t0;
};

struct tx_msg_t {
    enum {
        CMD_START_NONE,
        CMD_START_STOP_EDM,
    };
    uint8_t cmd;
};

extern UART_HandleTypeDef usart1;
extern DMA_HandleTypeDef hdma_usart1_tx;

void telemetry_init();
void telemetry_tx(tx_msg_t msg);

void    telemetry_get_rx_msg(rx_msg_t* msg);
uint8_t telemetry_get_rx_counter();
uint8_t telemetry_get_tx_counter();
uint8_t telemetry_get_desync_counter();
bool    telemetry_get_sync_state();
bool    telemetry_get_connection_state();


#endif // __TELEMETRY_H__
