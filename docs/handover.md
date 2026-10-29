# ESP32 OLED Lab

A bare-metal-style embedded development playground using **ESP-IDF, C, and an ESP32**, starting with a working OLED display and gradually evolving into a small graphics engine, animation system, and interactive applications.

This project is intended to be developed incrementally with an agentic coding assistant. The assistant should inspect the existing implementation, make focused changes, build and test them, and explain important technical decisions.

## 1. Project Overview

The project began as an ESP-IDF Hello World application and has progressed to controlling a physical I2C OLED display.

The immediate goal is to turn the OLED into a small graphics playground. Future experiments may include pixel art, animations, games, live system dashboards, and network-connected applications.

**The primary development environment is Linux under WSL2, using ESP-IDF and C. Arduino IDE is not the target workflow.**

## 2. Current Hardware

| Component | Details |
|---|---|
| Microcontroller | ESP32, ESP32-D0WD-V3 |
| Chip revision | v3.1 |
| Flash | 4 MB |
| Display | Four-pin I2C OLED |
| Display controller | Likely SSD1306; verify if necessary |
| Expected resolution | 128 × 64 pixels |
| Detected I2C address | `0x3C` |
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| Display supply | 3.3 V |
| Ground | GND |

OLED connections:

| OLED pin | ESP32 connection |
|---|---|
| GND | GND |
| VDD | 3V3 |
| SCK | GPIO 22 |
| SDA | GPIO 21 |

The display was detected at I2C address `0x3C` by an I2C scanner. The physical OLED has successfully displayed the Hello World output.

The controller and resolution are currently assumptions based on the common SSD1306 module. Verify them before relying on controller-specific behavior.

## 3. Development Environment

- Operating system: Ubuntu 24.04 under WSL2
- Development framework: ESP-IDF
- Language: C
- Target: `esp32`
- ESP-IDF version currently installed: `v6.1-dev-8861-g9c37a0a7567`
- USB serial device when attached to WSL: `/dev/ttyUSB0`
- USB serial bridge: CH340, USB VID:PID `1a86:7523`

### Project location

```text
~/projects/esp32-lab/firmware/hello_world/
```

Important files:

```text
hello_world/
├── CMakeLists.txt
├── sdkconfig
├── main/
│   ├── CMakeLists.txt
│   └── hello_world_main.c
├── build/
└── README.md
```

The exact project contents may differ. Inspect the filesystem rather than assuming this tree is exhaustive.

### Activate ESP-IDF

From a new WSL terminal:

```bash
. ~/esp/esp-idf/export.sh
```

Navigate to the project:

```bash
cd ~/projects/esp32-lab/firmware/hello_world
```

### Common development commands

Build firmware:

```bash
idf.py build
```

Flash firmware:

```bash
idf.py -p /dev/ttyUSB0 flash
```

Open the serial monitor:

```bash
idf.py -p /dev/ttyUSB0 monitor
```

Build, flash, and monitor:

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Exit the serial monitor using `Ctrl+]`.

If the serial device is unavailable, check whether the ESP32 is attached to WSL before attempting to flash. USB passthrough may require action from Windows PowerShell.

## 4. Current Implementation

The firmware is written in C and uses ESP-IDF's modern I2C master driver and built-in LCD panel interfaces.

Relevant headers include:

```c
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
```

The current implementation includes:

- I2C master bus initialization.
- OLED initialization using the SSD1306 panel driver.
- A monochrome framebuffer.
- Pixel drawing using `set_pixel()`.
- Basic character rendering using a small 5 × 7 font.
- Text rendering through `draw_text()`.
- Displaying a Hello World message.

The framebuffer is expected to occupy:

```c
128 * 64 / 8
```

That is 1,024 bytes for a 128 × 64 monochrome display.

The framebuffer layout and bit orientation must be verified against the panel driver's expectations before implementing more advanced drawing operations.

### Component dependencies

The main component currently declares dependencies on:

```cmake
REQUIRES esp_driver_i2c esp_lcd
```

Use the built-in ESP-IDF SSD1306 support where appropriate. Do not reintroduce the previously attempted legacy `espressif/ssd1306` component without a specific, justified reason.

## 5. Current Status

### Completed

