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

    bool m_is_step_active       { };
    bool m_step_pin_state       { };
    uint32_t m_t1_ms            { };
    uint32_t m_t0_ms            { };

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
    /// ***************************************************************************
    void enable();

    /// ***************************************************************************
    /// @brief  Disable stepper driver output stage
    /// ***************************************************************************
    void disable();

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