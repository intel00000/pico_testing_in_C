#include "SparkFun_Alphanumeric_Display.h"

/*--------------------------- Character Map ----------------------------------*/
#define SFE_ALPHANUM_UNKNOWN_CHAR 95

static const uint16_t alphanumeric_segs[96] = {
    0b00000000000000, // ' ' (space)
    0b00001000001000, // '!'
    0b00001000000010, // '"'
    0b01001101001110, // '#'
    0b01001101101101, // '$'
    0b10010000100100, // '%'
    0b00110011011001, // '&'
    0b00001000000000, // '''
    0b00000000111001, // '('
    0b00000000001111, // ')'
    0b11111010000000, // '*'
    0b01001101000000, // '+'
    0b10000000000000, // ','
    0b00000101000000, // '-'
    0b00000000000000, // '.'
    0b10010000000000, // '/'
    0b00000000111111, // '0'
    0b00010000000110, // '1'
    0b00000101011011, // '2'
    0b00000101001111, // '3'
    0b00000101100110, // '4'
    0b00000101101101, // '5'
    0b00000101111101, // '6'
    0b01010000000001, // '7'
    0b00000101111111, // '8'
    0b00000101100111, // '9'
    0b00000000000000, // ':'
    0b10001000000000, // ';'
    0b00110000000000, // '<'
    0b00000101001000, // '='
    0b01000010000000, // '>'
    0b01000100000011, // '?'
    0b00001100111011, // '@'
    0b00000101110111, // 'A'
    0b01001100001111, // 'B'
    0b00000000111001, // 'C'
    0b01001000001111, // 'D'
    0b00000101111001, // 'E'
    0b00000101110001, // 'F'
    0b00000100111101, // 'G'
    0b00000101110110, // 'H'
    0b01001000001001, // 'I'
    0b00000000011110, // 'J'
    0b00110001110000, // 'K'
    0b00000000111000, // 'L'
    0b00010010110110, // 'M'
    0b00100010110110, // 'N'
    0b00000000111111, // 'O'
    0b00000101110011, // 'P'
    0b00100000111111, // 'Q'
    0b00100101110011, // 'R'
    0b00000110001101, // 'S'
    0b01001000000001, // 'T'
    0b00000000111110, // 'U'
    0b10010000110000, // 'V'
    0b10100000110110, // 'W'
    0b10110010000000, // 'X'
    0b01010010000000, // 'Y'
    0b10010000001001, // 'Z'
    0b00000000111001, // '['
    0b00100010000000, // '\'
    0b00000000001111, // ']'
    0b10100000000000, // '^'
    0b00000000001000, // '_'
    0b00000010000000, // '`'
    0b00000101011111, // 'a'
    0b00100001111000, // 'b'
    0b00000101011000, // 'c'
    0b10000100001110, // 'd'
    0b00000001111001, // 'e'
    0b00000001110001, // 'f'
    0b00000110001111, // 'g'
    0b00000101110100, // 'h'
    0b01000000000000, // 'i'
    0b00000000001110, // 'j'
    0b01111000000000, // 'k'
    0b01001000000000, // 'l'
    0b01000101010100, // 'm'
    0b00100001010000, // 'n'
    0b00000101011100, // 'o'
    0b00010001110001, // 'p'
    0b00100101100011, // 'q'
    0b00000001010000, // 'r'
    0b00000110001101, // 's'
    0b00000001111000, // 't'
    0b00000000011100, // 'u'
    0b10000000010000, // 'v'
    0b10100000010100, // 'w'
    0b10110010000000, // 'x'
    0b00001100001110, // 'y'
    0b10010000001001, // 'z'
    0b10000011001001, // '{'
    0b01001000000000, // '|'
    0b00110100001001, // '}'
    0b00000101010010, // '~'
    0b11111111111111, // Unknown character (DEL or RUBOUT)
};

/*--------------------------- Device Status----------------------------------*/

