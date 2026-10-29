# ESP32 OLED Graphics Lab & Animation Engine

An embedded development playground using **ESP-IDF (native C) and an ESP32**, featuring a custom monochrome 2D graphics engine, 3D perspective projection math, real-time physics animations, a full ASCII font, and 1-bit bitmap rendering on a 128×64 SSD1306 I2C OLED display.

---

## 📸 Interactive Scenes (Cycled via BOOT Button)

Clicking the onboard **BOOT button (GPIO 0)** switches between 5 real-time modes:

| # | Mode | Description |
|---|---|---|
| **1** | **3D Wireframe Cube** | Real-time 3D rotation matrix calculation around X, Y, and Z axes with perspective 2D projection and Bresenham line rendering. |
| **2** | **3D Warp Starfield** | 64 stars flying toward the camera in 3D perspective with velocity-based motion streak trails. |
| **3** | **Bounce Physics** | Screen collision physics with an "ESP32" bouncing box and elastic balls reflecting off display borders. |
| **4** | **Hardware Diagnostics** | Real-time ESP32 uptime, free heap RAM monitor, concentric circles, nested rectangles, and geometric primitives showcase. |
| **5** | **Linux Tux Penguin** | 48×52 Floyd-Steinberg dithered 1-bit bitmap of Tux with a retro terminal system badge. |

---

## 🛠️ Hardware Setup

| Component | Pin / Value | Notes |
|---|---|---|
| **Microcontroller** | ESP32-D0WD-V3 (rev 3.1) | Dual-core 240 MHz, 4 MB Flash |
| **Display** | 0.96" I2C Monochrome OLED | 128×64 pixels, SSD1306 controller |
| **I2C SDA** | **GPIO 21** | ESP32 standard I2C data |
| **I2C SCL** | **GPIO 22** | ESP32 standard I2C clock |
| **I2C Frequency** | **400 kHz** | Fast-mode I2C (~35–40 FPS render loop) |
| **I2C Address** | `0x3C` | Standard SSD1306 address |
| **Display Power** | 3.3V & GND | Connected to ESP32 3V3 & GND |
| **Button Input** | **GPIO 0** | Built-in BOOT button (Internal pull-up, debounced) |
| **Optional Button** | **GPIO 18** | Breadboard pushbutton to GND (Internal pull-up) |

---

## 📂 Project Structure

```text
esp32-lab/
├── README.md                      # Main project documentation
├── .gitignore                     # Git ignore rules for ESP-IDF
├── docs/
│   └── handover.md                # Development history, stages & roadmap
├── host-tools/
│   └── image_to_c.py              # CLI tool to convert any PNG/JPG into a C bitmap header
└── firmware/
    └── hello_world/
        ├── CMakeLists.txt         # Project-level CMakeLists
        ├── sdkconfig              # ESP-IDF target configuration (esp32)
        └── main/
            ├── CMakeLists.txt     # Component registration & dependencies
            ├── hello_world_main.c # App entrypoint, I2C setup, debounced button loop
            ├── oled_graphics.h    # Graphics engine header
            ├── oled_graphics.c    # Framebuffer, drawing primitives & full ASCII font
            ├── scenes.h           # Scene state machine definitions
            ├── scenes.c           # 3D cube math, starfield, physics, diagnostics & Tux
            └── tux_bitmap.h       # 48x52 dithered Tux penguin 1-bit C array
```

---

## 🎨 Graphics Engine API (`oled_graphics.h`)

The graphics library operates on an internal 1,024-byte framebuffer (`128 * 64 / 8`). All functions include **hard boundary clipping** to protect against out-of-bounds memory writes:

* **Clearing & Pixels**:
  * `void oled_clear(oled_color_t color);`
  * `void oled_set_pixel(int x, int y, oled_color_t color);`
  * `uint8_t oled_get_pixel(int x, int y);`
* **Lines & Rectangles**:
  * `void oled_draw_line(int x0, int y0, int x1, int y1, oled_color_t color);` (Bresenham)
  * `void oled_draw_rect(int x, int y, int w, int h, oled_color_t color);`
  * `void oled_fill_rect(int x, int y, int w, int h, oled_color_t color);`
* **Circles**:
  * `void oled_draw_circle(int cx, int cy, int r, oled_color_t color);` (Midpoint circle)
  * `void oled_fill_circle(int cx, int cy, int r, oled_color_t color);`
* **Typography**:
  * `void oled_draw_char(int x, int y, char c, oled_color_t color);`
  * `void oled_draw_text(int x, int y, const char *text, oled_color_t color);`
  * Complete 95-glyph 5×7 ASCII font (characters 32 `' '` through 126 `'~'`).
* **Bitmaps**:
  * `void oled_draw_bitmap(int x, int y, int w, int h, const uint8_t *bitmap, oled_color_t color);`

---

## 🖼️ Converting Any Image to C (`image_to_c.py`)

A Python CLI tool is included in `host-tools/` to turn any image into an embedded C array:

```bash
# Convert an image with auto-aspect ratio scaling (max 48x48)
python host-tools/image_to_c.py my_icon.png icon --width 48 --height 48

# Use fixed threshold instead of Floyd-Steinberg dithering
python host-tools/image_to_c.py logo.png my_logo --threshold 128
```

This generates `icon_bitmap.h` with a packed `static const uint8_t icon_bitmap[]` array ready to `#include` in your C code.

---

## 🚀 Building & Flashing

### 1. Activate ESP-IDF (in WSL2 Ubuntu)
```bash
. ~/esp/esp-idf/export.sh
```

### 2. Navigate to Firmware Directory
```bash
cd ~/projects/esp32-lab/firmware/hello_world
```

### 3. Build Firmware
```bash
idf.py build
```

### 4. Flash and Monitor
```bash
idf.py -p /dev/ttyUSB0 flash monitor
```
*(Exit the monitor using `Ctrl + ]`)*

---

## 📜 License
MIT License. Free to use, modify, and build upon.

