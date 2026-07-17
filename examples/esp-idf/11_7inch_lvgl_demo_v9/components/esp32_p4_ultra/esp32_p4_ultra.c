#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_spiffs.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_ldo_regulator.h"
#include "esp_vfs_fat.h"
#include "usb/usb_host.h"
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#include "esp_lcd_ek79007.h"
#include "esp_lcd_jd9165.h"

#include "bsp/esp32_p4_ultra.h"
#include "bsp/display.h"
#include "bsp/touch.h"
#include "esp_lcd_touch_gt911.h"
#include "bsp_err_check.h"
#include "driver/i2s_pdm.h"

static const char *TAG = "ESP32_P4_EV";

#if (BSP_CONFIG_NO_GRAPHIC_LIB == 0)
static lv_indev_t *disp_indev = NULL;
#endif // (BSP_CONFIG_NO_GRAPHIC_LIB == 0)

sdmmc_card_t *bsp_sdcard = NULL;    // Global uSD card handler
static bool i2c_initialized = false;
static TaskHandle_t usb_host_task;  // USB Host Library task
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0))
static i2c_master_bus_handle_t i2c_handle = NULL;  // I2C Handle
#endif
static i2s_chan_handle_t i2s_tx_chan = NULL;  // NS4168 speaker TX channel (I2S_NUM_1)
static i2s_chan_handle_t i2s_rx_chan = NULL;  // MSM261D microphone RX channel (I2S_NUM_0)
// Note: i2s_data_if removed as NS4168 and MSM261D don't use codec devices

esp_err_t bsp_i2c_init(void)
{
    /* I2C was initialized before */
    if (i2c_initialized) {
        return ESP_OK;
    }

    i2c_master_bus_config_t i2c_bus_conf = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .sda_io_num = BSP_I2C_SDA,
        .scl_io_num = BSP_I2C_SCL,
        .i2c_port = BSP_I2C_NUM,
    };
    BSP_ERROR_CHECK_RETURN_ERR(i2c_new_master_bus(&i2c_bus_conf, &i2c_handle));

    i2c_initialized = true;

    return ESP_OK;
}

esp_err_t bsp_i2c_deinit(void)
{
    BSP_ERROR_CHECK_RETURN_ERR(i2c_del_master_bus(i2c_handle));
    i2c_initialized = false;
    return ESP_OK;
}

i2c_master_bus_handle_t bsp_i2c_get_handle(void)
{
    return i2c_handle;
}

esp_err_t bsp_sdcard_mount(void)
{
    const esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_BSP_SD_FORMAT_ON_MOUNT_FAIL
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif
        .max_files = 5,
        .allocation_unit_size = 64 * 1024
    };

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.slot = SDMMC_HOST_SLOT_0;
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

    sd_pwr_ctrl_ldo_config_t ldo_config = {
        .ldo_chan_id = 4,
    };
    sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;
    esp_err_t ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create a new on-chip LDO power control driver");
        return ret;
    }
    host.pwr_ctrl_handle = pwr_ctrl_handle;

    const sdmmc_slot_config_t slot_config = {
        /* SD card is connected to Slot 0 pins. Slot 0 uses IO MUX, so not specifying the pins here */
        .cd = SDMMC_SLOT_NO_CD,
        .wp = SDMMC_SLOT_NO_WP,
        .width = 4,
        .flags = 0,
    };

    return esp_vfs_fat_sdmmc_mount(BSP_SD_MOUNT_POINT, &host, &slot_config, &mount_config, &bsp_sdcard);
}

esp_err_t bsp_sdcard_unmount(void)
{
    return esp_vfs_fat_sdcard_unmount(BSP_SD_MOUNT_POINT, bsp_sdcard);
}

esp_err_t bsp_spiffs_mount(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = CONFIG_BSP_SPIFFS_MOUNT_POINT,
        .partition_label = CONFIG_BSP_SPIFFS_PARTITION_LABEL,
        .max_files = CONFIG_BSP_SPIFFS_MAX_FILES,
#ifdef CONFIG_BSP_SPIFFS_FORMAT_ON_MOUNT_FAIL
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif
    };

    esp_err_t ret_val = esp_vfs_spiffs_register(&conf);

    BSP_ERROR_CHECK_RETURN_ERR(ret_val);

    size_t total = 0, used = 0;
    ret_val = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret_val != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret_val));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    return ret_val;
}

