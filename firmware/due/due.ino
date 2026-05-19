// Include application, user and local libraries
#include "SPI.h"
#include "TFT_22_ILI9225.h"

#define TFT_RST                 (50)
#define TFT_RS                  (48)
#define TFT_CS                  (52)

#define T1_INC_BUTTON           (47)
#define T1_DEC_BUTTON           (53)
#define T0_INC_BUTTON           (49)
#define T0_DEC_BUTTON           (51)
#define START_STOP_BUTTON       (45)

#define HEAD_FEEDER_EN          (22)
#define HEAD_FEEDER_STEP        (8)
#define HEAD_FEEDER_PWM_CH      (5)

#define HEAD_BRAKE_EN           (23)
#define HEAD_BRAKE_STEP         (7)
#define HEAD_BRAKE_PWM_CH       (6)

#define MOSFET_GATE_CTRL        (6) // PWM timer
#define MOSFET_GATE_CTRL_PWM_CH (7)
#define MOSFET_GATE_GND         (5)

#define SPART_SHORT_CIRCUIT     (A1)

TFT_22_ILI9225 tft = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, 0, 255);

int32_t spark_freq    = 0;
int32_t spark_t1_us   = 50;
int32_t spark_t0_us   = 500;
int32_t spark_current = 0;
int32_t spark_voltage = 0;
bool spark_state      = false;
bool is_short_circuit = false;

void keyboard_process() {
    // Process buttons
    spark_t1_us += digitalRead(T1_INC_BUTTON) == LOW;
    spark_t1_us -= digitalRead(T1_DEC_BUTTON) == LOW;
    spark_t0_us += digitalRead(T0_INC_BUTTON) == LOW;
    spark_t0_us -= digitalRead(T0_DEC_BUTTON) == LOW;

    static bool s_last_start_stop_button_state = HIGH;
    bool v = digitalRead(START_STOP_BUTTON);
    if (v == LOW && s_last_start_stop_button_state == HIGH) {
        spark_state = !spark_state;
    }
    s_last_start_stop_button_state = v;

    // Validate values
    if (spark_t1_us < 10) spark_t1_us = 10;
    if (spark_t0_us < 10) spark_t0_us = 10;

    // Calc spark parameters
    spark_freq = 1000000 / (spark_t1_us + spark_t0_us);
}

void update_display() {
    static bool     is_init = false;
    static int32_t  last_freq = -1;
    static int32_t  last_state = -1;
    static int32_t  last_t1 = -1;
    static int32_t  last_t0 = -1;

    tft.setFont(Terminal6x8);

    if (!is_init) {
        tft.clear();
        tft.fillRectangle(0, 0, 176, 220, COLOR_BLACK);
        
        int x = 5;
        int y = 170;
        int w = 160;
        int h = 40;

        // Рисуем линии графика (белым цветом COLOR_WHITE)
        tft.drawLine(x, y, x, y + h, COLOR_WHITE);
        tft.drawLine(x, y, x + w / 2, y, COLOR_WHITE);
        tft.drawLine(x + w / 2, y, x + w / 2, y + h, COLOR_WHITE);
        tft.drawLine(x + w / 2, y + h, x + w, y + h, COLOR_WHITE);
        
        // Статические подписи, которые не меняются
        tft.drawText(5, 5, "F:", COLOR_WHITE);
        // tft.drawText(5, 30, "I: 10000 mA", COLOR_WHITE);
        // tft.drawText(95, 30, "U: 80 V", COLOR_WHITE);
        
        is_init = true;
    }

    // Freq
    if (last_freq != spark_freq) {
        tft.fillRectangle(23, 5, 90, 15, COLOR_BLACK);
        tft.drawText(23, 5, String(spark_freq) + " Hz", COLOR_WHITE);
        last_freq = spark_freq;
    }

    // State
    if (last_state != spark_state) {
        tft.fillRectangle(5, 20, 90, 30, COLOR_BLACK);
        if (is_short_circuit) {
            tft.drawText(5, 20, "SHORT CIRCUIT", COLOR_RED);
        } else {
            if (spark_state) {
                tft.drawText(5, 20, "ENABLED", COLOR_GREEN);
            } else {
                tft.drawText(5, 20, "DISABLED", COLOR_RED);
            }
        }
        
        last_state = spark_state;
    }

    // T1
    if (last_t1 != spark_t1_us) {
        tft.fillRectangle(15, 180, 75, 195, COLOR_BLACK);
        tft.drawText(25, 185, String(spark_t1_us) + " us", COLOR_YELLOW);
        last_t1 = spark_t1_us;
    }

    // T0
    if (last_t0 != spark_t0_us) {
        tft.fillRectangle(100, 180, 160, 195, COLOR_BLACK);
        tft.drawText(105, 185, String(spark_t0_us) + " us", COLOR_YELLOW);
        last_t0 = spark_t0_us;
    }
}

