#ifndef BME280_H
#define BME280_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"

#define BME280_ADDRESS (0x77)
#define BME280_ADDRESS_ALTERNATE (0x76)

enum
{
  BME280_REGISTER_DIG_T1 = 0x88,
  BME280_REGISTER_DIG_T2 = 0x8A,
  BME280_REGISTER_DIG_T3 = 0x8C,
  BME280_REGISTER_DIG_P1 = 0x8E,
  BME280_REGISTER_DIG_P2 = 0x90,
  BME280_REGISTER_DIG_P3 = 0x92,
  BME280_REGISTER_DIG_P4 = 0x94,
  BME280_REGISTER_DIG_P5 = 0x96,
  BME280_REGISTER_DIG_P6 = 0x98,
  BME280_REGISTER_DIG_P7 = 0x9A,
  BME280_REGISTER_DIG_P8 = 0x9C,
  BME280_REGISTER_DIG_P9 = 0x9E,
  BME280_REGISTER_DIG_H1 = 0xA1,
  BME280_REGISTER_DIG_H2 = 0xE1,
  BME280_REGISTER_DIG_H3 = 0xE3,
  BME280_REGISTER_DIG_H4 = 0xE4,
  BME280_REGISTER_DIG_H5 = 0xE5,
  BME280_REGISTER_DIG_H6 = 0xE7,

  BME280_REGISTER_CHIPID = 0xD0,
  BME280_REGISTER_VERSION = 0xD1,
  BME280_REGISTER_SOFTRESET = 0xE0,

  BME280_REGISTER_CAL26 = 0xE1,
  BME280_REGISTER_CONTROLHUMID = 0xF2,
  BME280_REGISTER_STATUS = 0xF3,
  BME280_REGISTER_CONTROL = 0xF4,
  BME280_REGISTER_CONFIG = 0xF5,

  BME280_REGISTER_PRESSUREDATA_MSB = 0xF7,
  BME280_REGISTER_PRESSUREDATA_LSB = 0xF8,
  BME280_REGISTER_PRESSUREDATA_XLSB = 0xF9,

  BME280_REGISTER_TEMPDATA_MSB = 0xFA,
  BME280_REGISTER_TEMPDATA_LSB = 0xFB,
  BME280_REGISTER_TEMPDATA_XLSB = 0xFC,

  BME280_REGISTER_HUMIDDATA_MSB = 0xFD,
  BME280_REGISTER_HUMIDDATA_LSB = 0xFE
};

typedef struct
{
  uint16_t dig_T1;
  int16_t dig_T2;
  int16_t dig_T3;
  uint16_t dig_P1;
  int16_t dig_P2;
  int16_t dig_P3;
  int16_t dig_P4;
  int16_t dig_P5;
  int16_t dig_P6;
  int16_t dig_P7;
  int16_t dig_P8;
  int16_t dig_P9;
  uint8_t dig_H1;
  int16_t dig_H2;
  uint8_t dig_H3;
  int16_t dig_H4;
  int16_t dig_H5;
  int8_t dig_H6;
} bme280_calib_data;

typedef enum
{
  SAMPLING_NONE = 0b000,
  SAMPLING_X1 = 0b001,
  SAMPLING_X2 = 0b010,
  SAMPLING_X4 = 0b011,
  SAMPLING_X8 = 0b100,
  SAMPLING_X16 = 0b101
} sensor_sampling;

typedef enum
{
  MODE_SLEEP = 0b00,
  MODE_FORCED = 0b01,
  MODE_NORMAL = 0b11
} sensor_mode;

typedef enum
{
  FILTER_OFF = 0b000,
  FILTER_X2 = 0b001,
  FILTER_X4 = 0b010,
  FILTER_X8 = 0b011,
  FILTER_X16 = 0b100
} sensor_filter;

typedef enum
{
  STANDBY_MS_0_5 = 0b000,
  STANDBY_MS_10 = 0b110,
  STANDBY_MS_20 = 0b111,
  STANDBY_MS_62_5 = 0b001,
  STANDBY_MS_125 = 0b010,
  STANDBY_MS_250 = 0b011,
  STANDBY_MS_500 = 0b100,
  STANDBY_MS_1000 = 0b101
} standby_duration;

typedef struct
{
  i2c_inst_t *i2c;
  spi_inst_t *spi;
  uint8_t address;
  int32_t sensorID;
  int32_t t_fine;
  int32_t t_fine_adjust;
  bme280_calib_data calib_data;
  sensor_mode mode;
  sensor_sampling temp_sampling;
  sensor_sampling press_sampling;
  sensor_sampling hum_sampling;
  sensor_filter filter;
  standby_duration duration;
} bme280_t;

bool bme280_init(bme280_t *dev, i2c_inst_t *i2c, uint8_t address);
bool bme280_init_spi(bme280_t *dev, spi_inst_t *spi, uint8_t cs_pin);
void bme280_set_sampling(bme280_t *dev, sensor_mode mode, sensor_sampling tempSampling, sensor_sampling pressSampling, sensor_sampling humSampling, sensor_filter filter, standby_duration duration);
bool bme280_take_forced_measurement(bme280_t *dev);
float bme280_read_temperature(bme280_t *dev);
float bme280_read_pressure(bme280_t *dev);
float bme280_read_humidity(bme280_t *dev);
float bme280_read_altitude(bme280_t *dev, float seaLevel);
float bme280_sea_level_for_altitude(bme280_t *dev, float altitude, float pressure);
uint32_t bme280_sensor_id(bme280_t *dev);
float bme280_get_temperature_compensation(bme280_t *dev);
void bme280_set_temperature_compensation(bme280_t *dev, float adjustment);

#endif // BME280_H