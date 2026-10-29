#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "oled_graphics.h"
#include "scenes.h"

static const char *TAG = "OLED_LAB";

#define OLED_SDA       GPIO_NUM_21
#define OLED_SCL       GPIO_NUM_22
#define OLED_ADDRESS   0x3C

#define BUTTON_BOOT    GPIO_NUM_0   // Built-in BOOT button (active LOW)
#define BUTTON_EXT     GPIO_NUM_18  // Optional external button (active LOW)

static void init_button(void)
{
    gpio_config_t btn_cfg = {
        .pin_bit_mask = (1ULL << BUTTON_BOOT) | (1ULL << BUTTON_EXT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&btn_cfg));
    ESP_LOGI(TAG, "Button inputs ready (BOOT: GPIO %d, EXT: GPIO %d)", BUTTON_BOOT, BUTTON_EXT);
}

void app_main(void)
{
    // Allow OLED charge-pump and power rails to stabilize after reset
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_LOGI(TAG, "Initializing I2C bus and OLED panel...");

    i2c_master_bus_handle_t bus_handle;
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = OLED_SDA,
        .scl_io_num = OLED_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    esp_lcd_panel_io_handle_t io_handle;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = OLED_ADDRESS,
        .scl_speed_hz = 400000, // 400 kHz Fast-mode for smooth animation
        .control_phase_bytes = 1,
        .dc_bit_offset = 6,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(bus_handle, &io_config, &io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = -1,
        .vendor_config = NULL,
    };

    esp_lcd_panel_handle_t panel_handle;
    esp_lcd_panel_ssd1306_config_t oled_config = {
        .height = OLED_HEIGHT,
        .contrast = 128,
    };
    panel_config.vendor_config = &oled_config;

    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    init_button();
    scenes_init();

    ESP_LOGI(TAG, "Graphics & scenes ready. Starting main render loop...");

    int btn_pressed_frames = 0;
    uint32_t last_press_ms = 0;

    // Small initial settle delay so reset electrical bounce on GPIO 0 is ignored
    vTaskDelay(pdMS_TO_TICKS(150));

    while (1) {
        // Read active-low buttons (pressed = 0)
        bool btn_down = (gpio_get_level(BUTTON_BOOT) == 0) || (gpio_get_level(BUTTON_EXT) == 0);
        uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);

        if (btn_down) {
            btn_pressed_frames++;
        } else {
            // Require button to be held for at least 2 frames (~50ms) to register a valid click
            if (btn_pressed_frames >= 2 && (now_ms - last_press_ms > 200)) {
                scenes_next();
                last_press_ms = now_ms;
                ESP_LOGI(TAG, "Button click detected -> Switched to Scene: %s",
                         scenes_get_name(scenes_get_current()));
            }
            btn_pressed_frames = 0;
        }

        // Render current scene to local framebuffer
        scenes_render_current_frame();

        // Push framebuffer to OLED via I2C
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(
            panel_handle, 0, 0, OLED_WIDTH, OLED_HEIGHT, oled_get_framebuffer()));

        // Delay ~25 ms (~35-40 FPS)
        vTaskDelay(pdMS_TO_TICKS(25));
    }
}
