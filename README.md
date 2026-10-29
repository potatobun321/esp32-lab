# ESP32 Embedded Graphics Engine and OLED Lab

A bare-metal-style embedded development environment and 2D/3D graphics engine implemented in native C using Espressif's ESP-IDF framework. This project controls a 128x64 monochrome SSD1306 OLED display over high-speed I2C, implementing custom framebuffers, graphics primitives, 3D perspective projection mathematics, spatial error-diffusion dithering, and debounced interrupt/polling control.

This repository serves as both a reference codebase and an educational guide for transitioning from basic microcontroller concepts to intermediate embedded systems engineering.

---

## 1. System Architecture and Hardware Specifications

### 1.1 Microcontroller: ESP32-D0WD-V3 (Revision 3.1)
* **Core Architecture**: Dual-core 32-bit Xtensa LX6 microprocessors running at up to 240 MHz.
* **Internal Memory**: 520 KB SRAM (organized into IRAM and DRAM blocks).
* **Non-Volatile Storage**: 4 MB external SPI Flash operating in Dual I/O (DIO) mode at 40 MHz.
* **Operating Voltage**: 3.0 V to 3.6 V (nominal 3.3 V).

### 1.2 Display Module: 0.96-inch Monochrome OLED (SSD1306 Controller)
* **Native Resolution**: 128 columns x 64 rows (8,192 addressable monochrome pixels).
* **Interface**: 2-wire Inter-Integrated Circuit (I2C), standard 7-bit slave address `0x3C`.
* **Pixel Technology**: Organic Light-Emitting Diode; pixels emit individual light with zero backlight power draw.

### 1.3 Pin Allocation and Physical Wiring

| Display Pin | ESP32 GPIO | Function | Electrical Characteristics |
|---|---|---|---|
| **GND** | GND | Ground Reference | 0 V common plane |
| **VDD** | 3V3 | System Power Supply | Regulated 3.3 V DC rail |
| **SCK / SCL** | GPIO 22 | I2C Serial Clock | Open-drain output with pull-up |
| **SDA** | GPIO 21 | I2C Serial Data | Open-drain bidirectional with pull-up |
| *(On-board)* | GPIO 0 | BOOT / User Button | Active-low; internal pull-up enabled |
| *(Auxiliary)* | GPIO 18 | External Pushbutton | Active-low; internal pull-up enabled |

---

## 2. Display Hardware Memory Model (SSD1306 GDDRAM)

Understanding how physical display memory is laid out is essential to writing embedded graphics engines. The SSD1306 contains a Graphic Display Data RAM (GDDRAM) consisting of 1,024 bytes ($128 \times 64 \div 8$).

### 2.1 Page Addressing Architecture
The display memory is organized horizontally into 8 pages (Page 0 through Page 7). Each page represents 8 vertical rows of pixels across all 128 columns:

```text
               Column 0        Column 1       ...      Column 127
          +---------------+---------------+       +---------------+
Page 0    | D0 (Row 0)    | D0 (Row 0)    |  ...  | D0 (Row 0)    |  Rows 0-7
          | D1 (Row 1)    | D1 (Row 1)    |       | D1 (Row 1)    |
          | ...           | ...           |       | ...           |
          | D7 (Row 7)    | D7 (Row 7)    |       | D7 (Row 7)    |
          +---------------+---------------+       +---------------+
          | ...           | ...           |  ...  | ...           |
          +---------------+---------------+       +---------------+
Page 7    | D0 (Row 56)   | D0 (Row 56)   |  ...  | D0 (Row 56)   |  Rows 56-63
          | ...           | ...           |       | ...           |
          | D7 (Row 63)   | D7 (Row 63)   |       | D7 (Row 63)   |
          +---------------+---------------+       +---------------+
```

* Each single byte transferred represents a **vertical slice of 8 vertical pixels** in a given column.
* Least Significant Bit (Bit 0) corresponds to the **top-most** pixel of that page.
* Most Significant Bit (Bit 7) corresponds to the **bottom-most** pixel of that page.

### 2.2 Memory Indexing Equation
To calculate the exact byte index and bit position inside the 1,024-byte local framebuffer:

$$\text{Byte Index} = x + \left(\left\lfloor \frac{y}{8} \right\rfloor \times 128\right)$$

