#include "core.h"
#include "display.h"
#include "ILI9225.h"
#include "telemetry.h"
#include "ui.h"

SPI_HandleTypeDef hspi1 = {0}; // For ILI9225

#define MAIN_MENU_ITEM_EDM_STATUS        (0)
#define MAIN_MENU_ITEM_MOVEMENT          (1)



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
    gpio.Pin       = GPIO_PIN_1;
    gpio.Mode      = GPIO_MODE_OUTPUT_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
}

static void draw_footer() {
    if (!telemetry_get_connection_state()) {
        ili9225_draw_string(5, 210, ILI9225_COLOR_RED, "TX/RX/DS:");
    } else {
        if (telemetry_get_sync_state()) {
            ili9225_draw_string(5, 210, ILI9225_COLOR_YELLOW, "TX/RX/DS:");
        } else {
            ili9225_draw_string(5, 210, ILI9225_COLOR_GREEN, "TX/RX/DS:");
        }
    }

    // TX/RX/DS
    char itoa_buffer[12];
    ili9225_draw_string(80, 210, ILI9225_COLOR_YELLOW, itoa(telemetry_get_tx_counter(), itoa_buffer, 10), 3);
    ili9225_draw_string(110, 210, ILI9225_COLOR_YELLOW, itoa(telemetry_get_rx_counter(), itoa_buffer, 10), 3);
    ili9225_draw_string(140, 210, ILI9225_COLOR_YELLOW, itoa(telemetry_get_desync_counter(), itoa_buffer, 10), 3);
}

static void draw_page_movement(bool u, bool d, bool l, bool r) {
    static uint32_t s_call_counter = 0;
    ++s_call_counter;

    // Draw static text
    if (s_call_counter == 1) {
        ili9225_draw_string(5, 10, ILI9225_COLOR_GREEN, "MOVEMENT", 5);
        ili9225_draw_hline(0, 27, 176, ILI9225_COLOR_WHITE);
        
        ili9225_draw_hline(0, 205, 176, ILI9225_COLOR_WHITE);
        return;
    }
    int y = 45;

    if (s_call_counter == 2) {
        ili9225_draw_hline(15,  y + 70, 60, l ? ILI9225_COLOR_YELLOW : ILI9225_COLOR_WHITE);
        ili9225_draw_hline(100, y + 70, 60, r ? ILI9225_COLOR_YELLOW : ILI9225_COLOR_WHITE);
        ili9225_draw_vline(88,  y,      60, u ? ILI9225_COLOR_YELLOW : ILI9225_COLOR_WHITE);
        ili9225_draw_vline(88,  y + 80, 60, d ? ILI9225_COLOR_YELLOW : ILI9225_COLOR_WHITE);
        return;
    }

    if (s_call_counter == 3) {
        ili9225_draw_string(15,  y + 55,  l ? ILI9225_COLOR_YELLOW : ILI9225_COLOR_WHITE, "-X");
        ili9225_draw_string(145, y + 55,  r ? ILI9225_COLOR_YELLOW : ILI9225_COLOR_WHITE, "+X");
        ili9225_draw_string(93,  y,       u ? ILI9225_COLOR_YELLOW : ILI9225_COLOR_WHITE, "+Y");
        ili9225_draw_string(93,  y + 133, d ? ILI9225_COLOR_YELLOW : ILI9225_COLOR_WHITE, "-Y");
        return;
    }

    // TX/RX/DS
    if (s_call_counter == 4) {
        draw_footer();
        return;
    }
    
    s_call_counter = 0;
}

static void draw_page_edm_status(rx_msg_t* telemetry) {
    static uint32_t s_call_counter = 0;
    ++s_call_counter;
    
    // Draw static text
    if (s_call_counter == 1) {
        ili9225_draw_string(5, 10, ILI9225_COLOR_GREEN, "EDM STATUS", 5);
        ili9225_draw_hline(0, 27, 176, ILI9225_COLOR_WHITE);
        
        int y = 40;
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, " Freq (Hz):"); y += 13;
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, "   Arc cnt:"); y += 13;
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, " Tens. (g):"); y += 13;
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, " Feed (us):"); y += 13;
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, "Brake (us):"); y += 13;
        y += 13;
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, "   T1 (us):"); y += 13;
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, "   T0 (us):"); y += 13;
        
        ili9225_draw_hline(0, 205, 176, ILI9225_COLOR_WHITE);
        
        return;
    }

    char itoa_buffer[12];

    // State
    if (s_call_counter == 2) {
        if (telemetry->arc_state) {
            ili9225_draw_string(135, 10, ILI9225_COLOR_GREEN, "[ ON]", 5);
        } else {
            ili9225_draw_string(135, 10, ILI9225_COLOR_RED, "[OFF]", 5);
        }
        return;
    }

    int y = 40;

    // Freq
    if (s_call_counter == 3) {
        ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(telemetry->freq_hz, itoa_buffer, 10), 5);
        return;
    }
    y += 13;

    // Arc counter
    if (s_call_counter == 4) {
        ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(telemetry->arc_counter, itoa_buffer, 10), 5);
        return;
    }
    y += 13;

    // Tension (g)
    if (s_call_counter == 5) {
        ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(telemetry->tension_g, itoa_buffer, 10), 5);
        return;
    }
    y += 13;

    // Feeder freq
    if (s_call_counter == 6) {
        ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(telemetry->feeder_us, itoa_buffer, 10), 5);
        return;
    }
    y += 13;

    // Brake freq
    if (s_call_counter == 7) {
        ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(telemetry->brake_us, itoa_buffer, 10), 5);
        return;
    }
    y += 13;
    y += 13;

    // T1
    if (s_call_counter == 8) {
        ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(telemetry->t1, itoa_buffer, 10), 5);
        return;
    }
    y += 13;

    // T0
    if (s_call_counter == 9) {
        ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(telemetry->t0, itoa_buffer, 10), 5);
        return;
    }
    y += 13;

    // TX/RX/DS
    if (s_call_counter == 10) {
        draw_footer();
        return;
    }

    s_call_counter = 0;
}

