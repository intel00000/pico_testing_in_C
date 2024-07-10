#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pico/cyw43_arch.h"
#include "blink.pio.h"

#define NUM_PIXELS 5
#define ZIP_LED_GPIO_PIN 0
#define ONBOARD_LED_PIN CYW43_WL_GPIO_LED_PIN

// function to put pixel data to the PIO state machine
static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

// function to convert RGB to 32-bit color, the color format is 0xGGRRBB
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(g) << 16) | ((uint32_t)(r) << 8) | (uint32_t)(b);
}

// Helper function to convert HSV to RGB
void hsv_to_rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    int i;
    float f, p, q, t;
    if (s == 0)
    {
        *r = *g = *b = v * 255;
        return;
    }
    h *= 6;
    i = (int)h;
    f = h - i;
    p = v * (1 - s);
    q = v * (1 - s * f);
    t = v * (1 - s * (1 - f));
    switch (i % 6)
    {
    case 0:
        *r = v * 255;
        *g = t * 255;
        *b = p * 255;
        break;
    case 1:
        *r = q * 255;
        *g = v * 255;
        *b = p * 255;
        break;
    case 2:
        *r = p * 255;
        *g = v * 255;
        *b = t * 255;
        break;
    case 3:
        *r = p * 255;
        *g = q * 255;
        *b = v * 255;
        break;
    case 4:
        *r = t * 255;
        *g = p * 255;
        *b = v * 255;
        break;
    case 5:
        *r = v * 255;
        *g = p * 255;
        *b = q * 255;
        break;
    }
}

// Turn off all the ZIP LEDs
void off(uint32_t *led_data, int numLEDs)
{
    for (int i = 0; i < numLEDs; i++)
    {
        led_data[i] = 0x000000;
        put_pixel(led_data[i]);
    }
}

// Function to generate the gradient of the rainbow, cycle through the entire color wheel space
// all led color will be the same
uint32_t full_colorspace_gradient(uint32_t *led_data, int *led_selected, int numLEDs, uint32_t start_color, int interval, int time_ms)
{
    uint32_t max_color_value = 0xFFFFFF;
    uint32_t min_color_value = 0x000000;
    for (int i = 0; i < numLEDs; i++)
    {
        if (led_selected[i])
        {
            led_data[i] += interval;
            if (led_data[i] > max_color_value)
            {
                led_data[i] = min_color_value;
            }
            printf("led[%d]: %x, ", i, led_data[i]);
            put_pixel(led_data[i]);
        }
    }
    printf("\n");
    sleep_ms(time_ms);
    return (start_color + interval) % max_color_value;
}

// run the full colorspace gradient effect
int run_full_colorspace_gradient()
{
    uint32_t led_data[NUM_PIXELS] = {0};
    int led_selected[NUM_PIXELS] = {1, 1, 1, 1, 1};
    uint32_t start_color = 0x000000;

    while (1)
    {
        start_color = full_colorspace_gradient(led_data, led_selected, NUM_PIXELS, start_color, 1, 10);
        printf("start_color: %x\n", start_color);
    }
}

// Function to create a single color gradient effect
void single_colorspace_gradient(uint32_t *led_data, int *led_selected, int numLEDs, char color_channel, uint8_t max_intensity, int *directions, int interval, int time_ms)
{
    uint32_t min_value = 0x000000;
    uint32_t max_value, full_value;
    int shift;
    switch (color_channel)
    {
    case 'r':
        shift = 8;
        full_value = 0x00FF00;
        max_value = (0xFF * max_intensity) << shift;
        break;
    case 'g':
        shift = 16;
        full_value = 0xFF0000;
        max_value = (0xFF * max_intensity) << shift;
        break;
    case 'b':
        shift = 0;
        full_value = 0x0000FF;
        max_value = (0xFF * max_intensity) << shift;
        break;
    default:
        return;
    }
    for (int i = 0; i < numLEDs; i++)
    {
        if (led_selected[i])
        {
            led_data[i] += directions[i] * (0x01 << shift) * interval;
            if ((led_data[i] & full_value) >= max_value)
            {
                led_data[i] = (led_data[i] & ~full_value) | max_value;
                directions[i] = -1;
            }
            else if ((led_data[i] & full_value) <= min_value)
            {
                led_data[i] = (led_data[i] & ~full_value) | min_value;
                directions[i] = 1;
            }

            printf("led[%d]: %x, ", i, led_data[i]);
            put_pixel(led_data[i]);
        }
    }
    printf("\n");
    sleep_ms(time_ms);
}

