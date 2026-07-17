/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file   BOARD_VIEWE_SMARTRING_PLUS_B.h
 * @brief  Configuration file for Viewe SmartRing-Plus
 * @author Viewe@VIEWESMART
 * @link   https://github.com/VIEWESMART/SmartRing-Plus
 */

#pragma once

// *INDENT-OFF*

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// Please update the following macros to configure general panel /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Board name
 */
#define ESP_PANEL_BOARD_NAME                "Viewe:SMARTRING-PLUS-B"

/**
 * @brief Panel resolution configuration in pixels
 */
#define ESP_PANEL_BOARD_WIDTH               (360)   // Panel width (horizontal, in pixels)
#define ESP_PANEL_BOARD_HEIGHT              (360)   // Panel height (vertical, in pixels)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// Please update the following macros to configure the LCD panel /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief LCD panel configuration flag (0/1)
 *
 * Set to `1` to enable LCD panel support, `0` to disable
 */
#define ESP_PANEL_BOARD_USE_LCD             (1)

#if ESP_PANEL_BOARD_USE_LCD
/**
 * @brief LCD controller selection
 */
#define ESP_PANEL_BOARD_LCD_CONTROLLER      ST77916

/**
 * @brief LCD bus type selection
 *
 * Supported bus types:
 * - `ESP_PANEL_BUS_TYPE_SPI`
 * - `ESP_PANEL_BUS_TYPE_QSPI`
 * - `ESP_PANEL_BUS_TYPE_RGB` (ESP32-S3 only)
 * - `ESP_PANEL_BUS_TYPE_MIPI_DSI` (ESP32-P4 only)
 */
#define ESP_PANEL_BOARD_LCD_BUS_TYPE        (ESP_PANEL_BUS_TYPE_QSPI)

#if (ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_SPI) || \
    (ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_QSPI)
/**
 * If set to 1, the bus will skip to initialize the corresponding host. Users need to initialize the host in advance.
 *
 * For drivers which created by this library, even if they use the same host, the host will be initialized only once.
 * So it is not necessary to set the macro to `1`. For other drivers (like `Wire`), please set the macro to `1`
 * ensure that the host is initialized only once.
 */
#define ESP_PANEL_BOARD_LCD_BUS_SKIP_INIT_HOST      (0)     // 0/1. Typically set to 0
#endif

/**
 * @brief LCD bus parameters configuration
 *
 * Configure parameters based on the selected bus type. Parameters for other bus types will be ignored.
 * For detailed parameter explanations, see:
 * https://docs.espressif.com/projects/esp-idf/en/v5.3.1/esp32s3/api-reference/peripherals/lcd/index.html
 * https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/lcd/index.html
 */
#if ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_QSPI

/**
 * @brief QSPI bus
 */
/* For general */
#define ESP_PANEL_BOARD_LCD_QSPI_HOST_ID        (1)     // Typically set to 1
#if !ESP_PANEL_BOARD_LCD_BUS_SKIP_INIT_HOST
/* For host */
#define ESP_PANEL_BOARD_LCD_QSPI_IO_SCK         (10)
#define ESP_PANEL_BOARD_LCD_QSPI_IO_DATA0       (12)
#define ESP_PANEL_BOARD_LCD_QSPI_IO_DATA1       (13)
#define ESP_PANEL_BOARD_LCD_QSPI_IO_DATA2       (15)
#define ESP_PANEL_BOARD_LCD_QSPI_IO_DATA3       (14)
#endif // ESP_PANEL_BOARD_LCD_BUS_SKIP_INIT_HOST
/* For panel */
#define ESP_PANEL_BOARD_LCD_QSPI_IO_CS          (11)    // -1 if not used
// #define ESP_PANEL_BOARD_LCD_QSPI_IO_DC          (17)    // -1 if not used
#define ESP_PANEL_BOARD_LCD_QSPI_MODE           (0)     // 0-3, typically set to 0
#define ESP_PANEL_BOARD_LCD_QSPI_CLK_HZ         (40 * 1000 * 1000)
                                                        // Should be an integer divisor of 80M, typically set to 40M
#define ESP_PANEL_BOARD_LCD_QSPI_CMD_BITS       (32)    // Typically set to 32
#define ESP_PANEL_BOARD_LCD_QSPI_PARAM_BITS     (8)     // Typically set to 8

#endif // ESP_PANEL_BOARD_LCD_BUS_TYPE