- [x] ESP-IDF installed and usable inside WSL2.
- [x] ESP32 target configured.
- [x] Firmware compiled successfully.
- [x] USB serial communication established.
- [x] I2C scanner detected the OLED at `0x3C`.
- [x] OLED initialization implemented.
- [x] Physical OLED displayed the Hello World output.
- [x] Modular graphics engine implemented (`oled_graphics.c`, `oled_graphics.h`) with complete 95-character ASCII font (32-126).
- [x] Drawing primitives: bounds-checked `oled_set_pixel`, Bresenham `oled_draw_line`, `oled_draw_rect`, `oled_fill_rect`, midpoint `oled_draw_circle`, and `oled_fill_circle`.
- [x] Hardware button handler implemented with debouncing on GPIO 0 (onboard BOOT button) and GPIO 18 (external).
- [x] Multi-scene animation engine (`scenes.c`, `scenes.h`) with 5 interactive modes:
  1. 3D Rotating Wireframe Cube (perspective 3D projection & matrix rotation).
  2. 3D Warp Starfield (64-star perspective simulation).
  3. Bouncing Physics Demo (screen boundary collision with bouncing box & ball).
  4. System Diagnostics Showcase (live ESP32 uptime, free heap memory, geometric test patterns).
  5. Linux Tux Penguin Bitmap Demo (48x52 dithered 1-bit bitmap rendered via oled_draw_bitmap).
- [x] Created image_to_c.py tool to convert any PNG/JPG image into an embedded 1-bit C bitmap array.
- [x] Firmware built and flashed to hardware at 400 kHz I2C Fast-Mode (~35-40 FPS).

### Important debugging history

An earlier attempt used a legacy SSD1306 component and the old `driver/i2c.h` API. That implementation encountered a runtime Guru Meditation error.

The project was subsequently moved toward the built-in ESP-IDF LCD panel driver and modern I2C master API.

**Do not revert to the old implementation simply to make new features easier.** First inspect the current source and verify that the modern implementation remains functional.

The previous build also exposed a typo in the SSD1306 configuration type. The correct type is:

```c
esp_lcd_panel_ssd1306_config_t
```

The typo was corrected, and the build subsequently succeeded.

## 6. Development Roadmap

Develop the project in small, independently testable stages.

### Stage 1: Stabilize the display driver

- Inspect the current source and project configuration.
- Verify that the firmware builds.
- Confirm the OLED still displays text after flashing.
- Verify framebuffer orientation, addressing, and dimensions.
- Fix missing glyphs in the current font.
- Add a clean display-clearing operation.
- Ensure the application does not reboot or produce persistent errors.

### Stage 2: Build a reusable graphics layer

Implement reusable functions such as:

```c
void clear_screen(void);
void set_pixel(int x, int y);
void draw_line(int x0, int y0, int x1, int y1);
void draw_rect(int x, int y, int width, int height);
void fill_rect(int x, int y, int width, int height);
void draw_circle(int cx, int cy, int radius);
void draw_text(int x, int y, const char *text);
```

These signatures are proposed interfaces, not a requirement to duplicate existing functions. Reuse and improve the current implementation where practical.

Requirements:

- Clip drawing operations to the display boundaries.
- Avoid out-of-bounds memory access.
- Keep the framebuffer representation consistent.
- Separate drawing operations from hardware initialization and display updates.
- Avoid unnecessary full-screen transfers when a smaller update is possible and supported.
- Add basic validation for invalid dimensions and coordinates.

### Stage 3: Animation engine

Build a simple animation loop with predictable timing.

Suggested experiments:

1. Bouncing pixel or ball.
2. Moving and scrolling text.
3. Starfield animation.
4. Animated pixel-art character.
5. Progress bars and loading animations.
6. Simple visual effects.

Keep rendering and timing separate from the display driver. Use FreeRTOS timing facilities rather than busy-wait loops.

### Stage 4: Interactive applications

Possible projects:

- Snake.
- Pong.
- A simple space shooter.
- A small menu interface.
- A digital clock.
- A sensor dashboard.
- Wi-Fi status display.
- A miniature system monitor that receives statistics from a PC or server.

Add physical buttons or sensors when needed. Do not assume that peripherals exist until the user confirms them.

### Stage 5: Network-connected experiments

After the graphics and animation layers are stable, explore:

