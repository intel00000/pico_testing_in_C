#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/sem.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "pico/cyw43_arch.h"
#include "SparkFun_Alphanumeric_Display.h"
#include "Adafruit_BME280.h"

#define SparkFun_I2C_PORT i2c1
#define SparkFun_I2C_SDA_PIN 14
#define SparkFun_I2C_SCL_PIN 15
#define BME280_I2C_PORT i2c0
#define BME280_I2C_SDA_PIN 16
#define BME280_I2C_SCL_PIN 17
#define Sea_Level_Pressure_HPA (1013.25)

// Define onboard LED
#ifndef PICO_DEFAULT_LED_PIN
#define ONBOARD_LED_PIN CYW43_WL_GPIO_LED_PIN
#endif

// Global variables for sensor data
volatile float temperature, pressure, altitude, humidity = 0.0;

// Semaphores for synchronization
semaphore_t data_ready;
semaphore_t display_done;

bme280_t bme;
HT16K33 display;

void setup_i2c();
void setup_bme280();
void setup_display();
void read_and_send_data();
void core1_display_task();

void setup_i2c()
{
    // Initialize I2C port at 100 kHz
    uint baudrate = i2c_init(SparkFun_I2C_PORT, 400 * 1000);
    gpio_set_function(SparkFun_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SparkFun_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SparkFun_I2C_SDA_PIN);
    gpio_pull_up(SparkFun_I2C_SCL_PIN);

    // Initialize the display
    if (!HT16K33_begin(&display, 0x70, DEFAULT_NOTHING_ATTACHED, DEFAULT_NOTHING_ATTACHED, DEFAULT_NOTHING_ATTACHED, SparkFun_I2C_PORT))
    {
        while (1)
        {
            printf("Failed to initialize display\n");
            sleep_ms(1000);
        }
    }
    printf("Display I2C initialized with baudrate: %d\n", baudrate);
}

void setup_bme280()
{
    // Initialize I2C port at 100 kHz
    uint baudrate = i2c_init(BME280_I2C_PORT, 100 * 1000);
    gpio_set_function(BME280_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(BME280_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(BME280_I2C_SDA_PIN);
    gpio_pull_up(BME280_I2C_SCL_PIN);

    // Initialize BME280 sensor
    if (!bme280_init(&bme, BME280_I2C_PORT, BME280_ADDRESS))
    {
        while (1)
        {
            printf("Could not find a valid BME280 sensor, check wiring!\n");
            sleep_ms(1000);
        }
    }
    printf("BME280 I2C initialized with baudrate: %d\n", baudrate);
}

void setup_display()
{
    HT16K33_clear(&display);
    sleep_ms(250);
    HT16K33_setBlinkRate(&display, 0);
    sleep_ms(250);
    HT16K33_print(&display, "-HI-");
    sleep_ms(250);
    HT16K33_setBrightness(&display, 15);
    sleep_ms(250);
    HT16K33_setBrightness(&display, 0);
    sleep_ms(250);
    HT16K33_clear(&display);
}

void read_sensor_and_send()
{
    float temp, pres, alt, hum;

    setup_bme280();

    while (1)
    {
        // Read data from BME280 sensor
        temp = bme280_read_temperature(&bme);
        pres = bme280_read_pressure(&bme) / 100.0F;
        alt = bme280_read_altitude(&bme, Sea_Level_Pressure_HPA);
        hum = bme280_read_humidity(&bme);

        // Print the data
        printf("Temperature = %.2f °C, Pressure = %.2f hPa, Altitude = %.2f m, Humidity = %.2f %%\n", temp, pres, alt, hum);

        // Wait for display task to complete
        bool sem_return = sem_try_acquire(&display_done);
        if (!sem_return)
        {
            printf("Display task not completed\n");
            sleep_ms(100);
            continue;
        }

        // Update global variables
        temperature = temp;
        pressure = pres;
        altitude = alt;
        humidity = hum;

        // Signal data ready
        sem_release(&data_ready);

        // Delay for 0.1 seconds
        sleep_ms(100);
    }
}

// function to format the data and display it
void display_data(float data)
{
    char displayString[16];
    int intPart = (int)data;

    if (intPart >= 1000)
    {
        snprintf(displayString, sizeof(displayString), "%4.0f", data);
    }
    else if (intPart >= 100)
    {
        snprintf(displayString, sizeof(displayString), "%4.1f", data);
    }
    else if (intPart >= 10)
    {
        snprintf(displayString, sizeof(displayString), " %4.1f", data);
    }
    else
    {
        snprintf(displayString, sizeof(displayString), "  %4.1f", data);
    }
    HT16K33_print(&display, displayString);
}

void display_task()
{
    // Initialize Wi-Fi
    if (cyw43_arch_init())
    {
        printf("Wi-Fi init failed");
        return;
    }

    setup_i2c();
    setup_display();

    bool onboarding_led_state = false;

    while (1)
    {
        // Wait for data ready signal
        sem_acquire_blocking(&data_ready);

        // Read global variables
        float temp = temperature;
        float pres = pressure;
        float alt = altitude;
        float hum = humidity;

        // Signal display task done
        sem_release(&display_done);

        // Display temperature
        HT16K33_print(&display, "TEMP");
        sleep_ms(250);
        display_data(temp);
        sleep_ms(750);

        // Toggle onboard LED
        onboarding_led_state = !onboarding_led_state;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, onboarding_led_state);

        // Display pressure
        HT16K33_print(&display, "PRES");
        sleep_ms(250);
        display_data(pres);
        sleep_ms(750);

        // Toggle onboard LED
        onboarding_led_state = !onboarding_led_state;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, onboarding_led_state);

        // Display altitude
        HT16K33_print(&display, "ALTD");
        sleep_ms(250);
        display_data(alt);
        sleep_ms(750);

        // Toggle onboard LED
        onboarding_led_state = !onboarding_led_state;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, onboarding_led_state);

        // Display humidity
        HT16K33_print(&display, "HUMD");
        sleep_ms(250);
        display_data(hum);
        sleep_ms(750);

        // Toggle onboard LED
        onboarding_led_state = !onboarding_led_state;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, onboarding_led_state);
    }
}

int main()
{
    stdio_init_all();

    // Initialize semaphores
    sem_init(&data_ready, 0, 1);
    sem_init(&display_done, 1, 1);

    // Sleep for 3 seconds to give time to open the serial terminal
    sleep_ms(3000);

    multicore_reset_core1();
    multicore_launch_core1(display_task);

    read_sensor_and_send();

    return 0;
}