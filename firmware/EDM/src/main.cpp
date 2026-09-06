#include "core.h"
#include "tension.h"
#include "spark.h"
#include "telemetry.h"

// #define KBRD_T1_INC_BUTTON          (PE2)
// #define KBRD_T1_DEC_BUTTON          (PE3)
// #define KBRD_T0_INC_BUTTON          (PE4)
// #define KBRD_T0_DEC_BUTTON          (PE5)
// #define KBRD_START_STOP_BUTTON      (PE6)


// #define DEBUG_PIN_1                 (PD10)
// #define DEBUG_PIN_2                 (PD9)
// #define DEBUG_PIN_3                 (PD8)
// #define DEBUG_PIN_4                 (PD15)


uint16_t g_axis_x_period_us = 10000;
uint16_t g_arc_counter = 0;

bool g_is_enabled = false;
bool g_is_axis_x_enabled = false;


void keyboard_process() {
    // g_axis_x_period_us += 10 * (int)(digitalRead(KBRD_T1_INC_BUTTON) == LOW);
    // g_axis_x_period_us -= 10 * (int)(digitalRead(KBRD_T1_DEC_BUTTON) == LOW);


    // static int s_last_start_stop_button_state = HIGH;
    // int v = digitalRead(KBRD_START_STOP_BUTTON);
    // if (v == LOW && s_last_start_stop_button_state == HIGH) {
    //     g_is_enabled = !g_is_enabled;
    // }
    // s_last_start_stop_button_state = v;
}


TIM_HandleTypeDef g_x_htim = {0};
TIM_HandleTypeDef g_htim8 = {0};


#define X_EN_PIN                (GPIO_PIN_10)
#define X_EN_PORT               (GPIOA)
#define X_STEP_PIN              (GPIO_PIN_11)
#define X_STEP_PORT             (GPIOA)
#define X_DIR_PIN               (GPIO_PIN_12)
#define X_DIR_PORT              (GPIOA)


#define FEEDBACK_PIN            (GPIO_PIN_6)
#define FEEDBACK_PORT           (GPIOC)

// Инициализация аппаратного измерения сигнала обратной связи на PC6 через TIM8.
// Таймер включается в slave reset mode по входу TI1FP1. В этой схеме активный trigger сбрасывает CNT в 0
//
// Каналы настраиваются в PWM input capture конфигурацию:
//    - CH1: захват по FALLING, reset CNT
//    - CH2: захват по RISING
//
// В результате при чтении регистров захвата в основном цикле получается:
//    - TIM_CHANNEL_1 -> длительность высокого уровня сигнала (мкс)
//    - TIM_CHANNEL_2 -> полный период сигнала между соседними фронтами (мкс)
//
void init_feedback() {
    GPIO_InitTypeDef x_step_gpio = {0};
    x_step_gpio.Pin       = FEEDBACK_PIN;
    x_step_gpio.Mode      = GPIO_MODE_AF_PP;
    x_step_gpio.Pull      = GPIO_NOPULL;
    x_step_gpio.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    x_step_gpio.Alternate = GPIO_AF3_TIM8;
    HAL_GPIO_Init(FEEDBACK_PORT, &x_step_gpio);

    // Setup TIM8
    g_htim8.Instance               = TIM8;
    g_htim8.Init.Prescaler         = 167; // Prescaler = 168 - 1 = 83: 1 tick = 1 us
    g_htim8.Init.CounterMode       = TIM_COUNTERMODE_UP;
    g_htim8.Init.Period            = 0xFFFF;
    g_htim8.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    g_htim8.Init.RepetitionCounter = 0;
    g_htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_IC_Init(&g_htim8);

    // Setup slave mode
    TIM_SlaveConfigTypeDef slave = {0};
    slave.SlaveMode        = TIM_SLAVEMODE_RESET;         // Reset CNT to 0 by trigger
    slave.InputTrigger     = TIM_TS_TI1FP1;               // Connect trigger to CH1 (TI1)
    slave.TriggerPolarity  = TIM_TRIGGERPOLARITY_FALLING; // Trigger polarity
    slave.TriggerFilter    = 0;
    HAL_TIM_SlaveConfigSynchro(&g_htim8, &slave);

    // Setup input CH2 (FALLING)
    TIM_IC_InitTypeDef in = {0};
    in.ICPolarity  = TIM_ICPOLARITY_FALLING;
    in.ICSelection = TIM_ICSELECTION_DIRECTTI;
    in.ICPrescaler = TIM_ICPSC_DIV1;
    in.ICFilter    = 4; 
    HAL_TIM_IC_ConfigChannel(&g_htim8, &in, TIM_CHANNEL_1);

    // Setup input CH1 (RISING)
    in.ICPolarity  = TIM_ICPOLARITY_RISING;
    in.ICSelection = TIM_ICSELECTION_INDIRECTTI; // Indirect link CH1 to CH2 pin
    HAL_TIM_IC_ConfigChannel(&g_htim8, &in, TIM_CHANNEL_2);

    HAL_TIM_IC_Start(&g_htim8, TIM_CHANNEL_1);
    HAL_TIM_IC_Start(&g_htim8, TIM_CHANNEL_2);
}

