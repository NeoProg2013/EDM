#include "core.h"
#include "ui.h"
#include "MCP23017.h"

// Keyboard
#define BUTTON_START_STOP   (0x04)
#define BUTTON_LEFT         (0x08)
#define BUTTON_DOWN         (0x10)
#define BUTTON_CENTER       (0x20)
#define BUTTON_RIGHT        (0x40)
#define BUTTON_UP           (0x80)



static ui_state_t g_ui_state {
    .page = PAGE_STATUS
};



void ui_init() {
    mcp23017_init();
}

void ui_process() {
    uint8_t v = mcp23017_read_porta();
    g_ui_state.button_start_stop = (v & BUTTON_START_STOP) == 0x00;
    g_ui_state.button_left       = (v & BUTTON_LEFT) == 0x00;
    g_ui_state.button_down       = (v & BUTTON_DOWN) == 0x00;
    g_ui_state.button_center     = (v & BUTTON_CENTER) == 0x00;
    g_ui_state.button_right      = (v & BUTTON_RIGHT) == 0x00;
    g_ui_state.button_up         = (v & BUTTON_UP) == 0x00;
}

ui_state_t* ui_get_state() {
    return &g_ui_state;
}