bool HT16K33_begin(HT16K33 *display, uint8_t addressDisplayOne, uint8_t addressDisplayTwo, uint8_t addressDisplayThree, uint8_t addressDisplayFour, i2c_inst_t *i2c_port)
{
    display->device_address_display_one = addressDisplayOne;
    display->device_address_display_two = addressDisplayTwo;
    display->device_address_display_three = addressDisplayThree;
    display->device_address_display_four = addressDisplayFour;

    if (display->device_address_display_four != DEFAULT_NOTHING_ATTACHED)
        display->number_of_displays = 4;
    else if (display->device_address_display_three != DEFAULT_NOTHING_ATTACHED)
        display->number_of_displays = 3;
    else if (display->device_address_display_two != DEFAULT_NOTHING_ATTACHED)
        display->number_of_displays = 2;
    else
        display->number_of_displays = 1;

    display->i2c_port = i2c_port;

    for (uint8_t i = 1; i <= display->number_of_displays; i++)
    {
        if (!HT16K33_isConnected(display, i))
        {
            return false;
        }
        sleep_ms(100);
    }

    if (!HT16K33_initialize(display))
    {
        return false;
    }

    if (!HT16K33_clear(display))
    {
        return false;
    }

    display->display_content[4 * 4] = '\0';

    return true;
}

bool HT16K33_isConnected(HT16K33 *display, uint8_t displayNumber)
{
    uint8_t triesBeforeGiveup = 5;
    uint8_t address = HT16K33_lookUpDisplayAddress(display, displayNumber);

    for (uint8_t x = 0; x < triesBeforeGiveup; x++)
    {
        if (i2c_write_blocking(display->i2c_port, address, NULL, 0, false) == PICO_ERROR_GENERIC)
        {
            return false;
        }
        sleep_ms(100);
    }
    return true;
}

bool HT16K33_initialize(HT16K33 *display)
{
    if (!HT16K33_enableSystemClock(display))
    {
        return false;
    }

    if (!HT16K33_setBrightness(display, 15))
    {
        return false;
    }

    if (!HT16K33_setBlinkRate(display, ALPHA_BLINK_RATE_NOBLINK))
    {
        return false;
    }

    if (!HT16K33_displayOn(display))
    {
        return false;
    }

    return true;
}

uint8_t HT16K33_lookUpDisplayAddress(HT16K33 *display, uint8_t displayNumber)
{
    switch (displayNumber)
    {
    case 1:
        return display->device_address_display_one;
    case 2:
        return display->device_address_display_two;
    case 3:
        return display->device_address_display_three;
    case 4:
        return display->device_address_display_four;
    }
    return 0;
}

/*-------------------------- Display configuration functions ---------------------------*/

bool HT16K33_clear(HT16K33 *display)
{
    memset(display->display_ram, 0, 16 * display->number_of_displays);
    display->digit_position = 0;
    return HT16K33_updateDisplay(display);
}

bool HT16K33_setBrightness(HT16K33 *display, uint8_t duty)
{
    bool status = true;
    for (uint8_t i = 1; i <= display->number_of_displays; i++)
    {
        if (!HT16K33_setBrightnessSingle(display, i, duty))
            status = false;
    }
    return status;
}

bool HT16K33_setBrightnessSingle(HT16K33 *display, uint8_t displayNumber, uint8_t duty)
{
    if (duty > 15)
        duty = 15;

    uint8_t dataToWrite = ALPHA_CMD_DIMMING_SETUP | duty;
    return HT16K33_writeRAM_single(display, HT16K33_lookUpDisplayAddress(display, displayNumber), dataToWrite);
}

bool HT16K33_setBlinkRate(HT16K33 *display, float rate)
{
    bool status = true;
    for (uint8_t i = 1; i <= display->number_of_displays; i++)
    {
        if (!HT16K33_setBlinkRateSingle(display, i, rate))
            status = false;
    }
    return status;
}

bool HT16K33_setBlinkRateSingle(HT16K33 *display, uint8_t displayNumber, float rate)
{
    uint8_t blinkRate = ALPHA_BLINK_RATE_NOBLINK;

    if (rate == 2.0)
        blinkRate = ALPHA_BLINK_RATE_2HZ;
    else if (rate == 1.0)
        blinkRate = ALPHA_BLINK_RATE_1HZ;
    else if (rate == 0.5)
        blinkRate = ALPHA_BLINK_RATE_0_5HZ;

    uint8_t dataToWrite = ALPHA_CMD_DISPLAY_SETUP | (blinkRate << 1) | display->display_on_off;
    return HT16K33_writeRAM_single(display, HT16K33_lookUpDisplayAddress(display, displayNumber), dataToWrite);
}

bool HT16K33_displayOn(HT16K33 *display)
{
    bool status = true;
    display->display_on_off = ALPHA_DISPLAY_ON;

    for (uint8_t i = 1; i <= display->number_of_displays; i++)
    {
        if (!HT16K33_displayOnSingle(display, i))
            status = false;
    }
    return status;
}