void start_axis_x() {
    HAL_TIM_PWM_Start(&g_x_htim, TIM_CHANNEL_4);
    // HAL_GPIO_WritePin(X_EN_PORT, X_EN_PIN, GPIO_PIN_RESET);
    g_is_axis_x_enabled = true;
}

void stop_axis_x() {
    HAL_TIM_PWM_Stop(&g_x_htim, TIM_CHANNEL_4);
    // HAL_GPIO_WritePin(X_EN_PORT, X_EN_PIN, GPIO_PIN_SET);
    g_is_axis_x_enabled = false;
}

void enable_axis_x() {
    HAL_TIM_PWM_Start(&g_x_htim, TIM_CHANNEL_4);
    HAL_GPIO_WritePin(X_EN_PORT, X_EN_PIN, GPIO_PIN_RESET);
}

void disable_axis_x() {
    HAL_TIM_PWM_Stop(&g_x_htim, TIM_CHANNEL_4);
    HAL_GPIO_WritePin(X_EN_PORT, X_EN_PIN, GPIO_PIN_SET);
    g_is_axis_x_enabled = false;
}

void update_axis_x_speed() {
    if (g_axis_x_period_us < 1000)  g_axis_x_period_us = 1000;
    if (g_axis_x_period_us > 30000) g_axis_x_period_us = 30000;

    // Update X axis freq
    __HAL_TIM_SET_AUTORELOAD(&g_x_htim, g_axis_x_period_us);
    __HAL_TIM_SET_COMPARE(&g_x_htim, TIM_CHANNEL_4, g_axis_x_period_us / 2);
}


static void system_clock_init() {
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    
    // Init HSE & PLL
    RCC_OscInitTypeDef osc = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState       = RCC_HSE_ON;
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = 8;             // 8 MHz -> 1 MHz
    osc.PLL.PLLN = 336;           // 1 MHz -> 336 MHz
    osc.PLL.PLLP = RCC_PLLP_DIV2; // SYSCLK: 336 MHz -> 168 MHz
    osc.PLL.PLLQ = 7;             // USB/SDIO: 336 MHz -> 48 MHz
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        while (1);
    }

    // Setup clock source as PLL
    // For 3V3 on 168 MHz require 5 ticks for flash memory
    RCC_ClkInitTypeDef clk = {0};
    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1; // HCLK = 168 MHz
    clk.APB1CLKDivider = RCC_HCLK_DIV4;   // PCLK1 = 42 MHz
    clk.APB2CLKDivider = RCC_HCLK_DIV2;   // PCLK2 = 84 MHz
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5) != HAL_OK) {
        while (1);
    }

    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();
    __HAL_RCC_TIM8_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
}


