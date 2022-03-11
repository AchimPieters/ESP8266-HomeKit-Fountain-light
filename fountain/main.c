/**

   Copyright 2022 Achim Pieters | StudioPieters®

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NON INFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Build upon: ESP-HomeKit - MIT License - Copyright (c) 2017 Maxim Kulkin

 **/

#include <stdio.h>
#include <stdlib.h>
#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>
#include <math.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include "characteristic_types.h"
#include "software_versioning.h"
#include <wifi_config.h>

#include <ws2812_i2s/ws2812_i2s.h>
#include <button.h>

#define BUTTON_GPIO 4
#define RELAY_GPIO 12
#define LED_ON 0     // this is the value to write to GPIO for led on (0 = GPIO low)
#define LED_INBUILT_GPIO 2      // this is the onboard LED used to show on/off only
#define LED_COUNT 16    // this is the number of WS2812B leds on the strip
#define LED_RGB_SCALE 255       // this is the scaling factor used for color conversion

// Global variables
float led_hue = 0;      // hue is scaled 0 to 360
float led_saturation = 50;      // saturation is scaled 0 to 100
float led_brightness = 100;     // brightness is scaled 0 to 100
bool led_on = false;    // on is boolean on or off
ws2812_pixel_t pixels[LED_COUNT];

//http://blog.saikoled.com/post/44677718712/how-to-convert-from-hsi-to-rgb-white
static void hsi2rgb(float h, float s, float i, ws2812_pixel_t* rgb) {
        int r, g, b;

        while (h < 0) { h += 360.0F; }; // cycle h around to 0-360 degrees
        while (h >= 360) { h -= 360.0F; };
        h = 3.14159F*h / 180.0F;// convert to radians.
        s /= 100.0F; // from percentage to ratio
        i /= 100.0F; // from percentage to ratio
        s = s > 0 ? (s < 1 ? s : 1) : 0; // clamp s and i to interval [0,1]
        i = i > 0 ? (i < 1 ? i : 1) : 0; // clamp s and i to interval [0,1]
        i = i * sqrt(i);// shape intensity to have finer granularity near 0

        if (h < 2.09439) {
                r = LED_RGB_SCALE * i / 3 * (1 + s * cos(h) / cos(1.047196667 - h));
                g = LED_RGB_SCALE * i / 3 * (1 + s * (1 - cos(h) / cos(1.047196667 - h)));
                b = LED_RGB_SCALE * i / 3 * (1 - s);
        }
        else if (h < 4.188787) {
                h = h - 2.09439;
                g = LED_RGB_SCALE * i / 3 * (1 + s * cos(h) / cos(1.047196667 - h));
                b = LED_RGB_SCALE * i / 3 * (1 + s * (1 - cos(h) / cos(1.047196667 - h)));
                r = LED_RGB_SCALE * i / 3 * (1 - s);
        }
        else {
                h = h - 4.188787;
                b = LED_RGB_SCALE * i / 3 * (1 + s * cos(h) / cos(1.047196667 - h));
                r = LED_RGB_SCALE * i / 3 * (1 + s * (1 - cos(h) / cos(1.047196667 - h)));
                g = LED_RGB_SCALE * i / 3 * (1 - s);
        }

        rgb->red = (uint8_t) r;
        rgb->green = (uint8_t) g;
        rgb->blue = (uint8_t) b;
        rgb->white = (uint8_t) 0; // white channel is not used
}

void led_string_fill(ws2812_pixel_t rgb) {
// write out the new color to each pixel
        for (int i = 0; i < LED_COUNT; i++) {
                pixels[i] = rgb;
        }
        ws2812_i2s_update(pixels, PIXEL_RGB);
}

