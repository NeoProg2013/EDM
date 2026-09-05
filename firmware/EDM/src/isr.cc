#include "core.h"
#include "telemetry.h"

extern "C" void HardFault_Handler(void) {
    while (true);
}

// extern "C" void SysTick_Handler(void) {
//     HAL_IncTick();
// }

// USART1 + DMA TX
// extern "C" void DMA1_Stream6_IRQHandler(void) {
//     HAL_DMA_IRQHandler(&dma_usart2_tx);
// }
// extern "C" void USART2_IRQHandler(void) {
//     HAL_UART_IRQHandler(&usart2);
// }