#include <math.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "Adafruit_BME280.h"

static uint8_t read8(bme280_t *dev, uint8_t reg);
static uint16_t read16(bme280_t *dev, uint8_t reg);
static uint32_t read24(bme280_t *dev, uint8_t reg);
static int16_t readS16(bme280_t *dev, uint8_t reg);
static uint16_t read16_LE(bme280_t *dev, uint8_t reg);
static int16_t readS16_LE(bme280_t *dev, uint8_t reg);
static void write8(bme280_t *dev, uint8_t reg, uint8_t value);
static void readCoefficients(bme280_t *dev);
static bool isReadingCalibration(bme280_t *dev);

bool bme280_init(bme280_t *dev, i2c_inst_t *i2c, uint8_t address)
{
  dev->i2c = i2c;
  dev->address = address;
  dev->spi = NULL;

  uint8_t id = read8(dev, BME280_REGISTER_CHIPID);
  if (id != 0x60)
    return false;

  write8(dev, BME280_REGISTER_SOFTRESET, 0xB6);
  sleep_ms(10);

  while (isReadingCalibration(dev))
    sleep_ms(10);

  readCoefficients(dev);
  bme280_set_sampling(dev, MODE_NORMAL, SAMPLING_X16, SAMPLING_X16, SAMPLING_X16, FILTER_OFF, STANDBY_MS_0_5);

  return true;
}

bool bme280_init_spi(bme280_t *dev, spi_inst_t *spi, uint8_t cs_pin)
{
  dev->spi = spi;
  gpio_init(cs_pin);
  gpio_set_dir(cs_pin, GPIO_OUT);
  gpio_put(cs_pin, 1);

  uint8_t id = read8(dev, BME280_REGISTER_CHIPID);
  if (id != 0x60)
    return false;

  write8(dev, BME280_REGISTER_SOFTRESET, 0xB6);
  sleep_ms(10);

  while (isReadingCalibration(dev))
    sleep_ms(10);

  readCoefficients(dev);
  bme280_set_sampling(dev, MODE_NORMAL, SAMPLING_X16, SAMPLING_X16, SAMPLING_X16, FILTER_OFF, STANDBY_MS_0_5);

  return true;
}

void bme280_set_sampling(bme280_t *dev, sensor_mode mode, sensor_sampling tempSampling, sensor_sampling pressSampling, sensor_sampling humSampling, sensor_filter filter, standby_duration duration)
{
  dev->mode = mode;
  dev->temp_sampling = tempSampling;
  dev->press_sampling = pressSampling;
  dev->hum_sampling = humSampling;
  dev->filter = filter;
  dev->duration = duration;

  write8(dev, BME280_REGISTER_CONTROLHUMID, humSampling);
  write8(dev, BME280_REGISTER_CONFIG, (duration << 5) | (filter << 2));
  write8(dev, BME280_REGISTER_CONTROL, (tempSampling << 5) | (pressSampling << 2) | mode);
}

bool bme280_take_forced_measurement(bme280_t *dev)
{
  if (dev->mode != MODE_FORCED)
    return true;

  write8(dev, BME280_REGISTER_CONTROL, (dev->temp_sampling << 5) | (dev->press_sampling << 2) | MODE_FORCED);

  uint32_t start = to_ms_since_boot(get_absolute_time());
  while (read8(dev, BME280_REGISTER_STATUS) & 0x08)
  {
    if (to_ms_since_boot(get_absolute_time()) - start > 2000)
      return false;
    sleep_ms(1);
  }
  return true;
}

float bme280_read_temperature(bme280_t *dev)
{
  int32_t var1, var2;

  int32_t adc_T = read24(dev, BME280_REGISTER_TEMPDATA_MSB) >> 4;

  var1 = ((((adc_T >> 3) - ((int32_t)dev->calib_data.dig_T1 << 1))) * ((int32_t)dev->calib_data.dig_T2)) >> 11;
  var2 = (((((adc_T >> 4) - ((int32_t)dev->calib_data.dig_T1)) * ((adc_T >> 4) - ((int32_t)dev->calib_data.dig_T1))) >> 12) * ((int32_t)dev->calib_data.dig_T3)) >> 14;

  dev->t_fine = var1 + var2 + dev->t_fine_adjust;

  float T = (dev->t_fine * 5 + 128) >> 8;

  return T / 100.0;
}

