#include "core.h"
#include "controls.h"
#include "MCP23017.h"

#define BUTTON_START_STOP       (0)
#define BUTTON_LEFT             (1)
#define BUTTON_DOWN             (2)
#define BUTTON_UP               (3)
#define BUTTON_RIGHT            (4)
#define BUTTON_CENTER           (5)

struct button_t {
    GPIO_TypeDef* gpio_port;
    uint16_t      gpio_pin;
    bool          state;
    
    // For debounce
    uint32_t      timer;
    bool          last_state;
};

static button_t g_buttons[] = {
    { .gpio_port = GPIOA, .gpio_pin = GPIO_PIN_11, .state = true, .timer = 0, .last_state = false }, // BUTTON_START_STOP
    { .gpio_port = GPIOB, .gpio_pin = GPIO_PIN_3,  .state = true, .timer = 0, .last_state = false }, // BUTTON_LEFT
    { .gpio_port = GPIOA, .gpio_pin = GPIO_PIN_12, .state = true, .timer = 0, .last_state = false }, // BUTTON_DOWN
    { .gpio_port = GPIOB, .gpio_pin = GPIO_PIN_5,  .state = true, .timer = 0, .last_state = false }, // BUTTON_UP
    { .gpio_port = GPIOB, .gpio_pin = GPIO_PIN_4,  .state = true, .timer = 0, .last_state = false }, // BUTTON_RIGHT
    { .gpio_port = GPIOA, .gpio_pin = GPIO_PIN_15, .state = true, .timer = 0, .last_state = false }, // BUTTON_CENTER
}; 
static controls_state_t g_state;

static void update_button_state(button_t* btn);



void controls_init() {
    for (auto& b : g_buttons) {
        GPIO_InitTypeDef gpio = {0};
        gpio.Pin   = b.gpio_pin;
        gpio.Mode  = GPIO_MODE_INPUT;
        gpio.Pull  = GPIO_PULLUP;
        gpio.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(b.gpio_port, &gpio);        
    }
}

void controls_process() {
    for (auto& b : g_buttons) {
        update_button_state(&b);
    }
    g_state.btn_start_stop = !g_buttons[BUTTON_START_STOP].state;
    g_state.btn_left       = !g_buttons[BUTTON_LEFT].state;
    g_state.btn_down       = !g_buttons[BUTTON_DOWN].state;
    g_state.btn_center     = !g_buttons[BUTTON_CENTER].state;
    g_state.btn_right      = !g_buttons[BUTTON_RIGHT].state;
    g_state.btn_up         = !g_buttons[BUTTON_UP].state;
}

controls_state_t* controls_get_state() {
    return &g_state;
}



static void update_button_state(button_t* btn) {
    // Read GPIO state
    uint8_t gpio_state = HAL_GPIO_ReadPin(btn->gpio_port, btn->gpio_pin);

    // State changed?
    if (gpio_state != btn->last_state) {
        btn->timer = HAL_GetTick();
        btn->last_state = gpio_state;
    }

    // Stable state?
    if (HAL_GetTick() - btn->timer > 50) {
        btn->state = gpio_state;
    }
}
