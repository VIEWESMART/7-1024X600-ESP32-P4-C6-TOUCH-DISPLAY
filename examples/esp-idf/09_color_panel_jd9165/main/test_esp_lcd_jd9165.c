/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "soc/soc_caps.h"

#if SOC_MIPI_DSI_SUPPORTED

#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_ldo_regulator.h"
#include "esp_dma_utils.h"
#include "unity.h"
#include "unity_test_runner.h"
#include "unity_test_utils_memory.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_jd9165.h"

#define TEST_LCD_H_RES                  (1024)
#define TEST_LCD_V_RES                  (600)
#define TEST_LCD_BIT_PER_PIXEL          (24)
#define TEST_PIN_NUM_LCD_RST            (-1)
#define TEST_PIN_NUM_BK_LIGHT           (-1)    // set to -1 if not used
#define TEST_LCD_BK_LIGHT_ON_LEVEL      (1)
#define TEST_LCD_BK_LIGHT_OFF_LEVEL     !TEST_LCD_BK_LIGHT_ON_LEVEL

#if TEST_LCD_BIT_PER_PIXEL == 24
#define TEST_MIPI_DPI_PX_FORMAT         (LCD_COLOR_PIXEL_FORMAT_RGB888)
#elif TEST_LCD_BIT_PER_PIXEL == 18
#define TEST_MIPI_DPI_PX_FORMAT         (LCD_COLOR_PIXEL_FORMAT_RGB666)
#elif TEST_LCD_BIT_PER_PIXEL == 16
#define TEST_MIPI_DPI_PX_FORMAT         (LCD_COLOR_PIXEL_FORMAT_RGB565)
#endif

#define TEST_DELAY_TIME_MS                      (3000)

#define TEST_MIPI_DSI_PHY_PWR_LDO_CHAN          (3)
#define TEST_MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV    (2500)

static char *TAG = "jd9165_test";
static esp_ldo_channel_handle_t ldo_mipi_phy = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_dsi_bus_handle_t mipi_dsi_bus = NULL;
static esp_lcd_panel_io_handle_t mipi_dbi_io = NULL;
static SemaphoreHandle_t refresh_finish = NULL;

IRAM_ATTR static bool test_notify_refresh_ready(esp_lcd_panel_handle_t panel, esp_lcd_dpi_panel_event_data_t *edata, void *user_ctx)
{
    SemaphoreHandle_t refresh_finish = (SemaphoreHandle_t)user_ctx;
    BaseType_t need_yield = pdFALSE;

    xSemaphoreGiveFromISR(refresh_finish, &need_yield);

    return (need_yield == pdTRUE);
}

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

static void test_init_lcd(void)
{
#if TEST_PIN_NUM_BK_LIGHT >= 0
    ESP_LOGI(TAG, "Turn on LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << TEST_PIN_NUM_BK_LIGHT
    };
    TEST_ESP_OK(gpio_config(&bk_gpio_config));
    TEST_ESP_OK(gpio_set_level(TEST_PIN_NUM_BK_LIGHT, TEST_LCD_BK_LIGHT_ON_LEVEL));
#endif

    // Turn on the power for MIPI DSI PHY, so it can go from "No Power" state to "Shutdown" state
#ifdef TEST_MIPI_DSI_PHY_PWR_LDO_CHAN
    ESP_LOGI(TAG, "MIPI DSI PHY Powered on");
    esp_ldo_channel_config_t ldo_mipi_phy_config = {
        .chan_id = TEST_MIPI_DSI_PHY_PWR_LDO_CHAN,
        .voltage_mv = TEST_MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV,
    };
    TEST_ESP_OK(esp_ldo_acquire_channel(&ldo_mipi_phy_config, &ldo_mipi_phy));
#endif

    ESP_LOGI(TAG, "Initialize MIPI DSI bus");
    esp_lcd_dsi_bus_config_t bus_config = JD9165_PANEL_BUS_DSI_2CH_CONFIG();
    TEST_ESP_OK(esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_dbi_io_config_t dbi_config = JD9165_PANEL_IO_DBI_CONFIG();
    TEST_ESP_OK(esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_config, &mipi_dbi_io));

    ESP_LOGI(TAG, "Install LCD driver of jd9165");
    esp_lcd_dpi_panel_config_t dpi_config = JD9165_1024_600_PANEL_60HZ_DPI_CONFIG(TEST_MIPI_DPI_PX_FORMAT);
    jd9165_vendor_config_t vendor_config = {
        .init_cmds = lcd_init_cmds,
        .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(jd9165_lcd_init_cmd_t),
        .mipi_config = {
            .dsi_bus = mipi_dsi_bus,
            .dpi_config = &dpi_config,
        },
    };
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = TEST_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = TEST_LCD_BIT_PER_PIXEL,
        .vendor_config = &vendor_config,
    };
    TEST_ESP_OK(esp_lcd_new_panel_jd9165(mipi_dbi_io, &panel_config, &panel_handle));
    TEST_ESP_OK(esp_lcd_panel_init(panel_handle));

    refresh_finish = xSemaphoreCreateBinary();
    TEST_ASSERT_NOT_NULL(refresh_finish);
    esp_lcd_dpi_panel_event_callbacks_t cbs = {
        .on_color_trans_done = test_notify_refresh_ready,
    };
    TEST_ESP_OK(esp_lcd_dpi_panel_register_event_callbacks(panel_handle, &cbs, refresh_finish));
}

