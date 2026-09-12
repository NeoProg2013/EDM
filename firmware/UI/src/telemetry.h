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
    uint16_t checksum;
};

struct tx_msg_t {
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
};

extern UART_HandleTypeDef usart1;
extern DMA_HandleTypeDef hdma_usart1_tx;

void telemetry_init();
void telemetry_process();

void      telemetry_get_rx_msg(rx_msg_t* msg);
tx_msg_t* telemetry_get_tx_msg();
uint8_t   telemetry_get_rx_counter();
uint8_t   telemetry_get_tx_counter();
uint8_t   telemetry_get_desync_counter();
bool      telemetry_get_sync_state();
bool      telemetry_get_connection_state();


#endif // __TELEMETRY_H__