bool HT16K33_displayOff(HT16K33 *display)
{
    bool status = true;
    display->display_on_off = ALPHA_DISPLAY_OFF;

    for (uint8_t i = 1; i <= display->number_of_displays; i++)
    {
        if (!HT16K33_displayOffSingle(display, i))
            status = false;
    }
    return status;
}

bool HT16K33_displayOnSingle(HT16K33 *display, uint8_t displayNumber)
{
    return HT16K33_setDisplayOnOff(display, displayNumber, true);
}

bool HT16K33_displayOffSingle(HT16K33 *display, uint8_t displayNumber)
{
    return HT16K33_setDisplayOnOff(display, displayNumber, false);
}

bool HT16K33_setDisplayOnOff(HT16K33 *display, uint8_t displayNumber, bool turnOnDisplay)
{
    display->display_on_off = turnOnDisplay ? ALPHA_DISPLAY_ON : ALPHA_DISPLAY_OFF;
    uint8_t dataToWrite = ALPHA_CMD_DISPLAY_SETUP | (display->blink_rate << 1) | display->display_on_off;
    return HT16K33_writeRAM_single(display, HT16K33_lookUpDisplayAddress(display, displayNumber), dataToWrite);
}

bool HT16K33_enableSystemClock(HT16K33 *display)
{
    bool status = true;
    for (uint8_t i = 1; i <= display->number_of_displays; i++)
    {
        if (!HT16K33_enableSystemClockSingle(display, i))
            status = false;
    }
    return status;
}

bool HT16K33_disableSystemClock(HT16K33 *display)
{
    bool status = true;
    for (uint8_t i = 1; i <= display->number_of_displays; i++)
    {
        if (!HT16K33_disableSystemClockSingle(display, i))
            status = false;
    }
    return status;
}

bool HT16K33_enableSystemClockSingle(HT16K33 *display, uint8_t displayNumber)
{
    uint8_t dataToWrite = ALPHA_CMD_SYSTEM_SETUP | 1; // Enable system clock
    bool status = HT16K33_writeRAM_single(display, HT16K33_lookUpDisplayAddress(display, displayNumber), dataToWrite);
    sleep_ms(1);
    return status;
}

bool HT16K33_disableSystemClockSingle(HT16K33 *display, uint8_t displayNumber)
{
    uint8_t dataToWrite = ALPHA_CMD_SYSTEM_SETUP | 0; // Standby mode
    return HT16K33_writeRAM_single(display, HT16K33_lookUpDisplayAddress(display, displayNumber), dataToWrite);
}

/*---------------------------- Light up functions ---------------------------------*/

void HT16K33_illuminateSegment(HT16K33 *display, uint8_t segment, uint8_t digit)
{
    uint8_t com = segment - 'A';

    if (com > 6)
        com -= 7;
    if (segment == 'I')
        com = 0;
    if (segment == 'H')
        com = 1;

    uint8_t row = digit % 4;
    if (segment > 'G')
        row += 4;

    uint8_t offset = digit / 4 * 16;
    uint8_t adr = com * 2 + offset;

    if (row > 7)
        adr++;

    if (row > 7)
        row -= 8;
    uint8_t dat = 1 << row;

    display->display_ram[adr] |= dat;
}

void HT16K33_illuminateChar(HT16K33 *display, uint16_t segmentsToTurnOn, uint8_t digit)
{
    for (uint8_t i = 0; i < 14; i++)
    {
        if ((segmentsToTurnOn >> i) & 0b1)
            HT16K33_illuminateSegment(display, 'A' + i, digit);
    }
}

void HT16K33_printChar(HT16K33 *display, uint8_t displayChar, uint8_t digit)
{
    uint16_t characterPosition = 65532;

    if (displayChar == ' ')
        characterPosition = 0;
    else if (displayChar >= '!' && displayChar <= '~')
    {
        characterPosition = displayChar - '!' + 1;
    }

    uint8_t dispNum = display->digit_position / 4;

    if (characterPosition == 14)
        HT16K33_decimalOnSingle(display, dispNum + 1, false);
    if (characterPosition == 26)
        HT16K33_colonOnSingle(display, dispNum + 1, false);
    if (characterPosition == 65532)
        characterPosition = SFE_ALPHANUM_UNKNOWN_CHAR;

    uint16_t segmentsToTurnOn = HT16K33_getSegmentsToTurnOn(display, characterPosition);
    HT16K33_illuminateChar(display, segmentsToTurnOn, digit);
}