$$\text{Bit Mask} = 1 \ll (y \pmod 8)$$

---

## 3. Toolchain and Build System

This project is built directly with Espressif's native **ESP-IDF v6.x** toolchain rather than using third-party wrappers such as Arduino.

### 3.1 Why Native ESP-IDF?
1. **FreeRTOS Integration**: Full, unabstracted access to multi-core preemptive multitasking, task notifications, queues, and software timers.
2. **Modern Driver Stack**: Uses `driver/i2c_master.h` and `esp_lcd` hardware abstractions instead of deprecated legacy drivers.
3. **Deterministic Memory Footprint**: Fine-grained configuration of Flash partitions, heap memory pools, and stack depth via `sdkconfig`.
4. **Compile Efficiency**: CMake and Ninja-driven parallel builds.

### 3.2 Linux Environment under WSL2
The development host environment runs Ubuntu 24.04 under Windows Subsystem for Linux (WSL2). The ESP32 is attached via USB serial pass-through using `usbipd-win`:

```bash
# Windows PowerShell (Host) - Attach serial bridge to WSL
usbipd attach --wsl Ubuntu-24.04 -b <BUSID> --auto-attach

# WSL2 Terminal (Build & Flash)
. ~/esp/esp-idf/export.sh
cd ~/projects/esp32-lab/firmware/hello_world
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

---

## 4. Software Implementation Details

The firmware architecture is decoupled into three primary layers:
1. **Hardware Driver Layer (`hello_world_main.c`)**: Initializes the I2C master peripheral, configures panel timing, registers button GPIOs, and runs the main FreeRTOS render task.
2. **Graphics Engine (`oled_graphics.c`, `oled_graphics.h`)**: Maintains the local 1,024-byte memory buffer and provides boundary-checked 2D primitives.
3. **Scene Pipeline (`scenes.c`, `scenes.h`)**: Implements mathematical projections, physics simulations, and state switching.

### 4.1 Low-Level I2C Bus Configuration
The I2C bus is initialized in Fast-Mode at **400 kHz**:

$$\text{Transfer Time per Frame} \approx \frac{1024 \times 9 \text{ bits}}{400{,}000 \text{ Hz}} \approx 23 \text{ ms} \implies \sim 40 \text{ FPS}$$

A 100 ms hardware stabilization delay is injected during `app_main` before bus initialization. This gives the SSD1306 internal charge pump and power rails adequate rise time after a hard MCU reset, preventing I2C transaction timeouts.

### 4.2 Drawing Primitives and Mathematical Algorithms

#### Hard Boundary Clipping
To guarantee that corrupted or out-of-range geometric coordinates do not trigger memory write violations into neighboring heap or task stack structures, `oled_set_pixel` performs strict bounds checking:

```c
void oled_set_pixel(int x, int y, oled_color_t color)
{
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) {
        return;
    }
    int index = x + (y / 8) * OLED_WIDTH;
    uint8_t mask = 1U << (y % 8);
    // Apply bitwise operation (Set, Clear, or Invert)
}
```

#### Bresenham's Integer Line Algorithm
Lines are drawn without floating-point math using Bresenham's algorithm, tracking error increments with integer addition and subtraction:

$$\Delta x = |x_1 - x_0|, \quad \Delta y = -|y_1 - y_0|$$

$$\text{err} = \Delta x + \Delta y$$

For each step along the dominant axis, if $2 \times \text{err} \ge \Delta y$, the error accumulator updates by $\Delta y$ and the coordinate advances along $X$. If $2 \times \text{err} \le \Delta x$, it advances along $Y$.

#### Midpoint Circle Algorithm
Circles are generated using 8-way symmetric octant reflection. Given center $(cx, cy)$ and radius $r$:
* Evaluate initial decision parameter: $d = 3 - 2r$.
* For each step $x$, plot 8 mirrored points: $(\pm x, \pm y)$ and $(\pm y, \pm x)$.
* If $d < 0$, advance $x$ and increment $d \gets d + 4x + 6$.
* Otherwise, advance both $x$ and $y$, updating $d \gets d + 4(x - y) + 10$.

#### 3D Perspective Projection Mathematics
The 3D wireframe cube rotates in Euclidean 3D space by applying three successive Euler rotation matrices around the X, Y, and Z axes:

$$\begin{bmatrix} x_1 \\ y_1 \\ z_1 \end{bmatrix} = \begin{bmatrix} 1 & 0 & 0 \\ 0 & \cos\theta_x & -\sin\theta_x \\ 0 & \sin\theta_x & \cos\theta_x \end{bmatrix} \begin{bmatrix} x \\ y \\ z \end{bmatrix}$$

$$\begin{bmatrix} x_2 \\ y_2 \\ z_2 \end{bmatrix} = \begin{bmatrix} \cos\theta_y & 0 & \sin\theta_y \\ 0 & 1 & 0 \\ -\sin\theta_y & 0 & \cos\theta_y \end{bmatrix} \begin{bmatrix} x_1 \\ y_1 \\ z_1 \end{bmatrix}$$

$$\begin{bmatrix} x_3 \\ y_3 \\ z_3 \end{bmatrix} = \begin{bmatrix} \cos\theta_z & -\sin\theta_z & 0 \\ \sin\theta_z & \cos\theta_z & 0 \\ 0 & 0 & 1 \end{bmatrix} \begin{bmatrix} x_2 \\ y_2 \\ z_2 \end{bmatrix}$$

Perspective projection onto the 2D plane divides coordinates by their distance along the depth axis $Z$:

$$X_{\text{screen}} = X_{\text{origin}} + \frac{x_3 \cdot \text{FOV}}{z_3 + \text{Distance}}$$

$$Y_{\text{screen}} = Y_{\text{origin}} + \frac{y_3 \cdot \text{FOV}}{z_3 + \text{Distance}}$$

#### Typography: 5x7 ASCII Character Matrix
The engine includes a full 95-character printable ASCII glyph table (characters 32 to 126). Each character is stored as a 5-byte array where each byte represents one vertical column of pixels. `oled_draw_text` scans string characters, looks up glyph bytes, and paints column-by-column with 1-pixel horizontal spacing.

---

## 5. Bitmap Pipeline and Dithering (`host-tools/image_to_c.py`)

Microcontrollers lack the memory to decompress large PNG/JPEG files at runtime. The host-side conversion pipeline translates continuous-tone raster graphics into 1-bit C arrays using spatial error diffusion:

```text
[Input Image (PNG/JPG)] 
       │
       ▼