/**
 * @brief LCD vendor initialization commands
 *
 * Vendor specific initialization can be different between manufacturers, should consult the LCD supplier for
 * initialization sequence code. Please uncomment and change the following macro definitions. Otherwise, the LCD driver
 * will use the default initialization sequence code.
 *
 * The initialization sequence can be specified in two formats:
 * 1. Raw format:
 *    {command, (uint8_t []){data0, data1, ...}, data_size, delay_ms}
 * 2. Helper macros:
 *    - ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(delay_ms, command, {data0, data1, ...})
 *    - ESP_PANEL_LCD_CMD_WITH_NONE_PARAM(delay_ms, command)
 */
#define ESP_PANEL_BOARD_LCD_VENDOR_INIT_CMD() \
{                                                                          \
    {0xF0, (uint8_t []){0x28}, 1, 0},\
    {0xF2, (uint8_t []){0x28}, 1, 0},\
    {0x73, (uint8_t []){0xF0}, 1, 0},\
    {0x76, (uint8_t []){0x0F}, 1, 0},\
    {0x7C, (uint8_t []){0xD1}, 1, 0},\
    {0x83, (uint8_t []){0xE0}, 1, 0},\
    {0x84, (uint8_t []){0x61}, 1, 0},\
    {0xF2, (uint8_t []){0x82}, 1, 0},\
    {0xF0, (uint8_t []){0x00}, 1, 0},\
    {0xF0, (uint8_t []){0x01}, 1, 0},\
    {0xF1, (uint8_t []){0x01}, 1, 0},\
    {0xB0, (uint8_t []){0x52}, 1, 0},\
    {0xB1, (uint8_t []){0x49}, 1, 0},\
    {0xB2, (uint8_t []){0x24}, 1, 0},\
    {0xB3, (uint8_t []){0x01}, 1, 0},\
    {0xB4, (uint8_t []){0x66}, 1, 0},\
    {0xB5, (uint8_t []){0x44}, 1, 0},\
    {0xB6, (uint8_t []){0xC5}, 1, 0},\
    {0xB7, (uint8_t []){0x40}, 1, 0},\
    {0xB8, (uint8_t []){0x86}, 1, 0},\
    {0xB9, (uint8_t []){0x15}, 1, 0},\
    {0xBA, (uint8_t []){0x00}, 1, 0},\
    {0xBB, (uint8_t []){0x08}, 1, 0},\
    {0xBC, (uint8_t []){0x08}, 1, 0},\
    {0xBD, (uint8_t []){0x00}, 1, 0},\
    {0xBE, (uint8_t []){0x00}, 1, 0},\
    {0xBF, (uint8_t []){0x07}, 1, 0},\
    {0xC0, (uint8_t []){0x80}, 1, 0},\
    {0xC1, (uint8_t []){0x10}, 1, 0},\
    {0xC2, (uint8_t []){0x37}, 1, 0},\
    {0xC3, (uint8_t []){0x80}, 1, 0},\
    {0xC4, (uint8_t []){0x10}, 1, 0},\
    {0xC5, (uint8_t []){0x37}, 1, 0},\
    {0xC6, (uint8_t []){0xA9}, 1, 0},\
    {0xC7, (uint8_t []){0x41}, 1, 0},\
    {0xC8, (uint8_t []){0x01}, 1, 0},\
    {0xC9, (uint8_t []){0xA9}, 1, 0},\
    {0xCA, (uint8_t []){0x41}, 1, 0},\
    {0xCB, (uint8_t []){0x01}, 1, 0},\
    {0xCC, (uint8_t []){0x7F}, 1, 0},\
    {0xCD, (uint8_t []){0x7F}, 1, 0},\
    {0xCE, (uint8_t []){0xFF}, 1, 0},\
    {0xD0, (uint8_t []){0x91}, 1, 0},\
    {0xD1, (uint8_t []){0x68}, 1, 0},\
    {0xD2, (uint8_t []){0x68}, 1, 0},\
    {0xF5, (uint8_t []){0x00, 0xA5}, 2, 0},\
    {0xF1, (uint8_t []){0x10}, 1, 0},\
    {0xF0, (uint8_t []){0x00}, 1, 0},\
    {0xF0, (uint8_t []){0x02}, 1, 0},\
    {0xE0, (uint8_t []){0xF0, 0x0E, 0x15, 0x0A, 0x0B, 0x07, 0x3E, 0x43, 0x53, 0x39, 0x17, 0x17, 0x35, 0x38}, 14, 0},\
    {0xE1, (uint8_t []){0xF0, 0x0D, 0x15, 0x0A, 0x0A, 0x06, 0x3D, 0x33, 0x52, 0x39, 0x16, 0x16, 0x35, 0x37}, 14, 0},\
    {0xF0, (uint8_t []){0x10}, 1, 0},\
    {0xF3, (uint8_t []){0x10}, 1, 0},\
    {0xE0, (uint8_t []){0x09}, 1, 0},\
    {0xE1, (uint8_t []){0x00}, 1, 0},\
    {0xE2, (uint8_t []){0x03}, 1, 0},\
    {0xE3, (uint8_t []){0x00}, 1, 0},\
    {0xE4, (uint8_t []){0xE0}, 1, 0},\
    {0xE5, (uint8_t []){0x06}, 1, 0},\
    {0xE6, (uint8_t []){0x21}, 1, 0},\
    {0xE7, (uint8_t []){0x00}, 1, 0},\
    {0xE8, (uint8_t []){0x05}, 1, 0},\
    {0xE9, (uint8_t []){0x82}, 1, 0},\
    {0xEA, (uint8_t []){0xDE}, 1, 0},\
    {0xEB, (uint8_t []){0xC0}, 1, 0},\
    {0xEC, (uint8_t []){0x40}, 1, 0},\
    {0xED, (uint8_t []){0x84}, 1, 0},\
    {0xEE, (uint8_t []){0xFF}, 1, 0},\
    {0xEF, (uint8_t []){0x71}, 1, 0},\
    {0xF8, (uint8_t []){0xFF}, 1, 0},\
    {0xF9, (uint8_t []){0x50}, 1, 0},\
    {0xFA, (uint8_t []){0xFF}, 1, 0},\
    {0xFB, (uint8_t []){0xF3}, 1, 0},\
    {0xFC, (uint8_t []){0x00}, 1, 0},\
    {0xFD, (uint8_t []){0x00}, 1, 0},\
    {0xFE, (uint8_t []){0x00}, 1, 0},\
    {0xFF, (uint8_t []){0x00}, 1, 0},\
    {0x60, (uint8_t []){0x42}, 1, 0},\
    {0x61, (uint8_t []){0xDF}, 1, 0},\
    {0x62, (uint8_t []){0x40}, 1, 0},\
    {0x63, (uint8_t []){0x40}, 1, 0},\
    {0x64, (uint8_t []){0x02}, 1, 0},\
    {0x65, (uint8_t []){0x00}, 1, 0},\
    {0x66, (uint8_t []){0x00}, 1, 0},\
    {0x67, (uint8_t []){0x00}, 1, 0},\
    {0x68, (uint8_t []){0x00}, 1, 0},\
    {0x69, (uint8_t []){0x00}, 1, 0},\
    {0x6A, (uint8_t []){0x00}, 1, 0},\
    {0x6B, (uint8_t []){0x00}, 1, 0},\
    {0x70, (uint8_t []){0x42}, 1, 0},\
    {0x71, (uint8_t []){0xDF}, 1, 0},\
    {0x72, (uint8_t []){0x40}, 1, 0},\
    {0x73, (uint8_t []){0x40}, 1, 0},\
    {0x74, (uint8_t []){0x01}, 1, 0},\
    {0x75, (uint8_t []){0x00}, 1, 0},\
    {0x76, (uint8_t []){0x00}, 1, 0},\
    {0x77, (uint8_t []){0x00}, 1, 0},\
    {0x78, (uint8_t []){0x00}, 1, 0},\
    {0x79, (uint8_t []){0x00}, 1, 0},\
    {0x7A, (uint8_t []){0x00}, 1, 0},\
    {0x7B, (uint8_t []){0x00}, 1, 0},\
    {0x80, (uint8_t []){0x48}, 1, 0},\
    {0x81, (uint8_t []){0x00}, 1, 0},\
    {0x82, (uint8_t []){0x04}, 1, 0},\
    {0x83, (uint8_t []){0x02}, 1, 0},\
    {0x84, (uint8_t []){0xDC}, 1, 0},\
    {0x85, (uint8_t []){0x00}, 1, 0},\
    {0x86, (uint8_t []){0x00}, 1, 0},\
    {0x87, (uint8_t []){0x00}, 1, 0},\
    {0x88, (uint8_t []){0x48}, 1, 0},\
    {0x89, (uint8_t []){0x00}, 1, 0},\
    {0x8A, (uint8_t []){0x06}, 1, 0},\
    {0x8B, (uint8_t []){0x02}, 1, 0},\
    {0x8C, (uint8_t []){0xDE}, 1, 0},\
    {0x8D, (uint8_t []){0x00}, 1, 0},\
    {0x8E, (uint8_t []){0x00}, 1, 0},\
    {0x8F, (uint8_t []){0x00}, 1, 0},\
    {0x90, (uint8_t []){0x48}, 1, 0},\
    {0x91, (uint8_t []){0x00}, 1, 0},\
    {0x92, (uint8_t []){0x08}, 1, 0},\
    {0x93, (uint8_t []){0x02}, 1, 0},\
    {0x94, (uint8_t []){0xE0}, 1, 0},\
    {0x95, (uint8_t []){0x00}, 1, 0},\
    {0x96, (uint8_t []){0x00}, 1, 0},\
    {0x97, (uint8_t []){0x00}, 1, 0},\
    {0x98, (uint8_t []){0x48}, 1, 0},\
    {0x99, (uint8_t []){0x00}, 1, 0},\
    {0x9A, (uint8_t []){0x0A}, 1, 0},\
    {0x9B, (uint8_t []){0x02}, 1, 0},\
    {0x9C, (uint8_t []){0xE2}, 1, 0},\
    {0x9D, (uint8_t []){0x00}, 1, 0},\
    {0x9E, (uint8_t []){0x00}, 1, 0},\
    {0x9F, (uint8_t []){0x00}, 1, 0},\
    {0xA0, (uint8_t []){0x48}, 1, 0},\
    {0xA1, (uint8_t []){0x00}, 1, 0},\
    {0xA2, (uint8_t []){0x03}, 1, 0},\
    {0xA3, (uint8_t []){0x02}, 1, 0},\
    {0xA4, (uint8_t []){0xDB}, 1, 0},\
    {0xA5, (uint8_t []){0x00}, 1, 0},\
    {0xA6, (uint8_t []){0x00}, 1, 0},\
    {0xA7, (uint8_t []){0x00}, 1, 0},\
    {0xA8, (uint8_t []){0x48}, 1, 0},\
    {0xA9, (uint8_t []){0x00}, 1, 0},\
    {0xAA, (uint8_t []){0x05}, 1, 0},\
    {0xAB, (uint8_t []){0x02}, 1, 0},\
    {0xAC, (uint8_t []){0xDD}, 1, 0},\
    {0xAD, (uint8_t []){0x00}, 1, 0},\
    {0xAE, (uint8_t []){0x00}, 1, 0},\
    {0xAF, (uint8_t []){0x00}, 1, 0},\
    {0xB0, (uint8_t []){0x48}, 1, 0},\
    {0xB1, (uint8_t []){0x00}, 1, 0},\
    {0xB2, (uint8_t []){0x07}, 1, 0},\
    {0xB3, (uint8_t []){0x02}, 1, 0},\
    {0xB4, (uint8_t []){0xDF}, 1, 0},\
    {0xB5, (uint8_t []){0x00}, 1, 0},\
    {0xB6, (uint8_t []){0x00}, 1, 0},\
    {0xB7, (uint8_t []){0x00}, 1, 0},\
    {0xB8, (uint8_t []){0x48}, 1, 0},\
    {0xB9, (uint8_t []){0x00}, 1, 0},\
    {0xBA, (uint8_t []){0x09}, 1, 0},\
    {0xBB, (uint8_t []){0x02}, 1, 0},\
    {0xBC, (uint8_t []){0xE1}, 1, 0},\
    {0xBD, (uint8_t []){0x00}, 1, 0},\
    {0xBE, (uint8_t []){0x00}, 1, 0},\
    {0xBF, (uint8_t []){0x00}, 1, 0},\
    {0xC0, (uint8_t []){0x65}, 1, 0},\
    {0xC1, (uint8_t []){0x74}, 1, 0},\
    {0xC2, (uint8_t []){0x47}, 1, 0},\
    {0xC3, (uint8_t []){0x56}, 1, 0},\
    {0xC4, (uint8_t []){0xAA}, 1, 0},\
    {0xC5, (uint8_t []){0x11}, 1, 0},\
    {0xC6, (uint8_t []){0x00}, 1, 0},\
    {0xC7, (uint8_t []){0x2A}, 1, 0},\
    {0xC8, (uint8_t []){0xA2}, 1, 0},\
    {0xC9, (uint8_t []){0x33}, 1, 0},\
    {0xD0, (uint8_t []){0x65}, 1, 0},\
    {0xD1, (uint8_t []){0x74}, 1, 0},\
    {0xD2, (uint8_t []){0x47}, 1, 0},\
    {0xD3, (uint8_t []){0x56}, 1, 0},\
    {0xD4, (uint8_t []){0xAA}, 1, 0},\
    {0xD5, (uint8_t []){0x11}, 1, 0},\
    {0xD6, (uint8_t []){0x00}, 1, 0},\
    {0xD7, (uint8_t []){0x2A}, 1, 0},\
    {0xD8, (uint8_t []){0xA2}, 1, 0},\
    {0xD9, (uint8_t []){0x33}, 1, 0},\
    {0xF3, (uint8_t []){0x01}, 1, 0},\
    {0xF0, (uint8_t []){0x00}, 1, 0},\
    {0x21, (uint8_t []){}, 0, 0},\
    {0x11, (uint8_t []){}, 0, 120},\
    {0x36, (uint8_t []){0x00}, 1, 0},\
    {0x3A, (uint8_t []){0x05}, 1, 120},\
    {0x29, (uint8_t []){}, 0, 0},    \
}
/**
 * @brief LCD color configuration
 */
