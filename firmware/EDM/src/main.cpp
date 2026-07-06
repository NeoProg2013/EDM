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
    spark_set_t1_us(spark_get_t1_us() + digitalRead(KBRD_T1_INC_BUTTON) == LOW);
    spark_set_t1_us(spark_get_t1_us() - digitalRead(KBRD_T1_DEC_BUTTON) == LOW);
    spark_set_t0_us(spark_get_t0_us() + digitalRead(KBRD_T0_INC_BUTTON) == LOW);
    spark_set_t0_us(spark_get_t0_us() - digitalRead(KBRD_T0_DEC_BUTTON) == LOW);

    static bool s_last_start_stop_button_state = HIGH;
    bool v = digitalRead(KBRD_START_STOP_BUTTON);
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

    // //
    // // Setup X axis
    // PWMC_ConfigureChannel(PWM, X_PWM_CH, PWM_CMR_CPRE_CLKA, 0, 0);
    // PWMC_SetPeriod(PWM, X_PWM_CH, 10000);
    // PWMC_SetDutyCycle(PWM, X_PWM_CH, 10000 / 2);

    // pinMode(X_EN, OUTPUT);
    // digitalWrite(X_EN, HIGH);
    // pinMode(X_STEP, OUTPUT);
    // digitalWrite(X_STEP, LOW);
    // pinMode(X_DIR, OUTPUT);
    // digitalWrite(X_DIR, LOW);

    // // Feedback
    // pinMode(SPARK_SHORT_CIRCUIT, INPUT_PULLUP);
    // attachInterrupt(SPARK_SHORT_CIRCUIT, spark_feedback_irq, CHANGE);
}

// void PWM_Handler(void) {
//     uint32_t status = PWM->PWM_ISR1; 
//     if (status & (1 << MOSFET_GATE_CTRL_PWM_CH)) {
//         ++spark_period_counter;
//     }
// }

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
    static uint32_t s_no_spark_counter = 0;
    if (spark_is_enabled() && g_spark_time_us < 5) {
        ++s_no_spark_counter;
        if (s_no_spark_counter > 100) {
            digitalWrite(X_DIR, LOW);
        }
    } else {
        s_no_spark_counter = 0;
        digitalWrite(X_DIR, HIGH);
    }
    
    // 
    // Tension control
    tension_process();

    //
    // Shutdown
    static bool is_periph_enabled = true;
    if (!g_is_enabled) {
        if (is_periph_enabled) {
            tension_start();
            spark_pwm_start();
            is_periph_enabled = true;
        }
    } else {
        if (!is_periph_enabled) {
            tension_stop();
            spark_pwm_stop();
            is_periph_enabled = false;
        }
    }
}