float bme280_read_pressure(bme280_t *dev)
{
  int64_t var1, var2, p;

  bme280_read_temperature(dev);

  int32_t adc_P = read24(dev, BME280_REGISTER_PRESSUREDATA_MSB) >> 4;

  var1 = ((int64_t)dev->t_fine) - 128000;
  var2 = var1 * var1 * (int64_t)dev->calib_data.dig_P6;
  var2 = var2 + ((var1 * (int64_t)dev->calib_data.dig_P5) << 17);
  var2 = var2 + (((int64_t)dev->calib_data.dig_P4) << 35);
  var1 = ((var1 * var1 * (int64_t)dev->calib_data.dig_P3) >> 8) + ((var1 * (int64_t)dev->calib_data.dig_P2) << 12);
  var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dev->calib_data.dig_P1) >> 33;

  if (var1 == 0)
    return 0;

  p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((int64_t)dev->calib_data.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((int64_t)dev->calib_data.dig_P8) * p) >> 19;

  p = ((p + var1 + var2) >> 8) + (((int64_t)dev->calib_data.dig_P7) << 4);

  return (float)p / 256;
}

float bme280_read_humidity(bme280_t *dev)
{
  bme280_read_temperature(dev);

  int32_t adc_H = read16(dev, BME280_REGISTER_HUMIDDATA_MSB);

  int32_t v_x1_u32r = dev->t_fine - ((int32_t)76800);

  v_x1_u32r = (((((adc_H << 14) - (((int32_t)dev->calib_data.dig_H4) << 20) - (((int32_t)dev->calib_data.dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)dev->calib_data.dig_H6)) >> 10) * (((v_x1_u32r * ((int32_t)dev->calib_data.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)dev->calib_data.dig_H2) + 8192) >> 14));

  v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)dev->calib_data.dig_H1)) >> 4));
  v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
  v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);

  return (float)(v_x1_u32r >> 12) / 1024.0;
}

float bme280_read_altitude(bme280_t *dev, float seaLevel)
{
  float pressure = bme280_read_pressure(dev) / 100.0;
  return 44330.0 * (1.0 - pow(pressure / seaLevel, 0.1903));
}

float bme280_sea_level_for_altitude(bme280_t *dev, float altitude, float pressure)
{
  return pressure / pow(1.0 - (altitude / 44330.0), 5.255);
}

uint32_t bme280_sensor_id(bme280_t *dev)
{
  return dev->sensorID;
}

float bme280_get_temperature_compensation(bme280_t *dev)
{
  return (float)(dev->t_fine_adjust * 5) / 128 / 100.0;
}

void bme280_set_temperature_compensation(bme280_t *dev, float adjustment)
{
  dev->t_fine_adjust = (int32_t)(adjustment * 100) << 8 / 5;
}

static uint8_t read8(bme280_t *dev, uint8_t reg)
{
  uint8_t value;
  if (dev->spi)
  {
    gpio_put(dev->address, 0);
    spi_write_blocking(dev->spi, &reg, 1);
    spi_read_blocking(dev->spi, 0, &value, 1);
    gpio_put(dev->address, 1);
  }
  else
  {
    i2c_write_blocking(dev->i2c, dev->address, &reg, 1, true);
    i2c_read_blocking(dev->i2c, dev->address, &value, 1, false);
  }
  return value;
}

static uint16_t read16(bme280_t *dev, uint8_t reg)
{
  uint8_t buffer[2];
  if (dev->spi)
  {
    gpio_put(dev->address, 0);
    spi_write_blocking(dev->spi, &reg, 1);
    spi_read_blocking(dev->spi, 0, buffer, 2);
    gpio_put(dev->address, 1);
  }
  else
  {
    i2c_write_blocking(dev->i2c, dev->address, &reg, 1, true);
    i2c_read_blocking(dev->i2c, dev->address, buffer, 2, false);
  }
  return (buffer[0] << 8) | buffer[1];
}

