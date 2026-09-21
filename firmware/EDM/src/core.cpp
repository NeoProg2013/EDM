#include "core.h"

uint64_t HAL_GetTickUs(void) {
    uint64_t ticks = 0;
    uint64_t value = 0;
    uint64_t load  = 0;

    do {
        ticks = HAL_GetTick();
        value = SysTick->VAL;
        load  = SysTick->LOAD;
    } while (ticks != HAL_GetTick());

    return (ticks * 1000) + ((load - value) * 1000 / (load + 1));
}