static void test_deinit_lcd(void)
{
    TEST_ESP_OK(esp_lcd_panel_del(panel_handle));
    TEST_ESP_OK(esp_lcd_panel_io_del(mipi_dbi_io));
    TEST_ESP_OK(esp_lcd_del_dsi_bus(mipi_dsi_bus));
    panel_handle = NULL;
    mipi_dbi_io = NULL;
    mipi_dsi_bus = NULL;

    if (ldo_mipi_phy) {
        TEST_ESP_OK(esp_ldo_release_channel(ldo_mipi_phy));
        ldo_mipi_phy = NULL;
    }

    vSemaphoreDelete(refresh_finish);
    refresh_finish = NULL;

#if TEST_PIN_NUM_BK_LIGHT >= 0
    TEST_ESP_OK(gpio_reset_pin(TEST_PIN_NUM_BK_LIGHT));
#endif
}

static void test_draw_color_bar(esp_lcd_panel_handle_t panel_handle, uint16_t h_res, uint16_t v_res)
{
    uint8_t byte_per_pixel = (TEST_LCD_BIT_PER_PIXEL + 7) / 8;
    uint16_t row_line = v_res / byte_per_pixel / 8;
    uint8_t *color = (uint8_t *)heap_caps_calloc(1, row_line * h_res * byte_per_pixel, MALLOC_CAP_DMA);

    for (int j = 0; j < byte_per_pixel * 8; j++) {
        for (int i = 0; i < row_line * h_res; i++) {
            for (int k = 0; k < byte_per_pixel; k++) {
                color[i * byte_per_pixel + k] = (BIT(j) >> (k * 8)) & 0xff;
            }
        }
        TEST_ESP_OK(esp_lcd_panel_draw_bitmap(panel_handle, 0, j * row_line, h_res, (j + 1) * row_line, color));
        xSemaphoreTake(refresh_finish, portMAX_DELAY);
    }

    uint16_t color_line = row_line * byte_per_pixel * 8;
    uint16_t res_line = v_res - color_line;
    if (res_line) {
        for (int i = 0; i < res_line * h_res; i++) {
            for (int k = 0; k < byte_per_pixel; k++) {
                color[i * byte_per_pixel + k] = 0xff;
            }
        }
        TEST_ESP_OK(esp_lcd_panel_draw_bitmap(panel_handle, 0, color_line, h_res, v_res, color));
        xSemaphoreTake(refresh_finish, portMAX_DELAY);
    }

    free(color);
}

