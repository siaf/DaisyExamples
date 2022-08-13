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
    //Fluid Module For Eurorack
// Features:
// Audio Out L & R
// Audio In L & R
// Knobs 1-5
// CV Out 1-2
// CV In 1-5
// OLED Display
// 2 Buttons
// 4 Reserved LEDs Slots


enum AdcChannel
{
    knob1 = 0,
    knob2,
    knob3,
    knob4,
    knob5,
    cvIn1,
    cvIn2,
    cvIn3,
    cvIn4,
    cvIn5,
    NUM_ADC_CHANNELS
};

enum
{
    CV_OUT_BOTH = 0,
    CV_OUT_1,
    CV_OUT_2,
};

class FluidModule
{
  public:
    //UI Elements
    FluidOledDisplay display;
    Switch        button1;
    Switch        button2;
    Led           led1;
    Led           led2;
    Led           led3;
    Led           led4;
    uint16_t      dac_output_[2];
    class Impl;

    void Init();

    void StartAdc();
    float GetAdcFloat(uint8_t control);
    uint16_t GetAdc(uint8_t control);
    
    void SetAudioBlockSize(size_t blocksize);
    float AudioSampleRate();
    void StartAudio(AudioHandle::AudioCallback cb);
    void StartAudio(AudioHandle::InterleavingAudioCallback cb);
    void StartDac(DacHandle::DacCallback callback);
    void StopDac() ;
    void WriteCvOut(const int channel, float voltage);

    /** \return a uint32_t value of milliseconds since the SysTick started 
    */
    uint32_t GetNow();
  private:
    // Declare a DaisySeed object called hardware
    DaisySeed hardware;



    /** Background callback for updating the DACs. */
    Impl *pimpl_;
};
} // namespace fluid