void led_string_set(void) {
        ws2812_pixel_t rgb = { { 0, 0, 0, 0 } };

        if (led_on) {
// convert HSI to RGBW
                hsi2rgb(led_hue, led_saturation, led_brightness, &rgb);
// set the inbuilt led
                gpio_write(LED_INBUILT_GPIO, LED_ON);
        }
        else {
                gpio_write(LED_INBUILT_GPIO, 1 - LED_ON);
        }
// write out the new color
        led_string_fill(rgb);
}

void led_init() {
// initialise the onboard led as a secondary indicator (handy for testing)
        gpio_enable(LED_INBUILT_GPIO, GPIO_OUTPUT);
// initialise the LED strip
        ws2812_i2s_init(LED_COUNT, PIXEL_RGB);
// set the initial state
        led_string_set();
}

void led_identify_task(void *_args) {
        const ws2812_pixel_t COLOR_PINK = { { 255, 0, 127, 0 } };
        const ws2812_pixel_t COLOR_BLACK = { { 0, 0, 0, 0 } };

        for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                        gpio_write(LED_INBUILT_GPIO, LED_ON);
                        led_string_fill(COLOR_PINK);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                        gpio_write(LED_INBUILT_GPIO, 1 - LED_ON);
                        led_string_fill(COLOR_BLACK);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                vTaskDelay(250 / portTICK_PERIOD_MS);
        }

        led_string_set();
        vTaskDelete(NULL);
}

void led_identify(homekit_value_t _value) {
        xTaskCreate(led_identify_task, "LED identify", 128, NULL, 2, NULL);
}

homekit_value_t led_on_get() {
        return HOMEKIT_BOOL(led_on);
}

void led_on_set(homekit_value_t value) {
        if (value.format != homekit_format_bool) {
                return;
        }

        led_on = value.bool_value;
        led_string_set();
}

homekit_value_t led_brightness_get() {
        return HOMEKIT_INT(led_brightness);
}
void led_brightness_set(homekit_value_t value) {
        if (value.format != homekit_format_int) {
                return;
        }
        led_brightness = value.int_value;
        led_string_set();
}

homekit_value_t led_hue_get() {
        return HOMEKIT_FLOAT(led_hue);
}

void led_hue_set(homekit_value_t value) {
        if (value.format != homekit_format_float) {
                return;
        }
        led_hue = value.float_value;
        led_string_set();
}

homekit_value_t led_saturation_get() {
        return HOMEKIT_FLOAT(led_saturation);
}

void led_saturation_set(homekit_value_t value) {
        if (value.format != homekit_format_float) {
                return;
        }
        led_saturation = value.float_value;
        led_string_set();
}


void switch_on_callback(homekit_characteristic_t *_ch, homekit_value_t on, void *context);
void button_callback(button_event_t event, void* context);

void relay_write(bool on) {
        gpio_write( RELAY_GPIO, on ? 1 : 0);
}

homekit_characteristic_t switch_on = HOMEKIT_CHARACTERISTIC_(
        ON, false, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(switch_on_callback)
        );

void gpio_init() {
        gpio_enable( RELAY_GPIO, GPIO_OUTPUT);
        relay_write(switch_on.value.bool_value);
}

void switch_on_callback(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
        relay_write(switch_on.value.bool_value);
}

void button_callback(button_event_t event, void* context) {
        switch (event) {
        case button_event_single_press:
                printf("Toggling relay\n");
                switch_on.value.bool_value = !switch_on.value.bool_value;
                relay_write(switch_on.value.bool_value);
                homekit_characteristic_notify(&switch_on, switch_on.value);
                break;
        case button_event_long_press:
                break;
        default:
                printf("Unknown button event: %d\n", event);
        }
}

#define DEVICE_NAME "WATER FOUNTAIN"
#define DEVICE_MANUFACTURER "StudioPieters®"
// See Naming convention.md
#define DEVICE_SERIAL "NLDA4SQN1466"
// See Naming convention.md
#define DEVICE_MODEL "SD466NL/A"
// Will get his version numer trough software_versioning
#define FW_VERSION "0.0.0"

