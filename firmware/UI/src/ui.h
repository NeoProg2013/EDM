#ifndef __UI_H__
#define __UI_H__
#include "core.h"

// UI pages
enum ui_page_t {
    PAGE_EDM_STATUS,
    PAGE_EDM_MOVEMENT,
    PAGE_EDM_PARAMETERS,
    PAGE_MENU,
};

struct ui_state_t {
    ui_page_t page;
    bool button_start_stop;    
    bool button_left;
    bool button_down;
    bool button_center;
    bool button_right;
    bool button_up;
};

void ui_init();
void ui_process();
ui_state_t* ui_get_state();

#endif // __UI_H__
