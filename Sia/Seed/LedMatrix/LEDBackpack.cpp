#include "LEDBackpack.h"
#include "daisy_seed.h"

using namespace daisy;

void BiColorMatrixLedDisplay::SetBrightness(uint8_t b)
{
    if(b > 15)
        b = 15; // limit to max brightness
    uint8_t buffer = HT16K33_CMD_BRIGHTNESS | b;
    i2c_.TransmitBlocking(_addr, &buffer, 1, 3000);
    //i2c_dev->write(&buffer, 1);
}

void BiColorMatrixLedDisplay::SetBlinkRate(uint8_t b)
{
    if(b > 3)
        b = 0; // turn off if not sure
    uint8_t buffer = HT16K33_BLINK_CMD | HT16K33_BLINK_DISPLAYON | (b << 1);
    //i2c_dev->write(&buffer, 1);

    i2c_.TransmitBlocking(_addr, &buffer, 1, 3000);
}

bool BiColorMatrixLedDisplay::Init(const Config& config)
{
    _addr     = config.i2c_address;
    _color    = config.color;
    _rotation = config.rotation;

    i2c_.Init(config.i2c_config);

    // turn on oscillator
    uint8_t buffer[1] = {0x21};
    i2c_.TransmitBlocking(_addr, buffer, 1, 3000);
    // i2c_dev->write(buffer, 1);

    // internal RAM powers up with garbage/random values.
    // ensure internal RAM is cleared before turning on display
    // this ensures that no garbage pixels show up on the display
    // when it is turned on.
    Clear();
    Update();

    SetBlinkRate(config.blink_rate);

    SetBrightness(config.brightness); // max brightness

    return true;
}

void BiColorMatrixLedDisplay::Clear()
{
    for(uint8_t i = 0; i < 8; i++)
    {
        displaybuffer[i] = 0;
    }
}

void BiColorMatrixLedDisplay::Update()
{
    uint8_t buffer[17];

    buffer[0] = 0x00; // start at address $00

    for(uint8_t i = 0; i < 8; i++)
    {
        buffer[1 + 2 * i] = displaybuffer[i] & 0xFF;
        buffer[2 + 2 * i] = displaybuffer[i] >> 8;
    }

    i2c_.TransmitBlocking(_addr, buffer, 17, 3000);
}

void BiColorMatrixLedDisplay::DrawPixelColor(int16_t  x,
                                             int16_t  y,
                                             uint16_t color)
{
    if((y < 0) || (y >= 8))
        return;
    if((x < 0) || (x >= 8))
        return;

    switch(GetRotation())
    {
        case 1:
            _swap_int16_t(x, y);
            x = 8 - x - 1;
            break;
        case 2:
            x = 8 - x - 1;
            y = 8 - y - 1;
            break;
        case 3:
            _swap_int16_t(x, y);
            y = 8 - y - 1;
            break;
    }

    if(color == LED_GREEN)
    {
        // Turn on green LED.
        displaybuffer[y] |= 1 << x;
        // Turn off red LED.
        displaybuffer[y] &= ~(1 << (x + 8));
    }
    else if(color == LED_RED)
    {
        // Turn on red LED.
        displaybuffer[y] |= 1 << (x + 8);
        // Turn off green LED.
        displaybuffer[y] &= ~(1 << x);
    }
    else if(color == LED_YELLOW)
    {
        // Turn on green and red LED.
        displaybuffer[y] |= (1 << (x + 8)) | (1 << x);
    }
    else if(color == LED_OFF)
    {
        // Turn off green and red LED.
        displaybuffer[y] &= ~(1 << x) & ~(1 << (x + 8));
    }
}