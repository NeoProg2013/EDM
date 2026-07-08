#include <Arduino.h>
#include <SPI.h>
#include "tension.h"
#include "spark.h"
#include "display.h"



#define KBRD_T1_INC_BUTTON          (PE2)
#define KBRD_T1_DEC_BUTTON          (PE3)
#define KBRD_T0_INC_BUTTON          (PE4)
#define KBRD_T0_DEC_BUTTON          (PE5)
#define KBRD_START_STOP_BUTTON      (PE6)





#define SPARK_SHORT_CIRCUIT       (14)
// #define SPART_SHORT_CIRCUIT       (ADC_CHANNEL_6) // A1

#define X_EN                      (29)
#define X_DIR                     (27)
#define X_STEP                    (9)
#define X_PWM_CH                  (4)


// #define DEBUG_PIN_1                 (PD10)
// #define DEBUG_PIN_2                 (PD9)
// #define DEBUG_PIN_3                 (PD8)
// #define DEBUG_PIN_4                 (PD15)







uint32_t g_spark_time_us = 0;
bool g_is_short_circuit = false;

bool g_is_enabled = false;


void keyboard_process() {
    spark_set_t1_us(spark_get_t1_us() + (int)(digitalRead(KBRD_T1_INC_BUTTON) == LOW));
    spark_set_t1_us(spark_get_t1_us() - (int)(digitalRead(KBRD_T1_DEC_BUTTON) == LOW));
    spark_set_t0_us(spark_get_t0_us() + (int)(digitalRead(KBRD_T0_INC_BUTTON) == LOW));
    spark_set_t0_us(spark_get_t0_us() - (int)(digitalRead(KBRD_T0_DEC_BUTTON) == LOW));

    static int s_last_start_stop_button_state = HIGH;
    int v = digitalRead(KBRD_START_STOP_BUTTON);
    if (v == LOW && s_last_start_stop_button_state == HIGH) {
        g_is_enabled = !g_is_enabled;
        g_is_short_circuit = false;
    }
    s_last_start_stop_button_state = v;
}




// void spark_feedback_irq(void) {
//     bool state = (g_APinDescription[SPARK_SHORT_CIRCUIT].pPort->PIO_PDSR & g_APinDescription[SPARK_SHORT_CIRCUIT].ulPin);

//     static uint32_t s_start = 0;
//     if (state == HIGH) {
//         g_spark_time_us = micros() - s_start;
//     } else {
//         s_start = micros();
//     }
// }

static TIM_HandleTypeDef g_x_htim = {0};

#define X_STEP_PIN        (GPIO_PIN_8)
#define X_STEP_PORT       (GPIOA)
#define X_DIR_PIN         (GPIO_PIN_9)
#define X_DIR_PORT        (GPIOA)
#define X_EN_PIN          (GPIO_PIN_10)
#define X_EN_PORT         (GPIOA)

void setup() {
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    
    // Keyboard
    pinMode(KBRD_T1_INC_BUTTON, INPUT_PULLUP); // T1 button +
    pinMode(KBRD_T1_DEC_BUTTON, INPUT_PULLUP); // T1 button -
    pinMode(KBRD_T0_INC_BUTTON, INPUT_PULLUP); // T0 button +
    pinMode(KBRD_T0_DEC_BUTTON, INPUT_PULLUP); // T0 button -
    pinMode(KBRD_START_STOP_BUTTON, INPUT_PULLUP); // Start / stop button
    
    // Debug
    // pinMode(DEBUG_PIN_1, OUTPUT);
    // pinMode(DEBUG_PIN_2, OUTPUT);
    // pinMode(DEBUG_PIN_3, OUTPUT);
    // pinMode(DEBUG_PIN_4, OUTPUT);
    
    
    display_init();
    tension_init();
    spark_pwm_init();

    //
    // Setup X axis

    // STEP: PA8 PWM
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
    g_x_htim.Init.Period            = 9999;
    g_x_htim.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    g_x_htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE; // To avoid glitch for period update
    HAL_TIM_PWM_Init(&g_x_htim);

    // Setup PWM channel (50%)
    TIM_OC_InitTypeDef x_pwm = {0};
    x_pwm.OCMode = TIM_OCMODE_PWM1; // HIGH while counter < CCR
    x_pwm.Pulse  = 5000;            // CCR
    HAL_TIM_PWM_ConfigChannel(&g_x_htim, &x_pwm, TIM_CHANNEL_1);

    __HAL_TIM_MOE_ENABLE(&g_x_htim);

    // EN: PA10
    GPIO_InitTypeDef x_en_gpio = {0};
    x_en_gpio.Pin   = X_EN_PIN;
    x_en_gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    x_en_gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(X_EN_PORT, &x_en_gpio);
    HAL_GPIO_WritePin(X_EN_PORT, X_EN_PIN, GPIO_PIN_SET);

    // DIR: PA9
    GPIO_InitTypeDef x_dir_gpio = {0};
    x_dir_gpio.Pin   = X_DIR_PIN;
    x_dir_gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    x_dir_gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(X_DIR_PORT, &x_dir_gpio);
    HAL_GPIO_WritePin(X_DIR_PORT, X_DIR_PIN, GPIO_PIN_SET);


    // // Feedback
    // pinMode(SPARK_SHORT_CIRCUIT, INPUT_PULLUP);
    // attachInterrupt(SPARK_SHORT_CIRCUIT, spark_feedback_irq, CHANGE);
}

void start_axis_x() {
    HAL_TIM_PWM_Start(&g_x_htim, TIM_CHANNEL_1);
    HAL_GPIO_WritePin(X_EN_PORT, X_EN_PIN, GPIO_PIN_RESET);
}

void stop_axis_x() {
    HAL_TIM_PWM_Stop(&g_x_htim, TIM_CHANNEL_1);
    HAL_GPIO_WritePin(X_EN_PORT, X_EN_PIN, GPIO_PIN_SET);
}

// void PWM_Handler(void) {
//     uint32_t status = PWM->PWM_ISR1; 
//     if (status & (1 << MOSFET_GATE_CTRL_PWM_CH)) {
//         ++spark_period_counter;
//     }
// }

void HardFault_Handler(void) {
    while (true);
}

void loop() {
    //
    // Keyboard & TFT
    static uint32_t s_last_update_params_time_ms = 0;
    if (millis() - s_last_update_params_time_ms > 50) {
        keyboard_process();
        spark_pwm_update();
        display_update();
        s_last_update_params_time_ms = millis();
    }

    //
    // Short circuit control
    // static uint32_t s_no_spark_counter = 0;
    // if (spark_is_enabled() && g_spark_time_us < 5) {
    //     ++s_no_spark_counter;
    //     if (s_no_spark_counter > 100) {
    //         digitalWrite(X_DIR, LOW);
    //     }
    // } else {
    //     s_no_spark_counter = 0;
    //     digitalWrite(X_DIR, HIGH);
    // }
    
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
            start_axis_x();
            is_periph_enabled = true;
        }
    } else {
        if (is_periph_enabled) {
            tension_stop();
            spark_pwm_stop();
            stop_axis_x();
            is_periph_enabled = false;
        }
    }
}