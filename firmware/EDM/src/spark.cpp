#include "spark.h"

#define MOSFET_GATE_CTRL_PIN        (GPIO_PIN_1)
#define MOSFET_GATE_CTRL_PORT       (GPIOA)
#define MOSFET_GATE_GND_PIN         (GPIO_PIN_0)
#define MOSFET_GATE_GND_PORT        (GPIOA)

static int32_t g_spark_t1_us = 3;
static int32_t g_spark_t0_us = 300;
static int32_t g_spark_freq  = 0;
static bool g_is_enabled     = false;
static TIM_HandleTypeDef g_spark_htim = {0};

void spark_pwm_init() {
    // MOFSET gnd: PA0 GND
    GPIO_InitTypeDef gnd_gpio = {0};
    gnd_gpio.Pin   = MOSFET_GATE_GND_PIN;
    gnd_gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gnd_gpio.Pull  = GPIO_NOPULL;
    gnd_gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(MOSFET_GATE_GND_PORT, &gnd_gpio);
    HAL_GPIO_WritePin(MOSFET_GATE_GND_PORT, MOSFET_GATE_GND_PIN, GPIO_PIN_RESET);

    // MOFSET ctrl: PA1 PWM
    GPIO_InitTypeDef ctrl_gpio = {0};
    ctrl_gpio.Pin       = MOSFET_GATE_CTRL_PIN;
    ctrl_gpio.Mode      = GPIO_MODE_AF_PP;
    ctrl_gpio.Pull      = GPIO_NOPULL;
    ctrl_gpio.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    ctrl_gpio.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(MOSFET_GATE_CTRL_PORT, &ctrl_gpio);

    // Setup TIM1: ABP1 clock source (42 MHz)
    g_spark_htim.Instance               = TIM2;
    g_spark_htim.Init.Prescaler         = 83; // Prescaler = 2 * 42 - 1 = 83: 1 tick = 1 us
    g_spark_htim.Init.CounterMode       = TIM_COUNTERMODE_UP;
    g_spark_htim.Init.Period            = g_spark_t1_us + g_spark_t0_us;
    g_spark_htim.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    g_spark_htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE; // To avoid glitch for period update
    HAL_TIM_PWM_Init(&g_spark_htim);

    // Setup PWM channel
    TIM_OC_InitTypeDef pwm = {0};
    pwm.OCMode     = TIM_OCMODE_PWM1; // HIGH while counter < CCR
    pwm.Pulse      = 0;               // CCR
    pwm.OCPolarity = TIM_OCPOLARITY_HIGH;
    pwm.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&g_spark_htim, &pwm, TIM_CHANNEL_2);

    HAL_TIM_PWM_Start(&g_spark_htim, TIM_CHANNEL_2);
}

void spark_pwm_start() {
    if (g_is_enabled) {
        return;
    }

    __HAL_TIM_SET_AUTORELOAD(&g_spark_htim, g_spark_t1_us + g_spark_t0_us);
    __HAL_TIM_SET_COMPARE(&g_spark_htim, TIM_CHANNEL_1, g_spark_t1_us);
    g_is_enabled = true;
}

void spark_pwm_stop() {
    if (!g_is_enabled) {
        return;
    }

    __HAL_TIM_SET_AUTORELOAD(&g_spark_htim, g_spark_t1_us + g_spark_t0_us);
    __HAL_TIM_SET_COMPARE(&g_spark_htim, TIM_CHANNEL_1, 0);
    g_is_enabled = false;
}

void spark_pwm_update() {
    g_spark_freq = 1000000 / (g_spark_t1_us + g_spark_t0_us);
    __HAL_TIM_SET_AUTORELOAD(&g_spark_htim, g_spark_t1_us + g_spark_t0_us);
    __HAL_TIM_SET_COMPARE(&g_spark_htim, TIM_CHANNEL_1, g_spark_t1_us);
}

void spark_pwm_process() {
    // //
    // // Short circuit control
    // static uint32_t s_no_spark_counter = 0;
    // if (g_spark_time_us < 5) {
    //     ++s_no_spark_counter;
    //     if (s_no_spark_counter > 100) {
    //         digitalWrite(X_DIR, LOW);
    //     }
    // } else {
    //     s_no_spark_counter = 0;
    //     digitalWrite(X_DIR, HIGH);
    // }
}



void spark_set_t1_us(int32_t t) { 
    if (t < 1) {
        t = 1;
    }
    g_spark_t1_us = t;
}

void spark_set_t0_us(int32_t t) {
    if (t < 100) {
        t = 100;
    }
    g_spark_t0_us = t;
}

int32_t spark_get_t1_us() { return g_spark_t1_us; }
int32_t spark_get_t0_us() { return g_spark_t0_us; }
int32_t spark_get_freq()  { return g_spark_freq;  }

bool spark_is_enabled()   { return g_is_enabled;  }