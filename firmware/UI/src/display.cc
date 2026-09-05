#include "core.h"
#include "display.h"
#include "ILI9225.h"
#include "telemetry.h"

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
    ili9225_set_font(ili9225_font_terminal6x8);
    ili9225_set_bg_color(ILI9225_COLOR_BLACK);

    display_update();
}

void display_update() {
    static bool s_is_init = false;
    static char itoa_buffer[12];
    static uint32_t s_call_counter = 0;

    ++s_call_counter;
    
    // Draw static text
    if (!s_is_init) {
        ili9225_draw_line(0, 27, 176, ILI9225_COLOR_WHITE);
        
        int y = 40;
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, " Freq (Hz):"); y += 13;
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, "   Arc cnt:"); y += 13;
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, " Tens. (g):"); y += 13;
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, " Feed (us):"); y += 13;
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, "Brake (us):"); y += 13;
        y += 13;
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, "   T1 (us):"); y += 13;
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, "   T0 (us):"); y += 13;
        y += 13;
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, "   X steps:"); y += 13;

        ili9225_draw_line(0, 205, 176, ILI9225_COLOR_WHITE);
        ili9225_draw_string(5, 210, ILI9225_COLOR_WHITE, "TX/RX/DS:");

        s_is_init = true;
    }

    // State
    if (s_call_counter == 1) {
        static int32_t s_last_v = -1;
        bool v = (rand() % 1000) > 500;
        if (s_last_v != v) {
            if (v) {
                ili9225_draw_string(135, 10, ILI9225_COLOR_GREEN, "[ ON]", 5);
            } else {
                ili9225_draw_string(135, 10, ILI9225_COLOR_RED, "[OFF]", 5);
            }
            s_last_v = v;
        }
        return;
    }

    int y = 40;

    // Freq
    if (s_call_counter == 2) {
        static uint32_t s_last_v = -1;
        uint32_t v = rand() % 100000;
        if (s_last_v != v) {
            ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(v, itoa_buffer, 10), 5);
            s_last_v = v;
        }
        return;
    }
    y += 13;

    // Arc counter
    if (s_call_counter == 3) {
        static uint32_t s_last_v = -1;
        uint32_t v = rand() % 100000;
        if (s_last_v != v) {
            ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(v, itoa_buffer, 10), 5);
            s_last_v = v;
        }
        return;
    }
    y += 13;

    // Tension (g)
    if (s_call_counter == 4) {
        static int32_t s_last_v = -1;
        int32_t v = rand() % 100000;
        if (s_last_v != v) {
            ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(v, itoa_buffer, 10), 5);
            s_last_v = v;
        }
        return;
    }
    y += 13;

    // Feeder freq
    if (s_call_counter == 5) {
        static int32_t s_last_v = -1;
        int32_t v = rand() % 100000;
        if (s_last_v != v) {
            ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(v, itoa_buffer, 10), 5);
            s_last_v = v;
        }
        return;
    }
    y += 13;

    // Brake freq
    if (s_call_counter == 6) {
        static int32_t s_last_v = -1;
        int32_t v = rand() % 100000;
        if (s_last_v != v) {
            ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(v, itoa_buffer, 10), 5);
            s_last_v = v;
        }
        return;
    }
    y += 13;
    y += 13;

    // T1
    if (s_call_counter == 7) {
        static int32_t s_last_v = -1;
        int32_t v = rand() % 100000;
        if (s_last_v != v) {
            ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(v, itoa_buffer, 10), 5);
            s_last_v = v;
        }
        return;
    }
    y += 13;

    // T0
    if (s_call_counter == 8) {
        static int32_t s_last_v = -1;
        int32_t v = rand() % 100000;
        if (s_last_v != v) {
            ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(v, itoa_buffer, 10), 5);
            s_last_v = v;
        }
        return;
    }
    y += 13;
    y += 13;

    // X steps
    if (s_call_counter == 9) {
        static int32_t s_last_v = -1;
        int32_t v = rand() % 100000;
        if (s_last_v != v) {
            ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(v, itoa_buffer, 10), 5);
            s_last_v = v;
        }
        return;
    }
    y += 13;


    // TX/RX/DS
    if (s_call_counter == 10) {
        static int32_t s_last_tx = -1;
        static int32_t s_last_rx = -1;
        static int32_t s_last_ds = -1;

        int32_t v = telemetry_get_tx_counter(); 
        if (s_last_tx != v) {
            ili9225_draw_string(70, 210, ILI9225_COLOR_YELLOW, itoa(v, itoa_buffer, 10), 3);
            s_last_tx = v;
        }

        v = telemetry_get_rx_counter();
        if (s_last_rx != v) {
            ili9225_draw_string(100, 210, ILI9225_COLOR_YELLOW, itoa(v, itoa_buffer, 10), 3);
            s_last_rx = v;
        }

        v = telemetry_get_desync_counter();
        if (s_last_ds != v) {
            ili9225_draw_string(130, 210, ILI9225_COLOR_YELLOW, itoa(v, itoa_buffer, 10), 3);
            s_last_ds = v;
        }
        return;
    }

    s_call_counter = 0;
}
