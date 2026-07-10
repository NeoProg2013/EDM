#include "tension.h"
#include "HX711.h"

#define HEAD_FEEDER_EN_PIN          (GPIO_PIN_11)
#define HEAD_FEEDER_EN_PORT         (GPIOD)
#define HEAD_FEEDER_STEP_PIN        (GPIO_PIN_12)
#define HEAD_FEEDER_STEP_PORT       (GPIOD)

#define HEAD_BRAKE_EN_PIN           (GPIO_PIN_8)
#define HEAD_BRAKE_EN_PORT          (GPIOA)
#define HEAD_BRAKE_STEP_PIN         (GPIO_PIN_9)
#define HEAD_BRAKE_STEP_PORT        (GPIOC)

#define HEAD_HX711_VCC_PIN          (GPIO_PIN_11)
#define HEAD_HX711_VCC_PORT         (GPIOB)
#define HEAD_HX711_GND_PIN          (GPIO_PIN_12)
#define HEAD_HX711_GND_PORT         (GPIOB)
#define HEAD_HX711_DOUT_PIN         (GPIO_PIN_13)
#define HEAD_HX711_DOUT_PORT        (GPIOB)
#define HEAD_HX711_SCK_PIN          (GPIO_PIN_14)
#define HEAD_HX711_SCK_PORT         (GPIOB)

#define TENSION_SCALE               (1700) // 1700 bins = 1g


static HX711 g_tension_sensor;

static TIM_HandleTypeDef g_head_feeder_htim = {0};
static TIM_HandleTypeDef g_head_brake_htim  = {0};

static bool    g_is_enabled       = false;
static int32_t g_feeder_period_us = 1000000UL / 100;
static int32_t g_brake_period_us  = 1000000UL / 75;
static int32_t g_tension_bins     = 0;

static void update_head_speed();


void tension_init() {
    //
    // FEEDER

    // Setup head feeder: PD11 EN
    GPIO_InitTypeDef feeder_en_gpio = {0};
    feeder_en_gpio.Pin       = HEAD_FEEDER_EN_PIN;
    feeder_en_gpio.Mode      = GPIO_MODE_OUTPUT_PP;
    feeder_en_gpio.Pull      = GPIO_NOPULL;
    feeder_en_gpio.Speed     = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(HEAD_FEEDER_EN_PORT, &feeder_en_gpio);
    HAL_GPIO_WritePin(HEAD_FEEDER_EN_PORT, HEAD_FEEDER_EN_PIN, GPIO_PIN_SET);

    // Setup head feeder: PD12 STEP PWM
    GPIO_InitTypeDef feeder_step_gpio = {0};
    feeder_step_gpio.Pin       = HEAD_FEEDER_STEP_PIN;
    feeder_step_gpio.Mode      = GPIO_MODE_AF_PP;
    feeder_step_gpio.Pull      = GPIO_NOPULL;
    feeder_step_gpio.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    feeder_step_gpio.Alternate = GPIO_AF2_TIM4;
    HAL_GPIO_Init(HEAD_FEEDER_STEP_PORT, &feeder_step_gpio);

    // Setup TIM4: ABP1 clock source (42 MHz)
    g_head_feeder_htim.Instance               = TIM4;
    g_head_feeder_htim.Init.Prescaler         = 83; // Prescaler = 2 * 42 - 1 = 83: one tick = 1 us
    g_head_feeder_htim.Init.CounterMode       = TIM_COUNTERMODE_UP;
    g_head_feeder_htim.Init.Period            = g_feeder_period_us;
    g_head_feeder_htim.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    g_head_feeder_htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE; // To avoid glitch for period update
    HAL_TIM_PWM_Init(&g_head_feeder_htim);

    // Setup PWM channel: duty 50%
    TIM_OC_InitTypeDef feeder_pwm = {0};
    feeder_pwm.OCMode     = TIM_OCMODE_PWM1; // HIGH while counter < CCR
    feeder_pwm.Pulse      = 0; // CCR
    feeder_pwm.OCPolarity = TIM_OCPOLARITY_HIGH;
    feeder_pwm.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&g_head_feeder_htim, &feeder_pwm, TIM_CHANNEL_1);

    //
    // BRAKE

    // Setup head brake: PA8 EN
    GPIO_InitTypeDef brake_en_gpio = {0};
    brake_en_gpio.Pin       = HEAD_BRAKE_EN_PIN;
    brake_en_gpio.Mode      = GPIO_MODE_OUTPUT_PP;
    brake_en_gpio.Pull      = GPIO_NOPULL;
    brake_en_gpio.Speed     = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(HEAD_BRAKE_EN_PORT, &brake_en_gpio);
    HAL_GPIO_WritePin(HEAD_BRAKE_EN_PORT, HEAD_BRAKE_EN_PIN, GPIO_PIN_SET);

    // Setup head brake: PC9 STEP PWM
    GPIO_InitTypeDef brake_step_gpio = {0};
    brake_step_gpio.Pin       = HEAD_BRAKE_STEP_PIN;
    brake_step_gpio.Mode      = GPIO_MODE_AF_PP;
    brake_step_gpio.Pull      = GPIO_NOPULL;
    brake_step_gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    brake_step_gpio.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(HEAD_BRAKE_STEP_PORT, &brake_step_gpio);

    // Setup TIM3: ABP1 clock source (42 MHz)
    g_head_brake_htim.Instance               = TIM3;
    g_head_brake_htim.Init.Prescaler         = 83; // Prescaler = 2 * 42 - 1 = 83: one tick = 1 us
    g_head_brake_htim.Init.CounterMode       = TIM_COUNTERMODE_UP;
    g_head_brake_htim.Init.Period            = g_brake_period_us;
    g_head_brake_htim.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    g_head_brake_htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE; // To avoid glitch for period update
    HAL_TIM_PWM_Init(&g_head_brake_htim);

    // Setup PWM channel: duty 50%
    TIM_OC_InitTypeDef brake_pwm = {0};
    brake_pwm.OCMode     = TIM_OCMODE_PWM1; // HIGH while counter < CCR
    brake_pwm.Pulse      = 0;  // CCR
    brake_pwm.OCPolarity = TIM_OCPOLARITY_HIGH;
    brake_pwm.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&g_head_brake_htim, &brake_pwm, TIM_CHANNEL_4);

    //
    // Setup head tension sensor

    // VCC: PB11
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = HEAD_HX711_VCC_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(HEAD_HX711_VCC_PORT, &gpio);
    HAL_GPIO_WritePin(HEAD_HX711_VCC_PORT, HEAD_HX711_VCC_PIN, GPIO_PIN_SET);

    // GND: PB12
    gpio.Pin   = HEAD_HX711_GND_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(HEAD_HX711_GND_PORT, &gpio);
    HAL_GPIO_WritePin(HEAD_HX711_GND_PORT, HEAD_HX711_GND_PIN, GPIO_PIN_RESET);

    // DOUT: PB13
    gpio.Pin   = HEAD_HX711_DOUT_PIN;
    gpio.Mode  = GPIO_MODE_INPUT;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(HEAD_HX711_DOUT_PORT, &gpio);

    // SCK: PB14
    gpio.Pin   = HEAD_HX711_SCK_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(HEAD_HX711_SCK_PORT, &gpio);
    HAL_GPIO_WritePin(HEAD_HX711_SCK_PORT, HEAD_HX711_SCK_PIN, GPIO_PIN_RESET);

    g_tension_sensor.begin(PB13, PB14);
    g_tension_sensor.set_scale(1);
    g_tension_sensor.set_offset(175000);
    // 500g = 850 000
}

