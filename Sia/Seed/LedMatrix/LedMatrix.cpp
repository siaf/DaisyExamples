#include "daisy_seed.h"
#include "daisysp.h"
#include "LEDBackpack.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        out[0][i] = in[0][i];
        out[1][i] = in[1][i];
    }
}

BiColorMatrixLedDisplay dp;

static const uint bmp[] = {0b000010001,
                           0b010000000,
                           0B010000000,
                           0B010000000,
                           0B010000000,
                           0B010000000,
                           0B010000000,
                           0B010000000};

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.StartAudio(AudioCallback);

    BiColorMatrixLedDisplay::Config config;
    config.brightness = 5;
    config.rotation   = 3;
    //config.blink_rate = 3;

    config.i2c_config.periph         = I2CHandle::Config::Peripheral::I2C_1;
    config.i2c_config.pin_config.scl = hw.GetPin(11);
    config.i2c_config.pin_config.sda = hw.GetPin(12);

    dp.Init(config);

    dp.SetRotation(3);
    dp.SetColor(LED_RED);

    char strbuff[128];
    sprintf(strbuff, "Testing. . .");

    while(1)
    {
        for(uint8_t i = 0; i < 12; i++)
        {
            dp.Fill(false);

            dp.SetCursor(0, 0);
            dp.WriteString(strbuff + i, Font_6x8, true);

            // dp.DrawCircle(4, 4, 3, true);

            dp.DrawPixelColor(i % 8, 0, (uint16_t)LED_GREEN);

            //dp.DrawBitmap(bmp, LED_GREEN);
            //dp.DrawRect(1, 1, 6, 4, true);

            dp.Update();
            hw.DelayMs(500);
        }
    }
}
