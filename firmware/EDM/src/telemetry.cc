#include "core.h"
#include "telemetry.h"
#define START_MARKER            (0xAA)
#define STOP_MARKER             (0xDD)

UART_HandleTypeDef usart2 = {0};
DMA_HandleTypeDef dma_usart2_tx = {0};

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

static tx_msg_t g_tx_msg = {0};
static rx_msg_t g_rx_msg = {0};


static void usart2_init();
static void usart2_gpio_init();
static uint16_t calc_checksum(const uint8_t* p, uint16_t size);
void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart);
void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart);



void telemetry_init() {
    usart2_gpio_init();
    usart2_init();

    // Start receiver
    HAL_UART_Receive_IT(&usart2, &g_rx_byte, 1);
}

void telemetry_process() {
    //
    // Check RX status
    static uint8_t s_prev_rx_counter = 0;
    static uint32_t s_last_check_time_ms = 0;
    if (HAL_GetTick() - s_last_check_time_ms > 500) {
        g_is_connected = (s_prev_rx_counter != g_rx_counter);
        s_prev_rx_counter = g_rx_counter;
        s_last_check_time_ms = HAL_GetTick();
    }

    //
    // TX. Send packet each 50 ms
    static uint32_t s_last_tx_time_ms = 0;
    if (HAL_GetTick() - s_last_tx_time_ms < 50 || !g_tx_ready) {
        return;
    }
    s_last_tx_time_ms = HAL_GetTick();

    // Prepare message
    g_tx_buffer[0]  = START_MARKER;
    g_tx_buffer[1]  = g_tx_msg.edm_status;
    g_tx_buffer[2]  = g_tx_msg.step_state;
    g_tx_buffer[3]  = g_tx_msg.freq_hz >> 8;
    g_tx_buffer[4]  = g_tx_msg.freq_hz & 0xFF;
    g_tx_buffer[5]  = g_tx_msg.arc_counter >> 8;
    g_tx_buffer[6]  = g_tx_msg.arc_counter & 0xFF;
    g_tx_buffer[7]  = g_tx_msg.tension_g >> 8;
    g_tx_buffer[8]  = g_tx_msg.tension_g & 0xFF;
    g_tx_buffer[9]  = g_tx_msg.feeder_us >> 8;
    g_tx_buffer[10] = g_tx_msg.feeder_us & 0xFF;
    g_tx_buffer[11] = g_tx_msg.brake_us >> 8;
    g_tx_buffer[12] = g_tx_msg.brake_us & 0xFF;
    g_tx_buffer[13] = g_tx_msg.t1 >> 8;
    g_tx_buffer[14] = g_tx_msg.t1 & 0xFF;
    g_tx_buffer[15] = g_tx_msg.t0 >> 8;
    g_tx_buffer[16] = g_tx_msg.t0 & 0xFF;
    uint16_t checksum = calc_checksum(&g_tx_buffer[1], sizeof(tx_msg_t) - sizeof(tx_msg_t::checksum));
    g_tx_buffer[17] = checksum >> 8;
    g_tx_buffer[18] = checksum >> 0;
    g_tx_buffer[19] = STOP_MARKER;
    if (HAL_UART_Transmit_DMA(&usart2, g_tx_buffer, sizeof(g_tx_buffer)) != HAL_OK) {
        return;
    }

    g_tx_ready = false;
    ++g_tx_counter;
}

void telemetry_get_rx_msg(rx_msg_t* msg) { 
    __disable_irq(); // To avoid half read g_rx_msg
    *msg = g_rx_msg;
    g_rx_msg.cmd = rx_msg_t::CMD_NONE;
    __enable_irq();
}

tx_msg_t* telemetry_get_tx_msg()      { return &g_tx_msg;      }
bool telemetry_get_connection_state() { return g_is_connected; }



