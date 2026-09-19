#ifndef _MOTION_CONTROLLER_H_
#define _MOTION_CONTROLLER_H_
#include "core.h"
#include "axis.h"


class motion_controller_t {
    struct motion_t {
        int32_t target_x            { };
        int32_t target_y            { };
    };

    static constexpr uint32_t QUEUE_SIZE = 64;

private:
    stepper_driver_t m_x_driver;
    stepper_driver_t m_y_driver;
    axis_t m_x_axis;
    axis_t m_y_axis;

    motion_t m_motion               { };
    motion_t m_queue[QUEUE_SIZE]    { };

    bool     m_is_locked            { };
    bool     m_is_busy              { };
    int32_t  m_start_x              { };
    int32_t  m_start_y              { };
    uint32_t m_queue_head           { };
    uint32_t m_queue_tail           { };
    uint32_t m_queue_size           { };

    // Bresenham
    int32_t m_dx                    { };
    int32_t m_dy                    { };
    int32_t m_err                   { };

public:
    /// ***************************************************************************
    /// @brief  Create motion controller object with internal axis and pin mapping
    /// ***************************************************************************
    motion_controller_t();

    /// ***************************************************************************
    /// @brief  Initialize motion controller and all internal axes/drivers
    /// ***************************************************************************
    void init();

    /// ***************************************************************************
    /// @brief  Process active motion and underlying axis drivers
    /// ***************************************************************************
    void process();

    /// ***************************************************************************
    /// @brief  Queue new linear motion to target point
    /// @param  target_x: target X coordinate in steps
    /// @param  target_y: target Y coordinate in steps
    /// ***************************************************************************
    void move_to(int32_t target_x, int32_t target_y);

    /// ***************************************************************************
    /// @brief  Lock motion progression and prevent issuing new steps
    /// ***************************************************************************
    void lock();

    /// ***************************************************************************
    /// @brief  Unlock motion progression and allow issuing new steps
    /// ***************************************************************************
    void unlock();

    /// ***************************************************************************
    /// @brief  Check lock state
    /// @return true if motion progression is locked
    /// ***************************************************************************
    bool is_locked() const;

    /// ***************************************************************************
    /// @brief  Check whether controller is executing active motion
    /// @return true if active motion is in progress
    /// ***************************************************************************
    bool is_busy() const;

private:
    /// ***************************************************************************
    /// @brief  Add one motion command to ring buffer queue
    /// @param  motion: motion command to enqueue
    /// @return true if queued successfully
    /// ***************************************************************************
    bool enqueue_motion(const motion_t& motion);

    /// ***************************************************************************
    /// @brief  Remove one motion command from ring buffer queue
    /// @param  motion: output pointer for dequeued motion
    /// @return true if one motion was dequeued
    /// ***************************************************************************
    bool dequeue_motion(motion_t* motion);

    /// ***************************************************************************
    /// @brief  Start one queued motion as active motion
    /// @param  motion: queued motion command
    /// ***************************************************************************
    void start_motion(const motion_t& motion);
};

#endif // _MOTION_CONTROLLER_H_