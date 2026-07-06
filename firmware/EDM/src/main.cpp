#include <Arduino.h>
#include <SPI.h>
#include "TFT_22_ILI9225.h"
#include "HX711.h"

#define TFT_RST                     (PA2)
#define TFT_RS                      (PA3)
#define TFT_CS                      (PA4)
#define TFT_LED                     (PA6)

#define KBRD_T1_INC_BUTTON          (PE2)
#define KBRD_T1_DEC_BUTTON          (PE3)
#define KBRD_T0_INC_BUTTON          (PE4)
#define KBRD_T0_DEC_BUTTON          (PE5)
#define KBRD_START_STOP_BUTTON      (PE6)

#define HEAD_FEEDER_EN_PIN          (GPIO_PIN_11)
#define HEAD_FEEDER_EN_PORT         (GPIOD)
#define HEAD_FEEDER_STEP_PIN        (GPIO_PIN_12)
#define HEAD_FEEDER_STEP_PORT       (GPIOD)

#define HEAD_BRAKE_EN_PIN           (GPIO_PIN_7)
#define HEAD_BRAKE_EN_PORT          (GPIOC)
#define HEAD_BRAKE_STEP_PIN         (GPIO_PIN_8)
#define HEAD_BRAKE_STEP_PORT        (GPIOC)

// #define HEAD_BRAKE_EN             (33)
// #define HEAD_BRAKE_STEP           (7)
// #define HEAD_BRAKE_PWM_CH         (6)

#define HEAD_TENSION_SENSOR_VCC   (35)
#define HEAD_TENSION_SENSOR_GND   (37)
#define HEAD_TENSION_SENSOR_DOUT  (39)
#define HEAD_TENSION_SENSOR_SCK   (41)

#define MOSFET_GATE_CTRL          (6) // PWM timer
#define MOSFET_GATE_CTRL_PWM_CH   (7)
#define MOSFET_GATE_GND           (5)

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




// TFT_22_ILI9225 tft(TFT_RST, TFT_RS, TFT_CS, 0, 255);
TFT_22_ILI9225 tft(TFT_RST, TFT_RS, TFT_CS, TFT_LED, 255);
HX711 head_tension;

int32_t spark_freq    = 0;
int32_t spark_t1_us   = 3;
int32_t spark_t0_us   = 300;
uint32_t g_spark_time_us = 0;
bool spark_generator_state = false;
bool is_short_circuit = false;

int32_t feeder_period_us = 1000000UL / 100;
int32_t brake_period_us = 1000000UL / 70;

#define TENSION_SCALE       (1700) // 1700 bins = 1g
int32_t tension_bins = 0;
int32_t tension_g = 0;


void keyboard_process() {
    // Process buttons
    spark_t1_us += digitalRead(KBRD_T1_INC_BUTTON) == LOW;
    spark_t1_us -= digitalRead(KBRD_T1_DEC_BUTTON) == LOW;
    spark_t0_us += digitalRead(KBRD_T0_INC_BUTTON) == LOW;
    spark_t0_us -= digitalRead(KBRD_T0_DEC_BUTTON) == LOW;

    static bool s_last_start_stop_button_state = HIGH;
    bool v = digitalRead(KBRD_START_STOP_BUTTON);
    if (v == LOW && s_last_start_stop_button_state == HIGH) {
        spark_generator_state = !spark_generator_state;
    }
    s_last_start_stop_button_state = v;

    // Validate values
    if (spark_t1_us < 1)   spark_t1_us = 1;
    if (spark_t0_us < 100) spark_t0_us = 100;

    // Calc spark parameters
    spark_freq = 1000000 / (spark_t1_us + spark_t0_us);
}

