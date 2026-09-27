#include "core.h"
#include "display.h"
#include "ILI9225.h"
#include "telemetry.h"
#include "controls.h"

#define MAIN_MENU_ITEM_EDM_STATUS        (0)
#define MAIN_MENU_ITEM_EDM_MOVEMENT      (1)

#define EDM_PARAMETERS_ITEM_T1           (0)
#define EDM_PARAMETERS_ITEM_T0           (1)

// UI pages
enum ui_page_t {
    PAGE_EDM_STATUS,
    PAGE_EDM_MOVEMENT,
    PAGE_EDM_PARAMETERS,
    PAGE_MENU,
};
ui_page_t g_page = PAGE_EDM_STATUS;
int8_t g_menu_item_idx = 0;



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

    ili9225_draw_hline(0, 205, 176, ILI9225_COLOR_WHITE);

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
        ili9225_draw_string(5, 6, ILI9225_COLOR_GREEN, "MOVEMENT");
        ili9225_draw_hline(0, 20, 176, ILI9225_COLOR_WHITE);
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

static void draw_page_edm_status() {
    static uint32_t s_call_counter = 0;
    static char itoa_buffer[12];
    ++s_call_counter;
    
    // Header
    if (s_call_counter == 1) {
        ili9225_draw_string(5, 6, ILI9225_COLOR_GREEN, "STATUS      RX    TX");
        ili9225_draw_hline(0, 20, 176, ILI9225_COLOR_WHITE);
        return;
    }
    
    
    int y = 30;

    rx_msg_t rx_msg;
    telemetry_get_rx_msg(&rx_msg);
    tx_msg_t* tx_msg = telemetry_get_tx_msg();

    // EDM
    if (s_call_counter == 2) {
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, "       EDM:");

        if (rx_msg.edm_status) {
            ili9225_draw_string(90, y, ILI9225_COLOR_GREEN, "ON", 3);
        } else {
            ili9225_draw_string(90, y, ILI9225_COLOR_RED, "OFF", 3);
        }
        if (tx_msg->edm_status) {
            ili9225_draw_string(130, y, ILI9225_COLOR_GREEN, "ON", 3);
        } else {
            ili9225_draw_string(130, y, ILI9225_COLOR_RED, "OFF", 3);
        }
        return;
    }
    y += 13;

    // X
    if (s_call_counter == 3) {
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, "         X:");
        ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(rx_msg.x, itoa_buffer, 10), 5);
        ili9225_draw_string(130, y, ILI9225_COLOR_YELLOW, "----");
        return;
    }
    y += 13;

    // Y
    if (s_call_counter == 4) {
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, "         Y:");
        ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(rx_msg.y, itoa_buffer, 10), 5);
        ili9225_draw_string(130, y, ILI9225_COLOR_YELLOW, "----");
        return;
    }
    y += 13;

    // T1
    if (s_call_counter == 5) {
        if (g_menu_item_idx == EDM_PARAMETERS_ITEM_T1) {
            ili9225_draw_string(5, y, ILI9225_COLOR_CYAN, "->      T1:");
        } else {
            ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, "        T1:");
        }
        ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(rx_msg.t1, itoa_buffer, 10), 5);
        ili9225_draw_string(130, y, ILI9225_COLOR_YELLOW, itoa(tx_msg->t1, itoa_buffer, 10), 5);
        return;
    }
    y += 13;

    // T0
    if (s_call_counter == 6) {
        if (g_menu_item_idx == EDM_PARAMETERS_ITEM_T0) {
            ili9225_draw_string(5, y, ILI9225_COLOR_CYAN, "->      T0:");
        } else {
            ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, "        T0:");
        }
        ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(rx_msg.t0, itoa_buffer, 10), 5);
        ili9225_draw_string(130, y, ILI9225_COLOR_YELLOW, itoa(tx_msg->t0, itoa_buffer, 10), 5);
        return;
    }
    y += 13;

    //
    // DEBUG
    y += 13;

    // Freq
    if (s_call_counter == 7) {
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, " Freq (Hz):");
        ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(rx_msg.freq_hz, itoa_buffer, 10), 5);
        return;
    }
    y += 13;

    // Arc counter
    if (s_call_counter == 8) {
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, "   Arc cnt:");
        ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(rx_msg.arc_counter, itoa_buffer, 10), 5);
        return;
    }
    y += 13;

    // Tension (g)
    if (s_call_counter == 9) {
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, " Tens. (g):");
        ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(rx_msg.tension_g, itoa_buffer, 10), 5);
        return;
    }
    y += 13;

    // Feeder freq
    if (s_call_counter == 10) {
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, " Feed (us):");
        ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(rx_msg.feeder_us, itoa_buffer, 10), 5);
        return;
    }
    y += 13;

    // Brake freq
    if (s_call_counter == 11) {
        ili9225_draw_string(5, y, ILI9225_COLOR_WHITE, "Brake (us):");
        ili9225_draw_string(90, y, ILI9225_COLOR_YELLOW, itoa(rx_msg.brake_us, itoa_buffer, 10), 5);
        return;
    }

    // TX/RX/DS
    if (s_call_counter == 12) {
        draw_footer();
        return;
    }

    s_call_counter = 0;
}

