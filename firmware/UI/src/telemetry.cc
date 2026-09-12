#include "core.h"
#include "telemetry.h"
#define START_MARKER            (0xAA)
#define STOP_MARKER             (0xDD)

UART_HandleTypeDef usart1 = {0};
DMA_HandleTypeDef hdma_usart1_tx = {0};

static uint8_t  g_tx_buffer[sizeof(tx_msg_t) + 2] = {0}; // +2 = start marker + stop marker
static uint8_t  g_rx_buffer[sizeof(rx_msg_t) + 2] = {0}; // +2 = start marker + stop marker
static uint16_t g_rx_bytes_count = 0;
static uint8_t  g_rx_byte        = 0;
static bool     g_is_sync_lost   = true;
static bool     g_tx_ready       = true;
static bool     g_is_connected   = false;

static uint8_t g_tx_counter = 0;
static uint8_t g_rx_counter = 0;
static uint8_t g_desync_counter = 0;

static rx_msg_t g_rx_msg = {0};
static tx_msg_t g_tx_msg = {
    .cmd        = tx_msg_t::CMD_NONE,
    .edm_status = 0,
    .t0         = 300,
    .t1         = 1,
    .checksum   = 0,
};


static uint16_t calc_checksum(const uint8_t* p, uint16_t size) {
    uint16_t checksum = 0;
    for (uint8_t i = 0; i < size; ++i) {
        checksum += p[i];
    }
    return checksum;
}

static void usart1_init() {
    // Init USART
    usart1.Instance          = USART1;
    usart1.Init.BaudRate     = 9600;
    usart1.Init.WordLength   = UART_WORDLENGTH_8B;
    usart1.Init.StopBits     = UART_STOPBITS_1;
    usart1.Init.Parity       = UART_PARITY_NONE;
    usart1.Init.Mode         = UART_MODE_TX_RX;
    usart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&usart1) != HAL_OK) {
        while(1);
    }

    // Setup DMA1 Channel 2 (TX)
    hdma_usart1_tx.Instance                 = DMA1_Channel2;
    hdma_usart1_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_usart1_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode                = DMA_NORMAL;
    hdma_usart1_tx.Init.Priority            = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK) {
        while(1);
    }
    __HAL_LINKDMA(&usart1, hdmatx, hdma_usart1_tx);

    // Setup USART IRQ 
    HAL_UART_Receive_IT(&usart1, &g_rx_byte, 1);
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    // Setup DMA IRQ
    HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
}