esp_err_t bsp_spiffs_unmount(void)
{
    return esp_vfs_spiffs_unregister(CONFIG_BSP_SPIFFS_PARTITION_LABEL);
}

esp_codec_dev_handle_t bsp_audio_codec_speaker_init(void)
{
    // NS4168是I2S数字音频功放，不需要codec芯片配置，直接使用I2S接口
    if (i2s_tx_chan == NULL) {
        // 初始化NS4168的GPIO控制引脚
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << BSP_NS4168_CTRL_PIN),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_ERROR_CHECK(gpio_config(&io_conf));
        gpio_set_level(BSP_NS4168_CTRL_PIN, 1); // 使能NS4168（假设分压电路已处理）
        ESP_LOGI(TAG, "NS4168 GPIO控制引脚初始化完成");

        // 初始化I2S发送通道用于NS4168（使用I2S_NUM_1）
        i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
        ESP_ERROR_CHECK(i2s_new_channel(&tx_chan_cfg, &i2s_tx_chan, NULL));

        // I2S标准模式配置（NS4168使用I2S接口，48kHz，单声道，MSB格式）
        i2s_std_config_t std_cfg = {
            .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(48000),  // 48kHz采样率
            .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
            .gpio_cfg = {
                .bclk = BSP_NS4168_BCLK_PIN,
                .ws = BSP_NS4168_LRCLK_PIN,
                .dout = BSP_NS4168_SDATA_PIN,
                .din = I2S_GPIO_UNUSED,
                .mclk = I2S_GPIO_UNUSED,
                .invert_flags = {
                    .mclk_inv = false,
                    .bclk_inv = false,
                    .ws_inv = false,
                },
            },
        };
        ESP_ERROR_CHECK(i2s_channel_init_std_mode(i2s_tx_chan, &std_cfg));
        ESP_ERROR_CHECK(i2s_channel_enable(i2s_tx_chan));
        ESP_LOGI(TAG, "NS4168 I2S扬声器初始化完成（48kHz，单声道）");
    }

    // NS4168不需要I2C配置和codec芯片，无法使用esp_codec_dev API
    // 用户应该直接使用i2s_channel_write来播放音频
    // 示例：i2s_channel_write(i2s_tx_chan, buffer, size, &bytes_written, timeout);
    ESP_LOGW(TAG, "NS4168不支持esp_codec_dev API，请直接使用i2s_channel_write");
    return NULL;
}

esp_codec_dev_handle_t bsp_audio_codec_microphone_init(void)
{
    // MSM261D使用PDM模式，不需要codec芯片
    // 初始化PDM麦克风通道（MSM261D使用I2S_NUM_0）
    if (i2s_rx_chan == NULL) {
        i2s_chan_config_t rx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
        ESP_ERROR_CHECK(i2s_new_channel(&rx_chan_cfg, NULL, &i2s_rx_chan));

        // PDM模式配置（MSM261D是PDM麦克风）
        // PDM时钟频率通常是采样率的64倍（16kHz * 64 = 1.024MHz）
        i2s_pdm_rx_config_t pdm_rx_cfg = {
            .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(16000),  // 16kHz采样率
            .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
            .gpio_cfg = {
                .clk = BSP_PDM_CLK,
                .din = BSP_PDM_DATA,
                .invert_flags = {
                    .clk_inv = false,
                },
            },
        };
        
        ESP_ERROR_CHECK(i2s_channel_init_pdm_rx_mode(i2s_rx_chan, &pdm_rx_cfg));
        ESP_LOGI(TAG, "MSM261D PDM麦克风初始化完成（I2S_NUM_0，16kHz，单声道）");
        // 注意：不在初始化时启用，而是在需要录音时启用（通过i2s_channel_enable）
    }

    // MSM261D是PDM麦克风，不需要codec芯片，无法使用esp_codec_dev API
    // 用户应该直接使用i2s_channel_read来读取数据
    // 示例：i2s_channel_enable(i2s_rx_chan); i2s_channel_read(i2s_rx_chan, buffer, size, &bytes_read, timeout);
    ESP_LOGW(TAG, "MSM261D PDM麦克风不支持esp_codec_dev API，请直接使用i2s_channel_read");
    return NULL;
}

