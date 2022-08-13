#include <stdio.h>
#include <string.h>
#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"
#include "FluidModule.h"


// Use the daisy namespace to prevent having to type
// daisy:: before all libdaisy functions
using namespace daisy;

/** Typedef the OledDisplay to make syntax cleaner below 
 *  This is a 4Wire SPI Transport controlling an 128x64 sized SSDD1306
 * 
 *  There are several other premade test 
*/
using FluidOledDisplay = OledDisplay<SSD130xI2c128x64Driver>;


namespace fluid
{

/** outside of class static buffer(s) for DMA access */
uint16_t DMA_BUFFER_MEM_SECTION dsy_patch_sm_dac_buffer[2][48];

class FluidModule::Impl
{
  public:
    Impl()
    {
        dac_running_            = false;
        dac_buffer_size_        = 48;
        dac_output_[0]          = 0;
        dac_output_[1]          = 0;
        internal_dac_buffer_[0] = dsy_patch_sm_dac_buffer[0];
        internal_dac_buffer_[1] = dsy_patch_sm_dac_buffer[1];
    }

    void InitDac();

    void StartDac(DacHandle::DacCallback callback);

    void StopDac();

    static void InternalDacCallback(uint16_t **output, size_t size);

    /** Based on a 0-5V output with a 0-4095 12-bit DAC */
    static inline uint16_t VoltageToCode(float input)
    {
        float pre = input * 819.f;
        if(pre > 4095.f)
            pre = 4095.f;
        else if(pre < 0.f)
            pre = 0.f;
        return (uint16_t)pre;
    }

    inline void WriteCvOut(int channel, float voltage)
    {
        if(channel == 0 || channel == 1)
            dac_output_[0] = VoltageToCode(voltage);
        if(channel == 0 || channel == 2)
            dac_output_[1] = VoltageToCode(voltage);
    }

    size_t    dac_buffer_size_;
    uint16_t *internal_dac_buffer_[2];
    uint16_t  dac_output_[2];
    DacHandle dac_;

