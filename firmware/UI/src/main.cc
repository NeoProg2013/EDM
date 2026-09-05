#include "core.h"
#include "display.h"
#include "telemetry.h"


void system_clock_init() {
    // Init HSI -> PLL
    RCC_OscInitTypeDef osc = {0};
    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState            = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState        = RCC_PLL_ON;
    osc.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    osc.PLL.PLLMUL          = RCC_PLL_MUL12; // 4 MHz (HSI/2) * 12 = 48 MHz
    osc.PLL.PREDIV          = RCC_PREDIV_DIV1;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        while(1);
    }

    // Init clocks (CPU, AHB, APB1)
    RCC_ClkInitTypeDef clk = {0};
    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1; // APB1 = 48 MHz
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_1) != HAL_OK) {
        while(1);
    }

    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
}


int main() {
    HAL_Init();
    system_clock_init();

    display_init();
    telemetry_init();

    while (true) {
        display_update();
    }

    return 0;
}
