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

#define DEBOUNCE_THRESHOLD  (20)



static ui_state_t g_ui_state;

static int16_t g_cnt_start_stop = 0;
static int16_t g_cnt_left       = 0;
static int16_t g_cnt_down       = 0;
static int16_t g_cnt_center     = 0;
static int16_t g_cnt_right      = 0;
static int16_t g_cnt_up         = 0;

static void debounce_button(bool is_pressed, int16_t& counter, bool& state) {
    if (is_pressed) {
        if (counter < DEBOUNCE_THRESHOLD) {
            counter++;
            if (counter >= DEBOUNCE_THRESHOLD) {
                state = true;
                counter = DEBOUNCE_THRESHOLD;
            }
        }
    } else {
        if (counter >= 0) {
            counter--;
            if (counter <= 0) {
                state = false;
                counter = 0;
            }
        }
    }
}



void ui_init() {
    mcp23017_init();
}

void ui_process() {
    uint8_t v = mcp23017_read_porta();
    debounce_button((v & BUTTON_START_STOP) == 0x00, g_cnt_start_stop, g_ui_state.button_start_stop);
    debounce_button((v & BUTTON_LEFT)       == 0x00, g_cnt_left,       g_ui_state.button_left);
    debounce_button((v & BUTTON_DOWN)       == 0x00, g_cnt_down,       g_ui_state.button_down);
    debounce_button((v & BUTTON_CENTER)     == 0x00, g_cnt_center,     g_ui_state.button_center);
    debounce_button((v & BUTTON_RIGHT)      == 0x00, g_cnt_right,      g_ui_state.button_right);
    debounce_button((v & BUTTON_UP)         == 0x00, g_cnt_up,         g_ui_state.button_up);
}

ui_state_t* ui_get_state() {
    return &g_ui_state;
}