#define ESP_PANEL_BOARD_LCD_COLOR_BITS          (ESP_PANEL_LCD_COLOR_BITS_RGB565)
                                                        // ESP_PANEL_LCD_COLOR_BITS_RGB565/RGB666/RGB888
#define ESP_PANEL_BOARD_LCD_COLOR_BGR_ORDER     (0)     // 0: RGB, 1: BGR
#define ESP_PANEL_BOARD_LCD_COLOR_INEVRT_BIT    (0)     // 0/1

/**
 * @brief LCD transformation configuration
 */
#define ESP_PANEL_BOARD_LCD_SWAP_XY             (0)     // 0/1
#define ESP_PANEL_BOARD_LCD_MIRROR_X            (0)     // 0/1
#define ESP_PANEL_BOARD_LCD_MIRROR_Y            (0)     // 0/1
#define ESP_PANEL_BOARD_LCD_GAP_X               (0)     // [0, ESP_PANEL_BOARD_WIDTH]
#define ESP_PANEL_BOARD_LCD_GAP_Y               (0)     // [0, ESP_PANEL_BOARD_HEIGHT]

/**
 * @brief LCD reset pin configuration
 */
#define ESP_PANEL_BOARD_LCD_RST_IO              (39)    // Reset pin, -1 if not used
#define ESP_PANEL_BOARD_LCD_RST_LEVEL           (0)     // Reset active level, 0: low, 1: high