static void usart1_gpio_init() {
    // PA2 -> TX
    GPIO_InitTypeDef tx = {0};
    tx.Pin       = GPIO_PIN_2;
    tx.Mode      = GPIO_MODE_AF_PP;
    tx.Pull      = GPIO_NOPULL;
    tx.Speed     = GPIO_SPEED_FREQ_HIGH;
    tx.Alternate = GPIO_AF1_USART1;
    HAL_GPIO_Init(GPIOA, &tx);

    // PA3 -> RX
    GPIO_InitTypeDef rx = {0};
    rx.Pin       = GPIO_PIN_3;
    rx.Mode      = GPIO_MODE_AF_PP;
    rx.Pull      = GPIO_PULLUP;
    rx.Speed     = GPIO_SPEED_FREQ_HIGH;
    rx.Alternate = GPIO_AF1_USART1;
    HAL_GPIO_Init(GPIOA, &rx);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
    if (huart->Instance != USART1) {
        return;
    }
    g_tx_ready = true;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {
    if (huart->Instance != USART1) {
        return;
    }

    do {
        if (g_is_sync_lost) {
            // Wait end of frame
            if (g_rx_byte != STOP_MARKER) {
                break;
            }
            
            // Sync! Next byte should be 0xAA
            g_rx_bytes_count = 0;
            g_is_sync_lost = false;
            break;
        }

        // Check first byte of incoming frame
        if (g_rx_bytes_count == 0 && g_rx_byte != START_MARKER) {
            g_is_sync_lost = true; // Desync. First frame byte not 
            ++g_desync_counter;
            break;
        }

        // Save incoming byte to buffer
        g_rx_buffer[g_rx_bytes_count] = g_rx_byte;
        ++g_rx_bytes_count;

        // Check incoming frame
        if (g_rx_bytes_count == sizeof(g_rx_buffer)) {
            if (g_rx_buffer[0] != START_MARKER || g_rx_buffer[g_rx_bytes_count - 1] != STOP_MARKER) {
                g_is_sync_lost = true;
                ++g_desync_counter;
                break;
            }

            // Calc checksum
            uint16_t recv_checksum = BUILD_UINT16(g_rx_buffer[17], g_rx_buffer[18]);
            uint16_t checksum = calc_checksum(&g_rx_buffer[1], sizeof(rx_msg_t) - sizeof(rx_msg_t::checksum));
            if (checksum != recv_checksum) { // Bad frame - resync
                g_is_sync_lost = true;
                ++g_desync_counter;
                break;
            }

            // Save new frame
            g_rx_msg.arc_state   = g_rx_buffer[1];
            g_rx_msg.step_state  = g_rx_buffer[2];
            g_rx_msg.freq_hz     = BUILD_UINT16(g_rx_buffer[3],  g_rx_buffer[4]);
            g_rx_msg.arc_counter = BUILD_UINT16(g_rx_buffer[5],  g_rx_buffer[6]);
            g_rx_msg.tension_g   = BUILD_UINT16(g_rx_buffer[7],  g_rx_buffer[8]);
            g_rx_msg.feeder_us   = BUILD_UINT16(g_rx_buffer[9],  g_rx_buffer[10]);
            g_rx_msg.brake_us    = BUILD_UINT16(g_rx_buffer[11], g_rx_buffer[12]);
            g_rx_msg.t1          = BUILD_UINT16(g_rx_buffer[13], g_rx_buffer[14]);
            g_rx_msg.t0          = BUILD_UINT16(g_rx_buffer[15], g_rx_buffer[16]);
            g_rx_msg.checksum    = recv_checksum;

            g_rx_bytes_count = 0;
            ++g_rx_counter;
            break;
        }
    } while (false);

    HAL_UART_Receive_IT(&usart1, &g_rx_byte, 1);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
    if (huart->Instance != USART1) {
        return;
    }

    // Clear all errors
    __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_FEF);

    // Restart receiver
    g_rx_bytes_count = 0;
    HAL_UART_Receive_IT(&usart1, &g_rx_byte, 1);
    g_is_sync_lost = true;
    ++g_desync_counter;
}



void telemetry_init() {
    usart1_gpio_init();
    usart1_init();

    // Start receiver
    HAL_UART_Receive_IT(&usart1, &g_rx_byte, 1);
}

void telemetry_process() {
    static uint8_t s_prev_rx_counter = 0;
    static uint32_t s_last_check_time_ms = 0;
    if (HAL_GetTick() - s_last_check_time_ms > 500) {
        g_is_connected = (s_prev_rx_counter != g_rx_counter);
        s_prev_rx_counter = g_rx_counter;
    }

    // TX
    static uint32_t s_last_tx_time_ms = 0;
    if (HAL_GetTick() - s_last_tx_time_ms < 100 || !g_tx_ready) {
        return;
    }
    s_last_tx_time_ms = HAL_GetTick();

    // Prepare message
    g_tx_buffer[0] = START_MARKER;
    g_tx_buffer[1] = g_tx_msg.cmd;
    g_tx_buffer[2] = g_tx_msg.edm_status;
    g_tx_buffer[3] = g_tx_msg.t0 >> 8;
    g_tx_buffer[4] = g_tx_msg.t0 >> 0;
    g_tx_buffer[5] = g_tx_msg.t1 >> 8;
    g_tx_buffer[6] = g_tx_msg.t1 >> 0;
    uint16_t checksum = calc_checksum(&g_tx_buffer[1], sizeof(tx_msg_t) - sizeof(tx_msg_t::checksum));
    g_tx_buffer[7] = checksum >> 8;
    g_tx_buffer[8] = checksum >> 0;
    g_tx_buffer[9] = STOP_MARKER;
    HAL_UART_Transmit_DMA(&usart1, g_tx_buffer, sizeof(g_tx_buffer));
    ++g_tx_counter;
}


void telemetry_get_rx_msg(rx_msg_t* msg) { 
    __disable_irq(); // To avoid half read g_rx_msg
    *msg = g_rx_msg;
    __enable_irq();
}
tx_msg_t* telemetry_get_tx_msg()       { return &g_tx_msg;        }
uint8_t telemetry_get_rx_counter()     { return g_rx_counter;     }
uint8_t telemetry_get_tx_counter()     { return g_tx_counter;     }
uint8_t telemetry_get_desync_counter() { return g_desync_counter; }
bool telemetry_get_sync_state()        { return g_is_sync_lost;   }
bool telemetry_get_connection_state()  { return g_is_connected;   }
