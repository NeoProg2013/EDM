# Project Context

## Overview
- Project path: `C:\Users\NeoProg\Desktop\EDM\firmware\UI`
- Target MCU: `STM32F030F4P6`
- Build system: `PlatformIO`
- Framework: `stm32cube` (STM32 HAL/CMSIS via PlatformIO)
- Debug/upload: `J-Link`

## Build Configuration
Source: `C:\Users\NeoProg\Desktop\EDM\firmware\UI\platformio.ini`

Key settings:
- Environment: `stm32_f030f4_jlink`
- Platform: `ststm32`
- Board: `demo_f030f4`
- Framework: `stm32cube`
- Serial monitor: `115200`
- Upload protocol: `jlink`
- Debug tool: `jlink`
- Extra library dependency: `nkawu/TFT 22 ILI9225@^1.4.5`
- `libdeps_dir = lib` so downloaded libraries are stored inside the project `lib` directory.

## High-Level Structure
Observed top-level areas:
- `C:\Users\NeoProg\Desktop\EDM\firmware\UI\src` — application source files
- `C:\Users\NeoProg\Desktop\EDM\firmware\UI\lib` — local and downloaded libraries
- `C:\Users\NeoProg\Desktop\EDM\firmware\UI\.pio` — PlatformIO build artifacts
- `C:\Users\NeoProg\Desktop\EDM\firmware\UI\.vscode` — IDE configuration

## Application Sources
### `src/main.cpp`
Role:
- Main entry point
- System clock setup
- SPI1 initialization
- Infinite loop with delay

Behavior summary:
- Calls `HAL_Init()`
- Configures system clock to `48 MHz`
- Initializes `SPI1` as master
- Loops forever with `HAL_Delay(1000)`

Clock configuration currently implemented:
- HSI enabled
- PLL source = HSI/2
- PLL multiplier = x12
- SYSCLK = PLLCLK = `48 MHz`
- AHB prescaler = `/1`
- APB1 prescaler = `/1`
- Flash latency = `1`

SPI configuration in `hspi1_init()`:
- Instance: `SPI1`
- Master mode
- 2-line full duplex
- 8-bit data
- CPOL low, CPHA 1st edge
- Baud prescaler `/4`
- With 48 MHz PCLK, intended SPI clock is `12 MHz`

### `src/core.h`
Role:
- Minimal common project header

Content:
- Includes `stm32f0xx_hal.h`
- Defines `SYSTEM_CLOCK_FREQUENCY` as `48000000`

Note:
- This macro is a project-local constant and should stay aligned with actual RCC configuration.

### `src/display.cpp`
Role:
- Appears to be an unfinished or disabled UI layer for TFT rendering

Current state:
- Entire file content is commented out
- References a `TFT_22_ILI9225` C++ driver API
- Contains planned UI layout and periodic field updates
- Suggests the display was intended to show EDM-related runtime parameters

Referenced but currently inactive concepts:
- spark enable state
- frequency
- short-circuit counter
- X/Y axis period
- tension in raw units and grams
- feeder/brake timing

### `src/display.h`
Role:
- Exists, but content was not inspected during this pass
- Likely intended interface for the disabled display module

## Libraries
### Local library: `lib/ILI9225`
Files:
- `C:\Users\NeoProg\Desktop\EDM\firmware\UI\lib\ILI9225\ILI9225.c`
- `C:\Users\NeoProg\Desktop\EDM\firmware\UI\lib\ILI9225\ILI9225.h`

Characteristics:
- C driver for ILI9225-compatible TFT LCDs
- Originally written for PIC, partially adapted to STM32 HAL
- Uses `stm32f0xx_hal.h`
- Hardcoded display geometry: `WIDTH=220`, `HEIGHT=176`, `LANDSCAPE=1`
- Hardcoded control pins:
  - `CSX` -> `GPIOC PIN 1`
  - `RESX` -> `GPIOC PIN 0`
  - `CMD` -> `GPIOB PIN 0`

Important observation:
- Header declares a global `SPI_HandleTypeDef hspi2;`
- Main application initializes `SPI1`, not `SPI2`
- This suggests the local C driver is not integrated with the current `main.cpp` as-is, or it is stale/incomplete.

Capabilities in the driver:
- Low-level command/data writes
- LCD init sequence
- Drawing primitives
- Character and string rendering
- Bitmap drawing