void update_display() {
    static bool s_is_init = false;
    static uint32_t s_call_counter = 0;

    ++s_call_counter;
    
    if (!s_is_init) {
        tft.clear();
        tft.fillRectangle(0, 0, 176, 220, COLOR_BLACK);

        // Static text
        tft.setFont(Terminal11x16, true);
        tft.drawText(5, 5, "DISABLED", COLOR_RED);
        tft.drawLine(0, 27, 176, 27, COLOR_GRAY);
        
        tft.setFont(Terminal6x8, true);
        int y = 40;
        tft.drawText(5, y, "    Freq (Hz): ---", COLOR_WHITE); y += 15;
        tft.drawText(5, y, "   Spark time: ---", COLOR_WHITE); y += 15;
        tft.drawText(5, y, "", COLOR_WHITE); y += 15;
        tft.drawText(5, y, "", COLOR_WHITE); y += 15;
        tft.drawText(5, y, "Tension (bin): ---", COLOR_WHITE); y += 15;
        tft.drawText(5, y, "  Tension (g): ---", COLOR_WHITE); y += 15;
        tft.drawText(5, y, "  Feeder (Hz): ---", COLOR_WHITE); y += 15;
        tft.drawText(5, y, "   Brake (Hz): ---", COLOR_WHITE); y += 15;

        // Draw pulse
        int x = 5;
        y = 170;
        int w = 160;
        int h = 40;
        tft.drawLine(x, y, x, y + h, COLOR_WHITE);
        tft.drawLine(x, y, x + w / 2, y, COLOR_WHITE);
        tft.drawLine(x + w / 2, y, x + w / 2, y + h, COLOR_WHITE);
        tft.drawLine(x + w / 2, y + h, x + w, y + h, COLOR_WHITE);
        
        s_is_init = true;
    }

    tft.setFont(Terminal11x16, true);

    // State
    if (s_call_counter == 1) {
        static int32_t s_last_state = -1;
        if (s_last_state != spark_generator_state) {
            tft.fillRectangle(5, 5, 176, 21, COLOR_BLACK);
            if (spark_generator_state) {
                tft.drawText(5, 5, "ENABLED", COLOR_GREEN);
            } else {
                tft.drawText(5, 5, "DISABLED", COLOR_RED);
            }
            s_last_state = spark_generator_state;
        }
        return;
    }

    tft.setFont(Terminal6x8, true);
    int y = 40;

    // Freq
    if (s_call_counter == 2) {
        static int32_t s_last_freq = -1;
        if (s_last_freq != spark_freq) {
            tft.fillRectangle(106, y, 150, y + 8, COLOR_BLACK);
            tft.drawText(106, y, String(spark_freq), COLOR_YELLOW);
            s_last_freq = spark_freq;
        }
        return;
    }
    y += 15;

    // Spark time
    if (s_call_counter == 3) {
        static uint32_t s_last_spark_time_us = -1;
        if (s_last_spark_time_us != g_spark_time_us) {
            tft.fillRectangle(106, y, 150, y + 8, COLOR_BLACK);
            tft.drawText(106, y, String(g_spark_time_us), COLOR_YELLOW);
            s_last_spark_time_us = g_spark_time_us;
        }
        return;
    }
    y += 15;

    // Current
    if (s_call_counter == 4) {
        // static int32_t s_last_current = -1;
        // if (s_last_current != spark_current) {
        //     tft.fillRectangle(106, y, 150, y + 8, COLOR_BLACK);
        //     tft.drawText(106, y, String(spark_current), COLOR_YELLOW);
        //     s_last_current = spark_current;
        // }
        return;
    }
    y += 15;

    // Spacing
    y += 15;

    // Tension (bins)
    if (s_call_counter == 5) {
        static int32_t s_last_tension_bins = -1;
        if (s_last_tension_bins != tension_bins) {
            tft.fillRectangle(106, y, 150, y + 8, COLOR_BLACK);
            tft.drawText(106, y, String(tension_bins), COLOR_YELLOW);
            s_last_tension_bins = tension_bins;
        }
        return;
    }
    y += 15;

    // Tension (g)
    if (s_call_counter == 6) {
        static int32_t s_last_tension_g = -1;
        if (s_last_tension_g != tension_g) {
            tft.fillRectangle(106, y, 150, y + 8, COLOR_BLACK);
            tft.drawText(106, y, String(tension_g), COLOR_YELLOW);
            s_last_tension_g = tension_g;
        }
        return;
    }
    y += 15;

    // Feeder freq
    if (s_call_counter == 7) {
        static int32_t s_last_feeder_period_us = -1;
        if (s_last_feeder_period_us != feeder_period_us) {
            tft.fillRectangle(106, y, 150, y + 8, COLOR_BLACK);
            tft.drawText(106, y, String(1000000 / feeder_period_us), COLOR_YELLOW);
            s_last_feeder_period_us = feeder_period_us;
        }
        return;
    }
    y += 15;

    // Brake freq
    if (s_call_counter == 8) {
        static int32_t s_last_brake_period_us = -1;
        if (s_last_brake_period_us != brake_period_us) {
            tft.fillRectangle(106, y, 150, y + 8, COLOR_BLACK);
            tft.drawText(106, y, String(1000000 / brake_period_us), COLOR_YELLOW);
            s_last_brake_period_us = brake_period_us;
        }
        return;
    }
    y += 15;

    // T1
    if (s_call_counter == 9) {
        static int32_t s_last_t1 = -1;
        if (s_last_t1 != spark_t1_us) {
            tft.fillRectangle(15, 180, 75, 195, COLOR_BLACK);
            tft.drawText(25, 185, String(spark_t1_us) + " us", COLOR_YELLOW);
            s_last_t1 = spark_t1_us;
        }
        return;
    }

    // T0
    if (s_call_counter == 10) {
        static int32_t s_last_t0 = -1;
        if (s_last_t0 != spark_t0_us) {
            tft.fillRectangle(100, 180, 160, 195, COLOR_BLACK);
            tft.drawText(105, 185, String(spark_t0_us) + " us", COLOR_YELLOW);
            s_last_t0 = spark_t0_us;
        }
        return;
    }

    s_call_counter = 0;
}