static void usart2_init() {
    // Init USART
    usart2.Instance          = USART2;
    usart2.Init.BaudRate     = 9600;
    usart2.Init.WordLength   = UART_WORDLENGTH_8B;
    usart2.Init.StopBits     = UART_STOPBITS_1;
    usart2.Init.Parity       = UART_PARITY_NONE;
    usart2.Init.Mode         = UART_MODE_TX_RX;
    usart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&usart2) != HAL_OK) {
        while(1);
    }

    // Setup DMA1 Channel 2 (TX)
    dma_usart2_tx.Instance                 = DMA1_Stream6;
    dma_usart2_tx.Init.Channel             = DMA_CHANNEL_4;
    dma_usart2_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    dma_usart2_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    dma_usart2_tx.Init.MemInc              = DMA_MINC_ENABLE;
    dma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    dma_usart2_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    dma_usart2_tx.Init.Mode                = DMA_NORMAL;
    if (HAL_DMA_Init(&dma_usart2_tx) != HAL_OK) {
        while(1);
    }
    __HAL_LINKDMA(&usart2, hdmatx, dma_usart2_tx);

    // Setup USART IRQ 
    HAL_UART_Receive_IT(&usart2, &g_rx_byte, 1);
    HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);

    // Setup TX DMA IRQ
    HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
}

static void usart2_gpio_init() {
    // PA2 -> TX
    GPIO_InitTypeDef tx = {0};
    tx.Pin       = GPIO_PIN_2;
    tx.Mode      = GPIO_MODE_AF_PP;
    tx.Pull      = GPIO_NOPULL;
    tx.Speed     = GPIO_SPEED_FREQ_HIGH;
    tx.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &tx);

    // PA3 -> RX
    GPIO_InitTypeDef rx = {0};
    rx.Pin       = GPIO_PIN_3;
    rx.Mode      = GPIO_MODE_AF_PP;
    rx.Pull      = GPIO_PULLUP;
    rx.Speed     = GPIO_SPEED_FREQ_HIGH;
    rx.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &rx);
}

static uint16_t calc_checksum(const uint8_t* p, uint16_t size) {
    uint16_t checksum = 0;
    for (uint8_t i = 0; i < size; ++i) {
        checksum += p[i];
    }
    return checksum;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* usart) {
    if (usart->Instance != USART2) {
        return;
    }
    g_tx_ready = true;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* usart) {
    if (usart->Instance != USART2) {
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
            uint16_t recv_checksum = BUILD_UINT16(g_rx_buffer[7], g_rx_buffer[8]);
            uint16_t checksum = calc_checksum(&g_rx_buffer[1], sizeof(rx_msg_t) - sizeof(rx_msg_t::checksum));
            if (checksum != recv_checksum) { // Bad frame - resync
                g_is_sync_lost = true;
                ++g_desync_counter;
                break;
            }

            // Save new frame
            g_rx_msg.cmd        = g_rx_buffer[1];
            g_rx_msg.edm_status = g_rx_buffer[2];
            g_rx_msg.t0         = BUILD_UINT16(g_rx_buffer[3],  g_rx_buffer[4]);
            g_rx_msg.t1         = BUILD_UINT16(g_rx_buffer[5],  g_rx_buffer[6]);
            g_rx_msg.checksum   = recv_checksum;

            ++g_rx_counter;
            g_rx_bytes_count = 0;
            break;
        }

    } while (false);

    HAL_UART_Receive_IT(&usart2, &g_rx_byte, 1);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* usart) {
    if (usart->Instance != USART2) {
        return;
    }

    // Clear all errors
    __HAL_UART_CLEAR_FLAG(&usart2, HAL_UART_ERROR_ORE | HAL_UART_ERROR_FE | HAL_UART_ERROR_NE | HAL_UART_ERROR_PE);

    // Restart receiver
    g_rx_bytes_count = 0;
    HAL_UART_Receive_IT(&usart2, &g_rx_byte, 1);
    g_is_sync_lost = true;
    ++g_desync_counter;
}