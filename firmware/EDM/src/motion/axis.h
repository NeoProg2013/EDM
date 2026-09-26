#ifndef _AXIS_H_
#define _AXIS_H_
#include "core.h"
#include "stepper_driver.h"

class axis_t {
private:
    stepper_driver_t* m_driver { };
    int32_t m_position         { };
    int32_t m_target           { };

public:
    /// ***************************************************************************
    /// @brief  Create axis object and bind stepper driver
    /// @param  driver: low-level stepper driver pointer
    /// ***************************************************************************
    axis_t(stepper_driver_t* driver);

    /// ***************************************************************************
    /// @brief  Initialize bound stepper driver and reset axis state
    /// ***************************************************************************
    void init();

    /// ***************************************************************************
    /// @brief  Process underlying stepper driver state machine
    /// ***************************************************************************
    void process();

    /// ***************************************************************************
    /// @brief  Set target axis position in steps
    /// @param  target: target position in steps
    /// ***************************************************************************
    void set_target(int32_t target);

    /// ***************************************************************************
    /// @brief  Start one step toward target position if axis is ready
    /// ***************************************************************************
    void step();

    /// ***************************************************************************
    /// @brief  Check whether axis is ready for the next step
    /// @return true if no step pulse is currently active
    /// ***************************************************************************
    bool is_ready() const { return m_driver->is_step_done(); }

    /// ***************************************************************************
    /// @brief  Check whether the axis has reached its target position
    /// @return true if current position matches the target position
    /// ***************************************************************************
    bool is_target_reached() const { return m_position == m_target; }

    /// ***************************************************************************
    /// @brief  Get current axis position in steps
    /// @return current position in steps
    /// ***************************************************************************
    int32_t get_position() const { return m_position; }

    /// ***************************************************************************
    /// @brief  Get target axis position in steps
    /// @return target position in steps
    /// ***************************************************************************
    int32_t get_target_position() const { return m_target; }
};

#endif // _AXIS_H_