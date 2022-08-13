#include "daisy_seed.h"
#include "daisysp.h"

// Use the daisy namespace to prevent having to type
// daisy:: before all libdaisy functions
using namespace daisy;
using namespace daisysp;

enum AdcChannel {
   knob1 = 0,
   knob2,
   knob3,
   knob4,
   knob5,
   knob6,
   NUM_ADC_CHANNELS
};

enum
    {
        CV_OUT_BOTH = 0,
        CV_OUT_1,
        CV_OUT_2,
    };

//DacVariables
uint16_t DMA_BUFFER_MEM_SECTION dsy_patch_sm_dac_buffer[2][48];
        size_t    dac_buffer_size_;
        uint16_t *internal_dac_buffer_[2];
        uint16_t  dac_output_[2];
        DacHandle dac_;
        bool dac_running_;

// Declare a DaisySeed object called hardware
DaisySeed  hardware;
Oscillator osc1;
Oscillator osc2;
Oscillator osc3;

AdEnv      env;

Switch button1;
bool flashAudioAsCv=false;

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
        
void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{

    float osc_out, osc1_out, osc2_out, osc3_out, env_out;

    //Nobody likes a bouncy button
    button1.Debounce();

    //If you push the button,...
    if(button1.RisingEdge())
    {
        env.Trigger(); //Trigger the envelope!
    }

    //Convert floating point knob to midi (0-127)
    //Then convert midi to freq. in Hz
    osc1.SetFreq(mtof(hardware.adc.GetFloat(knob1) * 127));
    osc1.SetAmp(hardware.adc.GetFloat(knob2));

    osc2.SetFreq(mtof(hardware.adc.GetFloat(knob3) * 127));
    osc2.SetAmp(hardware.adc.GetFloat(knob4));

    osc3.SetFreq(mtof(hardware.adc.GetFloat(knob5) * 127));
    osc3.SetAmp(hardware.adc.GetFloat(knob6));

    //osc.SetFreq(440.0f);

    //osc.SetAmp(hardware.adc.GetFloat(0));
    
    //Fill the block with samples
    for(size_t i = 0; i < size; i += 2)
    {
        //Get the next envelope value
        env_out = env.Process();
        //Set the oscillator volume to the latest env value
        osc1.SetAmp(1.0f);
        osc2.SetAmp(1.0f);
        osc3.SetAmp(1.0f);

        //get the next oscillator sample
        osc1_out = osc1.Process();
        osc2_out = osc2.Process();
        osc3_out = osc3.Process();

        osc_out=osc1_out;
        //Set the left and right outputs
        out[i]     = osc_out;
        out[i + 1] = osc_out;
    }
}

void InitDac()
    {
        DacHandle::Config dac_config;
        dac_config.mode     = DacHandle::Mode::DMA;
        dac_config.bitdepth = DacHandle::BitDepth::
            BITS_12; /**< Sets the output value to 0-4095 */
        dac_config.chn               = DacHandle::Channel::BOTH;
        dac_config.buff_state        = DacHandle::BufferState::ENABLED;
        dac_config.target_samplerate = 48000;
        dac_.Init(dac_config);
    }

void InternalDacCallback(uint16_t **output, size_t size)
    {
        /** We could add some smoothing, interp, or something to make this a bit less waste-y */
        // std::fill(&output[0][0], &output[0][size], patch_sm_hw.dac_output_[0]);
        // std::fill(&output[1][1], &output[1][size], patch_sm_hw.dac_output_[1]);
        for(size_t i = 0; i < size; i++)
        {
            output[0][i] = dac_output_[0];
            output[1][i] = dac_output_[1];
        }
    }