static uint32_t read24(bme280_t *dev, uint8_t reg)
{
  uint8_t buffer[3];
  if (dev->spi)
  {
    gpio_put(dev->address, 0);
    spi_write_blocking(dev->spi, &reg, 1);
    spi_read_blocking(dev->spi, 0, buffer, 3);
    gpio_put(dev->address, 1);
  }
  else
  {
    i2c_write_blocking(dev->i2c, dev->address, &reg, 1, true);
    i2c_read_blocking(dev->i2c, dev->address, buffer, 3, false);
  }
  return (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
}

static int16_t readS16(bme280_t *dev, uint8_t reg)
{
  return (int16_t)read16(dev, reg);
}

static uint16_t read16_LE(bme280_t *dev, uint8_t reg)
{
  uint16_t temp = read16(dev, reg);
  return (temp >> 8) | (temp << 8);
}

static int16_t readS16_LE(bme280_t *dev, uint8_t reg)
{
  return (int16_t)read16_LE(dev, reg);
}

static void write8(bme280_t *dev, uint8_t reg, uint8_t value)
{
  uint8_t buffer[2] = {reg, value};
  if (dev->spi)
  {
    gpio_put(dev->address, 0);
    spi_write_blocking(dev->spi, buffer, 2);
    gpio_put(dev->address, 1);
  }
  else
  {
    i2c_write_blocking(dev->i2c, dev->address, buffer, 2, false);
  }
}

static void readCoefficients(bme280_t *dev)
{
  dev->calib_data.dig_T1 = read16_LE(dev, BME280_REGISTER_DIG_T1);
  dev->calib_data.dig_T2 = readS16_LE(dev, BME280_REGISTER_DIG_T2);
  dev->calib_data.dig_T3 = readS16_LE(dev, BME280_REGISTER_DIG_T3);
  dev->calib_data.dig_P1 = read16_LE(dev, BME280_REGISTER_DIG_P1);
  dev->calib_data.dig_P2 = readS16_LE(dev, BME280_REGISTER_DIG_P2);
  dev->calib_data.dig_P3 = readS16_LE(dev, BME280_REGISTER_DIG_P3);
  dev->calib_data.dig_P4 = readS16_LE(dev, BME280_REGISTER_DIG_P4);
  dev->calib_data.dig_P5 = readS16_LE(dev, BME280_REGISTER_DIG_P5);
  dev->calib_data.dig_P6 = readS16_LE(dev, BME280_REGISTER_DIG_P6);
  dev->calib_data.dig_P7 = readS16_LE(dev, BME280_REGISTER_DIG_P7);
  dev->calib_data.dig_P8 = readS16_LE(dev, BME280_REGISTER_DIG_P8);
  dev->calib_data.dig_P9 = readS16_LE(dev, BME280_REGISTER_DIG_P9);
  dev->calib_data.dig_H1 = read8(dev, BME280_REGISTER_DIG_H1);
  dev->calib_data.dig_H2 = readS16_LE(dev, BME280_REGISTER_DIG_H2);
  dev->calib_data.dig_H3 = read8(dev, BME280_REGISTER_DIG_H3);
  dev->calib_data.dig_H4 = (read8(dev, BME280_REGISTER_DIG_H4) << 4) | (read8(dev, BME280_REGISTER_DIG_H4 + 1) & 0xF);
  dev->calib_data.dig_H5 = (read8(dev, BME280_REGISTER_DIG_H5 + 1) << 4) | (read8(dev, BME280_REGISTER_DIG_H5) >> 4);
  dev->calib_data.dig_H6 = (int8_t)read8(dev, BME280_REGISTER_DIG_H6);
}

static bool isReadingCalibration(bme280_t *dev)
{
  return (read8(dev, BME280_REGISTER_STATUS) & (1 << 0)) != 0;
}