// void update_mosfet_ctrl_pwm() {
//     static int32_t s_last_t1_us = 0;
//     static int32_t s_last_t0_us = 0;
//     static bool    s_last_state = 0;

//     if (s_last_t1_us != spark_t1_us || s_last_t0_us != spark_t0_us || s_last_state != spark_generator_state) {
//         if (spark_t1_us <= 0 || spark_t0_us <= 0) {
//             return;
//         }
//         s_last_t1_us = spark_t1_us;
//         s_last_t0_us = spark_t0_us;
//         s_last_state = spark_generator_state;

//         Serial.print("update MOSFET CTRL PWM: ");
//         Serial.print(spark_t1_us);
//         Serial.print(" ");
//         Serial.println(spark_t0_us);

//         uint32_t period = spark_t1_us + spark_t0_us;
//         uint32_t duty = spark_t1_us;
//         PWMC_SetPeriod(PWM, MOSFET_GATE_CTRL_PWM_CH, period);
//         PWMC_SetDutyCycle(PWM, MOSFET_GATE_CTRL_PWM_CH, duty);
//     }
// }


// void spark_feedback_irq(void) {
//     bool state = (g_APinDescription[SPARK_SHORT_CIRCUIT].pPort->PIO_PDSR & g_APinDescription[SPARK_SHORT_CIRCUIT].ulPin);

//     static uint32_t s_start = 0;
//     if (state == HIGH) {
//         g_spark_time_us = micros() - s_start;
//     } else {
//         s_start = micros();
//     }
// }

TIM_HandleTypeDef g_head_feeder_htim = {0};
TIM_HandleTypeDef g_head_brake_htim = {0};

void update_feeder_speed(uint32_t new_feeder_period_us, uint32_t new_brake_period_us) {
    // __HAL_TIM_SET_AUTORELOAD(&g_head_htim, new_feeder_period_us);
    // __HAL_TIM_SET_COMPARE(&g_head_htim, TIM_CHANNEL_1, new_feeder_period_us / 2);
    // __HAL_TIM_SET_COMPARE(&g_head_htim, TIM_CHANNEL_2, new_brake_period_us / 2);
}