[Bounding Box Auto-Crop]  --> Drops empty transparent margins
       │
       ▼
[Lanczos Downscaling]     --> Fits display boundary (e.g., 56x52)
       │
       ▼
[Floyd-Steinberg Dither]  --> Distributes quantization errors to adjacent pixels
       │
       ▼
[1-Bit Bitfield Packing]  --> 8 horizontal pixels per byte (MSB first)
       │
       ▼
[Generated .h Header]     --> static const uint8_t image_bitmap[] in Flash
```

### 5.1 Floyd-Steinberg Error Diffusion Formula
When a grayscale pixel (0-255) is quantized to 1 bit (0 or 255), the quantization error is:

$$\text{Error} = \text{Pixel}_{\text{original}} - \text{Pixel}_{\text{quantized}}$$

This error is diffused to surrounding unvisited pixels according to the kernel:

$$\frac{1}{16} \times \begin{bmatrix} & * & 7 \\ 3 & 5 & 1 \end{bmatrix}$$

This produces smooth visual gradients on a purely binary display.

### 5.2 Using the Image Converter
```bash
# Convert an image using auto-scaling
python host-tools/image_to_c.py pika.png pika --width 56 --height 52

# Force fixed thresholding instead of dithering
python host-tools/image_to_c.py logo.png logo --threshold 128
```

---

## 6. Real-Time Interactive Scenes

Clicking the onboard **BOOT button (GPIO 0)** advances through 6 distinct modes:

1. **Scene 1: 3D Wireframe Cube**
   * Real-time calculation of rotation matrices across 3 axes.
   * Perspective projection of 8 vertices and 12 connecting wireframe lines.
2. **Scene 2: 3D Warp Starfield**
   * 64 independent stars with random $(X, Y, Z)$ coordinates.
   * Velocity vectors project stars forward; distance thresholding draws motion streak lines.
3. **Scene 3: Bouncing Collision Physics**
   * Elastic collision reflection off display boundaries.
   * Independent velocity vectors for text box and filled circular particles.
4. **Scene 4: System Diagnostics and Geometry Showcase**
   * Real-time hardware performance monitor (`esp_timer_get_time()`, `esp_get_free_heap_size()`).
   * Primitives test: concentric circles, nested rectangles, and crosshairs.
5. **Scene 5: Linux Tux Penguin**
   * 48x52 dithered monochrome Tux bitmap paired with a split-screen system badge.
6. **Scene 6: Pikachu Pokedex Badge**
   * 56x52 dithered Pikachu bitmap alongside a formatted status card.

---

## 7. Input Conditioning and Debouncing

Mechanical switches suffer from electrical contact bounce lasting 5 ms to 20 ms. Because the graphics engine operates inside an active rendering loop, input is conditioned via multi-frame temporal debouncing:

```c
bool btn_down = (gpio_get_level(BUTTON_BOOT) == 0);
uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);

