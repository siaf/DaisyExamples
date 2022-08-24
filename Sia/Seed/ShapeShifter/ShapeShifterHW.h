#pragma once

#include "daisy.h"
#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"

using namespace daisy;

/** Typedef the OledDisplay to make syntax cleaner below 
 *  This is a 4Wire SPI Transport controlling an 128x64 sized SSDD1306
 * 
 *  There are several other premade test 
*/
using FluidOledDisplay = OledDisplay<SSD130xI2c128x64Driver>;

namespace fluid
{
//ShapeShifterFor Eurorack

// Features:
// Audio Out L & R
// Audio In L & R

// Controls
// 1. Knob
// 2 & 3 CV IN
// 4 & 5 CV with Attenuator
// 6 & 7 CV & Knob
// 8 & 9 CV & Att & Knob

// CV Out 1-2

// Gate Out 1-2
// Gate In  1-2

// OLED Display
// 2 Buttons
// 1 RGB LED
// 2 LEDs
// 1 Encoder with Switch


enum AdcChannel
{
    CTRL1 = 0,
    CTRL2,
    CTRL3,
    CTRL4,
    CTRL5,
    CTRL6,
    CTRL7,
    CTRL8,
    CTRL9,
    NUM_ADC_CHANNELS
};

enum
{
    CV_OUT_BOTH = 0,
    CV_OUT_1,
    CV_OUT_2,
};

class ShapeShifterHW
{
  public:
    //UI Elements
    FluidOledDisplay display;
    Switch           button1;
    Switch           button2;
    Encoder          encoder;
    Led              led1;
    Led              led2;
    RgbLed           rgbLed;
    GateIn           gate_in_1, gate_in_2;
    dsy_gpio         gate_out_1, gate_out_2;

    uint16_t dac_output_[2];
    class Impl;

    void Init();

    void     StartAdc();
    float    GetAdcFloat(uint8_t control);
    uint16_t GetAdc(uint8_t control);

    void  SetAudioBlockSize(size_t blocksize);
    float AudioSampleRate();
    void  StartAudio(AudioHandle::AudioCallback cb);
    void  StartAudio(AudioHandle::InterleavingAudioCallback cb);
    void  StartDac(DacHandle::DacCallback callback);
    void  StopDac();
    void  WriteCvOut(const int channel, float voltage);

    /** \return a uint32_t value of milliseconds since the SysTick started 
    */
    uint32_t GetNow();

    inline float CtrlVal(fluid::AdcChannel ctrl, bool inv = false)
    {
        float val = GetAdcFloat(ctrl);
        if(inv)
        {
            return (1.0f - val);
        }
        else
        {
            return val;
        }
    }

    inline uint16_t CtrlToPercentage(fluid::AdcChannel ctrl, bool inv = false)
    {
        return CtrlVal(ctrl, inv) * 100;
    }

  private:
    // Declare a DaisySeed object called hardware
    DaisySeed hw;


    /** Background callback for updating the DACs. */
    Impl *pimpl_;
};
} // namespace fluid