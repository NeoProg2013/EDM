#include "core.h"
#include "display.h"
#include "telemetry.h"
#include "ui.h"
#include "spi1.h"


static void system_clock_init() {
    // Init HSE 8 MHz -> PLL
    RCC_OscInitTypeDef osc = {0};
    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState            = RCC_HSE_ON;
    osc.HSEPredivValue      = RCC_HSE_PREDIV_DIV1;
    osc.PLL.PLLState        = RCC_PLL_ON;
    osc.PLL.PLLSource       = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL          = RCC_PLL_MUL9; // 8 MHz * 9 = 72 MHz
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        while(1);
    }

    // Init clocks (CPU, AHB, APB1)
    RCC_ClkInitTypeDef clk = {0};
    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1; // AHB (HCLK) = 72 MHz
    clk.APB1CLKDivider = RCC_HCLK_DIV2;   // PCLK1 = 36 MHz
    clk.APB2CLKDivider = RCC_HCLK_DIV1;   // PCLK2 = 72 MHz
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_1) != HAL_OK) {
        while(1);
    }

    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();
    // __HAL_RCC_DMA1_CLK_ENABLE();
    // __HAL_RCC_USART1_CLK_ENABLE();
    // __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    // PC13 - LED
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin       = GPIO_PIN_13;
    gpio.Mode      = GPIO_MODE_OUTPUT_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &gpio);

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
}

int main() {
    HAL_Init();
    system_clock_init();

    // spi1_init();
    display_init();
    // telemetry_init();
    // ui_init();

    while (true) {
        
        // uint8_t data = 0xAA;
        // spi1_write(&data, 1);
        display_process();
        // ui_process();
        // telemetry_process();
    }

    return 0;
}
