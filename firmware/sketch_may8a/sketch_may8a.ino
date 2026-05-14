#include <GyverOLED.h>

GyverOLED<SSD1306_128x64, OLED_BUFFER> oled; 


#define T1_INC_BUTTON           (3)
#define T1_DEC_BUTTON           (6)
#define T0_INC_BUTTON           (4)
#define T0_DEC_BUTTON           (5)
#define START_STOP_BUTTON       (2)
#define KEYBOARD_GND            (7)

#define MOSFET_GATE_CTRL        (9)
#define MOSFET_GATE_GND         (8)

int32_t spark_freq = 0;
int32_t spark_t1_us = 50;
int32_t spark_t0_us = 500;
int32_t spark_current = 0;
int32_t spark_voltage = 0;
bool spark_state = false;


void update_params() {
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
    oled.clear();          // Очистка экрана
  
    // Настройки для отрисовки
    int x = 5;
    int y = 38;
    int w = 128;
    int h = 25;

    oled.line(x, y, x, y + h);
    oled.line(x, y, x + w / 2, y);
    oled.line(x + w / 2, y, x + w / 2, y + h);
    oled.line(x + w / 2, y + h, x + w, y + h);
    
    oled.setScale(1);

    // F
    oled.setCursorXY(5, 5);
    oled.print("F: ");
    oled.print(spark_freq);
    oled.print(" Hz");

    // State 
    oled.setCursorXY(80, 5);
    if (spark_state) {
        oled.print("ENABLED");
    } else {
        oled.print("DISABLED");
    }
    
    // I
    oled.setCursorXY(5, 20);
    oled.print("I: ");
    oled.print(10000);
    oled.print(" mA");

    // U
    oled.setCursorXY(80, 20);
    oled.print("U: ");
    oled.print(80);
    oled.print(" V");

    // T1
    oled.setCursorXY(20, y + 10);
    oled.print(spark_t1_us);
    oled.print(" us");

    // T0
    oled.setCursorXY(80, y + 10);
    oled.print(spark_t0_us);
    oled.print(" us");

    oled.update();
}

void set_pulse() {
    static int32_t s_last_t1_us = 0;
    static int32_t s_last_t0_us = 0;
    static bool    s_last_state = 0;

    if (!spark_state || spark_t1_us == 0 || spark_t0_us == 0) {
        Serial.println("DISABLE TIMER");
        s_last_state = spark_state;

        TCCR1A &= ~((1 << COM1A1) | (1 << COM1A0));
        digitalWrite(MOSFET_GATE_CTRL, LOW);
        return;
    }

    if (s_last_t1_us != spark_t1_us || s_last_t0_us != spark_t0_us || s_last_state != spark_state) {
        if (spark_t1_us <= 0 || spark_t0_us <= 0) {
            return;
        }
        s_last_t1_us = spark_t1_us;
        s_last_t0_us = spark_t0_us;
        s_last_state = spark_state;

        Serial.print("UPDATE TIMER: ");
        Serial.print(spark_t1_us);
        Serial.print(" ");
        Serial.println(spark_t0_us);

        noInterrupts();

        // Disable timer
        TCCR1A &= ~((1 << COM1A1) | (1 << COM1A0));

        // Disable MOSFET
        digitalWrite(MOSFET_GATE_CTRL, LOW);

        int32_t period = spark_t1_us + spark_t0_us;
      
        // Setup timer: Fast PWM, mode 14 - ICR1 as TOP)
        TCNT1  = 0; // Reset counter
        ICR1   = (period * 2) - 1; // * 2 for convert 0.5 us per tick -> 1 us per ticks
        OCR1A  = (spark_t1_us * 2) - 1; // Set HIGH pulse time
        TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11); // 1 tick = 0.5 us
        TCCR1A = (1 << COM1A1) | (1 << WGM11); // Move pin for timer control

        interrupts();
    }
}

void setup() {
    Serial.begin(115200);

    oled.init();
    Wire.setClock(400000); 
    oled.clear();

    pinMode(T1_INC_BUTTON, INPUT_PULLUP); // T1 button +
    pinMode(T1_DEC_BUTTON, INPUT_PULLUP); // T1 button -
    pinMode(T0_INC_BUTTON, INPUT_PULLUP); // T0 button +
    pinMode(T0_DEC_BUTTON, INPUT_PULLUP); // T0 button -
    pinMode(START_STOP_BUTTON, INPUT_PULLUP); // Start / stop button
    pinMode(KEYBOARD_GND, OUTPUT);
    digitalWrite(KEYBOARD_GND, LOW);

    // MOSFET
    pinMode(MOSFET_GATE_CTRL, OUTPUT);
    pinMode(MOSFET_GATE_GND, OUTPUT);
    digitalWrite(MOSFET_GATE_GND, LOW);
}

void loop() {
    static uint32_t s_last_update_params_time_ms = 0;
    if (millis() - s_last_update_params_time_ms > 50) {
        update_params();
        set_pulse();
        update_display();
        s_last_update_params_time_ms = millis();
    }
}
