#include "display.h"
#include "TFT_22_ILI9225.h"
#include "spark.h"
#include "tension.h"

#define TFT_RST         (PA2)
#define TFT_RS          (PA3)
#define TFT_CS          (PA4)
#define TFT_LED         (PA6)

TFT_22_ILI9225 tft(TFT_RST, TFT_RS, TFT_CS, TFT_LED, 255);

extern uint32_t g_spark_time_us;


void display_init() {
    SPI.setMOSI(PA7);
    SPI.setMISO(PA6);
    SPI.setSCLK(PA5);

    tft.begin();
    tft.setOrientation(2);
    tft.setBackgroundColor(COLOR_BLACK);
    tft.clear();

    display_update();
}

void display_update() {
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
        tft.drawText(5, y, "  Feeder (us): ---", COLOR_WHITE); y += 15;
        tft.drawText(5, y, "   Brake (us): ---", COLOR_WHITE); y += 15;

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
        bool v = spark_is_enabled();
        if (s_last_state != v) {
            tft.fillRectangle(5, 5, 176, 21, COLOR_BLACK);
            if (v) {
                tft.drawText(5, 5, "ENABLED", COLOR_GREEN);
            } else {
                tft.drawText(5, 5, "DISABLED", COLOR_RED);
            }
            s_last_state = v;
        }
        return;
    }

    tft.setFont(Terminal6x8, true);
    int y = 40;

    // Freq
    if (s_call_counter == 2) {
        static int32_t s_last_freq = -1;
        int32_t v = spark_get_freq();
        if (s_last_freq != v) {
            tft.fillRectangle(106, y, 150, y + 8, COLOR_BLACK);
            tft.drawText(106, y, String(v), COLOR_YELLOW);
            s_last_freq = v;
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
        int32_t v = tension_get_tension_bins();
        if (s_last_tension_bins != v) {
            tft.fillRectangle(106, y, 150, y + 8, COLOR_BLACK);
            tft.drawText(106, y, String(v), COLOR_YELLOW);
            s_last_tension_bins = v;
        }
        return;
    }
    y += 15;

    // Tension (g)
    if (s_call_counter == 6) {
        int32_t v = tension_get_tension_g();
        static int32_t s_last_tension_g = -1;
        if (s_last_tension_g != v) {
            tft.fillRectangle(106, y, 150, y + 8, COLOR_BLACK);
            tft.drawText(106, y, String(v), COLOR_YELLOW);
            s_last_tension_g = v;
        }
        return;
    }
    y += 15;

    // Feeder freq
    if (s_call_counter == 7) {
        static int32_t s_last_feeder_period_us = -1;
        int32_t v = tension_get_feeder_period_us();
        if (s_last_feeder_period_us != v) {
            tft.fillRectangle(106, y, 150, y + 8, COLOR_BLACK);
            tft.drawText(106, y, String(1000000 / v), COLOR_YELLOW);
            s_last_feeder_period_us = v;
        }
        return;
    }
    y += 15;

    // Brake freq
    if (s_call_counter == 8) {
        static int32_t s_last_brake_period_us = -1;
        int32_t v = tension_get_brake_period_us();
        if (s_last_brake_period_us != v) {
            tft.fillRectangle(106, y, 150, y + 8, COLOR_BLACK);
            tft.drawText(106, y, String(1000000 / v), COLOR_YELLOW);
            s_last_brake_period_us = v;
        }
        return;
    }
    y += 15;

    // T1
    if (s_call_counter == 9) {
        static int32_t s_last_t1 = -1;
        int32_t v = spark_get_t1_us();
        if (s_last_t1 != v) {
            tft.fillRectangle(15, 180, 75, 195, COLOR_BLACK);
            tft.drawText(25, 185, String(v) + " us", COLOR_YELLOW);
            s_last_t1 = v;
        }
        return;
    }

    // T0
    if (s_call_counter == 10) {
        static int32_t s_last_t0 = -1;
        int32_t v = spark_get_t0_us();
        if (s_last_t0 != v) {
            tft.fillRectangle(100, 180, 160, 195, COLOR_BLACK);
            tft.drawText(105, 185, String(v) + " us", COLOR_YELLOW);
            s_last_t0 = v;
        }
        return;
    }

    s_call_counter = 0;
}
