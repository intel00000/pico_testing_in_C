#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "SparkFun_Alphanumeric_Display.h"

#define I2C_PORT i2c0
#define MY_I2C_SDA_PIN 20
#define MY_I2C_SCL_PIN 21

#define TEST_SAMPLES 100000
#define NUM_SAMPLES 1000

uint32_t adc0_sum = 0;
uint32_t adc1_sum = 0;
uint32_t adc2_sum = 0;
uint32_t temp_sum = 0;
uint32_t adc0_avg = 0;
uint32_t adc1_avg = 0;
uint32_t adc2_avg = 0;
uint32_t temp_avg = 0;
unsigned int counter = 0;
char tempString[10];

HT16K33 display;

void setup_i2c();
void setup_adc();
void serial_display_setup();
void serial_display_loop();
void run_serial_display();
void test_max_poll_rate();

int main()
{
    stdio_init_all();

    // Sleep for 1 seconds to give time to open the serial terminal
    sleep_ms(1000);

    setup_i2c();
    setup_adc();

    printf("I2C and ADC initialized\n");

    test_max_poll_rate();
    run_serial_display();

    return 0;
}

void setup_i2c()
{
    // Initialize I2C port at 100 kHz
    uint baudrate = i2c_init(I2C_PORT, 100 * 1000);
    printf("I2C initialized with baudrate: %d\n", baudrate);
    gpio_set_function(MY_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(MY_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(MY_I2C_SDA_PIN);
    gpio_pull_up(MY_I2C_SCL_PIN);

    // Initialize the display
    if (!HT16K33_begin(&display, 0x70, DEFAULT_NOTHING_ATTACHED, DEFAULT_NOTHING_ATTACHED, DEFAULT_NOTHING_ATTACHED, i2c0))
    {
        printf("Failed to initialize display\n");
        while (1)
            ;
    }
}

void setup_adc()
{
    adc_init();
    adc_gpio_init(26);
    adc_gpio_init(27);
    adc_gpio_init(28);
    adc_set_temp_sensor_enabled(true);
}

void serial_display_setup()
{
    HT16K33_setBrightness(&display, 15);
    HT16K33_clear(&display);
    HT16K33_print(&display, "-HI-");
    HT16K33_decimalOn(&display);
    sleep_ms(1000);
    HT16K33_setBrightness(&display, 0);
    sleep_ms(1000);
    HT16K33_setBrightness(&display, 10);
    sleep_ms(1000);
    HT16K33_setBlinkRate(&display, 0);
    HT16K33_clear(&display);
}

void serial_display_loop()
{
    uint64_t start_time = time_us_64();
    while (1)
    {
        adc_select_input(0);
        adc0_sum += adc_read();
        adc_select_input(1);
        adc1_sum += adc_read();
        adc_select_input(2);
        adc2_sum += adc_read();
        adc_select_input(4);
        temp_sum += adc_read();
        counter++;
        if (counter >= NUM_SAMPLES)
        {
            adc0_avg = adc0_sum / NUM_SAMPLES;
            adc1_avg = adc1_sum / NUM_SAMPLES;
            adc2_avg = adc2_sum / NUM_SAMPLES;
            temp_avg = temp_sum / NUM_SAMPLES;
            adc0_sum = 0;
            adc1_sum = 0;
            adc2_sum = 0;
            temp_sum = 0;
            counter = 0;
            uint32_t ADC1_value_scaled = adc1_avg * 9999 / 4095;
            snprintf(tempString, 5, "%4d", ADC1_value_scaled);
            HT16K33_print(&display, tempString);
            float conversion_factor = 3.3f / (1 << 12);
            float temp_celsius = 27 - (temp_avg * conversion_factor - 0.706) / 0.001721;
            float temp_fahrenheit = (temp_celsius * 9 / 5) + 32;
            float elapsed_time = (float)(time_us_64() - start_time) / 1000000.0f;
            printf("adc1_avg raw: %u, adc1_avg scaled: %u, adc2_avg raw: %u, adc0_avg raw: %u, ", adc1_avg, ADC1_value_scaled, adc2_avg, adc0_avg);
            printf("temp_avg raw: %u, RP2040 internal temperature in Celsius: %.3f C, in Fahrenheit: %.3f F, ", temp_avg, temp_celsius, temp_fahrenheit);
            printf("elapsed time: %f s, cpu ticks: %llu\n", elapsed_time, time_us_64());
        }
    }
}

void run_serial_display()
{
    sleep_ms(100);
    serial_display_setup();
    serial_display_loop();
}

void test_max_poll_rate()
{
    printf("test_max_poll_rate unoptimized\n");
    uint64_t start = time_us_64();
    for (int count = 0; count < TEST_SAMPLES; count++)
    {
        adc_select_input(1);
        adc_read();
    }
    uint64_t end = time_us_64();
    float elapsed_time_s = (float)(end - start) / 1000000.0f;
    float poll_rate = TEST_SAMPLES / elapsed_time_s;
    printf("Max poll rate: %.2f samples per second\n", poll_rate);
    printf("Elapsed time for %d samples: %.2f seconds\n", TEST_SAMPLES, elapsed_time_s);
}