static void draw_page_menu() {
    static uint32_t s_call_counter = 0;
    ++s_call_counter;

    // Header
    if (s_call_counter == 1) {
        ili9225_draw_string(5, 6, ILI9225_COLOR_GREEN, "MAIN MENU", 5);
        ili9225_draw_hline(0, 20, 176, ILI9225_COLOR_WHITE);
        return;
    }
    int y = 40;

    if (s_call_counter == 2) {
        if (g_menu_item_idx == MAIN_MENU_ITEM_EDM_STATUS) {
            ili9225_draw_string(5, y, ILI9225_COLOR_YELLOW, "-> EDM STATUS");
        } else {
            ili9225_draw_string(5, y, ILI9225_COLOR_WHITE,  "   EDM STATUS");
        }
        return;
    }
    y += 13;

    if (s_call_counter == 3) {
        if (g_menu_item_idx == MAIN_MENU_ITEM_EDM_MOVEMENT) {
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

static void switch_page(controls_state_t* controls_state, ui_page_t new_page) {
    ili9225_clear();
    g_page = new_page;
    g_menu_item_idx = 0;
}



void display_init() {
    ili9225_init();
    ili9225_clear();
    ili9225_set_font(ili9225_font_terminal6x8);
    ili9225_set_bg_color(ILI9225_COLOR_BLACK);
}

void display_process() {
    static bool is_buttons_release = false;

    controls_state_t* controls_state = controls_get_state();

    if (g_page == ui_page_t::PAGE_EDM_STATUS) {
        draw_page_edm_status();

        if (is_buttons_release) {
            if (controls_state->btn_start_stop) {
                telemetry_get_tx_msg()->edm_status = !telemetry_get_tx_msg()->edm_status;
            } else if (controls_state->btn_center) {
                switch_page(controls_state, ui_page_t::PAGE_MENU);
            } else if (controls_state->btn_down) {
                g_menu_item_idx++;
                if (g_menu_item_idx > 1) {
                    g_menu_item_idx = 0;
                }
            } else if (controls_state->btn_up) {
                g_menu_item_idx--;
                if (g_menu_item_idx < 0) {
                    g_menu_item_idx = 1;
                }
            } else if (controls_state->btn_left) {
                if (g_menu_item_idx == EDM_PARAMETERS_ITEM_T0) {
                    telemetry_get_tx_msg()->t0 = constrain(telemetry_get_tx_msg()->t0 - 10, 200, 500);
                } else if (g_menu_item_idx == EDM_PARAMETERS_ITEM_T1) {
                    telemetry_get_tx_msg()->t1 = constrain(telemetry_get_tx_msg()->t1 - 1, 1, 10);
                }
            } else if (controls_state->btn_right) {
                if (g_menu_item_idx == EDM_PARAMETERS_ITEM_T0) {
                    telemetry_get_tx_msg()->t0 = constrain(telemetry_get_tx_msg()->t0 + 10, 200, 500);
                } else if (g_menu_item_idx == EDM_PARAMETERS_ITEM_T1) {
                    telemetry_get_tx_msg()->t1 = constrain(telemetry_get_tx_msg()->t1 + 1, 1, 10);
                }
            }
        }
    } else if (g_page == ui_page_t::PAGE_EDM_MOVEMENT) {
        draw_page_movement(controls_state->btn_up, controls_state->btn_down, controls_state->btn_left, controls_state->btn_right);

        if (is_buttons_release) {
            if (controls_state->btn_center) {
                switch_page(controls_state, ui_page_t::PAGE_MENU);
            } 
            telemetry_get_tx_msg()->cmd = tx_msg_t::CMD_NONE;
        } else {
            if (controls_state->btn_up)    telemetry_get_tx_msg()->cmd = tx_msg_t::CMD_MOVE_UP;
            if (controls_state->btn_down)  telemetry_get_tx_msg()->cmd = tx_msg_t::CMD_MOVE_DOWN;
            if (controls_state->btn_left)  telemetry_get_tx_msg()->cmd = tx_msg_t::CMD_MOVE_LEFT;
            if (controls_state->btn_right) telemetry_get_tx_msg()->cmd = tx_msg_t::CMD_MOVE_RIGHT;
        }
    } else if (g_page == ui_page_t::PAGE_MENU) {
        draw_page_menu();

        if (is_buttons_release) {
            if (controls_state->btn_down) {
                g_menu_item_idx++;
                if (g_menu_item_idx > 1) {
                    g_menu_item_idx = 0;
                }
            } else if (controls_state->btn_up) {
                g_menu_item_idx--;
                if (g_menu_item_idx < 0) {
                    g_menu_item_idx = 1;
                }
            } else if (controls_state->btn_center) {
                if (g_menu_item_idx == MAIN_MENU_ITEM_EDM_STATUS) {
                    switch_page(controls_state, ui_page_t::PAGE_EDM_STATUS);
                } else if (g_menu_item_idx == MAIN_MENU_ITEM_EDM_MOVEMENT) {
                    switch_page(controls_state, ui_page_t::PAGE_EDM_MOVEMENT);
                }
            }
        }
    }

    // All buttons released?
    is_buttons_release = (!controls_state->btn_start_stop && !controls_state->btn_left && !controls_state->btn_down && 
                          !controls_state->btn_center && !controls_state->btn_right && !controls_state->btn_up);
}
