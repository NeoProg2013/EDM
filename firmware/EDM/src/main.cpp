#include <Arduino.h>
#include <SPI.h>
#include "TFT_22_ILI9225.h"
#include "HX711.h"

#define TFT_RST                   (50)
#define TFT_RS                    (48)
#define TFT_CS                    (52)

#define T1_INC_BUTTON             (47)
#define T1_DEC_BUTTON             (53)
#define T0_INC_BUTTON             (49)
#define T0_DEC_BUTTON             (51)
#define START_STOP_BUTTON         (45)

#define HEAD_FEEDER_EN            (31)
#define HEAD_FEEDER_STEP          (8)
#define HEAD_FEEDER_PWM_CH        (5)

#define HEAD_BRAKE_EN             (33)
#define HEAD_BRAKE_STEP           (7)
#define HEAD_BRAKE_PWM_CH         (6)

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


#define DEBUG_PIN_1                 (13)
#define DEBUG_PIN_2                 (12)
#define DEBUG_PIN_3                 (11)
#define DEBUG_PIN_4                 (10)




TFT_22_ILI9225 tft(TFT_RST, TFT_RS, TFT_CS, 0, 255);
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
    spark_t1_us += digitalRead(T1_INC_BUTTON) == LOW;
    spark_t1_us -= digitalRead(T1_DEC_BUTTON) == LOW;
    spark_t0_us += digitalRead(T0_INC_BUTTON) == LOW;
    spark_t0_us -= digitalRead(T0_DEC_BUTTON) == LOW;

    static bool s_last_start_stop_button_state = HIGH;
    bool v = digitalRead(START_STOP_BUTTON);
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

void update_mosfet_ctrl_pwm() {
    static int32_t s_last_t1_us = 0;
    static int32_t s_last_t0_us = 0;
    static bool    s_last_state = 0;

    if (s_last_t1_us != spark_t1_us || s_last_t0_us != spark_t0_us || s_last_state != spark_generator_state) {
        if (spark_t1_us <= 0 || spark_t0_us <= 0) {
            return;
        }
        s_last_t1_us = spark_t1_us;
        s_last_t0_us = spark_t0_us;
        s_last_state = spark_generator_state;

        Serial.print("update MOSFET CTRL PWM: ");
        Serial.print(spark_t1_us);
        Serial.print(" ");
        Serial.println(spark_t0_us);

        uint32_t period = spark_t1_us + spark_t0_us;
        uint32_t duty = spark_t1_us;
        PWMC_SetPeriod(PWM, MOSFET_GATE_CTRL_PWM_CH, period);
        PWMC_SetDutyCycle(PWM, MOSFET_GATE_CTRL_PWM_CH, duty);
    }
}


void spark_feedback_irq(void) {
    bool state = (g_APinDescription[SPARK_SHORT_CIRCUIT].pPort->PIO_PDSR & g_APinDescription[SPARK_SHORT_CIRCUIT].ulPin);

    static uint32_t s_start = 0;
    if (state == HIGH) {
        g_spark_time_us = micros() - s_start;
    } else {
        s_start = micros();
    }
}

