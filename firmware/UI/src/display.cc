#include "core.h"
#include "display.h"
#include "ILI9225.h"

SPI_HandleTypeDef hspi1 = {0}; // For ILI9225


static void hspi1_init(void) {
    // PA5 -> SPI1_SCK
    // PA7 -> SPI1_MOSI
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin       = GPIO_PIN_5 | GPIO_PIN_7;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF0_SPI1;
    HAL_GPIO_Init(GPIOA, &gpio);

    hspi1.Instance               = SPI1;
    hspi1.Init.Mode              = SPI_MODE_MASTER;         // Master mode
    hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;       // 8 bit
    hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;        // CPOL = 0
    hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;         // CPHA = 0
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2; // 48 MHz / 4 = 24 MHz
    hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;        // MSB first
    hspi1.Init.NSS               = SPI_NSS_SOFT;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        while(1);
    }

    __HAL_SPI_ENABLE(&hspi1);
}

// PA5 - SCK  (SPI1)
// PA7 - MOSI (SPI1)
// PA6 - CS
// PB1 - RS
// PA9 - RST
static void init_ili9225_gpio() {
    // PA6 - CS
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin       = GPIO_PIN_6;
    gpio.Mode      = GPIO_MODE_OUTPUT_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    // PB1 - RS
    gpio.Pin       = GPIO_PIN_1;
    gpio.Mode      = GPIO_MODE_OUTPUT_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);

    // PA9 - RST
    gpio.Pin       = GPIO_PIN_9;
    gpio.Mode      = GPIO_MODE_OUTPUT_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
}


void display_init() {
    hspi1_init();
    init_ili9225_gpio();

    ili9225_init();
    ili9225_clear();

    display_update();
}

void display_update() {
    static bool s_is_init = false;
    static char itoa_buffer[12];
    static uint32_t s_call_counter = 0;

    ++s_call_counter;
    
    // Draw static text
    if (!s_is_init) {
        ili9225_draw_string(5, 6, ILI9225_COLOR_RED, ILI9225_COLOR_BLACK, "DISABLED");
        ili9225_draw_line(0, 27, 176, ILI9225_COLOR_GRAY);
        
        int y = 40;
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, ILI9225_COLOR_BLACK, "  Freq (Hz): -----"); y += 15;
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, ILI9225_COLOR_BLACK, "        S/C: -----"); y += 15;
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, ILI9225_COLOR_BLACK, "Tension (g): -----"); y += 15;
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, ILI9225_COLOR_BLACK, "Feeder (us): -----"); y += 15;
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, ILI9225_COLOR_BLACK, " Brake (us): -----"); y += 15;
        y += 15;
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, ILI9225_COLOR_BLACK, "  High (us): -----"); y += 15;
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, ILI9225_COLOR_BLACK, "   Low (us): -----"); y += 15;

        s_is_init = true;
    }

    // State
    if (s_call_counter == 1) {
        static int32_t s_last_state = -1;
        bool v = !s_last_state;
        if (s_last_state != v) {
            if (v) {
                ili9225_draw_string(5, 6, ILI9225_COLOR_GREEN, ILI9225_COLOR_BLACK, "ENABLED", 8);
            } else {
                ili9225_draw_string(5, 6, ILI9225_COLOR_RED, ILI9225_COLOR_BLACK, "DISABLED", 8);
            }
            s_last_state = v;
        }
        return;
    }

    int y = 40;

    // Freq
    if (s_call_counter == 2) {
        static uint32_t s_last_freq = -1;
        uint32_t v = rand() % 100000;
        if (s_last_freq != v) {
            ili9225_draw_string(120, y, ILI9225_COLOR_YELLOW, ILI9225_COLOR_BLACK, itoa(v, itoa_buffer, 10), 5);
            s_last_freq = v;
        }
        return;
    }
    y += 15;

    // Short circuit count
    if (s_call_counter == 3) {
        static uint32_t s_last_short_circuit_counter = -1;
        uint32_t v = rand() % 100000;
        if (s_last_short_circuit_counter != v) {
            ili9225_draw_string(120, y, ILI9225_COLOR_YELLOW, ILI9225_COLOR_BLACK, itoa(v, itoa_buffer, 10), 5);
            s_last_short_circuit_counter = v;
        }
        return;
    }
    y += 15;

    // Tension (g)
    if (s_call_counter == 4) {
        static int32_t s_last_tension_g = -1;
        int32_t v = rand() % 100000;
        if (s_last_tension_g != v) {
            ili9225_draw_string(120, y, ILI9225_COLOR_YELLOW, ILI9225_COLOR_BLACK, itoa(v, itoa_buffer, 10), 5);
            s_last_tension_g = v;
        }
        return;
    }
    y += 15;

    // Feeder freq
    if (s_call_counter == 5) {
        static int32_t s_last_feeder_period_us = -1;
        int32_t v = rand() % 100000;
        if (s_last_feeder_period_us != v) {
            ili9225_draw_string(120, y, ILI9225_COLOR_YELLOW, ILI9225_COLOR_BLACK, itoa(v, itoa_buffer, 10), 5);
            s_last_feeder_period_us = v;
        }
        return;
    }
    y += 15;

    // Brake freq
    if (s_call_counter == 6) {
        static int32_t s_last_brake_period_us = -1;
        int32_t v = rand() % 100000;
        if (s_last_brake_period_us != v) {
            ili9225_draw_string(120, y, ILI9225_COLOR_YELLOW, ILI9225_COLOR_BLACK, itoa(v, itoa_buffer, 10), 5);
            s_last_brake_period_us = v;
        }
        return;
    }
    y += 15;
    y += 15;

    // T1
    if (s_call_counter == 7) {
        static int32_t s_last_t1_period_us = -1;
        int32_t v = rand() % 100000;
        if (s_last_t1_period_us != v) {
            ili9225_draw_string(120, y, ILI9225_COLOR_YELLOW, ILI9225_COLOR_BLACK, itoa(v, itoa_buffer, 10), 5);
            s_last_t1_period_us = v;
        }
        return;
    }
    y += 15;

    // T0
    if (s_call_counter == 8) {
        static int32_t s_last_t0_period_us = -1;
        int32_t v = rand() % 100000;
        if (s_last_t0_period_us != v) {
            ili9225_draw_string(120, y, ILI9225_COLOR_YELLOW, ILI9225_COLOR_BLACK, itoa(v, itoa_buffer, 10), 5);
            s_last_t0_period_us = v;
        }
        return;
    }

    s_call_counter = 0;
}
