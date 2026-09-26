#include "core.h"
#include "tension.h"
#include "spark.h"
#include "telemetry.h"
#include "motion_controller.h"

// #define DEBUG_PIN_1                 (PD10)
// #define DEBUG_PIN_2                 (PD9)
// #define DEBUG_PIN_3                 (PD8)
// #define DEBUG_PIN_4                 (PD15)

motion_controller_t motion_controller;


bool g_is_enabled = false;
uint16_t g_arc_counter = 0;
TIM_HandleTypeDef g_htim8 = {0};


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
    // tension_init();
    // tension_start();
    // spark_pwm_init();
    // init_feedback();

    // Motion core
    motion_controller.init();
    // motion_controller.move_to(-10000, -10000);
    // HAL_Delay(3000);

    // while (1) {
    //     //
    //     // Motion core
    //     motion_controller.process();
    // }

    while (1) {
        //
        // Telemetry
        tx_msg_t* tx_msg = telemetry_get_tx_msg();
        tx_msg->edm_status  = spark_is_enabled(),
        tx_msg->step_state  = true,
        tx_msg->freq_hz     = spark_get_freq(),
        tx_msg->arc_counter = g_arc_counter,
        tx_msg->tension_g   = tension_get_tension_g(),
        tx_msg->feeder_us   = tension_get_feeder_period_us(),
        tx_msg->brake_us    = tension_get_brake_period_us(),
        tx_msg->t1          = spark_get_t1_us(),
        tx_msg->t0          = spark_get_t0_us(),
        telemetry_process();

        if (telemetry_get_connection_state()) {
            rx_msg_t rx_msg;
            telemetry_get_rx_msg(&rx_msg);

            // Update EDM parameters
            static uint32_t s_last_update_params_time_ms = 0;
            if (HAL_GetTick() - s_last_update_params_time_ms > 500) {
                s_last_update_params_time_ms = HAL_GetTick();

                g_is_enabled = rx_msg.edm_status;
                // TODO: 
                // uint16_t t0;
                // uint16_t t1;
            }

            // Manual head adjust
            if (!motion_controller.is_busy()) {
                static const int32_t OFFSET = 1000;
                switch (rx_msg.cmd) {
                case rx_msg_t::CMD_MOVE_UP:
                    motion_controller.move_by(0, OFFSET);
                    break;
                case rx_msg_t::CMD_MOVE_DOWN:
                    motion_controller.move_by(0, -OFFSET);
                    break;
                case rx_msg_t::CMD_MOVE_LEFT:
                    motion_controller.move_by(-OFFSET, 0);
                    break;
                case rx_msg_t::CMD_MOVE_RIGHT:
                    motion_controller.move_by(OFFSET, 0);
                    break;
                }
            }
        } else { // Connection lost - shutdown
            g_is_enabled = false;
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
            //
            // low_us < 2 - for 3 us T1
            // low_us < 1 - for 1 us T1

            if (current_cnt > 1000 || low_us < 1) { // current_cnt > 1000 us -- no pulse long time
                motion_controller.lock();
                s_arc_last_time_ms = HAL_GetTick();
                ++g_arc_counter;
            } else {
                if (HAL_GetTick() - s_arc_last_time_ms > 100) { // 100 ms
                    motion_controller.unlock();
                }
            }
        }
        
        // 
        // Tension control
        tension_process();

        //
        // Motion core
        motion_controller.process();

        //
        // Shutdown
        static bool is_periph_enabled = false;
        if (g_is_enabled) {
            if (!is_periph_enabled) {
                tension_start();
                spark_pwm_start();
                motion_controller.unlock();
                is_periph_enabled = true;
            }
        } else {
            if (is_periph_enabled) {
                tension_stop();
                spark_pwm_stop();
                motion_controller.lock();
                is_periph_enabled = false;
            }
        }
    }
}