void setup_head() {
    //
    // FEEDER

    // Setup head feeder: PD11 EN, low state
    GPIO_InitTypeDef feeder_en_gpio = {0};
    feeder_en_gpio.Pin       = HEAD_FEEDER_EN_PIN;
    feeder_en_gpio.Mode      = GPIO_MODE_OUTPUT_PP;
    feeder_en_gpio.Pull      = GPIO_NOPULL;
    feeder_en_gpio.Speed     = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(HEAD_FEEDER_EN_PORT, &feeder_en_gpio);
    HAL_GPIO_WritePin(HEAD_FEEDER_EN_PORT, HEAD_FEEDER_EN_PIN, GPIO_PIN_RESET);

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
    g_head_feeder_htim.Init.Period            = feeder_period_us;
    g_head_feeder_htim.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    g_head_feeder_htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE; // To avoid glitch for period update
    HAL_TIM_PWM_Init(&g_head_feeder_htim);

    // Setup PWM channel: duty 50%
    TIM_OC_InitTypeDef feeder_pwm = {0};
    feeder_pwm.OCMode     = TIM_OCMODE_PWM1;      // HIGH while counter < CCR
    feeder_pwm.Pulse      = feeder_period_us / 2; // CCR
    feeder_pwm.OCPolarity = TIM_OCPOLARITY_HIGH;
    feeder_pwm.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&g_head_feeder_htim, &feeder_pwm, TIM_CHANNEL_1);

    // Start PWM in LOW state
    HAL_TIM_PWM_Start(&g_head_feeder_htim, TIM_CHANNEL_1); // Start feeder PWM
    __HAL_TIM_SET_AUTORELOAD(&g_head_feeder_htim, feeder_period_us);
    __HAL_TIM_SET_COMPARE(&g_head_feeder_htim, TIM_CHANNEL_1, 0);

    //
    // BRAKE

    // Setup head brake: PC7 EN, low state
    GPIO_InitTypeDef brake_en_gpio = {0};
    brake_en_gpio.Pin       = HEAD_BRAKE_EN_PIN;
    brake_en_gpio.Mode      = GPIO_MODE_OUTPUT_PP;
    brake_en_gpio.Pull      = GPIO_NOPULL;
    brake_en_gpio.Speed     = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(HEAD_BRAKE_EN_PORT, &brake_en_gpio);
    HAL_GPIO_WritePin(HEAD_BRAKE_EN_PORT, HEAD_BRAKE_EN_PIN, GPIO_PIN_RESET);

    // Setup head brake: PC8 STEP PWM
    GPIO_InitTypeDef brake_step_gpio = {0};
    brake_step_gpio.Pin       = HEAD_BRAKE_STEP_PIN;
    brake_step_gpio.Mode      = GPIO_MODE_AF_PP;
    brake_step_gpio.Pull      = GPIO_NOPULL;
    brake_step_gpio.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    brake_step_gpio.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(HEAD_BRAKE_STEP_PORT, &brake_step_gpio);

    // Setup TIM3: ABP1 clock source (42 MHz)
    g_head_brake_htim.Instance               = TIM3;
    g_head_brake_htim.Init.Prescaler         = 83; // Prescaler = 2 * 42 - 1 = 83: one tick = 1 us
    g_head_brake_htim.Init.CounterMode       = TIM_COUNTERMODE_UP;
    g_head_brake_htim.Init.Period            = brake_period_us;
    g_head_brake_htim.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    g_head_brake_htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE; // To avoid glitch for period update
    HAL_TIM_PWM_Init(&g_head_brake_htim);

    // Setup PWM channel: duty 50%
    TIM_OC_InitTypeDef brake_pwm = {0};
    brake_pwm.OCMode     = TIM_OCMODE_PWM1;      // HIGH while counter < CCR
    brake_pwm.Pulse      = brake_period_us / 2;  // CCR
    brake_pwm.OCPolarity = TIM_OCPOLARITY_HIGH;
    brake_pwm.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&g_head_brake_htim, &brake_pwm, TIM_CHANNEL_3);

    // Start PWM in LOW state
    HAL_TIM_PWM_Start(&g_head_brake_htim, TIM_CHANNEL_3); // Start brake PWM
    __HAL_TIM_SET_AUTORELOAD(&g_head_brake_htim, brake_period_us);
    __HAL_TIM_SET_COMPARE(&g_head_brake_htim, TIM_CHANNEL_3, 0);
}