TEST_CASE("test jd9165 to draw pattern with MIPI interface", "[jd9165][draw_pattern]")
{
    ESP_LOGI(TAG, "Initialize LCD device");
    test_init_lcd();

    ESP_LOGI(TAG, "Show color bar pattern drawn by hardware");
    TEST_ESP_OK(esp_lcd_dpi_panel_set_pattern(panel_handle, MIPI_DSI_PATTERN_BAR_VERTICAL));
    vTaskDelay(pdMS_TO_TICKS(TEST_DELAY_TIME_MS));
    TEST_ESP_OK(esp_lcd_dpi_panel_set_pattern(panel_handle, MIPI_DSI_PATTERN_BAR_HORIZONTAL));
    vTaskDelay(pdMS_TO_TICKS(TEST_DELAY_TIME_MS));
    TEST_ESP_OK(esp_lcd_dpi_panel_set_pattern(panel_handle, MIPI_DSI_PATTERN_NONE));

    ESP_LOGI(TAG, "Deinitialize LCD device");
    test_deinit_lcd();
}

TEST_CASE("test jd9165 to draw color bar with MIPI interface", "[jd9165][draw_color_bar]")
{
    ESP_LOGI(TAG, "Initialize LCD device");
    test_init_lcd();

    ESP_LOGI(TAG, "Show color bar drawn by software");
    test_draw_color_bar(panel_handle, TEST_LCD_H_RES, TEST_LCD_V_RES);
    vTaskDelay(pdMS_TO_TICKS(TEST_DELAY_TIME_MS));

    ESP_LOGI(TAG, "Deinitialize LCD device");
    test_deinit_lcd();
}

TEST_CASE("test jd9165 to rotate with MIPI interface", "[jd9165][rotate]")
{
    ESP_LOGI(TAG, "Initialize LCD device");
    test_init_lcd();

    ESP_LOGI(TAG, "Mirror the screen");
    for (size_t i = 0; i < 4; i++) {
        TEST_ASSERT_NOT_EQUAL(esp_lcd_panel_mirror(panel_handle, i & 2, i & 1), ESP_FAIL);

        ESP_LOGI(TAG, "Mirror: %d", i);
        test_draw_color_bar(panel_handle, TEST_LCD_H_RES, TEST_LCD_V_RES);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI(TAG, "Deinitialize LCD device");
    test_deinit_lcd();
}

// Some resources are lazy allocated in the LCD driver, the threadhold is left for that case
#define TEST_MEMORY_LEAK_THRESHOLD  (300)

static size_t before_free_8bit;
static size_t before_free_32bit;

void setUp(void)
{
    before_free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    before_free_32bit = heap_caps_get_free_size(MALLOC_CAP_32BIT);
}

void tearDown(void)
{
    size_t after_free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t after_free_32bit = heap_caps_get_free_size(MALLOC_CAP_32BIT);
    unity_utils_check_leak(before_free_8bit, after_free_8bit, "8BIT", TEST_MEMORY_LEAK_THRESHOLD);
    unity_utils_check_leak(before_free_32bit, after_free_32bit, "32BIT", TEST_MEMORY_LEAK_THRESHOLD);
}

void app_main(void)
{
    /**
    *      _ ____   ___  _  __  ____
    *     | |  _ \ / _ \/ |/ /_| ___|
    *  _  | | | | | (_) | | '_ \___ \
    * | |_| | |_| |\__, | | (_) |__) |
    *  \___/|____/   /_/|_|\___/____/
    */
    printf("      _ ____   ___  _  __  ____  \r\n");
    printf("     | |  _ \\ / _ \\/ |/ /_| ___| \r\n");
    printf("  _  | | | | | (_) | | '_ \\___ \\ \r\n");
    printf(" | |_| | |_| |\\__, | | (_) |__) |\r\n");
    printf(" \\___/|____/   /_/|_|\\___/____/ \r\n");
    // unity_run_menu();

    test_init_lcd();

    ESP_LOGI(TAG, "Show color bar pattern drawn by hardware");
    TEST_ESP_OK(esp_lcd_dpi_panel_set_pattern(panel_handle, MIPI_DSI_PATTERN_BAR_VERTICAL));
}
#endif
