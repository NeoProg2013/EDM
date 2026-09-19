# G-code Motion Plan

## Purpose
This document fixes the current design plan for adding G-code-driven motion control for axes X and Y.

The goal is to replace the current test approach based on continuous PWM stepping with explicit single-step execution controlled by EDM feedback.

This plan intentionally ignores the old manual movement commands. Manual mode is considered legacy/test behavior and is out of scope once the new parser/planner pipeline is activated.

## Scope of the First Version
The first implementation should support only:
- `G1`
- axes `X` and `Y`
- absolute coordinates only
- coordinates interpreted as step counts, not millimeters
- one active motion pipeline driven by EDM process readiness

The first version should not support:
- manual jog integration
- `G0`
- `G2` / `G3`
- `F`
- `G90` / `G91`
- comments or advanced G-code syntax
- Z axis
- acceleration planning
- velocity-based interpolation

## Current Firmware Context
From the current codebase:
- X axis is currently driven by continuous PWM on `TIM1`, channel 4
- X axis hardware mapping is:
  - `PA10` -> EN
  - `PA11` -> STEP
  - `PA12` -> DIR
- movement is currently started/stopped by enabling or disabling PWM
- EDM feedback is already measured and used to decide whether motion may continue

The new design changes the motion model from:
- continuous PWM motion with stop/start gating

to:
- geometry-driven step generation
- one explicit physical step at a time
- next step allowed only when the EDM process says it is safe to advance

## Core Design Principle
The new motion system is not time-driven CNC motion.

Instead, it is:
- trajectory defined by G-code geometry
- progress rate defined by EDM feedback

In practice, `G1` means:
- move from the current point to the target point along a straight line
- generate discrete X/Y step events for that line
- execute the next step only when the process is ready

## Architecture Overview
The proposed motion pipeline is:

1. G-code line input
2. G-code parser
3. G-code command queue
4. Active motion segment generator
5. Multi-axis step scheduler
6. Axis-level single-step execution
7. Current position update

There is only one motion mode in this plan:
- G-code-driven motion

Manual commands are not part of this architecture.

## Recommended Modules

### `drivers/stepper_driver.h` / `drivers/stepper_driver.cc`
Responsible for raw low-level stepper driver control:
- GPIO initialization for `EN`, `DIR`, and `STEP`
- driver enable / disable
- direction pin control
- start of one physical step pulse
- non-blocking pulse completion in `process()`

This layer does not know anything about axis coordinates or G-code targets.

### `drivers/axis.h` / `drivers/axis.cc`
Responsible for one logical axis on top of `stepper_driver_t`:
- current axis position in steps
- target axis position in steps
- choosing step direction from current position and target
- starting exactly one step toward target when commanded by the upper layer
- forwarding timing processing to the underlying stepper driver

This layer does not decide when to step continuously. That remains the responsibility of the upper motion layer so X and Y can stay synchronized.

### `gcode.h` / `gcode.cpp`
Responsible for parsing text lines into structured G-code commands.

### `motion.h` / `motion.cpp`
Responsible for:
- G-code command queue
- current machine position
- active line segment state
- generation of the next step event using a line algorithm such as Bresenham
- execution scheduling based on EDM readiness
- deciding which axis receives the next single-step command

## Data Structures

### G-code Command Type
```cpp
enum gcode_type_t {
    GCODE_NONE = 0,
    GCODE_G1,
};
```

### Parsed G-code Command
```cpp
struct gcode_cmd_t {
    gcode_type_t type;

    bool has_x;
    bool has_y;

    int32_t x;
    int32_t y;
};
```

Notes:
- `x` and `y` are stored in steps in the first version
- if an axis is omitted in a command, its target remains equal to the current position

Examples:
- `G1 X100` means move to X=100 while keeping current Y
- `G1 X100 Y40` means move to the point `(100, 40)`

### Current Motion Position
```cpp
struct motion_pos_t {
    int32_t x;
    int32_t y;
};
```

### Step Event
```cpp
struct step_event_t {
    bool step_x;
    bool dir_x;

    bool step_y;
    bool dir_y;
};
```

Meaning:
- `step_x` / `step_y` say whether a physical step pulse must be generated for the axis
- `dir_x` / `dir_y` say which direction must be set before pulsing

This structure is intentionally suitable for simultaneous X and Y stepping.