i2s_chan_handle_t bsp_audio_get_mic_i2s_chan(void)
{
    return i2s_rx_chan;
}

i2s_chan_handle_t bsp_audio_get_speaker_i2s_chan(void)
{
    return i2s_tx_chan;
}

// Bit number used to represent command and parameter
#define LCD_LEDC_CH            CONFIG_BSP_DISPLAY_BRIGHTNESS_LEDC_CH

esp_err_t bsp_display_brightness_init(void)
{
    // Setup LEDC peripheral for PWM backlight control
    const ledc_channel_config_t LCD_backlight_channel = {
        .gpio_num = BSP_LCD_BACKLIGHT,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LCD_LEDC_CH,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = 1,
        .duty = 0,
        .hpoint = 0
    };
    const ledc_timer_config_t LCD_backlight_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = 1,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };

    BSP_ERROR_CHECK_RETURN_ERR(ledc_timer_config(&LCD_backlight_timer));
    BSP_ERROR_CHECK_RETURN_ERR(ledc_channel_config(&LCD_backlight_channel));
    return ESP_OK;
}

esp_err_t bsp_display_brightness_set(int brightness_percent)
{
    if (brightness_percent > 100) {
        brightness_percent = 100;
    }
    if (brightness_percent < 0) {
        brightness_percent = 0;
    }

    ESP_LOGI(TAG, "Setting LCD backlight: %d%%", brightness_percent);
    uint32_t duty_cycle = (1023 * brightness_percent) / 100; // LEDC resolution set to 10bits, thus: 100% = 1023
    BSP_ERROR_CHECK_RETURN_ERR(ledc_set_duty(LEDC_LOW_SPEED_MODE, LCD_LEDC_CH, duty_cycle));
    BSP_ERROR_CHECK_RETURN_ERR(ledc_update_duty(LEDC_LOW_SPEED_MODE, LCD_LEDC_CH));
    return ESP_OK;
}

esp_err_t bsp_display_backlight_off(void)
{
    return bsp_display_brightness_set(0);
}

esp_err_t bsp_display_backlight_on(void)
{
    return bsp_display_brightness_set(100);
}

static esp_err_t bsp_enable_dsi_phy_power(void)
{
#if BSP_MIPI_DSI_PHY_PWR_LDO_CHAN > 0
    // Turn on the power for MIPI DSI PHY, so it can go from "No Power" state to "Shutdown" state
    static esp_ldo_channel_handle_t phy_pwr_chan = NULL;
    esp_ldo_channel_config_t ldo_cfg = {
        .chan_id = BSP_MIPI_DSI_PHY_PWR_LDO_CHAN,
        .voltage_mv = BSP_MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV,
    };
    ESP_RETURN_ON_ERROR(esp_ldo_acquire_channel(&ldo_cfg, &phy_pwr_chan), TAG, "Acquire LDO channel for DPHY failed");
    ESP_LOGI(TAG, "MIPI DSI PHY Powered on");
#endif // BSP_MIPI_DSI_PHY_PWR_LDO_CHAN > 0

    return ESP_OK;
}

esp_err_t bsp_display_new(const bsp_display_config_t *config, esp_lcd_panel_handle_t *ret_panel, esp_lcd_panel_io_handle_t *ret_io)
{
    esp_err_t ret = ESP_OK;
    bsp_lcd_handles_t handles;
    ret = bsp_display_new_with_handles(config, &handles);

    *ret_panel = handles.panel;
    *ret_io = handles.io;

    return ret;
}