void tension_start() {
    if (g_is_enabled) {
        return;
    }

    update_head_speed();

    HAL_GPIO_WritePin(HEAD_FEEDER_EN_PORT, HEAD_FEEDER_EN_PIN, GPIO_PIN_RESET); // Enable feeder driver
    HAL_TIM_PWM_Start(&g_head_feeder_htim, TIM_CHANNEL_1);

    HAL_GPIO_WritePin(HEAD_BRAKE_EN_PORT, HEAD_BRAKE_EN_PIN, GPIO_PIN_RESET); // Enable brake driver
    HAL_TIM_PWM_Start(&g_head_brake_htim, TIM_CHANNEL_4);

    g_is_enabled = true;
}

void tension_stop() {
    if (!g_is_enabled) {
        return;
    }

    HAL_GPIO_WritePin(HEAD_FEEDER_EN_PORT, HEAD_FEEDER_EN_PIN, GPIO_PIN_SET); // Disable feeder driver
    HAL_TIM_PWM_Stop(&g_head_feeder_htim, TIM_CHANNEL_1);

    HAL_GPIO_WritePin(HEAD_BRAKE_EN_PORT, HEAD_BRAKE_EN_PIN, GPIO_PIN_SET); // Disable brake driver
    HAL_TIM_PWM_Stop(&g_head_brake_htim, TIM_CHANNEL_4);

    g_is_enabled = false;
}

void tension_process() {
    static int32_t s_tension_acc = 0;
    static int32_t s_tension_acc_n = 0;

    if (g_tension_sensor.is_ready()) {
        s_tension_acc += g_tension_sensor.read();
        s_tension_acc_n++;
    }

    if (s_tension_acc_n >= 10) {
        if (g_is_enabled) {
            int32_t d = -1001000 - (s_tension_acc / s_tension_acc_n);
            if (d > 100) {
                g_brake_period_us = constrain(g_brake_period_us - 10, 2000, 14000);
            } else if (d < -100) {
                g_brake_period_us = constrain(g_brake_period_us + 10, 2000, 14000);
            }
            update_head_speed();
        }
        
        g_tension_bins = s_tension_acc / s_tension_acc_n;

        s_tension_acc = 0;
        s_tension_acc_n = 0;
    }
}



int32_t tension_get_feeder_period_us() { return g_feeder_period_us; }
int32_t tension_get_brake_period_us()  { return g_brake_period_us;  }
int32_t tension_get_tension_bins()     { return g_tension_bins; }
int32_t tension_get_tension_g()        { return abs(g_tension_bins) / TENSION_SCALE; }




static void update_head_speed() {
    if (g_feeder_period_us < 2) g_feeder_period_us = 2;
    if (g_brake_period_us < 2)  g_brake_period_us  = 2;

    // Update feeder freq
    __HAL_TIM_SET_AUTORELOAD(&g_head_feeder_htim, g_feeder_period_us);
    __HAL_TIM_SET_COMPARE(&g_head_feeder_htim, TIM_CHANNEL_1, g_feeder_period_us / 2);

    // Update brake freq
    __HAL_TIM_SET_AUTORELOAD(&g_head_brake_htim, g_brake_period_us);
    __HAL_TIM_SET_COMPARE(&g_head_brake_htim, TIM_CHANNEL_4, g_brake_period_us / 2);
}