#endif // ESP_PANEL_BOARD_USE_LCD

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// Please update the following macros to configure the touch panel ///////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Touch panel configuration flag (0/1)
 *
 * Set to `1` to enable touch panel support, `0` to disable
 */
#define ESP_PANEL_BOARD_USE_TOUCH               (1)

#if ESP_PANEL_BOARD_USE_TOUCH
/**
 * @brief Touch controller selection
 */
#define ESP_PANEL_BOARD_TOUCH_CONTROLLER        CST816S

/**
 * @brief Touch bus type selection
 * - `ESP_PANEL_BUS_TYPE_SPI`
 */
#define ESP_PANEL_BOARD_TOUCH_BUS_TYPE          (ESP_PANEL_BUS_TYPE_I2C)

#if (ESP_PANEL_BOARD_TOUCH_BUS_TYPE == ESP_PANEL_BUS_TYPE_I2C) || \
    (ESP_PANEL_BOARD_TOUCH_BUS_TYPE == ESP_PANEL_BUS_TYPE_SPI)
/**
 * If set to 1, the bus will skip to initialize the corresponding host. Users need to initialize the host in advance.
 *
 * For drivers which created by this library, even if they use the same host, the host will be initialized only once.
 * So it is not necessary to set the macro to `1`. For other drivers (like `Wire`), please set the macro to `1`
 * ensure that the host is initialized only once.
 */
