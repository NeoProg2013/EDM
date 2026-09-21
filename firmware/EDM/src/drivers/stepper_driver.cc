#include "core.h"
#include "stepper_driver.h"

/// ***************************************************************************
/// @brief  Create stepper driver object and bind GPIO pins/polarity settings
/// @param  en_port: enable GPIO port
/// @param  en_pin: enable GPIO pin
/// @param  dir_port: direction GPIO port
/// @param  dir_pin: direction GPIO pin
/// @param  step_port: step GPIO port
/// @param  step_pin: step GPIO pin
/// ***************************************************************************
stepper_driver_t::stepper_driver_t(GPIO_TypeDef* en_port, uint16_t en_pin, 
                                   GPIO_TypeDef* dir_port, uint16_t dir_pin, 
                                   GPIO_TypeDef* step_port, uint16_t step_pin) {
    m_en_port   = en_port;
    m_en_pin    = en_pin;
    m_dir_port  = dir_port;
    m_dir_pin   = dir_pin;
    m_step_port = step_port;
    m_step_pin  = step_pin;
}

/// ***************************************************************************
/// @brief  Initialize GPIO pins and set safe default output states
/// ***************************************************************************
void stepper_driver_t::init() {
    GPIO_InitTypeDef en_gpio = {0};
    en_gpio.Pin   = m_en_pin;
    en_gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    en_gpio.Pull  = GPIO_NOPULL;
    en_gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(m_en_port, &en_gpio);
    HAL_GPIO_WritePin(m_en_port, m_en_pin, GPIO_PIN_SET); // Disable stepper

    GPIO_InitTypeDef dir_gpio = {0};
    dir_gpio.Pin   = m_dir_pin;
    dir_gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    dir_gpio.Pull  = GPIO_NOPULL;
    dir_gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(m_dir_port, &dir_gpio);

    GPIO_InitTypeDef step_gpio = {0};
    step_gpio.Pin   = m_step_pin;
    step_gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    step_gpio.Pull  = GPIO_NOPULL;
    step_gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(m_step_port, &step_gpio);
    HAL_GPIO_WritePin(m_step_port, m_step_pin, GPIO_PIN_RESET);
}

/// ***************************************************************************
/// @brief  Enable stepper driver output stage
/// @param  enabled: true - power on, false - power off
/// ***************************************************************************
void stepper_driver_t::set_power_state(bool enabled) {
    if (enabled) {
        HAL_GPIO_WritePin(m_en_port, m_en_pin, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(m_en_port, m_en_pin, GPIO_PIN_SET);
    }
}

/// ***************************************************************************
/// @brief  Set step pulse parameters
/// @param  t1_us: HIGH state time, [us]
/// @param  t0_us: LOW state time, [us]
/// ***************************************************************************
void stepper_driver_t::set_timings(uint16_t t1_us, uint16_t t0_us) {
    m_t1_time_us = t1_us;
    m_t0_time_us = t0_us;
}

/// ***************************************************************************
/// @brief  Set logical motion direction for the driver
/// @param  is_forward: true for forward direction, false for reverse direction
/// ***************************************************************************
void stepper_driver_t::set_direction(bool is_forward) {
    HAL_GPIO_WritePin(m_dir_port, m_dir_pin, is_forward ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/// ***************************************************************************
/// @brief  Start physical step pulse on STEP pin
/// ***************************************************************************
void stepper_driver_t::step() {
    if (m_is_step_active) {
        return;
    }

    HAL_GPIO_WritePin(m_step_port, m_step_pin, GPIO_PIN_SET);
    m_start_t1_us = HAL_GetTickUs();
    m_is_step_active = true;
    m_step_pin_state = true;
}

/// ***************************************************************************
/// @brief  Check step pulse state
/// @return true - step pulse done, false - step in process
/// ***************************************************************************
bool stepper_driver_t::is_step_done() const {
    return m_is_step_active == false;
}

/// ***************************************************************************
/// @brief  Process step pulse timing state machine
/// ***************************************************************************
void stepper_driver_t::process() {
    if (!m_is_step_active) {
        return;
    }

    //  1ms  _____
    // _____| 1ms
    if (m_step_pin_state) {
        if (HAL_GetTickUs() - m_start_t1_us >= m_t1_time_us) {
            HAL_GPIO_WritePin(m_step_port, m_step_pin, GPIO_PIN_RESET);
            m_start_t0_us = HAL_GetTickUs();
            m_step_pin_state = false;
        }
        return;
    } 
    else if (HAL_GetTickUs() - m_start_t0_us >= m_t0_time_us) {
        m_is_step_active = false;
    }
}