### Active Motion Segment
```cpp
struct motion_segment_t {
    bool active;

    int32_t x0;
    int32_t y0;
    int32_t x1;
    int32_t y1;

    int32_t x;
    int32_t y;

    int32_t dx;
    int32_t dy;
    int32_t sx;
    int32_t sy;
    int32_t err;
};
```

Meaning:
- `(x0, y0)` is the starting point of the current segment
- `(x1, y1)` is the target point
- `(x, y)` is the planner's current discrete point while generating steps
- `dx`, `dy`, `sx`, `sy`, `err` hold the internal state of the line-stepping algorithm

### G-code Queue
```cpp
static const uint16_t GCODE_QUEUE_SIZE = 8;

struct gcode_queue_t {
    gcode_cmd_t data[GCODE_QUEUE_SIZE];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
};
```

## Why Not a Full Step FIFO
The idea of storing a large prebuilt step buffer like `{ X, Y, Z bool }` is workable, but it is not the preferred design for this project.

Recommended approach:
- keep a queue of high-level G-code commands
- keep one active motion segment
- generate the next `step_event_t` on demand

Reasons:
- less RAM usage
- easier cancellation or stop handling
- no need to precompute long trajectories into large buffers
- naturally matches the EDM rule: advance only one step when the process is ready

## G1 Semantics for This Project
For this firmware, `G1` should be interpreted as:

- build a straight-line move from current position to target position
- decompose that line into discrete X/Y step events
- execute one next step when EDM feedback allows movement

This means that movement speed is not primarily defined by feedrate.
Instead, the effective speed is defined by the technological process and feedback conditions.

## Planner Strategy
The recommended line stepping strategy for `G1` is Bresenham-style interpolation.

Why Bresenham is appropriate here:
- integer-only logic
- no floating-point dependency
- well suited to discrete step generation
- easy to emit either X-only, Y-only, or simultaneous XY steps

The planner should not generate continuous PWM.
It should generate one `step_event_t` at a time.

## Command Execution Flow

### 1. Parse Input Line
A string such as:
```txt
G1 X10 Y6
```

is parsed into:
```cpp
gcode_cmd_t {
    .type = GCODE_G1,
    .has_x = true,
    .has_y = true,
    .x = 10,
    .y = 6,
}
```

### 2. Enqueue Command
The parsed command is pushed into `gcode_queue_t`.

### 3. Load Active Segment
If no segment is active and the queue is not empty:
- pop the next `gcode_cmd_t`
- resolve omitted axes against current position
- create a new `motion_segment_t`

### 4. Request Next Step
When the EDM process allows movement:
- generate the next `step_event_t` from the active segment

### 5. Execute the Physical Step
The executor:
- sets X/Y direction pins as needed
- raises X/Y step pins as required
- waits for minimum pulse width
- lowers X/Y step pins
- waits for minimum recovery time if needed

### 6. Update Position
After a successful pulse:
- update the current machine position
- advance the segment state

### 7. Finish Segment
When the target point is reached:
- mark the segment as inactive
- load the next command when available

## EDM Gating Logic
The new motion layer must not decide motion tempo by itself.

Instead, it should depend on a policy function such as:

```cpp
bool motion_can_step();
```

This function should answer one question:
- is it allowed to perform one more physical step right now?

The exact implementation can be based on the existing feedback logic already present in `main.cpp`.

### Required behavior
The gating policy should eventually include at least:
- EDM process readiness based on measured feedback
- a minimum interval between steps to prevent uncontrolled stepping in a tight CPU loop

Even if feedback says motion may continue, the firmware must still limit step issue rate.

## Axis Execution Model
The new design replaces timer-driven continuous stepping with GPIO-generated pulses.

Recommended low-level axis abstraction:

```cpp
struct axis_hw_t {
    GPIO_TypeDef* en_port;
    uint16_t en_pin;

    GPIO_TypeDef* dir_port;
    uint16_t dir_pin;

    GPIO_TypeDef* step_port;
    uint16_t step_pin;
};
```

Expected operations:
- axis initialization
- axis enable
- axis disable
- set direction
- generate one step pulse

## Simultaneous XY Step Behavior
The current agreed architecture does not require `axis_t` to fire both axes by itself.