#define ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST        (0)     // 0/1. Typically set to 0
#endif

/**
 * @brief Touch bus parameters configuration
 */
#if ESP_PANEL_BOARD_TOUCH_BUS_TYPE == ESP_PANEL_BUS_TYPE_I2C

    /**
     * @brief I2C bus
     */
    /* For general */
    #define ESP_PANEL_BOARD_TOUCH_I2C_HOST_ID           (1)     // Typically set to 0
#if !ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST
    /* For host */
    #define ESP_PANEL_BOARD_TOUCH_I2C_CLK_HZ            (400 * 1000)
                                                                // Typically set to 400K
    #define ESP_PANEL_BOARD_TOUCH_I2C_SCL_PULLUP        (1)     // 0/1. Typically set to 1
    #define ESP_PANEL_BOARD_TOUCH_I2C_SDA_PULLUP        (1)     // 0/1. Typically set to 1
    #define ESP_PANEL_BOARD_TOUCH_I2C_IO_SCL            (9)
    #define ESP_PANEL_BOARD_TOUCH_I2C_IO_SDA            (8)
#endif
    /* For panel */
    #define ESP_PANEL_BOARD_TOUCH_I2C_ADDRESS           (0)     // Typically set to 0 to use the default address.
                                                                // - For touchs with only one address, set to 0
                                                                // - For touchs with multiple addresses, set to 0 or
                                                                //   the address. Like GT911, there are two addresses:
                                                                //   0x5D(default) and 0x14