void start_head() {
    HAL_GPIO_WritePin(HEAD_FEEDER_EN_PORT, HEAD_FEEDER_EN_PIN, GPIO_PIN_SET); // Enable feeder driver
    HAL_TIM_PWM_Start(&g_head_feeder_htim, TIM_CHANNEL_1); // Start feeder PWM
    __HAL_TIM_SET_AUTORELOAD(&g_head_feeder_htim, feeder_period_us);
    __HAL_TIM_SET_COMPARE(&g_head_feeder_htim, TIM_CHANNEL_1, feeder_period_us / 2);

    HAL_GPIO_WritePin(HEAD_BRAKE_EN_PORT, HEAD_BRAKE_EN_PIN, GPIO_PIN_SET); // Enable brake driver
    HAL_TIM_PWM_Start(&g_head_brake_htim, TIM_CHANNEL_3); // Start brake PWM
    __HAL_TIM_SET_AUTORELOAD(&g_head_brake_htim, brake_period_us);
    __HAL_TIM_SET_COMPARE(&g_head_brake_htim, TIM_CHANNEL_3, brake_period_us / 2);
}

void stop_head() {
    HAL_GPIO_WritePin(HEAD_FEEDER_EN_PORT, HEAD_FEEDER_EN_PIN, GPIO_PIN_RESET); // Disable feeder driver
    __HAL_TIM_SET_COMPARE(&g_head_feeder_htim, TIM_CHANNEL_1, 0); // Set PWM to low state

    HAL_GPIO_WritePin(HEAD_BRAKE_EN_PORT, HEAD_BRAKE_EN_PIN, GPIO_PIN_RESET); // Disable brake driver
    __HAL_TIM_SET_COMPARE(&g_head_brake_htim, TIM_CHANNEL_3, 0); // Set PWM to low state
}

void set_head_speed(uint32_t feeder_period_us, uint32_t brake_period_us) {
    if (feeder_period_us < 2) feeder_period_us = 2;
    if (brake_period_us < 2)  brake_period_us  = 2;

    // Update feeder freq
    __HAL_TIM_SET_AUTORELOAD(&g_head_feeder_htim, feeder_period_us);
    __HAL_TIM_SET_COMPARE(&g_head_feeder_htim, TIM_CHANNEL_1, feeder_period_us / 2);

    // Update brake freq
    __HAL_TIM_SET_AUTORELOAD(&g_head_brake_htim, brake_period_us);
    __HAL_TIM_SET_COMPARE(&g_head_brake_htim, TIM_CHANNEL_3, brake_period_us / 2);
}

