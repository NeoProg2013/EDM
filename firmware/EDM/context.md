# EDM Firmware Context

## Overview
This repository contains firmware for an EDM controller based on STM32F407VE, built with PlatformIO. The firmware is responsible for:
- EDM spark pulse generation
- X-axis step control
- Wire tension control
- Telemetry and command exchange with another controller board

Target environment:
- PlatformIO env: `stm32_f407_jlink`
- MCU/board: `black_f407ve`
- Platform: `ststm32`
- Framework: `stm32cube`
- Upload/debug tool: J-Link

## Repository Structure
- `platformio.ini` — PlatformIO build, upload, and debug configuration
- `src/main.cpp` — application entry point, clock init, peripheral init, main control loop
- `src/spark.h`, `src/spark.cpp` — spark PWM generation
- `src/tension.h`, `src/tension.cpp` — wire tension measurement and control
- `src/telemetry.h`, `src/telemetry.cc` — UART telemetry protocol and command reception
- `src/isr.cc` — interrupt handlers
- `lib/HX711/HX711.h`, `lib/HX711/HX711.cpp` — local HX711 driver
- `test/` — currently no real unit tests, only the default PlatformIO README

## Build Environment
From `platformio.ini`:
- `platform = ststm32`
- `board = black_f407ve`
- `framework = stm32cube`
- `upload_protocol = jlink`
- `debug_tool = jlink`
- `monitor_speed = 115200`
- build flag: `-D HSE_VALUE=8000000U`

Toolchain / language:
- arm-none-eabi GCC
- C standard: `gnu17`
- C++ standard: `gnu++17`

## Clock Configuration
Defined in `src/main.cpp::system_clock_init()`:
- HSE: 8 MHz
- PLLM = 8
- PLLN = 336
- PLLP = 2
- PLLQ = 7
- SYSCLK = 168 MHz
- AHB = 168 MHz
- APB1 = 42 MHz
- APB2 = 84 MHz

Enabled clocks include:
- TIM1, TIM2, TIM3, TIM4, TIM8
- USART2
- DMA1
- GPIOA, GPIOB, GPIOC, GPIOD, GPIOE
- SYSCFG

## Main Runtime Flow
Entry point is custom `int main()`, not Arduino `setup()/loop()`.

Startup sequence:
1. `HAL_Init()`
2. `system_clock_init()`
3. `telemetry_init()`
4. `tension_init()`
5. `spark_pwm_init()`
6. X-axis GPIO/PWM init
7. feedback capture init via TIM8

Main loop responsibilities:
- fill telemetry TX message
- send telemetry periodically
- receive control state from peer board
- perform spark / no-spark feedback logic
- run tension control loop
- globally start/stop subsystems based on `g_is_enabled`

## Hardware / Pin Mapping

### USART2
- PA2 -> TX
- PA3 -> RX

### Spark Module
- PA1 -> spark PWM output (`TIM2`, channel 2)
- PA0 -> MOSFET gate GND control

### X Axis
- PA10 -> EN
- PA11 -> STEP PWM (`TIM1`, channel 4)
- PA12 -> DIR

### Feedback Input
- PC6 -> feedback capture via `TIM8`

### Tension / Feeder
- PD11 -> feeder EN
- PD12 -> feeder STEP PWM (`TIM4`, channel 1)

### Tension / Brake
- PA8 -> brake EN
- PC9 -> brake STEP PWM (`TIM3`, channel 4)

### HX711
- PB11 -> VCC
- PB12 -> GND
- PB13 -> DOUT
- PB14 -> SCK

## Modules

### Spark Module
Files:
- `src/spark.h`
- `src/spark.cpp`

Responsibilities:
- generate EDM spark pulses using PWM
- maintain pulse timing parameters:
  - `t1` = pulse ON time, in microseconds
  - `t0` = pulse OFF time, in microseconds
- compute current spark frequency

Implementation details:
- Timer: `TIM2`
- Channel: `TIM_CHANNEL_2`
- Timer resolution: 1 us