void StartDac(DacHandle::DacCallback callback)
    {
        if(dac_running_)
            dac_.Stop();
        dac_.Start(internal_dac_buffer_[0],
                   internal_dac_buffer_[1],
                   dac_buffer_size_,
                   callback == nullptr ? InternalDacCallback : callback);
        dac_running_ = true;
    }

    void StopDac()
    {
        dac_.Stop();
        dac_running_ = false;
    }



        
 
 /** Gets a random 32-bit value */
        inline uint32_t GetRandomValue() { return Random::GetValue(); }

        /** Gets a random floating point value between the specified minimum, and maxmimum */
        inline float GetRandomFloat(float min = 0.f, float max = 1.f)
        {
            return Random::GetFloat(min, max);
        }

int main(void)
{
    // Configure and Initialize the Daisy Seed
    // These are separate to allow reconfiguration of any of the internal
    // components before initialization.
    hardware.Configure();
    hardware.Init();
    hardware.SetAudioBlockSize(4);

    //How many samples we'll output per second
    float samplerate = hardware.AudioSampleRate();

    //Create an ADC configuration
    AdcChannelConfig adcConfig[NUM_ADC_CHANNELS];

    adcConfig[knob1].InitSingle(hardware.GetPin(20));
    adcConfig[knob2].InitSingle(hardware.GetPin(19));
    adcConfig[knob3].InitSingle(hardware.GetPin(18));
    adcConfig[knob4].InitSingle(hardware.GetPin(17));
    adcConfig[knob5].InitSingle(hardware.GetPin(16));
    adcConfig[knob6].InitSingle(hardware.GetPin(15));

    //Initialize the button on pin 28
    button1.Init(hardware.GetPin(28), samplerate / 48.f);

    //Set the ADC to use our configuration
    hardware.adc.Init(adcConfig, NUM_ADC_CHANNELS);

    //Set up oscillator
    osc1.Init(samplerate);
    osc1.SetWaveform(osc1.WAVE_SQUARE);
    
    osc1.SetAmp(1.f);
    osc1.SetFreq(1000);

    osc2.Init(samplerate);
    osc2.SetWaveform(osc2.WAVE_TRI);
    
    osc2.SetAmp(1.f);
    osc2.SetFreq(1000);

    osc3.Init(samplerate);
    osc3.SetWaveform(osc3.WAVE_SQUARE);
    
    osc3.SetAmp(1.f);
    osc3.SetFreq(1000);

    //Set up volume envelope
    env.Init(samplerate);
    //Envelope attack and decay times
    env.SetTime(ADENV_SEG_ATTACK, .01);
    env.SetTime(ADENV_SEG_DECAY, .4);
    //minimum and maximum envelope values
    env.SetMin(0.0);
    env.SetMax(1.f);
    env.SetCurve(0); // linear

    //Start the adc
    hardware.adc.Start();

    //Start calling the audio callback
    hardware.StartAudio(AudioCallback);


            dac_running_            = false;
            dac_buffer_size_        = 48;
            dac_output_[0]          = 0;
            dac_output_[1]          = 0;
            internal_dac_buffer_[0] = dsy_patch_sm_dac_buffer[0];
            internal_dac_buffer_[1] = dsy_patch_sm_dac_buffer[1];
    
    InitDac();
    StartDac(InternalDacCallback);

    int r1=0;
    int r2=0;
    while(1)
    {
        /** Delay half a second */
        System::Delay(300);
        /** Get a truly random float from the hardware */
        float voltage = GetRandomFloat(0.0, 5.0);
        /** Write it to both CV Outputs */
        
        flashAudioAsCv=!flashAudioAsCv;
        
        if(r1==2){
            WriteCvOut(CV_OUT_1, 5.0);
            r1=0;
        }
        else{
            WriteCvOut(CV_OUT_1, 0.0);
            r1++;
            }

             if(r2==1){
            WriteCvOut(CV_OUT_2, 5.0);
            r2=0;
        }
        else{
            WriteCvOut(CV_OUT_2, 0.0);
            r2++;
            }
    }

    // Loop forever
    for(;;) {}
}


