# BSP: ESP32-P4-S070-WBA

Board Support Package for ESP32-P4-S070-WBA (7-inch 1024x600 MIPI DSI).

## Configuration

Configuration in `menuconfig`.

Selection LCD panel `Board Support Package (ESP32-P4-S070-WBA) --> Display --> Select LCD panel`
- JD9165 1024x600 (default)
- EK79007 1024x600

Selection color format `Board Support Package (ESP32-P4-S070-WBA) --> Display --> Select LCD color format`
- RGB565 (default)
- RGB888

### Capabilities and dependencies
|  Capability |     Available    |                                                 Component                                                |Version|
|-------------|------------------|----------------------------------------------------------------------------------------------------------|-------|
|   DISPLAY   |:heavy_check_mark:|    [espressif/esp_lcd_ek79007](https://components.espressif.com/components/espressif/esp_lcd_ek79007)    |  1.*  |
|   DISPLAY   |:heavy_check_mark:|     [espressif/esp_lcd_jd9165](https://components.espressif.com/components/espressif/esp_lcd_jd9165)     |  1.*  |
|  LVGL_PORT  |:heavy_check_mark:|      [espressif/esp_lvgl_port](https://components.espressif.com/components/espressif/esp_lvgl_port)      |   ^2  |
|    TOUCH    |:heavy_check_mark:|[espressif/esp_lcd_touch_gt911](https://components.espressif.com/components/espressif/esp_lcd_touch_gt911)|   ^1  |
|    AUDIO    |:heavy_check_mark:|      [espressif/esp_codec_dev](https://components.espressif.com/components/espressif/esp_codec_dev)      | 1.2.* |
|    SDCARD   |:heavy_check_mark:|                                                    idf                                                   | >=5.3 |