void setup() {
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    HAL_GPIO_DeInit(HEAD_BRAKE_STEP_PORT, HEAD_BRAKE_STEP_PIN);

    // Change SPI pinout
    SPI.setMOSI(PA7);
    SPI.setMISO(PA6);
    SPI.setSCLK(PA5);

    tft.begin();
    tft.setOrientation(2);
    tft.setBackgroundColor(COLOR_BLACK);
    tft.clear();
    update_display();

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

    // Head
    setup_head();

    // //
    // // Setup head tension sensor
    // pinMode(HEAD_TENSION_SENSOR_VCC, OUTPUT);
    // digitalWrite(HEAD_TENSION_SENSOR_VCC, HIGH);
    // pinMode(HEAD_TENSION_SENSOR_GND, OUTPUT);
    // digitalWrite(HEAD_TENSION_SENSOR_GND, LOW);
    // head_tension.begin(HEAD_TENSION_SENSOR_DOUT, HEAD_TENSION_SENSOR_SCK);
    // head_tension.set_scale(1);
    // head_tension.set_offset(175000);
    // // 500g = 850 000

   

    // PWMC_ConfigureChannel(PWM, HEAD_FEEDER_PWM_CH, PWM_CMR_CPRE_CLKA, 0, 0);
    // PWMC_SetPeriod(PWM, HEAD_FEEDER_PWM_CH, feeder_period_us);
    // PWMC_SetDutyCycle(PWM, HEAD_FEEDER_PWM_CH, feeder_period_us / 2);

    // pinMode(HEAD_FEEDER_EN, OUTPUT);
    // digitalWrite(HEAD_FEEDER_EN, HIGH);
    // pinMode(HEAD_FEEDER_STEP, OUTPUT);
    // digitalWrite(HEAD_FEEDER_STEP, LOW);

    // //
    // // Setup head brake
    // PWMC_ConfigureChannel(PWM, HEAD_BRAKE_PWM_CH, PWM_CMR_CPRE_CLKA, 0, 0);
    // PWMC_SetPeriod(PWM, HEAD_BRAKE_PWM_CH, brake_period_us);
    // PWMC_SetDutyCycle(PWM, HEAD_BRAKE_PWM_CH, brake_period_us / 2);

    // pinMode(HEAD_BRAKE_EN, OUTPUT);
    // digitalWrite(HEAD_BRAKE_EN, HIGH);
    // pinMode(HEAD_BRAKE_STEP, OUTPUT);
    // digitalWrite(HEAD_BRAKE_STEP, LOW);
    
    // //
    // // Setup MOSFET gate PWM
    // int32_t period_ticks = spark_t1_us + spark_t0_us;
    // int32_t duty_ticks = spark_t1_us;
    // PWMC_ConfigureChannel(PWM, MOSFET_GATE_CTRL_PWM_CH, PWM_CMR_CPRE_CLKA, 0, 0);
    // PWMC_SetPeriod(PWM, MOSFET_GATE_CTRL_PWM_CH, period_ticks);
    // PWMC_SetDutyCycle(PWM, MOSFET_GATE_CTRL_PWM_CH, duty_ticks);

    // pinMode(MOSFET_GATE_CTRL, OUTPUT);
    // digitalWrite(MOSFET_GATE_CTRL, LOW);
    // pinMode(MOSFET_GATE_GND, OUTPUT);
    // digitalWrite(MOSFET_GATE_GND, LOW);

    // // // Enable IRQ for gate PWM
    // // PWM->PWM_IER1 = (1 << MOSFET_GATE_CTRL_PWM_CH);
    // // NVIC_DisableIRQ(PWM_IRQn);
    // // NVIC_ClearPendingIRQ(PWM_IRQn);
    // // NVIC_SetPriority(PWM_IRQn, 0);
    // // NVIC_EnableIRQ(PWM_IRQn);

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
        // update_mosfet_ctrl_pwm();
        update_display();
        s_last_update_params_time_ms = millis();
    }
    if (spark_generator_state) {
        is_short_circuit = false;
    }

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

    // 
    // Tension control
    // static int32_t s_tension_acc = 0;
    // static int32_t s_tension_acc_n = 0;
    // if (head_tension.is_ready()) {
    //     s_tension_acc += head_tension.read();
    //     s_tension_acc_n++;
    // }

    // if (spark_generator_state) {
    //     if (s_tension_acc_n >= 10) {
    //         int32_t d = -1001000 - (s_tension_acc / s_tension_acc_n);
    //         if (d > 100) {
    //             brake_period_us = constrain(brake_period_us - 10, 2000, 15000);
    //         } else if (d < -100) {
    //             brake_period_us = constrain(brake_period_us + 10, 2000, 15000);
    //         }
    //         set_head_speed(feeder_period_us, brake_period_us);

    //         tension_bins = s_tension_acc / s_tension_acc_n;
    //         tension_g = abs(tension_bins) / TENSION_SCALE;

    //         s_tension_acc = 0;
    //         s_tension_acc_n = 0;
    //     }
    // } else {
    //     s_tension_acc = 0;
    //     s_tension_acc_n = 0;
    // }

    //
    // Shutdown
    static bool is_periph_enabled = true;
    if (!spark_generator_state) {
        if (is_periph_enabled) {
            stop_head();
            // PWMC_DisableChannel(PWM, MOSFET_GATE_CTRL_PWM_CH);
            // pinMode(MOSFET_GATE_CTRL, OUTPUT);
            // digitalWrite(MOSFET_GATE_CTRL, LOW);

            // PWMC_DisableChannel(PWM, HEAD_BRAKE_PWM_CH);
            // pinMode(HEAD_BRAKE_STEP, OUTPUT);
            // digitalWrite(HEAD_BRAKE_STEP, LOW);

            // PWMC_DisableChannel(PWM, HEAD_FEEDER_PWM_CH);
            // pinMode(HEAD_FEEDER_STEP, OUTPUT);
            // digitalWrite(HEAD_FEEDER_STEP, LOW);

            // PWMC_DisableChannel(PWM, X_PWM_CH);
            // pinMode(X_STEP, OUTPUT);
            // digitalWrite(X_STEP, LOW);

            // digitalWrite(HEAD_FEEDER_EN, HIGH);
            // digitalWrite(HEAD_BRAKE_EN, HIGH);
            // digitalWrite(X_EN, HIGH);

            // Serial.println("periph disabled");
            is_periph_enabled = false;
        }
    } else {
        if (!is_periph_enabled) {
            start_head();
            // PIO_Configure(g_APinDescription[HEAD_FEEDER_STEP].pPort, 
            //     g_APinDescription[HEAD_FEEDER_STEP].ulPinType, 
            //     g_APinDescription[HEAD_FEEDER_STEP].ulPin, 
            //     g_APinDescription[HEAD_FEEDER_STEP].ulPinConfiguration);
            // PWMC_EnableChannel(PWM, HEAD_FEEDER_PWM_CH);
            // digitalWrite(HEAD_FEEDER_EN, LOW);

            // PIO_Configure(g_APinDescription[HEAD_BRAKE_STEP].pPort, 
            //               g_APinDescription[HEAD_BRAKE_STEP].ulPinType, 
            //               g_APinDescription[HEAD_BRAKE_STEP].ulPin, 
            //               g_APinDescription[HEAD_BRAKE_STEP].ulPinConfiguration);
            // PWMC_EnableChannel(PWM, HEAD_BRAKE_PWM_CH);
            // digitalWrite(HEAD_BRAKE_EN, LOW);

            // PIO_Configure(g_APinDescription[MOSFET_GATE_CTRL].pPort, 
            //               g_APinDescription[MOSFET_GATE_CTRL].ulPinType, 
            //               g_APinDescription[MOSFET_GATE_CTRL].ulPin, 
            //               g_APinDescription[MOSFET_GATE_CTRL].ulPinConfiguration);
            // PWMC_EnableChannel(PWM, MOSFET_GATE_CTRL_PWM_CH);

            // PIO_Configure(g_APinDescription[X_STEP].pPort, 
            //               g_APinDescription[X_STEP].ulPinType, 
            //               g_APinDescription[X_STEP].ulPin, 
            //               g_APinDescription[X_STEP].ulPinConfiguration);
            // PWMC_EnableChannel(PWM, X_PWM_CH);
            // digitalWrite(X_EN, LOW);

            // Serial.println("periph enabled");
            is_periph_enabled = true;
        }
    }
}