Defaults:
- `t1 = 1 us`
- `t0 = 300 us`

Key functions:
- `spark_pwm_init()`
- `spark_pwm_start()`
- `spark_pwm_stop()`
- `spark_pwm_update()`
- `spark_set_t1_us()`
- `spark_set_t0_us()`

### Tension Module
Files:
- `src/tension.h`
- `src/tension.cpp`

Responsibilities:
- read load cell via HX711
- control feeder and brake step signals
- regulate wire tension

Current control approach:
- average 20 HX711 samples
- compute deviation from target bins
- apply a simple proportional correction to brake period
- feeder period is currently mostly static

Calibration-related values:
- `TENSION_SCALE = 1700` bins per gram
- HX711 offset initialized to `175000`

Key functions:
- `tension_init()`
- `tension_start()`
- `tension_stop()`
- `tension_process()`

### Telemetry Module
Files:
- `src/telemetry.h`
- `src/telemetry.cc`

Responsibilities:
- send telemetry to another board
- receive control/status frames from another board
- maintain frame synchronization and checksum validation

UART / DMA details:
- USART: `USART2`
- baud rate: `9600`
- TX via DMA: `DMA1_Stream6`
- RX via interrupt, one byte at a time

Frame markers:
- start marker: `0xAA`
- stop marker: `0xDD`

TX payload (`tx_msg_t`):
- `edm_status`
- `step_state`
- `freq_hz`
- `arc_counter`
- `tension_g`
- `feeder_us`
- `brake_us`
- `t1`
- `t0`
- `checksum`

RX payload (`rx_msg_t`):
- `cmd`
- `edm_status`
- `t0`
- `t1`
- `checksum`

Checksum:
- simple additive checksum over payload bytes excluding the checksum field itself

Transmission behavior:
- telemetry is sent roughly every 100 ms when DMA is free

Current limitation:
- received `t0` / `t1` values are not yet applied in `main.cpp`; there is still a TODO there

### Telemetry Frame Layout

#### TX Frame (`g_tx_buffer`, 20 bytes total)
Byte layout:

| Byte index | Meaning |
|---|---|
| 0 | start marker `0xAA` |
| 1 | `edm_status` |
| 2 | `step_state` |
| 3 | `freq_hz` MSB |
| 4 | `freq_hz` LSB |
| 5 | `arc_counter` MSB |
| 6 | `arc_counter` LSB |
| 7 | `tension_g` MSB |
| 8 | `tension_g` LSB |
| 9 | `feeder_us` MSB |
| 10 | `feeder_us` LSB |
| 11 | `brake_us` MSB |
| 12 | `brake_us` LSB |
| 13 | `t1` MSB |
| 14 | `t1` LSB |
| 15 | `t0` MSB |
| 16 | `t0` LSB |
| 17 | checksum MSB |
| 18 | checksum LSB |
| 19 | stop marker `0xDD` |

Notes:
- multibyte values are serialized in big-endian order: MSB first, then LSB
- checksum is computed over bytes `1..16`
- checksum does not include the start or stop marker

#### RX Frame (`g_rx_buffer`, 11 bytes total)
Byte layout:

| Byte index | Meaning |
|---|---|
| 0 | start marker `0xAA` |
| 1 | `cmd` |
| 2 | `edm_status` |
| 3 | `t0` MSB |
| 4 | `t0` LSB |
| 5 | `t1` MSB |
| 6 | `t1` LSB |
| 7 | checksum MSB |
| 8 | checksum LSB |
| 9 | stop marker `0xDD` |
| 10 | currently part of `sizeof(g_rx_buffer)`-based frame buffer accounting; verify carefully before changing frame struct/layout |

Implementation note:
- RX parsing code validates markers using the actual buffer size and reconstructs checksum from `g_rx_buffer[7]` and `g_rx_buffer[8]`
- before any protocol refactor, re-check the relationship between `sizeof(rx_msg_t)`, `g_rx_buffer`, and the current parser state machine in `src/telemetry.cc`