homekit_characteristic_t ota_trigger = API_OTA_TRIGGER;
homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, DEVICE_NAME);
homekit_characteristic_t manufacturer = HOMEKIT_CHARACTERISTIC_(MANUFACTURER,  DEVICE_MANUFACTURER);
homekit_characteristic_t serial = HOMEKIT_CHARACTERISTIC_(SERIAL_NUMBER, DEVICE_SERIAL);
homekit_characteristic_t model= HOMEKIT_CHARACTERISTIC_(MODEL, DEVICE_MODEL);
homekit_characteristic_t revision = HOMEKIT_CHARACTERISTIC_(FIRMWARE_REVISION,  FW_VERSION);

homekit_accessory_t *accessories[] = {
        HOMEKIT_ACCESSORY(.id = 1, .category = homekit_accessory_category_lightbulb, .services = (homekit_service_t*[]) {
                HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics = (homekit_characteristic_t*[]) {
                        &name,
                        &manufacturer,
                        &serial,
                        &model,
                        &revision,
                        HOMEKIT_CHARACTERISTIC(IDENTIFY, led_identify),
                        NULL
                }),
                HOMEKIT_SERVICE(LIGHTBULB, .primary = true, .characteristics = (homekit_characteristic_t*[]) {
                        HOMEKIT_CHARACTERISTIC(NAME, "NEOPIXEL RING"),

                        HOMEKIT_CHARACTERISTIC(
                                ON, true,
                                .getter = led_on_get,
                                .setter = led_on_set
                                ),
                        HOMEKIT_CHARACTERISTIC(
                                BRIGHTNESS, 100,
                                .getter = led_brightness_get,
                                .setter = led_brightness_set
                                ),
                        HOMEKIT_CHARACTERISTIC(
                                HUE, 0,
                                .getter = led_hue_get,
                                .setter = led_hue_set
                                ),
                        HOMEKIT_CHARACTERISTIC(
                                SATURATION, 0,
                                .getter = led_saturation_get,
                                .setter = led_saturation_set
                                ),
                        NULL
                }),
                HOMEKIT_SERVICE(SWITCH, .characteristics=(homekit_characteristic_t*[]) {
                        HOMEKIT_CHARACTERISTIC(NAME, "WATER PUMP"),
                        &switch_on,
                        &ota_trigger,
                        NULL
                }),
                NULL
        }),
        NULL
};

void on_event(homekit_event_t event) {

}

// tools/gen_qrcode 5 252-87-678 4G4J qrcode.png
homekit_server_config_t config = {
        .accessories = accessories,
        .password = "252-87-678",
        .setupId= "4G4J",
        .on_event = on_event,
};

void create_accessory_name() {
        int serialLength = snprintf(NULL, 0, "%d", sdk_system_get_chip_id());
        char *serialNumberValue = malloc(serialLength + 1);
        snprintf(serialNumberValue, serialLength + 1, "%d", sdk_system_get_chip_id());
        int name_len = snprintf(NULL, 0, "%s-%s", DEVICE_NAME, serialNumberValue);
        if (name_len > 63) { name_len = 63; }
        char *name_value = malloc(name_len + 1);
        snprintf(name_value, name_len + 1, "%s-%s", DEVICE_NAME, serialNumberValue);
        name.value = HOMEKIT_STRING(name_value);
}

void on_wifi_ready(){

}

void user_init(void) {
        uart_set_baud(0, 115200);
        homekit_server_init(&config);
        led_init();
        gpio_init();

// Get Github version number
        int c_hash=ota_read_sysparam(&revision.value.string_value);
        config.accessories[0]->config_number=c_hash;

        button_config_t config = BUTTON_CONFIG(
                button_active_low,
                .long_press_time = 4000,
                .max_repeat_presses = 3,
                );

        int b = button_create(BUTTON_GPIO, config, button_callback, NULL);
        if (b) {
                printf("Failed to initialize a button\n");
        }
}
