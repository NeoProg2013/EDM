#include "core.h"

extern "C" void HardFault_Handler(void) {
    while (true);
}

extern "C" void SysTick_Handler(void) {
    HAL_IncTick();
}