#endif // ESP_PANEL_BOARD_TOUCH_BUS_TYPE

/**
 * @brief Touch panel transformation flags
 */
#define ESP_PANEL_BOARD_TOUCH_SWAP_XY           (0)     // 0/1
#define ESP_PANEL_BOARD_TOUCH_MIRROR_X          (0)     // 0/1
#define ESP_PANEL_BOARD_TOUCH_MIRROR_Y          (0)     // 0/1

/**
 * @brief Touch panel control pins
 */
#define ESP_PANEL_BOARD_TOUCH_RST_IO            (40)    // Reset pin, -1 if not used
#define ESP_PANEL_BOARD_TOUCH_RST_LEVEL         (0)     // Reset active level, 0: low, 1: high
#define ESP_PANEL_BOARD_TOUCH_INT_IO            (41)    // Interrupt pin, -1 if not used
#define ESP_PANEL_BOARD_TOUCH_INT_LEVEL         (0)     // Interrupt active level, 0: low, 1: high

#endif // ESP_PANEL_BOARD_USE_TOUCH

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Please update the following macros to configure the backlight ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Backlight configuration flag (0/1)
 *
 * Set to `1` to enable backlight support, `0` to disable
 */
#define ESP_PANEL_BOARD_USE_BACKLIGHT           (1)

#if ESP_PANEL_BOARD_USE_BACKLIGHT
/**
 * @brief Backlight control type selection
 */
#define ESP_PANEL_BOARD_BACKLIGHT_TYPE          (ESP_PANEL_BACKLIGHT_TYPE_PWM_LEDC)

#if (ESP_PANEL_BOARD_BACKLIGHT_TYPE == ESP_PANEL_BACKLIGHT_TYPE_SWITCH_GPIO) || \
    (ESP_PANEL_BOARD_BACKLIGHT_TYPE == ESP_PANEL_BACKLIGHT_TYPE_SWITCH_EXPANDER) || \
    (ESP_PANEL_BOARD_BACKLIGHT_TYPE == ESP_PANEL_BACKLIGHT_TYPE_PWM_LEDC)

    /**
     * @brief Backlight control pin configuration
     */
    #define ESP_PANEL_BOARD_BACKLIGHT_IO        (46)    // Output GPIO pin number
    #define ESP_PANEL_BOARD_BACKLIGHT_ON_LEVEL  (1)     // Active level, 0: low, 1: high

#endif // ESP_PANEL_BOARD_BACKLIGHT_TYPE

/**
 * @brief Backlight idle state configuration (0/1)
 *
 * Set to 1 if want to turn off the backlight after initializing. Otherwise, the backlight will be on.
 */
#define ESP_PANEL_BOARD_BACKLIGHT_IDLE_OFF      (0)

#endif // ESP_PANEL_BOARD_USE_BACKLIGHT

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Please update the following macros to configure the IO expander //////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief IO expander configuration flag (0/1)
 *
 * Set to `1` to enable IO expander support, `0` to disable
 */
#define ESP_PANEL_BOARD_USE_EXPANDER            (0)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////// Please utilize the following macros to execute any additional code if required /////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////// File Version ///////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Do not change the following versions. These version numbers are used to check compatibility between this
 * configuration file and the library. Rules for version numbers:
 * 1. Major version mismatch: Configurations are incompatible, must use library version
 * 2. Minor version mismatch: May be missing new configurations, recommended to update
 * 3. Patch version mismatch: No impact on functionality
 */
#define ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_MAJOR 1
#define ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_MINOR 0
#define ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_PATCH 0

// *INDENT-ON*