int main() {
    HAL_Init();
    system_clock_init();

    //
    // Periph
    telemetry_init();
    tension_init();
    spark_pwm_init();

    //
    // Setup X axis

    // STEP: PA11 PWM
    GPIO_InitTypeDef x_step_gpio = {0};
    x_step_gpio.Pin       = X_STEP_PIN;
    x_step_gpio.Mode      = GPIO_MODE_AF_PP;
    x_step_gpio.Pull      = GPIO_NOPULL;
    x_step_gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    x_step_gpio.Alternate = GPIO_AF1_TIM1;
    HAL_GPIO_Init(X_STEP_PORT, &x_step_gpio);

    // Setup TIM1: ABP2 clock source (168 MHz)
    g_x_htim.Instance               = TIM1;
    g_x_htim.Init.Prescaler         = 167; // Prescaler = 168 - 1 = 83: 1 tick = 1 us
    g_x_htim.Init.CounterMode       = TIM_COUNTERMODE_UP;
    g_x_htim.Init.Period            = g_axis_x_period_us;
    g_x_htim.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    g_x_htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE; // To avoid glitch for period update
    HAL_TIM_PWM_Init(&g_x_htim);

    // Setup PWM channel (50%)
    TIM_OC_InitTypeDef x_pwm = {0};
    x_pwm.OCMode = TIM_OCMODE_PWM1; // HIGH while counter < CCR
    x_pwm.Pulse  = g_axis_x_period_us / 2; // CCR
    HAL_TIM_PWM_ConfigChannel(&g_x_htim, &x_pwm, TIM_CHANNEL_4);

    __HAL_TIM_MOE_ENABLE(&g_x_htim);

    // EN: PA10
    GPIO_InitTypeDef x_en_gpio = {0};
    x_en_gpio.Pin   = X_EN_PIN;
    x_en_gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    x_en_gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(X_EN_PORT, &x_en_gpio);
    HAL_GPIO_WritePin(X_EN_PORT, X_EN_PIN, GPIO_PIN_SET);

    // DIR: PA12
    GPIO_InitTypeDef x_dir_gpio = {0};
    x_dir_gpio.Pin   = X_DIR_PIN;
    x_dir_gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    x_dir_gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(X_DIR_PORT, &x_dir_gpio);
    HAL_GPIO_WritePin(X_DIR_PORT, X_DIR_PIN, GPIO_PIN_SET);


    init_feedback();

    while (1) {
        //
        // Telemetry
        static uint32_t s_last_update_params_time_ms = 0;
        if (HAL_GetTick() - s_last_update_params_time_ms > 50) {
            s_last_update_params_time_ms = HAL_GetTick();

            tx_msg_t tx_msg {
                .arc_state   = spark_is_enabled(),
                .step_state  = g_is_axis_x_enabled,
                .freq_hz     = spark_get_freq(),
                .arc_counter = g_arc_counter,
                .tension_g   = tension_get_tension_g(),
                .feeder_us   = tension_get_feeder_period_us(),
                .brake_us    = tension_get_brake_period_us(),
                .t1          = spark_get_t1_us(),
                .t0          = spark_get_t0_us(),
            };
            telemetry_tx(&tx_msg);

            keyboard_process();
        }

        //
        // Short circuit control
        static uint32_t s_arc_last_time_ms = 0;
        if (spark_is_enabled()) {
            uint32_t current_cnt = __HAL_TIM_GET_COUNTER(&g_htim8);
            uint32_t low_us = HAL_TIM_ReadCapturedValue(&g_htim8, TIM_CHANNEL_2);

            // Алгоритм работы:
            // - При коротком замыкании через проволоку длительность импульса составляет 3us
            // - Условие "current_cnt > 10000" защита от жесткого КЗ, но такого быть не должно,
            //   т.к. проволока имеет сопротивление и напряжение не упадет ниже порога срабатывания оптопары
            // - Длительность импульса холостого хода - 3-12 us, видимо это связано с закрытием транзистора оптопары
            // - При обычной работе генератора во время реза, длительность 1-2 us.
            // Мы ждем пока станок полностью прорежет текущий отрезок и только потом делаем следующий шаг
            // В качестве критерия используется отсутствие искры, т.е. длительность HIGH >= 3 более 100мс
            if (current_cnt > 1000 || low_us < 2) { // current_cnt > 1000 us -- no pulse long time
                stop_axis_x();
                s_arc_last_time_ms = HAL_GetTick();
                ++g_arc_counter;
            } else {
                if (HAL_GetTick() - s_arc_last_time_ms > 100) { // 100 ms
                    start_axis_x();
                }
            }
        }
        
        // 
        // Tension control
        tension_process();

        //
        // Shutdown
        static bool is_periph_enabled = false;
        if (g_is_enabled) {
            if (!is_periph_enabled) {
                tension_start();
                spark_pwm_start();
                enable_axis_x();
                is_periph_enabled = true;
            }
        } else {
            if (is_periph_enabled) {
                tension_stop();
                spark_pwm_stop();
                disable_axis_x();
                is_periph_enabled = false;
            }
        }
    }
}