bool HT16K33_updateDisplay(HT16K33 *display)
{
    bool status = true;
    for (uint8_t i = 1; i <= display->number_of_displays; i++)
    {
        if (!HT16K33_writeRAM(display, HT16K33_lookUpDisplayAddress(display, i), 0, display->display_ram + ((i - 1) * 16), 16))
        {
            status = false;
        }
    }
    return status;
}

bool HT16K33_defineChar(HT16K33 *display, uint8_t displayChar, uint16_t segmentsToTurnOn)
{
    if (displayChar >= '!' && displayChar <= '~')
    {
        uint16_t characterPosition = displayChar - '!' + 1;
        struct CharDef *pNewCharDef = (struct CharDef *)calloc(1, sizeof(struct CharDef));
        pNewCharDef->position = characterPosition;
        pNewCharDef->segments = segmentsToTurnOn & 0x3FFF;
        pNewCharDef->next = NULL;

        if (display->char_def_list == NULL)
        {
            display->char_def_list = pNewCharDef;
        }
        else
        {
            struct CharDef *pTail = display->char_def_list;
            while (pTail->next != NULL)
            {
                pTail = pTail->next;
            }
            pTail->next = pNewCharDef;
        }
        return true;
    }
    return false;
}

uint16_t HT16K33_getSegmentsToTurnOn(HT16K33 *display, uint8_t charPos)
{
    struct CharDef *pDefChar = display->char_def_list;

    while (pDefChar && (pDefChar->position != charPos))
    {
        pDefChar = pDefChar->next;
    }

    if (pDefChar != NULL)
    {
        return pDefChar->segments;
    }
    else
    {
        return alphanumeric_segs[charPos];
    }
}

bool HT16K33_readRAM(HT16K33 *display, uint8_t address, uint8_t reg, uint8_t *buff, uint8_t buffSize)
{
    uint8_t displayNum = 1;
    if (address == display->device_address_display_two)
        displayNum = 2;
    else if (address == display->device_address_display_three)
        displayNum = 3;
    else if (address == display->device_address_display_four)
        displayNum = 4;
    HT16K33_isConnected(display, displayNum);

    if (i2c_write_blocking(display->i2c_port, address, &reg, 1, true) == PICO_ERROR_GENERIC)
    {
        return false;
    }

    if (i2c_read_blocking(display->i2c_port, address, buff, buffSize, false) > 0)
    {
        return true;
    }

    return false;
}

bool HT16K33_writeRAM(HT16K33 *display, uint8_t address, uint8_t reg, uint8_t *buff, uint8_t buffSize)
{
    uint8_t displayNum = 1;
    if (address == display->device_address_display_two)
        displayNum = 2;
    else if (address == display->device_address_display_three)
        displayNum = 3;
    else if (address == display->device_address_display_four)
        displayNum = 4;
    HT16K33_isConnected(display, displayNum);

    uint8_t data[buffSize + 1];
    data[0] = reg;
    memcpy(&data[1], buff, buffSize);

    if (i2c_write_blocking(display->i2c_port, address, data, buffSize + 1, false) == PICO_ERROR_GENERIC)
    {
        return false;
    }

    return true;
}

bool HT16K33_writeRAM_single(HT16K33 *display, uint8_t address, uint8_t dataToWrite)
{
    return HT16K33_writeRAM(display, address, dataToWrite, NULL, 0);
}

/*---------------------------- Decimal functions ---------------------------------*/

bool HT16K33_decimalOn(HT16K33 *display)
{
    bool status = true;
    display->decimal_on_off = ALPHA_DECIMAL_ON;

    for (uint8_t i = 1; i <= display->number_of_displays; i++)
    {
        if (!HT16K33_decimalOnSingle(display, i, true))
            status = false;
    }

    return status;
}

bool HT16K33_decimalOff(HT16K33 *display)
{
    bool status = true;
    display->decimal_on_off = ALPHA_DECIMAL_OFF;

    for (uint8_t i = 1; i <= display->number_of_displays; i++)
    {
        if (!HT16K33_decimalOffSingle(display, i, true))
            status = false;
    }
    return status;
}

bool HT16K33_decimalOnSingle(HT16K33 *display, uint8_t displayNumber, bool updateNow)
{
    return HT16K33_setDecimalOnOff(display, displayNumber, true, updateNow);
}