if (btn_down) {
    btn_pressed_frames++;
} else {
    // Only register press on release after being stably held for >= 2 frames (50 ms)
    if (btn_pressed_frames >= 2 && (now_ms - last_press_ms > 200)) {
        scenes_next();
        last_press_ms = now_ms;
    }
    btn_pressed_frames = 0;
}
```

* Eliminates false triggers from noise glitches.
* Triggers cleanly on release.
* Enforces a 200 ms lockout window preventing accidental double-advancing.

---

## 8. Directory Structure

```text
esp32-lab/
├── README.md                      # Primary technical documentation
├── .gitignore                     # Git exclusion rules for ESP-IDF artifacts
├── docs/
│   └── handover.md                # Development trajectory, logs, and stage plan
├── host-tools/
│   ├── image_to_c.py              # Image-to-C bitmap converter utility
│   └── pika.png                   # Source asset for Pikachu bitmap
└── firmware/
    └── hello_world/
        ├── CMakeLists.txt         # Root project CMake configuration
        ├── sdkconfig              # ESP32 Kconfig hardware settings
        └── main/
            ├── CMakeLists.txt     # Component registration and dependencies
            ├── hello_world_main.c # Entrypoint, I2C master init, main render loop
            ├── oled_graphics.h    # Graphics library public API
            ├── oled_graphics.c    # Framebuffer storage, algorithms, ASCII font
            ├── scenes.h           # Scene enum definitions and function prototypes
            ├── scenes.c           # Mathematical implementations of scenes 1-6
            ├── tux_bitmap.h       # 48x52 dithered Tux penguin bitmap
            └── pika_bitmap.h      # 56x52 dithered Pikachu bitmap
```

---

## 9. Build, Flash, and Monitor

### Prerequisites
* ESP-IDF v5.x or v6.x installed on Linux (or WSL2).
* Serial bridge driver (CH340 / CP2102) configured on host.

### Compilation
From the project root:
```bash
# 1. Source the ESP-IDF environment variables
. ~/esp/esp-idf/export.sh

# 2. Enter the firmware directory
cd firmware/hello_world

# 3. Compile the bootloader, partition table, and application
idf.py build
```

### Flashing to Target
```bash
# Flash at 460800 baud and launch serial terminal
idf.py -p /dev/ttyUSB0 -b 460800 flash monitor
```

To exit the serial monitor, press `Ctrl + ]`.

---

## 10. Technical Concepts Reference (Glossary)

* **Frame Buffer**: A dedicated block of contiguous memory that holds the complete bitmap representation of an image frame before it is transferred to the physical display controller.
* **Open-Drain Bus**: An electronic bus configuration (such as I2C) where devices can pull lines LOW, but rely on passive pull-up resistors to return lines to HIGH logic level.
* **Bresenham's Algorithm**: An algorithm that determines which points of an n-dimensional raster should be selected to form a close approximation to a straight line between two points using only integer math.
* **Error Diffusion**: A type of halftoning technique in which the quantization residual is distributed to neighboring pixels that have not yet been processed.
* **FreeRTOS**: A real-time operating system kernel for embedded devices providing predictable task scheduling, timing guarantees, and inter-task communication.

---

## 11. License
This project is open-source software licensed under the MIT License.