#if DISPLAY_JD9165
static const jd9165_lcd_init_cmd_t lcd_init_cmds[] = { 
    //  {cmd, { data }, data_size, delay_ms}
    {0x30, (uint8_t []){0x00}, 1, 0},
    {0xF7, (uint8_t []){0x49, 0x61, 0x02, 0x00}, 4, 0},
    {0x30, (uint8_t []){0x01}, 1, 0},
    {0x04, (uint8_t []){0x0C}, 1, 0},
    {0x05, (uint8_t []){0x08}, 1, 0},
    {0x0B, (uint8_t []){0x11}, 1, 0}, //0x11(2lanes),0x12(3lanes),0x13(4lanes)
    {0x20, (uint8_t []){0x04}, 1, 0}, //r_lansel_sel_reg  //A2 add
    {0x1F, (uint8_t []){0x00}, 1, 0},  //mipi_hs_settle  //0x05->0x00 (P7_r01=04)
    {0x23, (uint8_t []){0x38}, 1, 0},
    {0x28, (uint8_t []){0x18}, 1, 0},
    {0x29, (uint8_t []){0x29}, 1, 0},
    {0x2A, (uint8_t []){0x01}, 1, 0},
    {0x2B, (uint8_t []){0x29}, 1, 0},
    {0x2C, (uint8_t []){0x01}, 1, 0},
    {0x30, (uint8_t []){0x02}, 1, 0},
    {0x00, (uint8_t []){0x05}, 1, 0},
    {0x01, (uint8_t []){0x22}, 1, 0},
    {0x02, (uint8_t []){0x08}, 1, 0},
    {0x03, (uint8_t []){0x12}, 1, 0},
    {0x04, (uint8_t []){0x16}, 1, 0},
    {0x05, (uint8_t []){0x64}, 1, 0},
    {0x06, (uint8_t []){0x00}, 1, 0},
    {0x07, (uint8_t []){0x00}, 1, 0},
    {0x08, (uint8_t []){0x78}, 1, 0},
    {0x09, (uint8_t []){0x00}, 1, 0},
    {0x0A, (uint8_t []){0x04}, 1, 0},
    {0x0B, (uint8_t []){0x16,0x17,0x0B,0x0D,0x0D,0x0D,0x11,0x10,0x07,0x07,0x09}, 11, 0},
    {0x0C, (uint8_t []){0x09,0x1E,0x1E,0x1C,0x1C,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D}, 11, 0},
    {0x0D, (uint8_t []){0x0A,0x05,0x0B,0x0D,0x0D,0x0D,0x11,0x10,0x06,0x06,0x08}, 11, 0},
    {0x0E, (uint8_t []){0x08,0x1F,0x1F,0x1D,0x1D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D}, 11, 0},
    {0x0F, (uint8_t []){0x0A,0x05,0x0D,0x0B,0x0D,0x0D,0x11,0x10,0x1D,0x1D,0x1F}, 11, 0},
    {0x10, (uint8_t []){0x1F,0x08,0x08,0x06,0x06,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D}, 11, 0},
    {0x11, (uint8_t []){0x16,0x17,0x0D,0x0B,0x0D,0x0D,0x11,0x10,0x1C,0x1C,0x1E}, 11, 0},
    {0x12, (uint8_t []){0x1E,0x09,0x09,0x07,0x07,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D}, 11, 0},
    {0x13, (uint8_t []){0x00,0x00,0x00,0x00}, 4, 0},
    {0x14, (uint8_t []){0x00,0x00,0x41,0x41}, 4, 0},
    {0x15, (uint8_t []){0x00,0x00,0x00,0x00}, 4, 0},
    {0x17, (uint8_t []){0x00}, 1, 0},
    {0x18, (uint8_t []){0x85}, 1, 0},
    {0x19, (uint8_t []){0x06,0x09}, 2, 0},
    {0x1A, (uint8_t []){0x05,0x08}, 2, 0},
    {0x1B, (uint8_t []){0x0A,0x04}, 2, 0},
    {0x26, (uint8_t []){0x00}, 1, 0},
    {0x27, (uint8_t []){0x00}, 1, 0},
    {0x30, (uint8_t []){0x06}, 1, 0},
    {0x12, (uint8_t []){0x3F,0x26,0x27,0x35,0x2D,0x34,0x3F,0x3F,0x3F,0x35,0x2A,0x20,0x16,0x08}, 14, 0},
    {0x13, (uint8_t []){0x3F,0x26,0x28,0x35,0x27,0x29,0x29,0x2F,0x35,0x2F,0x26,0x20,0x16,0x08}, 14, 0},
    {0x30, (uint8_t []){0x0A}, 1, 0},
    {0x02, (uint8_t []){0x4F}, 1, 0},
    {0x0B, (uint8_t []){0x40}, 1, 0},
    {0x30, (uint8_t []){0x0D}, 1, 0},
    {0x0D, (uint8_t []){0x04}, 1, 0}, //mipi add  //0x0C, 0x04
    {0x10, (uint8_t []){0x0C}, 1, 0},
    {0x11, (uint8_t []){0x0C}, 1, 0},
    {0x12, (uint8_t []){0x0C}, 1, 0},
    {0x13, (uint8_t []){0x0C}, 1, 0},
    {0x30, (uint8_t []){0x00}, 1, 0},
    {0x11, (uint8_t []){0x00}, 0, 120},
    {0x29, (uint8_t []){0x00}, 0, 20},
};
#endif