bool HT16K33_decimalOffSingle(HT16K33 *display, uint8_t displayNumber, bool updateNow)
{
    return HT16K33_setDecimalOnOff(display, displayNumber, false, updateNow);
}

bool HT16K33_setDecimalOnOff(HT16K33 *display, uint8_t displayNumber, bool turnOnDecimal, bool updateNow)
{
    uint8_t adr = 0x03;
    uint8_t dat = turnOnDecimal ? 0x01 : 0x00;

    display->display_ram[adr + (displayNumber - 1) * 16] &= 0xFE;
    display->display_ram[adr + (displayNumber - 1) * 16] |= dat;

    if (updateNow)
    {
        return HT16K33_updateDisplay(display);
    }
    else
    {
        return true;
    }
}

/*---------------------------- Colon functions ---------------------------------*/

bool HT16K33_colonOn(HT16K33 *display)
{
    bool status = true;
    display->colon_on_off = ALPHA_COLON_ON;

    for (uint8_t i = 1; i <= display->number_of_displays; i++)
    {
        if (!HT16K33_colonOnSingle(display, i, true))
            status = false;
    }
    return status;
}

bool HT16K33_colonOff(HT16K33 *display)
{
    bool status = true;
    display->colon_on_off = ALPHA_COLON_OFF;

    for (uint8_t i = 1; i <= display->number_of_displays; i++)
    {
        if (!HT16K33_colonOffSingle(display, i, true))
            status = false;
    }
    return status;
}

bool HT16K33_colonOnSingle(HT16K33 *display, uint8_t displayNumber, bool updateNow)
{
    return HT16K33_setColonOnOff(display, displayNumber, true, updateNow);
}

bool HT16K33_colonOffSingle(HT16K33 *display, uint8_t displayNumber, bool updateNow)
{
    return HT16K33_setColonOnOff(display, displayNumber, false, updateNow);
}

bool HT16K33_setColonOnOff(HT16K33 *display, uint8_t displayNumber, bool turnOnColon, bool updateNow)
{
    uint8_t adr = 0x01;
    uint8_t dat = turnOnColon ? 0x01 : 0x00;

    display->display_ram[adr + (displayNumber - 1) * 16] &= 0xFE;
    display->display_ram[adr + (displayNumber - 1) * 16] |= dat;

    if (updateNow)
    {
        return HT16K33_updateDisplay(display);
    }
    else
    {
        return true;
    }
}

/*---------------------------- Shifting functions ---------------------------------*/

bool HT16K33_shiftRight(HT16K33 *display, uint8_t shiftAmt)
{
    for (int x = (4 * display->number_of_displays) - shiftAmt; x >= shiftAmt; x--)
    {
        display->display_content[x] = display->display_content[x - shiftAmt];
    }

    for (uint8_t x = 0; x < shiftAmt; x++)
    {
        if (x + shiftAmt > (4 * display->number_of_displays))
            break;
        display->display_content[0 + x] = ' ';
    }

    return HT16K33_print(display, display->display_content);
}

bool HT16K33_shiftLeft(HT16K33 *display, uint8_t shiftAmt)
{
    for (int x = 0; x < 4 * display->number_of_displays; x++)
    {
        if (x + shiftAmt > (4 * display->number_of_displays))
            break;
        display->display_content[x] = display->display_content[x + shiftAmt];
    }

    for (int x = 0; x < shiftAmt; x++)
    {
        if (4 * display->number_of_displays - 1 - x < 0)
            break;
        display->display_content[4 * display->number_of_displays - 1 - x] = ' ';
    }

    return HT16K33_print(display, display->display_content);
}

bool HT16K33_print(HT16K33 *display, const char *str)
{
    if (str == NULL)
        return false;

    memset(display->display_ram, 0, 16 * display->number_of_displays);
    display->digit_position = 0;

    for (size_t i = 0; i < strlen(str) && display->digit_position < (4 * display->number_of_displays); i++)
    {
        char c = str[i];
        if (c == '.')
            HT16K33_printChar(display, '.', 0);
        else if (c == ':')
            HT16K33_printChar(display, ':', 0);
        else
        {
            HT16K33_printChar(display, c, display->digit_position);
            display->display_content[display->digit_position] = c;
            display->digit_position++;
        }
    }

    return HT16K33_updateDisplay(display);
}