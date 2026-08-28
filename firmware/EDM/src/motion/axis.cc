#include "axis.h"

/// ***************************************************************************
/// @brief  Create axis object and bind stepper driver
/// @param  driver: low-level stepper driver pointer
/// ***************************************************************************
axis_t::axis_t(stepper_driver_t* driver) {
    m_driver = driver;
}

/// ***************************************************************************
/// @brief  Initialize bound stepper driver and reset axis state
/// ***************************************************************************
void axis_t::init() {
    m_position = 0;
    m_target   = 0;
    m_driver->init();
    m_driver->set_power_state(true);
}

/// ***************************************************************************
/// @brief  Process underlying stepper driver state machine
/// ***************************************************************************
void axis_t::process() {
    m_driver->process();
}

/// ***************************************************************************
/// @brief  Set target axis position in steps
/// @param  target: target position in steps
/// ***************************************************************************
void axis_t::set_target(int32_t target) {
    m_target = target;
}

/// ***************************************************************************
/// @brief  Start one step toward target position if axis is ready
/// ***************************************************************************
void axis_t::step() {
    if (!is_ready()) {
        return;
    }

    if (m_position == m_target) {
        return;
    }

    if (m_target > m_position) {
        m_driver->set_direction(true);
        m_position++;
    } else {
        m_driver->set_direction(false);
        m_position--;
    }

    m_driver->step();
}

/// ***************************************************************************
/// @brief  Check whether axis is ready for the next step
/// @return true if no step pulse is currently active
/// ***************************************************************************
bool axis_t::is_ready() const {
    return m_driver->is_step_done();
}

/// ***************************************************************************
/// @brief  Get current axis position in steps
/// @return current position in steps
/// ***************************************************************************
int32_t axis_t::get_position() const {
    return m_position;
}
