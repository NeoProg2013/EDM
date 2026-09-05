#include "core.h"

extern "C" void HardFault_Handler(void) {
    while (true);
}

extern "C" void SysTick_Handler(void) {
    HAL_IncTick();
}

extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern DMA_HandleTypeDef hdma_usart1_rx;

// USART1 + DMA TX/RX
extern "C" void DMA1_Channel2_3_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_usart1_tx);
    HAL_DMA_IRQHandler(&hdma_usart1_rx);
}
extern "C" void USART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart1);
}