### Downloaded library: `lib/stm32_f030f4_jlink/TFT 22 ILI9225`
Characteristics:
- PlatformIO-downloaded dependency from `nkawu/TFT 22 ILI9225`
- Contains:
  - `src/TFT_22_ILI9225.cpp`
  - `src/TFT_22_ILI9225.h`
  - `src/DefaultFonts.c`
  - many font headers under `fonts/`

Observation:
- The commented-out `src/display.cpp` matches this library's API style.
- The project currently contains both:
  1. a local C driver in `lib/ILI9225`
  2. an external C++ TFT library in `lib/stm32_f030f4_jlink/TFT 22 ILI9225`
- This may indicate a migration from one display driver approach to another.

## Build Artifacts / Evidence of Additional Sources
Observed compiled objects in `.pio/build/...` include:
- `src/display.o`
- `src/drivers/systimer.o`

Important note:
- The source files for `src/drivers/systimer.*` were not found by direct read at the expected path during this pass.
- Since `systimer.o` exists in build artifacts from previous builds, those sources may:
  - exist in a path not yet resolved,
  - have been removed while artifacts remained,
  - or be generated/symlinked through a mechanism outside this scan.

## Runtime/Architecture Notes
### Initialization flow
Current startup flow in `main()`:
1. `HAL_Init()`
2. `system_clock_init()`
3. `hspi1_init()`
4. infinite loop with `HAL_Delay(1000)`

### Clocking summary
The project is configured for a 48 MHz system clock using internal HSI and PLL.
This is appropriate for STM32F030F4P6 when using:
- HSI = 8 MHz
- PLL input = HSI/2 = 4 MHz
- PLL multiplier = x12
- SYSCLK = 48 MHz
- Flash latency = 1 wait state

### GPIO clock enables
`system_clock_init()` enables clocks for:
- SYSCFG
- GPIOA
- GPIOB
- GPIOC
- GPIOD

Note:
- On small STM32F030 packages not all GPIO ports may be physically present, but enabling their RCC clocks is usually harmless if supported by the device line/framework.

## Inferred Project Intent
Based on file names and commented UI strings, this appears to be a UI/diagnostic firmware component for an EDM-related system. Likely responsibilities:
- configure core MCU timing
- communicate with a TFT display over SPI
- display machine/runtime parameters

## Risks / Inconsistencies Found
1. **Two different display driver approaches coexist**
   - local C driver: `lib/ILI9225`
   - external C++ library: `TFT 22 ILI9225`
   - likely technical debt or unfinished migration

2. **Display code is currently disabled**
   - `src/display.cpp` is entirely commented out
   - no active display initialization in `main.cpp`

3. **SPI instance mismatch risk**
   - app initializes `SPI1`
   - local ILI9225 header exposes `hspi2`
   - integration would fail unless unified

4. **Project may contain stale build artifacts**
   - `.pio/build/.../src/drivers/systimer.o` exists
   - corresponding source was not located in this scan

5. **Hardcoded frequency macro**
   - `SYSTEM_CLOCK_FREQUENCY` duplicates actual RCC setup
   - can drift if clock configuration changes later

## Useful Files for Further Investigation
- `C:\Users\NeoProg\Desktop\EDM\firmware\UI\platformio.ini`
- `C:\Users\NeoProg\Desktop\EDM\firmware\UI\src\main.cpp`
- `C:\Users\NeoProg\Desktop\EDM\firmware\UI\src\core.h`
- `C:\Users\NeoProg\Desktop\EDM\firmware\UI\src\display.cpp`
- `C:\Users\NeoProg\Desktop\EDM\firmware\UI\src\display.h`
- `C:\Users\NeoProg\Desktop\EDM\firmware\UI\lib\ILI9225\ILI9225.c`
- `C:\Users\NeoProg\Desktop\EDM\firmware\UI\lib\ILI9225\ILI9225.h`
- `C:\Users\NeoProg\Desktop\EDM\firmware\UI\lib\stm32_f030f4_jlink\TFT 22 ILI9225\src\TFT_22_ILI9225.cpp`
- `C:\Users\NeoProg\Desktop\EDM\firmware\UI\lib\stm32_f030f4_jlink\TFT 22 ILI9225\src\TFT_22_ILI9225.h`

## Recommended Next Steps
1. Resolve which display driver is the intended one.
2. Find or restore the actual `systimer` sources if they are expected to be part of the project.
3. Verify SPI pin mapping and whether `SPI1` or `SPI2` should be used.
4. If display support is needed, re-enable and adapt `src/display.cpp` to the chosen driver.
5. Consider documenting pin assignments and module responsibilities in a `README.md` or extending this context file.
