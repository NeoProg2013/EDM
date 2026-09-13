#include "core.h"
#include "display.h"
#include "ILI9225.h"
#include "telemetry.h"
#include "ui.h"

#define MAIN_MENU_ITEM_EDM_STATUS        (0)
#define MAIN_MENU_ITEM_EDM_MOVEMENT      (1)
#define MAIN_MENU_ITEM_EDM_PARAMETERS    (2)

#define EDM_PARAMETERS_ITEM_T1           (0)
#define EDM_PARAMETERS_ITEM_T0           (1)


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
        ili9225_draw_string(5, 10, ILI9225_COLOR_GREEN, "EDM STATUS");
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

static void draw_page_edm_parameters(rx_msg_t* rx_msg, int8_t menu_item_idx, bool is_buttons_release, bool left_btn, bool right_btn) {
    static uint32_t s_call_counter = 0;
    ++s_call_counter;

    tx_msg_t* tx_msg = telemetry_get_tx_msg();

    if (is_buttons_release) {
        if (menu_item_idx == EDM_PARAMETERS_ITEM_T0) {
            if (left_btn && tx_msg->t0 > 200) {
                tx_msg->t0 -= 5;
            } else if (right_btn && tx_msg->t0 < 1000) {
                tx_msg->t0 += 5;
            }
        } else if (menu_item_idx == EDM_PARAMETERS_ITEM_T1) {
            if (left_btn && tx_msg->t1 > 1) {
                --tx_msg->t1;
            } else if (right_btn && tx_msg->t1 < 10) {
                ++tx_msg->t1;
            }
        }
    }
    
    // Draw static text
    if (s_call_counter == 1) {
        ili9225_draw_string(5, 10, ILI9225_COLOR_GREEN, "EDM PARAMETERS");
        ili9225_draw_hline(0, 27, 176, ILI9225_COLOR_WHITE);

        ili9225_draw_hline(0, 205, 176, ILI9225_COLOR_WHITE);        
        return;
    }
    int y = 40;

    char itoa_buffer[12];
    char str_buffer[32];

    if (s_call_counter == 2) {
        char* p = str_buffer;
        p += strlen(strcpy(p, "-> T1 (us): "));
        p += strlen(strcpy(p, itoa(tx_msg->t1, itoa_buffer, 10)));
        p += strlen(strcpy(p, " | "));
        p += strlen(strcpy(p, itoa(rx_msg->t1, itoa_buffer, 10)));
        if (menu_item_idx == EDM_PARAMETERS_ITEM_T1) {
            ili9225_draw_string(5, y, ILI9225_COLOR_YELLOW, str_buffer, 25);
        } else {
            ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, str_buffer, 25);
        }
        return;
    }
    y += 13;

    if (s_call_counter == 3) {
        char* p = str_buffer;
        p += strlen(strcpy(p, "-> T0 (us): "));
        p += strlen(strcpy(p, itoa(tx_msg->t0, itoa_buffer, 10)));
        p += strlen(strcpy(p, " | "));
        p += strlen(strcpy(p, itoa(rx_msg->t0, itoa_buffer, 10)));
        if (menu_item_idx == EDM_PARAMETERS_ITEM_T0) {
            ili9225_draw_string(5, y, ILI9225_COLOR_YELLOW, str_buffer, 25);
        } else {
            ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, str_buffer, 25);
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

static void draw_page_menu(int8_t menu_item_idx) {
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
        if (menu_item_idx == MAIN_MENU_ITEM_EDM_MOVEMENT) {
            ili9225_draw_string(5, y, ILI9225_COLOR_YELLOW, "-> MOVEMENT");
        } else {
            ili9225_draw_string(5, y, ILI9225_COLOR_WHITE,  "   MOVEMENT");
        }
        return;
    }
    y += 13;

    if (s_call_counter == 4) {
        if (menu_item_idx == MAIN_MENU_ITEM_EDM_PARAMETERS) {
            ili9225_draw_string(5, y, ILI9225_COLOR_YELLOW, "-> PARAMETERS");
        } else {
            ili9225_draw_string(5, y, ILI9225_COLOR_WHITE,  "   PARAMETERS");
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

static void switch_page(int8_t* menu_item_idx, ui_state_t* ui_state, ui_page_t new_page) {
    ili9225_clear();
    ui_state->page = new_page;
    *menu_item_idx = 0;
}


void display_init() {
    ili9225_init();
    ili9225_clear();
    ili9225_set_font(ili9225_font_terminal6x8);
    ili9225_set_bg_color(ILI9225_COLOR_BLACK);
}

void display_process() {
    static int8_t s_menu_item_idx = 0;
    static bool is_buttons_release = false;

    rx_msg_t rx_msg;
    telemetry_get_rx_msg(&rx_msg);

    ui_state_t* ui_state = ui_get_state();

    if (ui_state->page == ui_page_t::PAGE_EDM_STATUS) {
        draw_page_edm_status(&rx_msg);

        if (is_buttons_release) {
            if (ui_state->button_start_stop) {
                telemetry_get_tx_msg()->edm_status = !telemetry_get_tx_msg()->edm_status;
            } else if (ui_state->button_center) {
                switch_page(&s_menu_item_idx, ui_state, ui_page_t::PAGE_MENU);
            }
        }
    } else if (ui_state->page == ui_page_t::PAGE_EDM_MOVEMENT) {
        draw_page_movement(ui_state->button_up, ui_state->button_down, ui_state->button_left, ui_state->button_right);

        if (is_buttons_release) {
            if (ui_state->button_center) {
                switch_page(&s_menu_item_idx, ui_state, ui_page_t::PAGE_MENU);
            } 
            telemetry_get_tx_msg()->cmd = tx_msg_t::CMD_NONE;
        } else {
            if (ui_state->button_up)    telemetry_get_tx_msg()->cmd = tx_msg_t::CMD_MOVE_UP;
            if (ui_state->button_down)  telemetry_get_tx_msg()->cmd = tx_msg_t::CMD_MOVE_DOWN;
            if (ui_state->button_left)  telemetry_get_tx_msg()->cmd = tx_msg_t::CMD_MOVE_LEFT;
            if (ui_state->button_right) telemetry_get_tx_msg()->cmd = tx_msg_t::CMD_MOVE_RIGHT;
        }
    } else if (ui_state->page == ui_page_t::PAGE_EDM_PARAMETERS) {
        draw_page_edm_parameters(&rx_msg, s_menu_item_idx, is_buttons_release, ui_state->button_left, ui_state->button_right);

        if (is_buttons_release) {
            if (ui_state->button_down) {
                s_menu_item_idx++;
                if (s_menu_item_idx > 1) {
                    s_menu_item_idx = 0;
                }
            } else if (ui_state->button_up) {
                s_menu_item_idx--;
                if (s_menu_item_idx < 0) {
                    s_menu_item_idx = 1;
                }
            } else if (ui_state->button_center) {
                switch_page(&s_menu_item_idx, ui_state, ui_page_t::PAGE_MENU);
            } 
        }
    } else if (ui_state->page == ui_page_t::PAGE_MENU) {
        draw_page_menu(s_menu_item_idx);

        if (is_buttons_release) {
            if (ui_state->button_down) {
                s_menu_item_idx++;
                if (s_menu_item_idx > 2) {
                    s_menu_item_idx = 0;
                }
            } else if (ui_state->button_up) {
                s_menu_item_idx--;
                if (s_menu_item_idx < 0) {
                    s_menu_item_idx = 2;
                }
            } else if (ui_state->button_center) {
                if (s_menu_item_idx == MAIN_MENU_ITEM_EDM_STATUS) {
                    switch_page(&s_menu_item_idx, ui_state, ui_page_t::PAGE_EDM_STATUS);
                } else if (s_menu_item_idx == MAIN_MENU_ITEM_EDM_MOVEMENT) {
                    switch_page(&s_menu_item_idx, ui_state, ui_page_t::PAGE_EDM_MOVEMENT);
                } else if (s_menu_item_idx == MAIN_MENU_ITEM_EDM_PARAMETERS) {
                    switch_page(&s_menu_item_idx, ui_state, ui_page_t::PAGE_EDM_PARAMETERS);
                }
            }
        }
    }

    // All buttons released?
    is_buttons_release = (!ui_state->button_start_stop && !ui_state->button_left && !ui_state->button_down && 
                          !ui_state->button_center && !ui_state->button_right && !ui_state->button_up);
}