// run the single colorspace gradient effect
int run_single_colorspace_gradient()
{
    float intensity = 1;
    uint32_t led_data[NUM_PIXELS] = {0x000000, 0x000000, 0x000000, 0x000000, 0x000000};
    int led_selected[NUM_PIXELS] = {1, 1, 1, 1, 1};
    int directions[NUM_PIXELS] = {1, 1, 1, 1, 1};

    while (1)
    {
        single_colorspace_gradient(led_data, led_selected, NUM_PIXELS, 'r', intensity, directions, 1, 10);
    }
}

// Function to create a rainbow effect
float rainbow_effect(uint32_t *led_data, int *led_selected, int numLEDs, float hue, float max_intensity, int time_ms)
{
    for (int i = 0; i < numLEDs; i++)
    {
        if (led_selected[i])
        {
            float led_hue = (hue + i * (1.0 / numLEDs)) - (int)(hue + i * (1.0 / numLEDs));
            uint8_t r, g, b;
            hsv_to_rgb(led_hue, 1.0, max_intensity, &r, &g, &b);
            led_data[i] = urgb_u32(r, g, b);
            put_pixel(led_data[i]);
        }
    }
    sleep_ms(time_ms);
    return (hue + 0.01) - (int)(hue + 0.01);
}

// run the rainbow effect
int run_rainbow_effect()
{
    uint32_t led_data[NUM_PIXELS] = {0};
    int led_selected[NUM_PIXELS] = {1, 1, 1, 1, 1};
    float hue = 0;

    while (1)
    {
        hue = rainbow_effect(led_data, led_selected, NUM_PIXELS, hue, 0.01, 10);
    }
}

// Function to create a breathing effect
void breathing_effect(uint32_t *led_data, int *led_selected, int numLEDs, float max_intensity, int *intensity, int *direction, uint8_t r, uint8_t g, uint8_t b, int time_ms)
{
    float scaled_intensity = (*intensity / 255.0) * max_intensity;
    uint32_t color = urgb_u32(r * scaled_intensity, g * scaled_intensity, b * scaled_intensity);
    for (int i = 0; i < numLEDs; i++)
    {
        if (led_selected[i])
        {
            led_data[i] = color;
            put_pixel(led_data[i]);
        }
    }
    sleep_ms(time_ms);
    *intensity += *direction;
    if (*intensity >= 255 || *intensity <= 0)
    {
        *direction *= -1;
    }
}

// run the breathing effect
int run_breathing_effect()
{
    uint32_t led_data[NUM_PIXELS] = {0};
    int led_selected[NUM_PIXELS] = {1, 1, 1, 1, 1};
    int intensity = 0;
    int direction = 1;

    while (1)
    {
        breathing_effect(led_data, led_selected, NUM_PIXELS, 0.1, &intensity, &direction, 255, 0, 0, 1);
    }
}

// Function to create a blink effect
void blink_effect(uint32_t *led_data, int *led_selected, int numLEDs, uint8_t r, uint8_t g, uint8_t b, bool *state, int time_ms)
{
    uint32_t color = *state ? urgb_u32(r, g, b) : 0x000000;
    for (int i = 0; i < numLEDs; i++)
    {
        if (led_selected[i])
        {
            led_data[i] = color;
            put_pixel(led_data[i]);
        }
    }
    *state = !*state;
    sleep_ms(time_ms);
}

// run the blink effect
int run_blink_effect()
{
    uint32_t led_data[NUM_PIXELS] = {0};
    int led_selected[NUM_PIXELS] = {1, 1, 1, 1, 1};
    bool blink_state = true;

    while (1)
    {
        blink_effect(led_data, led_selected, NUM_PIXELS, 100, 0, 0, &blink_state, 500);
    }
}

