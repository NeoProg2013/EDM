#include "core.h"
#include "motion_controller.h"

static GPIO_TypeDef* const X_PORT    = GPIOA;
static constexpr uint16_t X_EN_PIN   = GPIO_PIN_8;
static constexpr uint16_t X_STEP_PIN = GPIO_PIN_11;
static constexpr uint16_t X_DIR_PIN  = GPIO_PIN_12;

static GPIO_TypeDef* const Y_PORT    = GPIOA;
static constexpr uint16_t Y_EN_PIN   = GPIO_PIN_8;
static constexpr uint16_t Y_STEP_PIN = GPIO_PIN_9;
static constexpr uint16_t Y_DIR_PIN  = GPIO_PIN_10;



/// ***************************************************************************
/// @brief  Create motion controller object with internal axis and pin mapping
/// ***************************************************************************
motion_controller_t::motion_controller_t() :
    m_x_driver(GPIOA, GPIO_PIN_8, GPIOA, X_DIR_PIN, GPIOA, X_STEP_PIN),
    m_y_driver(GPIOA, GPIO_PIN_8, GPIOA, Y_DIR_PIN, GPIOA, Y_STEP_PIN),
    m_x_axis(&m_x_driver),
    m_y_axis(&m_y_driver) {
        
}

/// ***************************************************************************
/// @brief  Initialize motion controller and all internal axes/drivers
/// ***************************************************************************
void motion_controller_t::init() {
    m_x_axis.init();
    m_y_axis.init();
}

/// ***************************************************************************
/// @brief  Process active motion and underlying axis drivers
/// ***************************************************************************
void motion_controller_t::process() {
    m_x_axis.process();
    m_y_axis.process();

    // We ready do next motion?
    if (!m_is_busy) {
        motion_t motion;
        if (!dequeue_motion(&motion)) {
            return;
        }
        start_new_motion(motion);
    }

    // Current motion done?
    if (m_x_axis.is_target_reached() && m_y_axis.is_target_reached()) {
        m_is_busy = false;
        return;
    }

    // Motion steps locked?
    if (m_is_locked) {
        return;
    }

    // We ready do next motion step?
    if (!m_x_axis.is_ready() || !m_y_axis.is_ready()) {
        return;
    }

    // Bresenham's line algorithm
    if (m_dx >= m_dy) {
        // Main axis = X
        // Every controller tick we try to advance X once.
        // Y is advanced only when accumulated error says so.
        if (!m_x_axis.is_target_reached()) {
            m_x_axis.step();
            m_err += m_dy;
        }
        if (m_err >= m_dx && !m_y_axis.is_target_reached()) {
            m_err -= m_dx;
            m_y_axis.step();
        }
    } else {
        // Main axis = Y
        // Every controller tick we try to advance Y once.
        // X is advanced only when accumulated error says so.
        if (!m_y_axis.is_target_reached()) {
            m_y_axis.step();
            m_err += m_dx;
        }
        if (m_err >= m_dy && !m_x_axis.is_target_reached()) {
            m_err -= m_dy;
            m_x_axis.step();
        }
    }
}

/// ***************************************************************************
/// @brief  Queue new linear motion to target point
/// @param  x: target X coordinate in steps
/// @param  y: target Y coordinate in steps
/// ***************************************************************************
void motion_controller_t::move_to(int32_t x, int32_t y) {
    enqueue_motion(motion_t{ .type = motion_t::type_t::ABSOLUTE, .x = x, .y = y });
}

/// ***************************************************************************
/// @brief  Queue new linear motion offset
/// @param  x: target X coordinate in steps
/// @param  y: target Y coordinate in steps
/// ***************************************************************************
void motion_controller_t::move_by(int32_t x, int32_t y) {
    enqueue_motion(motion_t{ .type = motion_t::type_t::RELATIVE, .x = x, .y = y });
}

/// ***************************************************************************
/// @brief  Lock motion progression and prevent issuing new steps
/// ***************************************************************************
void motion_controller_t::lock() {
    m_is_locked = true;
}

/// ***************************************************************************
/// @brief  Unlock motion progression and allow issuing new steps
/// ***************************************************************************
void motion_controller_t::unlock() {
    m_is_locked = false;
}

/// ***************************************************************************
/// @brief  Check lock state
/// @return true if motion progression is locked
/// ***************************************************************************
bool motion_controller_t::is_locked() const {
    return m_is_locked;
}

/// ***************************************************************************
/// @brief  Check whether controller is executing active motion
/// @return true if active motion is in progress
/// ***************************************************************************
bool motion_controller_t::is_busy() const {
    return m_is_busy;
}


//
// private API
/// ***************************************************************************
/// @brief  Add one motion command to ring buffer queue
/// @param  motion: motion command to enqueue
/// @return true if queued successfully
/// ***************************************************************************
bool motion_controller_t::enqueue_motion(const motion_t& motion) {
    if (m_queue_size >= QUEUE_SIZE) {
        return false;
    }

    m_queue[m_queue_tail] = motion;
    m_queue_tail = (m_queue_tail + 1) % QUEUE_SIZE;
    m_queue_size++;
    return true;
}

/// ***************************************************************************
/// @brief  Remove one motion command from ring buffer queue
/// @param  motion: output pointer for dequeued motion
/// @return true if one motion was dequeued
/// ***************************************************************************
bool motion_controller_t::dequeue_motion(motion_t* motion) {
    if (motion == nullptr) {
        return false;
    }

    if (m_queue_size == 0) {
        return false;
    }

    *motion = m_queue[m_queue_head];
    m_queue_head = (m_queue_head + 1) % QUEUE_SIZE;
    m_queue_size--;
    return true;
}

/// ***************************************************************************
/// @brief  Start one queued motion as active motion
/// @param  motion: queued motion command
/// ***************************************************************************
void motion_controller_t::start_new_motion(const motion_t& motion) {
    // Set target point
    if (motion.ABSOLUTE) {
        m_x_axis.set_target(motion.x);
        m_y_axis.set_target(motion.y);
    } else { // RELATIVE
        m_x_axis.set_target(m_x_axis.get_position() + motion.x);
        m_y_axis.set_target(m_y_axis.get_position() + motion.y);
    }
    m_current_motion = motion;

    // Motion already done?
    if (m_x_axis.is_target_reached() && m_x_axis.is_target_reached()) {
        return;
    }

    // Bresenham's line algorithm
    int32_t x0 = m_x_axis.get_position();
    int32_t y0 = m_y_axis.get_position();
    m_dx = abs(motion.x - x0);
    m_dy = abs(motion.y - y0);
    m_err = 0;

    m_is_busy = true;
}