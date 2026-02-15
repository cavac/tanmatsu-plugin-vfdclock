// SPDX-License-Identifier: MIT
// Tanmatsu VFD Clock Plugin
//
// Displays the current time on a NE-HCS12SS59T-R1 I2C VFD (12-character ASCII).
// Connected via QWIIC on the external I2C bus (bus 1).
//
// VFD Register Map:
//   Register 0:     System control (bit 0 = enable, bit 1 = test, bit 2 = LED)
//   Register 1:     Display offset
//   Registers 4-5:  Scroll speed
//   Register 6:     Brightness (0-255, default 110)
//   Registers 10+:  ASCII text data buffer
//
// To change bus/address, modify VFD_BUS and VFD_ADDRESS below.

#include "tanmatsu_plugin.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

// VFD configuration
#define VFD_BUS       1       // External I2C bus (QWIIC/SAO)
#define VFD_ADDRESS   0x10    // 7-bit I2C address
#define VFD_SPEED_HZ  100000  // 100kHz
#define VFD_CHARS     12      // Display character count

// VFD register addresses
#define VFD_REG_CONTROL     0
#define VFD_REG_OFFSET      1
#define VFD_REG_BRIGHTNESS  6
#define VFD_REG_TEXT        10

// VFD control bits
#define VFD_CTRL_ENABLE  (1 << 0)
#define VFD_CTRL_TEST    (1 << 1)
#define VFD_CTRL_LED     (1 << 2)

static plugin_context_t* plugin_ctx = NULL;
static asp_i2c_device_t vfd_dev = NULL;

static void vfd_write_reg(uint8_t reg, uint8_t value) {
    uint8_t data[2] = { reg, value };
    asp_i2c_write(vfd_dev, data, sizeof(data));
}

static bool vfd_init(void) {
    // Scan both buses to find devices
    for (uint8_t bus = 0; bus <= 1; bus++) {
        asp_log_info("vfdclock", "Scanning I2C bus %d for devices...", bus);
        for (uint16_t addr = 0x08; addr < 0x78; addr++) {
            if (asp_i2c_probe(bus, addr) == ASP_OK) {
                asp_log_info("vfdclock", "  Bus %d: found device at 0x%02X", bus, addr);
            }
        }
    }

    asp_err_t err = asp_i2c_open(plugin_ctx, &vfd_dev, VFD_BUS, VFD_ADDRESS, VFD_SPEED_HZ);
    if (err != ASP_OK) {
        asp_log_error("vfdclock", "Failed to open I2C device at 0x%02X on bus %d", VFD_ADDRESS, VFD_BUS);
        return false;
    }

    // Enable display and set max brightness
    vfd_write_reg(VFD_REG_CONTROL, VFD_CTRL_ENABLE);
    vfd_write_reg(VFD_REG_BRIGHTNESS, 110); // DON'T GO MUCH HIGHER, THIS WILL REDUCE THE LIFESPAN DRASTICALLY
    return true;
}

static void vfd_write_text(const char* text) {
    size_t len = strlen(text);
    if (len > VFD_CHARS) len = VFD_CHARS;

    // Build I2C message: register address followed by text data
    uint8_t buf[1 + VFD_CHARS];
    buf[0] = VFD_REG_TEXT;
    memcpy(&buf[1], text, len);
    // Pad remaining characters with spaces
    for (size_t i = len; i < VFD_CHARS; i++) {
        buf[1 + i] = ' ';
    }
    asp_i2c_write(vfd_dev, buf, 1 + VFD_CHARS);
}

static void vfd_cleanup(void) {
    if (vfd_dev != NULL) {
        // Clear display text
        vfd_write_text("            ");
        // Disable display
        vfd_write_reg(VFD_REG_CONTROL, 0);
        asp_i2c_close(vfd_dev);
        vfd_dev = NULL;
    }
}

// Plugin metadata
static const plugin_info_t plugin_info = {
    .name = "VFD Clock",
    .slug = "vfdclock",
    .version = "1.0.0",
    .author = "cavac",
    .description = "Displays clock on I2C VFD display",
    .api_version = TANMATSU_PLUGIN_API_VERSION,
    .type = PLUGIN_TYPE_SERVICE,
    .flags = 0,
};

static const plugin_info_t* get_info(void) {
    return &plugin_info;
}

static int plugin_init(plugin_context_t* ctx) {
    plugin_ctx = ctx;

    if (!vfd_init()) {
        return -1;
    }

    asp_log_info("vfdclock", "VFD Clock plugin initialized");
    return 0;
}

static void plugin_cleanup(plugin_context_t* ctx) {
    (void)ctx;
    vfd_cleanup();
    plugin_ctx = NULL;
    asp_log_info("vfdclock", "VFD Clock plugin cleaned up");
}

static void plugin_service_run(plugin_context_t* ctx) {
    asp_log_info("vfdclock", "VFD Clock service starting");

    char display_buf[VFD_CHARS + 1];

    while (!asp_plugin_should_stop(ctx)) {
        time_t now = time(NULL);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);

        // Format: "  HH MM SS  " (centered in 12 chars)
        snprintf(display_buf, sizeof(display_buf), "  %02d %02d %02d  ",
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

        vfd_write_text(display_buf);

        asp_plugin_delay_ms(500);
    }

    // Blank display and power down before exiting the service
    vfd_write_text("            ");
    vfd_write_reg(VFD_REG_CONTROL, 0);

    asp_log_info("vfdclock", "VFD Clock service stopped");
}

static const plugin_entry_t entry = {
    .get_info = get_info,
    .init = plugin_init,
    .cleanup = plugin_cleanup,
    .menu_render = NULL,
    .menu_select = NULL,
    .service_run = plugin_service_run,
    .hook_event = NULL,
};

TANMATSU_PLUGIN_REGISTER(entry);