// Function to create a chase effect
void chase_effect(uint32_t *led_data, int *led_selected, int numLEDs, int *index, uint8_t r, uint8_t g, uint8_t b, int time_ms)
{
    for (int i = 0; i < numLEDs; i++)
    {
        if (led_selected[i])
        {
            if (i == *index)
            {
                led_data[i] = urgb_u32(r, g, b);
            }
            else
            {
                led_data[i] = 0x000000;
            }
            put_pixel(led_data[i]);
        }
    }
    sleep_ms(time_ms);
    *index = (*index + 1) % numLEDs;
}

// run the chase effect
int run_chase_effect()
{
    uint32_t led_data[NUM_PIXELS] = {0};
    int led_selected[NUM_PIXELS] = {1, 1, 1, 1, 1};
    int chase_index = 0;

    while (1)
    {
        chase_effect(led_data, led_selected, NUM_PIXELS, &chase_index, 100, 0, 0, 100);
    }
}

// Function to create a sparkle effect
void sparkle_effect(uint32_t *led_data, int *led_selected, int numLEDs, float probability, float max_intensity, int time_ms)
{
    for (int i = 0; i < numLEDs; i++)
    {
        if (led_selected[i])
        {
            if ((rand() / (float)RAND_MAX) < probability)
            {
                uint8_t r = rand() % (int)(255 * max_intensity);
                uint8_t g = rand() % (int)(255 * max_intensity);
                uint8_t b = rand() % (int)(255 * max_intensity);
                led_data[i] = urgb_u32(r, g, b);
            }
            else
            {
                led_data[i] = 0x000000;
            }
            put_pixel(led_data[i]);
        }
    }
    sleep_ms(time_ms);
}

// run the sparkle effect
int run_sparkle_effect()
{
    uint32_t led_data[NUM_PIXELS] = {0};
    int led_selected[NUM_PIXELS] = {1, 1, 1, 1, 1};

    while (1)
    {
        sparkle_effect(led_data, led_selected, NUM_PIXELS, 0.1, 0.1, 1000);
    }
}

// Combined Chase and Rainbow Effect
float chase_rainbow_effect(uint32_t *led_data, int *led_selected, int numLEDs, float max_intensity, float hue, int *index, int time_ms)
{
    for (int j = 0; j < numLEDs; j++)
    {
        if (led_selected[j])
        {
            float led_hue = (hue + j * (1.0 / numLEDs)) - (int)(hue + j * (1.0 / numLEDs));
            uint8_t r, g, b;
            hsv_to_rgb(led_hue, 1.0, max_intensity, &r, &g, &b);
            if (j == *index)
            {
                led_data[j] = urgb_u32(r, g, b);
            }
            else
            {
                led_data[j] = 0x000000;
            }
            put_pixel(led_data[j]);
        }
    }
    sleep_ms(time_ms);
    *index = (*index + 1) % numLEDs;
    return (hue + 0.01) - (int)(hue + 0.01);
}

// run the chase rainbow effect
int run_chase_rainbow_effect()
{
    uint32_t led_data[NUM_PIXELS] = {0};
    int led_selected[NUM_PIXELS] = {1, 1, 1, 1, 1};
    int chase_index = 0;
    float hue = 0;
    int onboarding_led_state = 0;

    while (1)
    {
        onboarding_led_state = !onboarding_led_state;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, onboarding_led_state);
        hue = chase_rainbow_effect(led_data, led_selected, NUM_PIXELS, 0.02, hue, &chase_index, 1000);
    }
}

int blink_zip_led()
{
    // Initialize Wi-Fi
    if (cyw43_arch_init())
    {
        printf("Wi-Fi init failed");
        return -1;
    }

    // setup PIO and state machine
    PIO pio = pio0;
    // get the available state machine
    int sm = pio_claim_unused_sm(pio, true);
    // load the program into the PIO
    uint offset = pio_add_program(pio, &ws2812_program);

    // initialize the program
    ws2812_program_init(pio, sm, offset, ZIP_LED_GPIO_PIN, 800000, false);

    run_chase_rainbow_effect();

    return 0;
}