esp_err_t bsp_display_new_with_handles(const bsp_display_config_t *config, bsp_lcd_handles_t *ret_handles)
{
    esp_err_t ret = ESP_OK;

    gpio_config_t bl_io_conf = {
        .pin_bit_mask = (1ULL << BSP_BL_LCD1V8) | (1ULL << BSP_BL_LCD3V3),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&bl_io_conf);
    
    // GPIO4-EN-LCM1V8=1 delay 30ms
    gpio_set_level(BSP_BL_LCD1V8, 1);
    vTaskDelay(pdMS_TO_TICKS(30));
    gpio_set_level(BSP_BL_LCD3V3, 1);

    ESP_RETURN_ON_ERROR(bsp_display_brightness_init(), TAG, "Brightness init failed");
    ESP_RETURN_ON_ERROR(bsp_enable_dsi_phy_power(), TAG, "DSI PHY power failed");

    /* create MIPI DSI bus first, it will initialize the DSI PHY as well */
    esp_lcd_dsi_bus_handle_t mipi_dsi_bus;
    esp_lcd_dsi_bus_config_t bus_config = {
        .bus_id = 0,
        .num_data_lanes = BSP_LCD_MIPI_DSI_LANE_NUM,
        .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
        .lane_bit_rate_mbps = BSP_LCD_MIPI_DSI_LANE_BITRATE_MBPS,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus), TAG, "New DSI bus init failed");

    ESP_LOGI(TAG, "Install MIPI DSI LCD control panel");
    // we use DBI interface to send LCD commands and parameters
    esp_lcd_panel_io_handle_t io;
    esp_lcd_dbi_io_config_t dbi_config = {
        .virtual_channel = 0,
        .lcd_cmd_bits = 8,   // according to the LCD EK79007 spec
        .lcd_param_bits = 8, // according to the LCD EK79007 spec
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_config, &io), err, TAG, "New panel IO failed");

    // create EK79007 control panel
    ESP_LOGI(TAG, "Install EK79007 LCD control panel");
    esp_lcd_panel_handle_t disp_panel = NULL;

#if CONFIG_BSP_LCD_COLOR_FORMAT_RGB888
#if DISPLAY_JD9165
    esp_lcd_dpi_panel_config_t dpi_config = JD9165_1024_600_PANEL_60HZ_DPI_CONFIG(LCD_COLOR_PIXEL_FORMAT_RGB888);
#else
    esp_lcd_dpi_panel_config_t dpi_config = EK79007_1024_600_PANEL_60HZ_CONFIG(LCD_COLOR_PIXEL_FORMAT_RGB888);
#endif
#else
#if DISPLAY_JD9165
    esp_lcd_dpi_panel_config_t dpi_config = JD9165_1024_600_PANEL_60HZ_DPI_CONFIG(LCD_COLOR_PIXEL_FORMAT_RGB565);
#else
    esp_lcd_dpi_panel_config_t dpi_config = EK79007_1024_600_PANEL_60HZ_CONFIG(LCD_COLOR_PIXEL_FORMAT_RGB565);
#endif
#endif
    dpi_config.num_fbs = CONFIG_BSP_LCD_DPI_BUFFER_NUMS;

#if DISPLAY_JD9165
    jd9165_vendor_config_t vendor_config = {
        .init_cmds = lcd_init_cmds,
        .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(jd9165_lcd_init_cmd_t),
        .mipi_config = {
            .dsi_bus = mipi_dsi_bus,
            .dpi_config = &dpi_config,
        },
    };
