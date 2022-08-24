#include <stdio.h>
#include <string.h>
#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"
#include "ShapeShifterHW.h"


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

class ShapeShifterHW::Impl
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
static ShapeShifterHW::Impl static_fluid_module;

/** Impl function definintions */

void ShapeShifterHW::Impl::InitDac()
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

void ShapeShifterHW::Impl::StartDac(DacHandle::DacCallback callback)
{
    if(dac_running_)
        dac_.Stop();
    dac_.Start(internal_dac_buffer_[0],
               internal_dac_buffer_[1],
               dac_buffer_size_,
               callback == nullptr ? InternalDacCallback : callback);
    dac_running_ = true;
}

void ShapeShifterHW::Impl::StopDac()
{
    dac_.Stop();
    dac_running_ = false;
}

void ShapeShifterHW::Impl::InternalDacCallback(uint16_t **output, size_t size)
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


void ShapeShifterHW::Init()
{
    /** Assign pimpl pointer */
    pimpl_ = &static_fluid_module;

    // Configure and Initialize the Daisy Seed
    // These are separate to allow reconfiguration of any of the internal
    // components before initialization.
    hw.Configure();
    hw.Init();
    //How many samples we'll output per second
    float samplerate = hw.AudioSampleRate();

    //Create an ADC configuration
    AdcChannelConfig adcConfig[NUM_ADC_CHANNELS];

    adcConfig[CTRL1].InitSingle(hw.GetPin(15)); //D15:PIN 22
    adcConfig[CTRL2].InitSingle(hw.GetPin(16)); //D16:PIN 23
    adcConfig[CTRL3].InitSingle(hw.GetPin(17)); //D17:PIN 24
    adcConfig[CTRL4].InitSingle(hw.GetPin(18)); //D18:PIN 25
    adcConfig[CTRL5].InitSingle(hw.GetPin(19)); //D19:PIN 26
    adcConfig[CTRL6].InitSingle(hw.GetPin(20)); //D20:PIN 27
    adcConfig[CTRL7].InitSingle(hw.GetPin(21)); //D21:PIN 28
    adcConfig[CTRL8].InitSingle(hw.GetPin(24)); //D24:PIN 31
    adcConfig[CTRL9].InitSingle(hw.GetPin(25)); //D25:PIN 32

    //Initialized Gates
    dsy_gpio_pin pg1;
    pg1.pin  = hw.GetPin(29).pin;
    pg1.port = hw.GetPin(29).port;
    gate_in_1.Init(&pg1);

    dsy_gpio_pin pg2;
    pg2.pin  = hw.GetPin(30).pin;
    pg2.port = hw.GetPin(30).port;
    gate_in_2.Init(&pg2);

    gate_out_1.mode = DSY_GPIO_MODE_OUTPUT_PP;
    gate_out_1.pull = DSY_GPIO_NOPULL;
    gate_out_1.pin  = hw.GetPin(27);
    dsy_gpio_init(&gate_out_1);

    gate_out_2.mode = DSY_GPIO_MODE_OUTPUT_PP;
    gate_out_2.pull = DSY_GPIO_NOPULL;
    gate_out_2.pin  = hw.GetPin(26);
    dsy_gpio_init(&gate_out_2);

    //Initialize the buttons on pins 1 & 2
    button1.Init(hw.GetPin(0), samplerate / 48.f);
    button2.Init(hw.GetPin(1), samplerate / 48.f);

    //Initialize encoder Pin A: D3 (4), B: D4(5), Click: D2(3)
    encoder.Init(hw.GetPin(3), hw.GetPin(4), hw.GetPin(2));

    //Initialize 2 LEDs on pins 9 & 10.
    led1.Init(hw.GetPin(8),
              false,
              1000); //todo: add no pull because we have resistors externally
    led2.Init(hw.GetPin(9),
              false,
              1000); //todo: add no pull because we have resistors externally

    //Initialize RGB LED on pins B:6, G:7, R:8
    rgbLed.Init(hw.GetPin(7),
                hw.GetPin(6),
                hw.GetPin(5),
                true); //todo: add no pull because we have resistors externally

    //Set the ADC to use our configuration
    hw.adc.Init(adcConfig, NUM_ADC_CHANNELS);

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

void ShapeShifterHW::StartAdc()
{
    //Start the adc for CV ins & Knobs
    hw.adc.Start();
}

float ShapeShifterHW::GetAdcFloat(uint8_t control)
{
    float val = hw.adc.GetFloat(control);
    return val;
}

uint16_t ShapeShifterHW::GetAdc(uint8_t control)
{
    return hw.adc.Get(control);
}

uint32_t ShapeShifterHW::GetNow()
{
    return hw.system.GetNow();
}

void ShapeShifterHW::SetAudioBlockSize(size_t blocksize)
{
    hw.SetAudioBlockSize(blocksize);
}

float ShapeShifterHW::AudioSampleRate()
{
    return hw.AudioSampleRate();
}

void ShapeShifterHW::StartAudio(AudioHandle::AudioCallback cb)
{
    hw.StartAudio(cb);
}

void ShapeShifterHW::StartAudio(AudioHandle::InterleavingAudioCallback cb)
{
    hw.StartAudio(cb);
}

void ShapeShifterHW::StartDac(DacHandle::DacCallback callback)
{
    pimpl_->StartDac(callback);
}

void ShapeShifterHW::StopDac()
{
    pimpl_->StopDac();
}

void ShapeShifterHW::WriteCvOut(const int channel, float voltage)
{
    pimpl_->WriteCvOut(channel, voltage);
}


} // namespace fluid
