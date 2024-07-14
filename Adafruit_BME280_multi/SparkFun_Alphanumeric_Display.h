#ifndef __SPARKFUN_ALPHANUMERIC_DISPLAY_H__
#define __SPARKFUN_ALPHANUMERIC_DISPLAY_H__

#include "hardware/i2c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_ADDRESS 0x70 // Default I2C address when A0, A1 are floating
#define DEFAULT_NOTHING_ATTACHED 0xFF

#define SEG_A 0x0001
#define SEG_B 0x0002
#define SEG_C 0x0004
#define SEG_D 0x0008
#define SEG_E 0x0010
#define SEG_F 0x0020
#define SEG_G 0x0040
#define SEG_H 0x0080
#define SEG_I 0x0100
#define SEG_J 0x0200
#define SEG_K 0x0400
#define SEG_L 0x0800
#define SEG_M 0x1000
#define SEG_N 0x2000

typedef enum
{
    ALPHA_BLINK_RATE_NOBLINK = 0b00,
    ALPHA_BLINK_RATE_2HZ = 0b01,
    ALPHA_BLINK_RATE_1HZ = 0b10,
    ALPHA_BLINK_RATE_0_5HZ = 0b11,
} alpha_blink_rate_t;

typedef enum
{
    ALPHA_DISPLAY_ON = 0b1,
    ALPHA_DISPLAY_OFF = 0b0,
} alpha_display_t;

typedef enum
{
    ALPHA_DECIMAL_ON = 0b1,
    ALPHA_DECIMAL_OFF = 0b0,
} alpha_decimal_t;

typedef enum
{
    ALPHA_COLON_ON = 0b1,
    ALPHA_COLON_OFF = 0b0,
} alpha_colon_t;

typedef enum
{
    ALPHA_CMD_SYSTEM_SETUP = 0b00100000,
    ALPHA_CMD_DISPLAY_SETUP = 0b10000000,
    ALPHA_CMD_DIMMING_SETUP = 0b11100000,
} alpha_command_t;

struct CharDef
{
    uint8_t position;
    int16_t segments;
    struct CharDef *next;
};

typedef struct
{
    i2c_inst_t *i2c_port;
    uint8_t device_address_display_one;
    uint8_t device_address_display_two;
    uint8_t device_address_display_three;
    uint8_t device_address_display_four;
    uint8_t digit_position;
    uint8_t number_of_displays;
    bool display_on_off;
    bool decimal_on_off;
    bool colon_on_off;
    uint8_t blink_rate;
    uint8_t display_ram[16 * 4];
    char display_content[4 * 4 + 1];
    struct CharDef *char_def_list;
} HT16K33;

bool HT16K33_begin(HT16K33 *display, uint8_t addressDisplayOne, uint8_t addressDisplayTwo, uint8_t addressDisplayThree, uint8_t addressDisplayFour, i2c_inst_t *i2c_port);
bool HT16K33_isConnected(HT16K33 *display, uint8_t displayNumber);
bool HT16K33_initialize(HT16K33 *display);
uint8_t HT16K33_lookUpDisplayAddress(HT16K33 *display, uint8_t displayNumber);

bool HT16K33_clear(HT16K33 *display);
bool HT16K33_setBrightness(HT16K33 *display, uint8_t duty);
bool HT16K33_setBrightnessSingle(HT16K33 *display, uint8_t displayNumber, uint8_t duty);
bool HT16K33_setBlinkRate(HT16K33 *display, float rate);
bool HT16K33_setBlinkRateSingle(HT16K33 *display, uint8_t displayNumber, float rate);
bool HT16K33_displayOn(HT16K33 *display);
bool HT16K33_displayOff(HT16K33 *display);
bool HT16K33_displayOnSingle(HT16K33 *display, uint8_t displayNumber);
bool HT16K33_displayOffSingle(HT16K33 *display, uint8_t displayNumber);
bool HT16K33_setDisplayOnOff(HT16K33 *display, uint8_t displayNumber, bool turnOnDisplay);

bool HT16K33_enableSystemClock(HT16K33 *display);
bool HT16K33_disableSystemClock(HT16K33 *display);
bool HT16K33_enableSystemClockSingle(HT16K33 *display, uint8_t displayNumber);
bool HT16K33_disableSystemClockSingle(HT16K33 *display, uint8_t displayNumber);

void HT16K33_illuminateSegment(HT16K33 *display, uint8_t segment, uint8_t digit);
void HT16K33_illuminateChar(HT16K33 *display, uint16_t disp, uint8_t digit);
void HT16K33_printChar(HT16K33 *display, uint8_t display_char, uint8_t digit);
bool HT16K33_updateDisplay(HT16K33 *display);

bool HT16K33_defineChar(HT16K33 *display, uint8_t displayChar, uint16_t segmentsToTurnOn);
uint16_t HT16K33_getSegmentsToTurnOn(HT16K33 *display, uint8_t charPos);

bool HT16K33_decimalOn(HT16K33 *display);
bool HT16K33_decimalOff(HT16K33 *display);
bool HT16K33_decimalOnSingle(HT16K33 *display, uint8_t displayNumber, bool updateNow);
bool HT16K33_decimalOffSingle(HT16K33 *display, uint8_t displayNumber, bool updateNow);
bool HT16K33_setDecimalOnOff(HT16K33 *display, uint8_t displayNumber, bool turnOnDecimal, bool updateNow);

bool HT16K33_colonOn(HT16K33 *display);
bool HT16K33_colonOff(HT16K33 *display);
bool HT16K33_colonOnSingle(HT16K33 *display, uint8_t displayNumber, bool updateNow);
bool HT16K33_colonOffSingle(HT16K33 *display, uint8_t displayNumber, bool updateNow);
bool HT16K33_setColonOnOff(HT16K33 *display, uint8_t displayNumber, bool turnOnColon, bool updateNow);

bool HT16K33_shiftRight(HT16K33 *display, uint8_t shiftAmt);
bool HT16K33_shiftLeft(HT16K33 *display, uint8_t shiftAmt);
bool HT16K33_print(HT16K33 *display, const char *str);

bool HT16K33_readRAM(HT16K33 *display, uint8_t address, uint8_t reg, uint8_t *buff, uint8_t buffSize);
bool HT16K33_writeRAM(HT16K33 *display, uint8_t address, uint8_t reg, uint8_t *buff, uint8_t buffSize);
bool HT16K33_writeRAM_single(HT16K33 *display, uint8_t address, uint8_t dataToWrite);

#endif