### HX711 Local Library
Files:
- `lib/HX711/HX711.h`
- `lib/HX711/HX711.cpp`

Notes:
- GPIO bit-banged implementation
- interrupts are disabled during `read()` to preserve HX711 timing
- supports gain selection and tare offset

## X-Axis Control
Implemented directly in `src/main.cpp`.

Timer:
- `TIM1`, channel 4
- 1 us resolution
- PWM is used as step generator

Behavior:
- axis can be enabled or disabled globally
- `start_axis_x()` / `stop_axis_x()` control motion pulses
- speed is controlled by `g_axis_x_period_us`
- current allowed range: 1000..30000 us period

## Spark Feedback / Short-Circuit Logic
Feedback input is measured using `TIM8` input capture on PC6.

Purpose:
- detect missing pulses or abnormal spark condition
- stop X-axis motion when cutting is not ready to advance
- restart X-axis after a stable delay

Current heuristic in `main.cpp`:
- if no pulse is seen for too long or pulse width is below threshold, stop X
- after 100 ms without fault, restart X
- arc / no-arc behavior is inferred from capture timing, not from a dedicated higher-level state machine

## Interrupts
Defined in `src/isr.cc`:
- `HardFault_Handler`
- `SysTick_Handler`
- `DMA1_Stream6_IRQHandler`
- `USART2_IRQHandler`

## Development Notes
- The project uses STM32 HAL directly.
- The code does not follow a CubeMX-generated file layout.
- Hardware init and control logic are mostly manual and colocated with application logic.
- There are currently no real unit tests.
- `.vscode/c_cpp_properties.json` is auto-generated and includes Arduino STM32 package paths; treat it as IDE metadata, not the source of truth for the firmware setup.
- The real source of truth for build configuration is `platformio.ini`.

## Useful Commands
Typical PlatformIO commands:
- build: `pio run`
- upload: `pio run -t upload`
- clean: `pio run -t clean`

Debug:
- use PlatformIO debug configuration in VS Code
- J-Link is configured as both upload and debug tool

## Known Gaps / TODO
- RX parameters `t0` and `t1` are parsed but not yet applied to the spark module in `main.cpp`
- There is no top-level project README yet
- Telemetry protocol is implemented in code, but not yet documented as a byte-level external spec
- Unit tests are not implemented

## Glossary
- **EDM** — Electrical Discharge Machining.
- **Spark** — the generated EDM pulse train applied to the machining circuit.
- **T1** — pulse ON duration in microseconds.
- **T0** — pulse OFF duration in microseconds.
- **Feeder** — the motor/driver path feeding the wire forward.
- **Brake** — the motor/driver path providing tension or drag to stabilize the wire.
- **HX711** — ADC front-end commonly used with load cells; here it measures wire tension indirectly through a sensor.
- **Tension bins** — raw or averaged HX711 counts before conversion to grams.
- **Arc counter** — software counter incremented when the control logic detects a no-pulse / arc-loss condition significant enough to stop X motion.
- **Feedback** — captured signal on PC6 measured by TIM8 and used to infer spark state.
- **Peer board** — the second controller/display board communicating over USART2.
- **PWM step generation** — use of a timer PWM output as a pulse source for a step/dir motor driver.
- **1 us resolution** — timers are configured so one timer tick corresponds to one microsecond, simplifying direct timing math.
- **Sync lost** — telemetry receiver state in which byte alignment to frame boundaries is no longer trusted and the parser waits for a valid marker sequence.
- **Checksum** — additive sum of payload bytes used for basic frame integrity validation.

## Guidance for Future Changes
- Prefer existing STM32 HAL patterns already used in the repository.
- Keep timer and pin mapping comments close to initialization code.
- When changing telemetry protocol, update all of:
  - `src/telemetry.h`
  - TX encoding in `telemetry_process()`
  - RX parsing in the UART callback
  - consuming logic in `src/main.cpp`
- Be careful with timing assumptions; many modules rely on 1 us timer resolution.
- Keep interrupt-related additions consistent with `src/isr.cc`.