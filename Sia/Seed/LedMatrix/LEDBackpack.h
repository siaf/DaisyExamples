#pragma once
#ifndef LEDBackpack_h
#define LEDBackpack_h

#include "daisy_seed.h"


#define LED_OFF 0 ///< GFX color of unlit LED segments (single-color displays)
#define LED_RED 1 ///< GFX color for red LED segments (bi-color displays)
#define LED_YELLOW 2 ///< GFX color for yellow LED segments (bi-color displays)
#define LED_GREEN 3  ///< GFX color for green LED segments (bi-color displays)

#define HT16K33_BLINK_CMD 0x80       ///< I2C register for BLINK setting
#define HT16K33_BLINK_DISPLAYON 0x01 ///< I2C value for steady on
#define HT16K33_BLINK_OFF 0          ///< I2C value for steady off
#define HT16K33_BLINK_2HZ 1          ///< I2C value for 2 Hz blink
#define HT16K33_BLINK_1HZ 2          ///< I2C value for 1 Hz blink
#define HT16K33_BLINK_HALFHZ 3       ///< I2C value for 0.5 Hz blink

#define HT16K33_CMD_BRIGHTNESS 0xE0 ///< I2C register for BRIGHTNESS setting
#define HT16K33_BASE_ADDRESS 0x70

#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#endif
#ifndef pgm_read_word
#define pgm_read_word(addr) (*(const unsigned short *)(addr))
#endif
#ifndef pgm_read_dword
#define pgm_read_dword(addr) (*(const unsigned long *)(addr))
#endif
#ifndef _swap_int16_t
#define _swap_int16_t(a, b) \
    {                       \
        int16_t t = a;      \
        a         = b;      \
        b         = t;      \
    } ///< 16-bit var swap
#endif

using namespace daisy;

class BiColorMatrixLedDisplay
: public OneBitGraphicsDisplayImpl<BiColorMatrixLedDisplay>
{
  public:
    struct Config
    {
        I2CHandle::Config i2c_config;
        uint8_t           i2c_address;
        uint8_t           rotation;
        uint8_t           color;
        uint8_t           blink_rate;
        uint8_t           brightness;

        Config()
        {
            i2c_config.speed          = I2CHandle::Config::Speed::I2C_400KHZ;
            i2c_config.mode           = I2CHandle::Config::Mode::I2C_MASTER;
            i2c_config.periph         = I2CHandle::Config::Peripheral::I2C_1;
            i2c_config.pin_config.scl = {DSY_GPIOB, 8};
            i2c_config.pin_config.sda = {DSY_GPIOB, 9};
            i2c_address               = HT16K33_BASE_ADDRESS;
            color                     = LED_GREEN;
            rotation                  = 0;
            blink_rate                = HT16K33_BLINK_OFF;
            brightness                = 15;
        };
    };

    bool Init(const Config &config);

    /*!
    @brief  Set display brightness.
    @param  b  Brightness: 0 (min) to 15 (max).
  */
    void SetBrightness(uint8_t b);

    /*!
    @brief  Set display blink rate.
    @param  b  One of:
               HT16K33_BLINK_DISPLAYON = steady on
               HT16K33_BLINK_OFF       = steady off
               HT16K33_BLINK_2HZ       = 2 Hz blink
               HT16K33_BLINK_1HZ       = 1 Hz blink
               HT16K33_BLINK_HALFHZ    = 0.5 Hz blink
  */
    void SetBlinkRate(uint8_t b);

    /*!
    @brief  Issue buffered data in RAM to display.
  */
    void Update();

    /*!
    @brief  Clear display.
  */
    void Clear();


    uint16_t displaybuffer[8]; ///< Raw display data

    void DrawBitmap(const uint bitmap[], uint16_t color)
    {
        uint8_t x = 0, y = 0;
        uint8_t w = 8, h = 8;
        int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
        uint8_t b         = 0;


        for(int16_t j = 0; j < h; j++, y++)
        {
            for(int16_t i = 0; i < w; i++)
            {
                if(i & 7)
                    b <<= 1;
                else
                    b = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
                if(b & 0x80)
                    DrawPixelColor(x + i, y, color);
            }
        }
    }

    /** 
    Fills the entire display with either on/off using color that's been set.
    \param on Sets on or off.
    */
    void Fill(bool on) override
    {
        for(int i = 0; i < 8; i++)
        {
            for(int j = 0; j < 8; j++)
            {
                if(on)
                {
                    DrawPixelColor(i, j, _color);
                }
                else
                {
                    DrawPixelColor(i, j, LED_OFF);
                }
            }
        }
    }
    /*!
    @brief  Lowest-level pixel drawing function required by Adafruit_GFX.
            Does not have an immediate effect -- must call writeDisplay()
            after any drawing operations to refresh display contents.
    @param  x      Pixel column (horizontal).
    @param  y      Pixel row (vertical).
    @param  color  Pixel color (LED_OFF, LED_GREEN, LED_YELLOW or LED_RED).
  */
    void DrawPixelColor(int16_t x, int16_t y, uint16_t color);

    /**
    Sets the pixel at the specified coordinate to be on/off.
    \param x   x Coordinate
    \param y   y coordinate
    \param on  on or off
    */
    void DrawPixel(uint_fast8_t x, uint_fast8_t y, bool on) override
    {
        if(on)
        {
            DrawPixelColor(x, y, _color);
        }
        else
        {
            DrawPixelColor(x, y, LED_OFF);
        }
    }
    /************************************************************************/
    /*!
    @brief      Get rotation setting for display
    @returns    0 thru 3 corresponding to 4 cardinal rotations
  */
    /************************************************************************/
    uint8_t GetRotation() const { return _rotation; }

    /**************************************************************************/
    /*!
    @brief      Set rotation setting for display
    @param  x   0 thru 3 corresponding to 4 cardinal rotations
*/
    /**************************************************************************/
    void SetRotation(uint8_t x) { _rotation = (x & 3); }


    uint8_t GetColor() const { return _color; }
    void    SetColor(uint8_t color) { _color = color; }

    uint16_t Height() const override { return 8; }
    uint16_t Width() const override { return 8; }

  private:
    uint8_t          _rotation; ///< Display rotation (0 thru 3)
    uint8_t          _color = LED_GREEN;
    daisy::I2CHandle i2c_; ///< Pointer to I2C bus interface

    uint8_t _addr;
    uint8_t rotation; ///< Display rotation (0 thru 3)
};
#endif