  private:
    bool dac_running_;
};

/** Static Local Object */
static FluidModule::Impl static_fluid_module;

/** Impl function definintions */

void FluidModule::Impl::InitDac()
{
    DacHandle::Config dac_config;
    dac_config.mode = DacHandle::Mode::DMA;
    dac_config.bitdepth
        = DacHandle::BitDepth::BITS_12; /**< Sets the output value to 0-4095 */
    dac_config.chn               = DacHandle::Channel::BOTH;
    dac_config.buff_state        = DacHandle::BufferState::ENABLED;
    dac_config.target_samplerate = 48000;
    dac_.Init(dac_config);
}

void FluidModule::Impl::StartDac(DacHandle::DacCallback callback)
{
    if(dac_running_)
        dac_.Stop();
    dac_.Start(internal_dac_buffer_[0],
               internal_dac_buffer_[1],
               dac_buffer_size_,
               callback == nullptr ? InternalDacCallback : callback);
    dac_running_ = true;
}

void FluidModule::Impl::StopDac()
{
    dac_.Stop();
    dac_running_ = false;
}

void FluidModule::Impl::InternalDacCallback(uint16_t **output, size_t size)
{
    /** We could add some smoothing, interp, or something to make this a bit less waste-y */
    // std::fill(&output[0][0], &output[0][size], patch_sm_hw.dac_output_[0]);
    // std::fill(&output[1][1], &output[1][size], patch_sm_hw.dac_output_[1]);
    for(size_t i = 0; i < size; i++)
    {
        output[0][i] = static_fluid_module.dac_output_[0];
        output[1][i] = static_fluid_module.dac_output_[1];
    }
}


void FluidModule::Init()
{
    /** Assign pimpl pointer */
    pimpl_ = &static_fluid_module;

    // Configure and Initialize the Daisy Seed
    // These are separate to allow reconfiguration of any of the internal
    // components before initialization.
    hardware.Configure();
    hardware.Init();
    //How many samples we'll output per second
    float samplerate = hardware.AudioSampleRate();

    //Create an ADC configuration
    AdcChannelConfig adcConfig[NUM_ADC_CHANNELS];

    adcConfig[knob1].InitSingle(hardware.GetPin(15)); //D15:PIN 22
    adcConfig[knob2].InitSingle(hardware.GetPin(16)); //D16:PIN 23
    adcConfig[knob3].InitSingle(hardware.GetPin(17)); //D17:PIN 24
    adcConfig[knob4].InitSingle(hardware.GetPin(18)); //D18:PIN 25
    adcConfig[knob5].InitSingle(hardware.GetPin(21)); //D21:PIN 28
    adcConfig[cvIn1].InitSingle(hardware.GetPin(19)); //D19:PIN 26
    adcConfig[cvIn2].InitSingle(hardware.GetPin(20)); //D20:PIN 27
    adcConfig[cvIn3].InitSingle(hardware.GetPin(24)); //D24:PIN 31
    adcConfig[cvIn4].InitSingle(hardware.GetPin(25)); //D25:PIN 32
    adcConfig[cvIn5].InitSingle(hardware.GetPin(28)); //D28:PIN 35

    //Initialize the buttons on pins 1 & 2
    button1.Init(hardware.GetPin(0), samplerate / 48.f);
    button2.Init(hardware.GetPin(1), samplerate / 48.f);

    //Initialize 4 LEDs on pins 2-5.
    led1.Init(hardware.GetPin(2), false, 1000);
    led2.Init(hardware.GetPin(3), false, 1000);
    led3.Init(hardware.GetPin(4), false, 1000);
    led4.Init(hardware.GetPin(5), false, 1000);

    //Set the ADC to use our configuration
    hardware.adc.Init(adcConfig, NUM_ADC_CHANNELS);

    /** DAC init */
    pimpl_->InitDac();

    /** Configure the Display */
    FluidOledDisplay::Config disp_cfg;
    disp_cfg.driver_config.transport_config.i2c_config.pin_config.scl
        = {DSY_GPIOB, 8};
    disp_cfg.driver_config.transport_config.i2c_config.pin_config.sda
        = {DSY_GPIOB, 9};
    disp_cfg.driver_config.transport_config.i2c_address = 0x3C;

    /** And Initialize */
    display.Init(disp_cfg);
}

void FluidModule::StartAdc()
{
    //Start the adc for CV ins & Knobs
    hardware.adc.Start();
}

float FluidModule::GetAdcFloat(uint8_t control)
{
    float val = hardware.adc.GetFloat(control);
    if(control > knob5
       && val < 0.2f) //testing to see if the noise on CV Ins can be removed. Maybe a breadboard issue.
    {
        return 0.0f;
    }
    else
    {
        return val;
    }
}

uint16_t FluidModule::GetAdc(uint8_t control)
{
    return hardware.adc.Get(control);
}

uint32_t FluidModule::GetNow()
{
    return hardware.system.GetNow();
}

void FluidModule::SetAudioBlockSize(size_t blocksize)
{
    hardware.SetAudioBlockSize(blocksize);
}

float FluidModule::AudioSampleRate()
{
    return hardware.AudioSampleRate();
}

void FluidModule::StartAudio(AudioHandle::AudioCallback cb)
{
    hardware.StartAudio(cb);
}

void FluidModule::StartAudio(AudioHandle::InterleavingAudioCallback cb)
{
    hardware.StartAudio(cb);
}

void FluidModule::StartDac(DacHandle::DacCallback callback)
{
    pimpl_->StartDac(callback);
}

void FluidModule::StopDac()
{
    pimpl_->StopDac();
}

void FluidModule::WriteCvOut(const int channel, float voltage)
{
    pimpl_->WriteCvOut(channel, voltage);
}


} // namespace fluid