void update_mosfet_ctrl_pwm() {
    static int32_t s_last_t1_us = 0;
    static int32_t s_last_t0_us = 0;
    static bool    s_last_state = 0;

    if (s_last_t1_us != spark_t1_us || s_last_t0_us != spark_t0_us || s_last_state != spark_state) {
        if (spark_t1_us <= 0 || spark_t0_us <= 0) {
            return;
        }
        s_last_t1_us = spark_t1_us;
        s_last_t0_us = spark_t0_us;
        s_last_state = spark_state;

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

void setup() {
    // Enable PWM periph
    pmc_enable_periph_clk(ID_PWM);
    PWMC_ConfigureClocks(1000000UL, 0, VARIANT_MCK);


    Serial.begin(115200);
    tft.begin();
    tft.clear();
    update_display();

    // Keyboard
    pinMode(T1_INC_BUTTON, INPUT_PULLUP); // T1 button +
    pinMode(T1_DEC_BUTTON, INPUT_PULLUP); // T1 button -
    pinMode(T0_INC_BUTTON, INPUT_PULLUP); // T0 button +
    pinMode(T0_DEC_BUTTON, INPUT_PULLUP); // T0 button -
    pinMode(START_STOP_BUTTON, INPUT_PULLUP); // Start / stop button

    // Feedback
    pinMode(SPART_SHORT_CIRCUIT, INPUT);

    //
    // Setup head feeder
    int32_t freq = 100;
    int32_t period_ticks = 1000000UL / freq;
    int32_t duty_ticks = period_ticks / 2;
    PWMC_ConfigureChannel(PWM, HEAD_FEEDER_PWM_CH, PWM_CMR_CPRE_CLKA, 0, 0);
    PWMC_SetPeriod(PWM, HEAD_FEEDER_PWM_CH, period_ticks);
    PWMC_SetDutyCycle(PWM, HEAD_FEEDER_PWM_CH, duty_ticks);

    pinMode(HEAD_FEEDER_EN, OUTPUT);
    pinMode(HEAD_FEEDER_EN, HIGH);
    pinMode(HEAD_FEEDER_STEP, OUTPUT);
    digitalWrite(HEAD_FEEDER_STEP, LOW);
    
    //
    // Setup head brake
    freq = 90;
    period_ticks = 1000000UL / freq;
    duty_ticks = period_ticks / 2;
    PWMC_ConfigureChannel(PWM, HEAD_BRAKE_PWM_CH, PWM_CMR_CPRE_CLKA, 0, 0);
    PWMC_SetPeriod(PWM, HEAD_BRAKE_PWM_CH, period_ticks);
    PWMC_SetDutyCycle(PWM, HEAD_BRAKE_PWM_CH, duty_ticks);

    pinMode(HEAD_BRAKE_EN, OUTPUT);
    pinMode(HEAD_BRAKE_EN, HIGH);
    pinMode(HEAD_BRAKE_STEP, OUTPUT);
    digitalWrite(HEAD_BRAKE_STEP, LOW);
    
    //
    // Setup MOSFET gate PWM
    period_ticks = spark_t1_us + spark_t0_us;
    duty_ticks = spark_t1_us;
    PWMC_ConfigureChannel(PWM, MOSFET_GATE_CTRL_PWM_CH, PWM_CMR_CPRE_CLKA, 0, 0);
    PWMC_SetPeriod(PWM, MOSFET_GATE_CTRL_PWM_CH, period_ticks);
    PWMC_SetDutyCycle(PWM, MOSFET_GATE_CTRL_PWM_CH, duty_ticks);

    pinMode(MOSFET_GATE_CTRL, OUTPUT);
    digitalWrite(MOSFET_GATE_CTRL, LOW);
    pinMode(MOSFET_GATE_GND, OUTPUT);
    digitalWrite(MOSFET_GATE_GND, LOW);
}

void loop() {
    static uint32_t s_last_update_params_time_ms = 0;
    if (millis() - s_last_update_params_time_ms > 50) {
        keyboard_process();
        update_mosfet_ctrl_pwm();
        update_display();
        s_last_update_params_time_ms = millis();
    }
    if (spark_state) {
        is_short_circuit = false;
    }

    // Short circuit control
    static uint32_t s_spark_meas_time_us = 0;
    static uint32_t s_spark_data_acc = 0;
    static uint32_t s_spark_data_n = 0;
    static uint32_t s_no_spark_counter = 0;
    if (micros() - s_spark_meas_time_us > 5000) {
        if (s_spark_data_n > 0) {
            spark_voltage = s_spark_data_acc / s_spark_data_n;
            // Serial.println(spark_voltage);
            
            s_spark_data_acc = 0;
            s_spark_data_n = 0;

            if (spark_voltage < 160 && spark_state) {
                ++s_no_spark_counter;
                if (s_no_spark_counter > 5) {
                    spark_state = false;
                    is_short_circuit = true;
                    s_no_spark_counter = 0;
                }
            } else {
                s_no_spark_counter = 0;
            }
        }

        s_spark_meas_time_us = micros();
    }
    if (spark_state) {
        s_spark_data_acc += analogRead(SPART_SHORT_CIRCUIT);
        ++s_spark_data_n;
    }

    //
    // Shutdown
    static bool is_periph_enabled = false;
    if (!spark_state) {
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

            digitalWrite(HEAD_FEEDER_EN, HIGH);
            digitalWrite(HEAD_BRAKE_EN, HIGH);

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

            Serial.println("periph enabled");
            is_periph_enabled = true;
        }
    }
}