#else
    ek79007_vendor_config_t vendor_config = {
        .mipi_config = {
            .dsi_bus = mipi_dsi_bus,
            .dpi_config = &dpi_config,
        },
    };
#endif
    esp_lcd_panel_dev_config_t lcd_dev_config = {
        .bits_per_pixel = 16,
        .rgb_ele_order = BSP_LCD_COLOR_SPACE,
        .reset_gpio_num = BSP_LCD_RST,
        .vendor_config = &vendor_config,
    };
#if DISPLAY_JD9165
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_jd9165(io, &lcd_dev_config, &disp_panel), err, TAG, "New LCD panel EK79007 failed");
#else
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_ek79007(io, &lcd_dev_config, &disp_panel), err, TAG, "New LCD panel EK79007 failed");
#endif
    ESP_GOTO_ON_ERROR(esp_lcd_panel_reset(disp_panel), err, TAG, "LCD panel reset failed");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_init(disp_panel), err, TAG, "LCD panel init failed");

    /* Return all handles */
    ret_handles->io = io;
    ret_handles->mipi_dsi_bus = mipi_dsi_bus;
    ret_handles->panel = disp_panel;
    ret_handles->control = NULL;

    ESP_LOGI(TAG, "Display initialized");

    return ret;

err:
    if (disp_panel) {
        esp_lcd_panel_del(disp_panel);
    }
    if (io) {
        esp_lcd_panel_io_del(io);
    }
    if (mipi_dsi_bus) {
        esp_lcd_del_dsi_bus(mipi_dsi_bus);
    }
    return ret;
}

esp_err_t bsp_touch_new(const bsp_touch_config_t *config, esp_lcd_touch_handle_t *ret_touch)
{
    /* Initilize I2C */
    BSP_ERROR_CHECK_RETURN_ERR(bsp_i2c_init());

    /* Initialize touch */
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = BSP_LCD_H_RES,
        .y_max = BSP_LCD_V_RES,
        .rst_gpio_num = BSP_LCD_TOUCH_RST, // Shared with LCD reset
        .int_gpio_num = BSP_LCD_TOUCH_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
#if CONFIG_BSP_LCD_TYPE_1024_600
            .mirror_x = 0,
            .mirror_y = 0,
#else
            .mirror_x = 0,
            .mirror_y = 0,
#endif
        },
    };
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    tp_io_config.scl_speed_hz = CONFIG_BSP_I2C_CLK_SPEED_HZ;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(i2c_handle, &tp_io_config, &tp_io_handle), TAG, "");
    return esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, ret_touch);
}

#if (BSP_CONFIG_NO_GRAPHIC_LIB == 0)
static lv_display_t *bsp_display_lcd_init(const bsp_display_cfg_t *cfg)
{
    assert(cfg != NULL);
    bsp_lcd_handles_t lcd_panels;
    BSP_ERROR_CHECK_RETURN_NULL(bsp_display_new_with_handles(NULL, &lcd_panels));

    /* Add LCD screen */
    ESP_LOGD(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = lcd_panels.io,
        .panel_handle = lcd_panels.panel,
        .control_handle = lcd_panels.control,
        .buffer_size = cfg->buffer_size,
        .double_buffer = cfg->double_buffer,
        .hres = BSP_LCD_H_RES,
        .vres = BSP_LCD_V_RES,
        .monochrome = false,
        /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
        .rotation = {
            .swap_xy = false,
            .mirror_x = true,
            .mirror_y = true,
        },
#if LVGL_VERSION_MAJOR >= 9
#if CONFIG_BSP_LCD_COLOR_FORMAT_RGB888
        .color_format = LV_COLOR_FORMAT_RGB888,
#else
        .color_format = LV_COLOR_FORMAT_RGB565,
#endif
#endif
        .flags = {
            .buff_dma = cfg->flags.buff_dma,
            .buff_spiram = cfg->flags.buff_spiram,
#if LVGL_VERSION_MAJOR >= 9
            .swap_bytes = (BSP_LCD_BIGENDIAN ? true : false),
#endif
#if CONFIG_BSP_DISPLAY_LVGL_AVOID_TEAR
            .sw_rotate = false,                /* Avoid tearing is not supported for SW rotation */
#else
            .sw_rotate = cfg->flags.sw_rotate, /* Only SW rotation is supported for 90° and 270° */
#endif
#if CONFIG_BSP_DISPLAY_LVGL_FULL_REFRESH
            .full_refresh = true,
#elif CONFIG_BSP_DISPLAY_LVGL_DIRECT_MODE
            .direct_mode = true,
#endif
        }
    };

    const lvgl_port_display_dsi_cfg_t dpi_cfg = {
        .flags = {
#if CONFIG_BSP_DISPLAY_LVGL_AVOID_TEAR
            .avoid_tearing = true,
#else
            .avoid_tearing = false,
#endif
        }
    };

    return lvgl_port_add_disp_dsi(&disp_cfg, &dpi_cfg);
}

