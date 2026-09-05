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
static bool     g_tx_ready       = true;
static bool     g_is_sync_lost   = true;

static uint8_t g_tx_counter = 0;
static uint8_t g_rx_counter = 0;
static uint8_t g_desync_counter = 0;

static rx_msg_t g_rx_msg = {0};



static void usart1_init() {
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
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    // Setup TX DMA IRQ
    HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
}

static void usart1_gpio_init() {
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

// void HAL_UART_TxCpltCallback(UART_HandleTypeDef* usart) {
//     if (usart->Instance != USART2) {
//         return;
//     }
//     g_tx_ready = true;
// }

// void HAL_UART_RxCpltCallback(UART_HandleTypeDef* usart) {
//     if (usart->Instance != USART2) {
//         return;
//     }

//     do {
//         if (g_is_sync_lost) {
//             // Wait end of frame
//             if (g_rx_byte != STOP_MARKER) {
//                 break;
//             }
            
//             // Sync! Next byte should be 0xAA
//             g_rx_bytes_count = 0;
//             g_is_sync_lost = false;
//             break;
//         }

//         // Check first byte of incoming frame
//         if (g_rx_bytes_count == 0 && g_rx_byte != START_MARKER) {
//             g_is_sync_lost = true; // Desync. First frame byte not 
//             ++g_desync_counter;
//             break;
//         }

//         // Save incoming byte to buffer
//         g_rx_buffer[g_rx_bytes_count] = g_rx_byte;
//         ++g_rx_bytes_count;

//         // Check incoming frame
//         if (g_rx_bytes_count == sizeof(g_rx_buffer)) {
//             if (g_rx_buffer[0] != START_MARKER || g_rx_buffer[g_rx_bytes_count - 1] != STOP_MARKER) {
//                 g_is_sync_lost = true;
//                 ++g_desync_counter;
//                 break;
//             }

//             // Frame received
//             g_rx_msg.cmd = g_rx_buffer[1];

//             ++g_rx_counter;
//             g_rx_bytes_count = 0;
//             return;
//         }

//     } while (false);

//     HAL_UART_Receive_IT(&usart2, &g_rx_byte, 1);
// }

// void HAL_UART_ErrorCallback(UART_HandleTypeDef* usart) {
//     if (usart->Instance != USART2) {
//         return;
//     }

//     // Clear all errors
//     __HAL_UART_CLEAR_FLAG(&usart2, HAL_UART_ERROR_ORE | HAL_UART_ERROR_FE | HAL_UART_ERROR_NE | HAL_UART_ERROR_PE);

//     // Restart receiver
//     g_rx_bytes_count = 0;
//     HAL_UART_Receive_IT(&usart2, &g_rx_byte, 1);
//     g_is_sync_lost = true;
//     ++g_desync_counter;
// }



void telemetry_init() {
    usart1_gpio_init();
    usart1_init();

    // Start receiver
    HAL_UART_Receive_IT(&usart2, &g_rx_byte, 1);
}

void telemetry_tx(tx_msg_t* msg) {
    if (!g_tx_ready) {
        return;
    }

    g_tx_buffer[0]  = START_MARKER;
    g_tx_buffer[1]  = msg->arc_state;
    g_tx_buffer[2]  = msg->step_state;
    g_tx_buffer[3]  = msg->freq_hz >> 8;
    g_tx_buffer[4]  = msg->freq_hz & 0xFF;
    g_tx_buffer[5]  = msg->arc_counter >> 8;
    g_tx_buffer[6]  = msg->arc_counter & 0xFF;
    g_tx_buffer[7]  = msg->tension_g >> 8;
    g_tx_buffer[8]  = msg->tension_g & 0xFF;
    g_tx_buffer[9]  = msg->feeder_us >> 8;
    g_tx_buffer[10] = msg->feeder_us & 0xFF;
    g_tx_buffer[11] = msg->brake_us >> 8;
    g_tx_buffer[12] = msg->brake_us & 0xFF;
    g_tx_buffer[13] = msg->t1 >> 8;
    g_tx_buffer[14] = msg->t1 & 0xFF;
    g_tx_buffer[15] = msg->t0 >> 8;
    g_tx_buffer[16] = msg->t0 & 0xFF;
    g_tx_buffer[17] = STOP_MARKER;
    HAL_UART_Transmit_DMA(&usart2, g_tx_buffer, sizeof(g_tx_buffer));
    ++g_tx_counter;
}

void telemetry_get_rx_msg(rx_msg_t* msg) { 
    __disable_irq(); // To avoid half read g_rx_msg
    *msg = g_rx_msg;
    __enable_irq();
}