- Fetching data over Wi-Fi.
- Displaying remote sensor measurements.
- Showing server or PC status.
- Receiving commands from another device.
- Displaying metadata from external services.
- Streaming low-resolution, preprocessed monochrome frames from a host computer as an experimental video project.

The OLED is a small monochrome display, not a conventional multimedia screen. Full-colour YouTube playback is outside the capabilities of this display configuration. Any frame-streaming experiment must account for I2C bandwidth, transfer time, memory, and frame rate.

## 7. Instructions for the Agentic Coding Assistant

Act as an embedded systems development assistant, not merely a code generator.

### Before making changes

1. Read this README.
2. Inspect `main/hello_world_main.c`.
3. Inspect `main/CMakeLists.txt`, the root `CMakeLists.txt`, and relevant project configuration.
4. Check the ESP-IDF version and existing component dependencies.
5. Determine which functionality is already implemented.
6. Identify any uncertainty about the hardware or driver behavior.

Do not overwrite working source code before inspecting it.

### While making changes

- Preserve the existing working OLED functionality.
- Prefer ESP-IDF-native APIs and the modern I2C master driver.
- Use C and the existing build system.
- Keep changes focused and incremental.
- Avoid unnecessary external components and dependencies.
- Explain the purpose of new functions and important embedded systems concepts.
- Avoid adding large frameworks for small tasks.
- Do not change GPIO assignments, flash settings, partition tables, or other hardware configuration without a reason.
- Do not introduce Arduino-specific libraries or require Arduino IDE.
- Never claim a feature works merely because the code was generated.

### Verification workflow

For each meaningful change:

1. Explain the planned change briefly.
2. Modify the relevant source files.
3. Build with `idf.py build`.
4. Fix any compilation errors before proceeding.
5. If hardware testing is required, explain the flashing and testing steps.
6. Ask the user to report the actual OLED behavior or serial output.
7. Update the project documentation when the implementation changes.

A successful compilation does not prove that the display renders correctly. Hardware verification is a separate step.

If the agent has terminal and USB access, it may run commands directly, but it must verify the actual result before reporting success. If hardware access is unavailable, clearly distinguish compile-time verification from physical testing.

### Error handling

When a build or runtime failure occurs:

- Identify the first meaningful error.
- Inspect the relevant source and API definitions.
- Make the smallest reasonable correction.
- Rebuild before proposing unrelated changes.
- Avoid replacing the entire project to fix a localized issue.
- Preserve useful logs and explain any destructive operations before running them.

Do not erase flash, reset configuration, or remove project dependencies without a clear technical justification.

## 8. Working Style

The project owner is learning embedded C and ESP-IDF while building the hardware.

The assistant should therefore:

- Explain concepts clearly without assuming advanced embedded systems knowledge.
- Prefer one meaningful step at a time.
- Provide complete commands that can be copied into the terminal.
- Explain which directory a command must run from.
- Ask for output when the next action depends on a result.
- Explain compiler errors rather than silently patching them.
- Introduce abstractions only when they solve a real problem.
- Balance learning with getting the hardware to work.

When possible, teach the underlying concept as part of the implementation rather than presenting code without explanation.

## 9. Immediate Next Task

**Build a reusable monochrome graphics engine on top of the working OLED implementation.**

Begin by inspecting the current project and confirming the current firmware builds. Then:

1. Verify the framebuffer format and the SSD1306 driver's expected pixel layout.
2. Complete the character font used by the existing Hello World message.
3. Implement or refactor basic drawing functions.
4. Display a test pattern containing pixels, lines, rectangles, circles, and text.
5. Build and flash the firmware.
6. Verify the output on the physical OLED before moving on to animation.

Keep the first graphics demo simple and reliable. Do not start with a game engine, Wi-Fi integration, or video streaming before the rendering layer is validated.

## 10. Definition of Done

The initial graphics-engine milestone is complete when:

- The project builds successfully using ESP-IDF.
- The ESP32 runs the newly flashed firmware without reboot loops.
- The OLED displays the expected test pattern.
- Text and shapes render in the correct positions.
- Drawing operations remain within framebuffer bounds.
- The display can be cleared and redrawn reliably.
- The implementation and build instructions are documented.

Once this milestone is complete, proceed to animation and then interactive applications.

---

**Project principle:** Build from working hardware outward. Every new layer should be small enough to understand, test, and debug independently.
