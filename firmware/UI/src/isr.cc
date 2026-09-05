#include "core.h"
#include "telemetry.h"

extern "C" void HardFault_Handler(void) {
    while (true);
}

extern "C" void SysTick_Handler(void) {
    HAL_IncTick();
}

// USART1 + DMA TX
extern "C" void DMA1_Channel2_3_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_usart1_tx);
}
extern "C" void USART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&usart1);
}