#ifndef __CONTROLS_H__
#define __CONTROLS_H__
#include "core.h"

struct controls_state_t {
    bool btn_start_stop;    
    bool btn_left;
    bool btn_down;
    bool btn_center;
    bool btn_right;
    bool btn_up;
};

void controls_init();
void controls_process();
controls_state_t* controls_get_state();

#endif // __CONTROLS_H__
