#ifndef _STEPPER_DRIVER_H_
#define _STEPPER_DRIVER_H_
#include "stm32f4xx_hal.h"

class stepper_driver_t {
private:
    GPIO_TypeDef* m_en_port     { };
    uint16_t      m_en_pin      { };
    GPIO_TypeDef* m_dir_port    { };
    uint16_t      m_dir_pin     { };
    GPIO_TypeDef* m_step_port   { };
    uint16_t      m_step_pin    { };

    uint64_t m_t1_time_us       {50};
    uint64_t m_t0_time_us       {50};

    bool m_is_step_active       { };
    bool m_step_pin_state       { };
    uint64_t m_start_t1_us      { };
    uint64_t m_start_t0_us      { };

public:
    /// ***************************************************************************
    /// @brief  Create stepper driver object and bind GPIO pins/polarity settings
    /// @param  en_port: enable GPIO port
    /// @param  en_pin: enable GPIO pin
    /// @param  dir_port: direction GPIO port
    /// @param  dir_pin: direction GPIO pin
    /// @param  step_port: step GPIO port
    /// @param  step_pin: step GPIO pin
    /// ***************************************************************************
    stepper_driver_t(GPIO_TypeDef* en_port, uint16_t en_pin,
                     GPIO_TypeDef* dir_port, uint16_t dir_pin, 
                     GPIO_TypeDef* step_port, uint16_t step_pin);

    /// ***************************************************************************
    /// @brief  Initialize GPIO pins and set safe default output states
    /// ***************************************************************************
    void init();

    /// ***************************************************************************
    /// @brief  Enable stepper driver output stage
    /// @param  enabled: true - power on, false - power off
    /// ***************************************************************************
    void set_power_state(bool enabled);

    /// ***************************************************************************
    /// @brief  Set step pulse parameters
    /// @param  t1_us: HIGH state time, [us]
    /// @param  t0_us: LOW state time, [us]
    /// ***************************************************************************
    void set_timings(uint16_t t1_us, uint16_t t0_us);

    /// ***************************************************************************
    /// @brief  Set logical motion direction for the driver
    /// @param  is_forward: true for forward direction, false for reverse direction
    /// ***************************************************************************
    void set_direction(bool is_forward);

    /// ***************************************************************************
    /// @brief  Start physical step pulse on STEP pin
    /// ***************************************************************************
    void step();

    /// ***************************************************************************
    /// @brief  Check step pulse state
    /// @return true - step pulse done, false - step in process
    /// ***************************************************************************
    bool is_step_done() const;

    /// ***************************************************************************
    /// @brief  Process step pulse timing state machine
    /// ***************************************************************************
    void process();
};

#endif // _STEPPER_DRIVER_H_