static void draw_page_menu(int32_t menu_item_idx) {
    static uint32_t s_call_counter = 0;
    ++s_call_counter;

    // Draw static text
    if (s_call_counter == 1) {
        ili9225_draw_string(5, 10, ILI9225_COLOR_GREEN, "MAIN MENU", 5);
        ili9225_draw_hline(0, 27, 176, ILI9225_COLOR_WHITE);
        
        ili9225_draw_hline(0, 205, 176, ILI9225_COLOR_WHITE);
        return;
    }
    int y = 40;

    if (s_call_counter == 2) {
        if (menu_item_idx == MAIN_MENU_ITEM_EDM_STATUS) {
            ili9225_draw_string(5, y, ILI9225_COLOR_YELLOW, "-> EDM STATUS");
        } else {
            ili9225_draw_string(5, y, ILI9225_COLOR_WHITE,  "   EDM STATUS");
        }
        return;
    }
    y += 13;

    if (s_call_counter == 3) {
        if (menu_item_idx == MAIN_MENU_ITEM_MOVEMENT) {
            ili9225_draw_string(5, y, ILI9225_COLOR_YELLOW, "-> MOVEMENT");
        } else {
            ili9225_draw_string(5, y, ILI9225_COLOR_WHITE,  "   MOVEMENT");
        }
        return;
    }
    y += 13;

    // TX/RX/DS
    if (s_call_counter == 4) {
        draw_footer();
        return;
    }
    
    s_call_counter = 0;
}


void display_init() {
    hspi1_init();
    init_ili9225_gpio();

    ili9225_init();
    ili9225_clear();
    ili9225_set_font(ili9225_font_terminal6x8);
    ili9225_set_bg_color(ILI9225_COLOR_BLACK);
}

void display_process() {
    static int32_t s_menu_item_idx = 0;
    static bool is_button_release = false;

    rx_msg_t telemetry;
    telemetry_get_rx_msg(&telemetry);
    ui_state_t* ui_state = ui_get_state();

    if (ui_state->page == ui_page_t::PAGE_STATUS) {
        draw_page_edm_status(&telemetry);

        if (is_button_release) {
            if (ui_state->button_start_stop) {
                telemetry_tx(tx_msg_t{ .cmd = tx_msg_t::CMD_START_STOP_EDM });
            } else if (ui_state->button_center) {
                ui_state->page = ui_page_t::PAGE_MENU;
                ili9225_clear();
            }
        }
    } else if (ui_state->page == ui_page_t::PAGE_MOVEMENT) {
        draw_page_movement(ui_state->button_up, ui_state->button_down, ui_state->button_left, ui_state->button_right);

        if (is_button_release) {
            if (ui_state->button_center) {
                ui_state->page = ui_page_t::PAGE_MENU;
                ili9225_clear();
            }
        }
    } else if (ui_state->page == ui_page_t::PAGE_MENU) {
        draw_page_menu(s_menu_item_idx);

        if (is_button_release) {
            if (ui_state->button_down) {
                s_menu_item_idx++;
                if (s_menu_item_idx > 3) {
                    s_menu_item_idx = 0;
                }
            } else if (ui_state->button_up) {
                s_menu_item_idx--;
                if (s_menu_item_idx < 0) {
                    s_menu_item_idx = 2;
                }
            } else if (ui_state->button_center) {
                if (s_menu_item_idx == MAIN_MENU_ITEM_EDM_STATUS) {
                    ui_state->page = ui_page_t::PAGE_STATUS;
                } else if (s_menu_item_idx == MAIN_MENU_ITEM_MOVEMENT) {
                    ui_state->page = ui_page_t::PAGE_MOVEMENT;
                }
                
                ili9225_clear();
            }
        }
    }

    // All buttons released?
    is_button_release = (!ui_state->button_start_stop && !ui_state->button_left && !ui_state->button_down && 
                         !ui_state->button_center && !ui_state->button_right && !ui_state->button_up);
}
