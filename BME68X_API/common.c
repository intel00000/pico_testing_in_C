/**
 * Copyright (C) 2023 Bosch Sensortec GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "bme68x.h"
#include "common.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"

/******************************************************************************/
/*!                 Macro definitions                                         */
/*! BME68X shuttle board ID */
#define BME68X_SHUTTLE_ID 0x93
// define i2c channel
#define I2C_CHANNEL i2c0

// define custom i2c pins
#define PICO_CUSTOM_I2C_SCL_PIN 21

// define custom i2c pins
#define PICO_CUSTOM_I2C_SDA_PIN 20

/******************************************************************************/
/*!                Static variable definition                                 */
static uint8_t dev_addr;

/******************************************************************************/
/*!                User interface functions                                   */

/*!
 * I2C read function map to Pico SDK platform
 */
BME68X_INTF_RET_TYPE bme68x_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    uint8_t device_addr = *(uint8_t *)intf_ptr;

    return i2c_write_blocking(I2C_CHANNEL, device_addr, &reg_addr, 1, true) == PICO_ERROR_GENERIC ||
                   i2c_read_blocking(I2C_CHANNEL, device_addr, reg_data, len, false) == PICO_ERROR_GENERIC
               ? BME68X_E_COM_FAIL
               : BME68X_OK;
}

/*!
 * I2C write function map to Pico SDK platform
 */
BME68X_INTF_RET_TYPE bme68x_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    uint8_t device_addr = *(uint8_t *)intf_ptr;
    uint8_t temp_buff[256]; /* Typically temp_buff size is twice the len */
    temp_buff[0] = reg_addr;
    memcpy(&temp_buff[1], reg_data, len);

    return i2c_write_blocking(I2C_CHANNEL, device_addr, temp_buff, len + 1, false) == PICO_ERROR_GENERIC ? BME68X_E_COM_FAIL : BME68X_OK;
}

/*!
 * SPI read function map to Pico SDK platform
 */
BME68X_INTF_RET_TYPE bme68x_spi_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    uint8_t device_addr = *(uint8_t *)intf_ptr;
    uint8_t temp_buff[256]; /* Typically temp_buff size is len + 1 */
    temp_buff[0] = reg_addr | 0x80;

    gpio_put(device_addr, 0);                  // Set CS low
    spi_write_blocking(spi0, temp_buff, 1);    // Send register address
    spi_read_blocking(spi0, 0, reg_data, len); // Read data
    gpio_put(device_addr, 1);                  // Set CS high

    return BME68X_OK;
}

/*!
 * SPI write function map to Pico SDK platform
 */
BME68X_INTF_RET_TYPE bme68x_spi_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    uint8_t device_addr = *(uint8_t *)intf_ptr;
    uint8_t temp_buff[256]; /* Typically temp_buff size is len + 1 */
    temp_buff[0] = reg_addr;
    memcpy(&temp_buff[1], reg_data, len);

    gpio_put(device_addr, 0);                     // Set CS low
    spi_write_blocking(spi0, temp_buff, len + 1); // Send register address and data
    gpio_put(device_addr, 1);                     // Set CS high

    return BME68X_OK;
}

/*!
 * Delay function map to Pico SDK platform
 */
void bme68x_delay_us(uint32_t period, void *intf_ptr)
{
    sleep_us(period);
}

void bme68x_check_rslt(const char api_name[], int8_t rslt)
{
    switch (rslt)
    {
    case BME68X_OK:

        /* Do nothing */
        break;
    case BME68X_E_NULL_PTR:
        printf("API name [%s]  Error [%d] : Null pointer\r\n", api_name, rslt);
        break;
    case BME68X_E_COM_FAIL:
        printf("API name [%s]  Error [%d] : Communication failure\r\n", api_name, rslt);
        break;
    case BME68X_E_INVALID_LENGTH:
        printf("API name [%s]  Error [%d] : Incorrect length parameter\r\n", api_name, rslt);
        break;
    case BME68X_E_DEV_NOT_FOUND:
        printf("API name [%s]  Error [%d] : Device not found\r\n", api_name, rslt);
        break;
    case BME68X_E_SELF_TEST:
        printf("API name [%s]  Error [%d] : Self test error\r\n", api_name, rslt);
        break;
    case BME68X_W_NO_NEW_DATA:
        printf("API name [%s]  Warning [%d] : No new data found\r\n", api_name, rslt);
        break;
    default:
        printf("API name [%s]  Error [%d] : Unknown error code\r\n", api_name, rslt);
        break;
    }
}

int8_t bme68x_interface_init(struct bme68x_dev *bme, uint8_t intf)
{
    int8_t rslt = BME68X_OK;

    if (bme != NULL)
    {
        stdio_init_all();

#if defined(PC)
        setbuf(stdout, NULL);
#endif

        /* Bus configuration : I2C */
        if (intf == BME68X_I2C_INTF)
        {
            printf("I2C Interface\n");
            dev_addr = BME68X_I2C_ADDR_LOW;
            bme->read = bme68x_i2c_read;
            bme->write = bme68x_i2c_write;
            bme->intf = BME68X_I2C_INTF;

            /* Initialize I2C */
            i2c_init(I2C_CHANNEL, 100 * 1000); // 100 kHz
            gpio_set_function(PICO_CUSTOM_I2C_SDA_PIN, GPIO_FUNC_I2C);
            gpio_set_function(PICO_CUSTOM_I2C_SCL_PIN, GPIO_FUNC_I2C);
            gpio_pull_up(PICO_CUSTOM_I2C_SDA_PIN);
            gpio_pull_up(PICO_CUSTOM_I2C_SCL_PIN);
        }
        /* Bus configuration : SPI */
        else if (intf == BME68X_SPI_INTF)
        {
            printf("SPI Interface\n");
            dev_addr = PICO_DEFAULT_SPI_CSN_PIN;
            bme->read = bme68x_spi_read;
            bme->write = bme68x_spi_write;
            bme->intf = BME68X_SPI_INTF;

            /* Initialize SPI */
            spi_init(spi0, 1000 * 1000); // 1 MHz
            gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
            gpio_set_function(PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI);
            gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
            gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
            gpio_init(dev_addr);
            gpio_set_dir(dev_addr, GPIO_OUT);
            gpio_put(dev_addr, 1); // CS high
        }

        bme->delay_us = bme68x_delay_us;
        bme->intf_ptr = &dev_addr;
        bme->amb_temp = 25; /* The ambient temperature in deg C is used for defining the heater temperature */
    }
    else
    {
        rslt = BME68X_E_NULL_PTR;
    }

    return rslt;
}

void bme68x_pico_deinit(void)
{
    (void)fflush(stdout);

    /* Reset GPIO pins */
    gpio_put(dev_addr, 1);
    gpio_set_function(PICO_CUSTOM_I2C_SDA_PIN, GPIO_FUNC_NULL);
    gpio_set_function(PICO_CUSTOM_I2C_SCL_PIN, GPIO_FUNC_NULL);
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_NULL);
    gpio_set_function(PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_NULL);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_NULL);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_NULL);
}