static lv_indev_t *bsp_display_indev_init(lv_display_t *disp)
{
    esp_lcd_touch_handle_t tp;
    BSP_ERROR_CHECK_RETURN_NULL(bsp_touch_new(NULL, &tp));
    assert(tp);

    /* Add touch input (for selected screen) */
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp,
        .handle = tp,
    };

    return lvgl_port_add_touch(&touch_cfg);
}

lv_display_t *bsp_display_start(void)
{
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_DRAW_BUFF_SIZE,
        .double_buffer = BSP_LCD_DRAW_BUFF_DOUBLE,
        .flags = {
#if CONFIG_BSP_LCD_COLOR_FORMAT_RGB888
            .buff_dma = false,
#else
            .buff_dma = true,
#endif
            .buff_spiram = false,
            .sw_rotate = true,
        }
    };
    return bsp_display_start_with_config(&cfg);
}

lv_display_t *bsp_display_start_with_config(const bsp_display_cfg_t *cfg)
{
    lv_display_t *disp;

    assert(cfg != NULL);
    BSP_ERROR_CHECK_RETURN_NULL(lvgl_port_init(&cfg->lvgl_port_cfg));

    BSP_ERROR_CHECK_RETURN_NULL(bsp_display_brightness_init());

    BSP_NULL_CHECK(disp = bsp_display_lcd_init(cfg), NULL);

    BSP_NULL_CHECK(disp_indev = bsp_display_indev_init(disp), NULL);

    return disp;
}

lv_indev_t *bsp_display_get_input_dev(void)
{
    return disp_indev;
}

void bsp_display_rotate(lv_display_t *disp, lv_disp_rotation_t rotation)
{
    lv_disp_set_rotation(disp, rotation);
}

bool bsp_display_lock(uint32_t timeout_ms)
{
    return lvgl_port_lock(timeout_ms);
}

void bsp_display_unlock(void)
{
    lvgl_port_unlock();
}

#endif // (BSP_CONFIG_NO_GRAPHIC_LIB == 0)

static void usb_lib_task(void *arg)
{
    while (1) {
        // Start handling system events
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            ESP_ERROR_CHECK(usb_host_device_free_all());
        }
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
            ESP_LOGI(TAG, "USB: All devices freed");
            // Continue handling USB events to allow device reconnection
            // The only way this task can be stopped is by calling bsp_usb_host_stop()
        }
    }
}

esp_err_t bsp_usb_host_start(bsp_usb_host_power_mode_t mode, bool limit_500mA)
{
    //Install USB Host driver. Should only be called once in entire application
    ESP_LOGI(TAG, "Installing USB Host");
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };
    BSP_ERROR_CHECK_RETURN_ERR(usb_host_install(&host_config));

    // Create a task that will handle USB library events
    if (xTaskCreate(usb_lib_task, "usb_lib", 4096, NULL, 10, &usb_host_task) != pdTRUE) {
        ESP_LOGE(TAG, "Creating USB host lib task failed");
        abort();
    }

    return ESP_OK;
}

esp_err_t bsp_usb_host_stop(void)
{
    usb_host_uninstall();
    if (usb_host_task) {
        vTaskSuspend(usb_host_task);
        vTaskDelete(usb_host_task);
    }
    return ESP_OK;
}