void setup() {
    // Enable PWM periph
    pmc_enable_periph_clk(ID_PWM);
    pmc_enable_periph_clk(ID_ADC);
    PWMC_ConfigureClocks(1000000UL, 0, VARIANT_MCK); // 1 tick = 1 us

    // Setup debug
    pinMode(DEBUG_PIN_1, OUTPUT);
    pinMode(DEBUG_PIN_2, OUTPUT);
    pinMode(DEBUG_PIN_3, OUTPUT);
    pinMode(DEBUG_PIN_4, OUTPUT);

    Serial.begin(115200);

    tft.begin();
    tft.setOrientation(2);
    tft.setBackgroundColor(COLOR_BLACK);
    tft.clear();
    update_display();

    // Keyboard
    pinMode(T1_INC_BUTTON, INPUT_PULLUP); // T1 button +
    pinMode(T1_DEC_BUTTON, INPUT_PULLUP); // T1 button -
    pinMode(T0_INC_BUTTON, INPUT_PULLUP); // T0 button +
    pinMode(T0_DEC_BUTTON, INPUT_PULLUP); // T0 button -
    pinMode(START_STOP_BUTTON, INPUT_PULLUP); // Start / stop button

    //
    // Setup head tension sensor
    pinMode(HEAD_TENSION_SENSOR_VCC, OUTPUT);
    digitalWrite(HEAD_TENSION_SENSOR_VCC, HIGH);
    pinMode(HEAD_TENSION_SENSOR_GND, OUTPUT);
    digitalWrite(HEAD_TENSION_SENSOR_GND, LOW);
    head_tension.begin(HEAD_TENSION_SENSOR_DOUT, HEAD_TENSION_SENSOR_SCK);
    head_tension.set_scale(1);
    head_tension.set_offset(175000);
    // 500g = 850 000

    //
    // Setup head feeder
    PWMC_ConfigureChannel(PWM, HEAD_FEEDER_PWM_CH, PWM_CMR_CPRE_CLKA, 0, 0);
    PWMC_SetPeriod(PWM, HEAD_FEEDER_PWM_CH, feeder_period_us);
    PWMC_SetDutyCycle(PWM, HEAD_FEEDER_PWM_CH, feeder_period_us / 2);

    pinMode(HEAD_FEEDER_EN, OUTPUT);
    digitalWrite(HEAD_FEEDER_EN, HIGH);
    pinMode(HEAD_FEEDER_STEP, OUTPUT);
    digitalWrite(HEAD_FEEDER_STEP, LOW);

    //
    // Setup head brake
    PWMC_ConfigureChannel(PWM, HEAD_BRAKE_PWM_CH, PWM_CMR_CPRE_CLKA, 0, 0);
    PWMC_SetPeriod(PWM, HEAD_BRAKE_PWM_CH, brake_period_us);
    PWMC_SetDutyCycle(PWM, HEAD_BRAKE_PWM_CH, brake_period_us / 2);

    pinMode(HEAD_BRAKE_EN, OUTPUT);
    digitalWrite(HEAD_BRAKE_EN, HIGH);
    pinMode(HEAD_BRAKE_STEP, OUTPUT);
    digitalWrite(HEAD_BRAKE_STEP, LOW);
    
    //
    // Setup MOSFET gate PWM
    int32_t period_ticks = spark_t1_us + spark_t0_us;
    int32_t duty_ticks = spark_t1_us;
    PWMC_ConfigureChannel(PWM, MOSFET_GATE_CTRL_PWM_CH, PWM_CMR_CPRE_CLKA, 0, 0);
    PWMC_SetPeriod(PWM, MOSFET_GATE_CTRL_PWM_CH, period_ticks);
    PWMC_SetDutyCycle(PWM, MOSFET_GATE_CTRL_PWM_CH, duty_ticks);

    pinMode(MOSFET_GATE_CTRL, OUTPUT);
    digitalWrite(MOSFET_GATE_CTRL, LOW);
    pinMode(MOSFET_GATE_GND, OUTPUT);
    digitalWrite(MOSFET_GATE_GND, LOW);

    // // Enable IRQ for gate PWM
    // PWM->PWM_IER1 = (1 << MOSFET_GATE_CTRL_PWM_CH);
    // NVIC_DisableIRQ(PWM_IRQn);
    // NVIC_ClearPendingIRQ(PWM_IRQn);
    // NVIC_SetPriority(PWM_IRQn, 0);
    // NVIC_EnableIRQ(PWM_IRQn);

    //
    // Setup X axis
    PWMC_ConfigureChannel(PWM, X_PWM_CH, PWM_CMR_CPRE_CLKA, 0, 0);
    PWMC_SetPeriod(PWM, X_PWM_CH, 10000);
    PWMC_SetDutyCycle(PWM, X_PWM_CH, 10000 / 2);

    pinMode(X_EN, OUTPUT);
    digitalWrite(X_EN, HIGH);
    pinMode(X_STEP, OUTPUT);
    digitalWrite(X_STEP, LOW);
    pinMode(X_DIR, OUTPUT);
    digitalWrite(X_DIR, LOW);

    // Feedback
    pinMode(SPARK_SHORT_CIRCUIT, INPUT_PULLUP);
    attachInterrupt(SPARK_SHORT_CIRCUIT, spark_feedback_irq, CHANGE);
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
        update_mosfet_ctrl_pwm();
        update_display();
        s_last_update_params_time_ms = millis();
    }
    if (spark_generator_state) {
        is_short_circuit = false;
    }

    //
    // Short circuit control
    static uint32_t s_no_spark_counter = 0;
    if (g_spark_time_us < 5) {
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
    static int32_t s_tension_acc = 0;
    static int32_t s_tension_acc_n = 0;
    if (head_tension.is_ready()) {
        s_tension_acc += head_tension.read();
        s_tension_acc_n++;
    }

    if (spark_generator_state) {
        if (s_tension_acc_n >= 10) {
            int32_t d = -1001000 - (s_tension_acc / s_tension_acc_n);
            if (d > 100) {
                brake_period_us = constrain(brake_period_us - 10, 2000, 15000);
            } else if (d < -100) {
                brake_period_us = constrain(brake_period_us + 10, 2000, 15000);
            }
            PWMC_SetPeriod(PWM, HEAD_BRAKE_PWM_CH, brake_period_us);
            PWMC_SetDutyCycle(PWM, HEAD_BRAKE_PWM_CH, brake_period_us / 2);

            tension_bins = s_tension_acc / s_tension_acc_n;
            tension_g = abs(tension_bins) / TENSION_SCALE;

            s_tension_acc = 0;
            s_tension_acc_n = 0;
        }
    } else {
        s_tension_acc = 0;
        s_tension_acc_n = 0;
    }

    //
    // Shutdown
    static bool is_periph_enabled = true;
    if (!spark_generator_state) {
        if (is_periph_enabled) {
            PWMC_DisableChannel(PWM, MOSFET_GATE_CTRL_PWM_CH);
            pinMode(MOSFET_GATE_CTRL, OUTPUT);
            digitalWrite(MOSFET_GATE_CTRL, LOW);

            PWMC_DisableChannel(PWM, HEAD_BRAKE_PWM_CH);
            pinMode(HEAD_BRAKE_STEP, OUTPUT);
            digitalWrite(HEAD_BRAKE_STEP, LOW);

            PWMC_DisableChannel(PWM, HEAD_FEEDER_PWM_CH);
            pinMode(HEAD_FEEDER_STEP, OUTPUT);
            digitalWrite(HEAD_FEEDER_STEP, LOW);

            PWMC_DisableChannel(PWM, X_PWM_CH);
            pinMode(X_STEP, OUTPUT);
            digitalWrite(X_STEP, LOW);

            digitalWrite(HEAD_FEEDER_EN, HIGH);
            digitalWrite(HEAD_BRAKE_EN, HIGH);
            digitalWrite(X_EN, HIGH);

            Serial.println("periph disabled");
            is_periph_enabled = false;
        }
    } else {
        if (!is_periph_enabled) {
            PIO_Configure(g_APinDescription[HEAD_FEEDER_STEP].pPort, 
                g_APinDescription[HEAD_FEEDER_STEP].ulPinType, 
                g_APinDescription[HEAD_FEEDER_STEP].ulPin, 
                g_APinDescription[HEAD_FEEDER_STEP].ulPinConfiguration);
            PWMC_EnableChannel(PWM, HEAD_FEEDER_PWM_CH);
            digitalWrite(HEAD_FEEDER_EN, LOW);

            PIO_Configure(g_APinDescription[HEAD_BRAKE_STEP].pPort, 
                          g_APinDescription[HEAD_BRAKE_STEP].ulPinType, 
                          g_APinDescription[HEAD_BRAKE_STEP].ulPin, 
                          g_APinDescription[HEAD_BRAKE_STEP].ulPinConfiguration);
            PWMC_EnableChannel(PWM, HEAD_BRAKE_PWM_CH);
            digitalWrite(HEAD_BRAKE_EN, LOW);

            PIO_Configure(g_APinDescription[MOSFET_GATE_CTRL].pPort, 
                          g_APinDescription[MOSFET_GATE_CTRL].ulPinType, 
                          g_APinDescription[MOSFET_GATE_CTRL].ulPin, 
                          g_APinDescription[MOSFET_GATE_CTRL].ulPinConfiguration);
            PWMC_EnableChannel(PWM, MOSFET_GATE_CTRL_PWM_CH);

            PIO_Configure(g_APinDescription[X_STEP].pPort, 
                          g_APinDescription[X_STEP].ulPinType, 
                          g_APinDescription[X_STEP].ulPin, 
                          g_APinDescription[X_STEP].ulPinConfiguration);
            PWMC_EnableChannel(PWM, X_PWM_CH);
            digitalWrite(X_EN, LOW);

            Serial.println("periph enabled");
            is_periph_enabled = true;
        }
    }
}