Instead:
- the upper motion layer decides when X should step
- the upper motion layer decides when Y should step
- each `axis_t` performs one non-blocking step toward its own target

This separation is intentional because diagonal motion and synchronized XY stepping must be coordinated above the single-axis layer.

## Y Axis Status
The architecture must be designed as fully two-axis from the beginning.

However:
- Y axis hardware mapping does not exist yet in the current firmware
- concrete Y GPIO pin assignment can be added later

Therefore:
- parser, planner, position tracking, and step event generation should already support both X and Y
- hardware binding for Y can remain a follow-up task

## First-Version Coordinate Policy
For the first implementation, `G1` coordinates should be interpreted as step counts.

Example:
- `G1 X100 Y50` means target position `(100 steps, 50 steps)`

This is recommended for the first stage because:
- it avoids floating-point math
- it avoids steps-per-mm calibration concerns
- it makes validation of the planner much simpler

Later, the system can evolve to millimeter-based G-code and convert to steps internally.

## Main Loop Integration Plan
The existing continuous PWM motion logic should eventually be replaced conceptually by a step-driven pipeline.

Current model:
- start X PWM
- stop X PWM
- gate continuous movement with feedback

Target model:
- no continuous axis PWM for motion
- call `motion_process()` in the main loop
- let feedback logic answer whether one next step may be executed
- let the motion layer decide which axis receives that next step

The most important architectural change is:
- from "axis is moving / axis is stopped"
- to "one next step is allowed / not allowed"

## Current Low-Level Motion API
The currently agreed low-level layers are:

### `stepper_driver_t`
```cpp
class stepper_driver_t {
public:
    stepper_driver_t(GPIO_TypeDef* en_port, uint16_t en_pin,
                     GPIO_TypeDef* dir_port, uint16_t dir_pin,
                     GPIO_TypeDef* step_port, uint16_t step_pin);

    void init();
    void enable();
    void disable();
    void set_direction(bool is_forward);
    void step();
    bool is_step_done() const;
    void process();
};
```

Behavior:
- `step()` starts one physical step pulse if the driver is idle
- `process()` completes the pulse non-blockingly
- `is_step_done()` reports whether the driver is ready for another step

### `axis_t`
```cpp
class axis_t {
public:
    axis_t(stepper_driver_t* driver);

    void init();
    void home();
    void process();
    void set_target(int32_t target);
    void step();
    bool is_ready() const;
    int32_t get_position() const;
};
```

Behavior:
- `set_target()` stores target position in steps
- `step()` makes exactly one step toward target if the axis is ready
- `step()` does nothing if the axis is busy or already at target
- `step()` updates logical position before starting the physical pulse
- `process()` forwards low-level pulse processing to the underlying driver
- `is_ready()` tells the upper motion layer whether the axis may receive the next step command

## Suggested Public Motion API
One possible API shape for the motion module:

```cpp
void motion_init();
bool motion_enqueue_gcode(const char* line);
void motion_process();
bool motion_is_idle();
```

Potential parser API:

```cpp
bool gcode_parse_line(const char* line, gcode_cmd_t* out);
```

## Constraints and Assumptions
- only one G-code motion pipeline exists
- only one segment is active at a time
- no blending between adjacent segments in the first version
- no acceleration ramping in the first version
- no feedrate scheduling in the first version
- motion progress is dominated by EDM process readiness, not by a classic planner clock
- `axis_t` does not own multi-axis timing decisions
- the upper motion layer is responsible for synchronized diagonal stepping

## Future Extensions
After the first version works, the following can be added later:
- real Y-axis pin mapping and hardware integration
- millimeter input with steps-per-mm conversion
- `G90` / `G91`
- `F`
- queue flush / pause / stop behavior
- status reporting for planner state
- protocol for receiving G-code externally
- optional support for `G0`

## Summary
The agreed first design is:

- no manual mode in the new architecture
- only `G1`
- X and Y only
- coordinates initially treated as steps
- `stepper_driver_t` as non-blocking raw GPIO step driver
- `axis_t` as a minimal logical axis wrapper with `position` and `target`
- a queue of parsed G-code commands
- one active motion segment
- Bresenham-style next-step generation
- no large precomputed step FIFO as the primary mechanism
- one explicit physical step at a time
- every step gated by EDM feedback readiness
- the upper motion layer decides when each axis receives its next step

This design is the baseline for future implementation work.