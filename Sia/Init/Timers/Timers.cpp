#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM hw;


class Metronome_WithTrigger
{
  private:
    float sample_rate_;
    float interval_;         //how many samples for each cycle of delay;
    float trigger_duration_; // how many samples will the trigger last.
    float trigger_inc;

    uint32_t counter       = 0;
    bool     is_triggered_ = false;
    float    trigger_value = 0.f;

  public:
    void Init(float sample_rate)
    {
        sample_rate_ = sample_rate;
        SetTempo(10.0f);
        SetTriggerDuration(1.0f);
    }

    void SetTempo(float sec) { interval_ = sample_rate_ * sec; }

    void SetTempo(uint16_t bpm)
    {
        interval_ = sample_rate_ * (60.0f / (float)bpm);
    }

    void SetTriggerDuration(float sec)
    {
        trigger_duration_ = sample_rate_ * sec;

        //calculate how much the trigger should be incremented to reach 1.0f in half trigger duration
        trigger_inc = 1.0f / trigger_duration_ / 2.0f;
    }

    bool IsTriggered() { return is_triggered_; }

    float Process()
    {
        counter++;
        if(is_triggered_)
        {
            //count until trigger duration is passed.
            if(counter > trigger_duration_)
            {
                is_triggered_ = false;
            }

            if(trigger_value < 1.0f)
            {
                //trigger to be incremented until it reaches 1.0f
                trigger_value += trigger_inc;
            }
        }
        else
        {
            //trigger to be decremented until it reaches 0.0f
            if(trigger_value > 0.0f)
            {
                trigger_value -= trigger_inc;
            }
        }

        if(counter > interval_)
        {
            counter       = 0;
            is_triggered_ = true;
        }

        return trigger_value;
    }
};

class Metronome
{
  private:
    float    sample_rate_;
    float    interval_; //how many samples for each cycle of delay;
    uint32_t counter       = 0;
    bool     is_triggered_ = false;

  public:
    void Init(float sample_rate) { sample_rate_ = sample_rate; }

    void SetTempo(float sec) { interval_ = sample_rate_ * sec; }
    void SetTempo(uint16_t bpm)
    {
        interval_ = sample_rate_ * (60.0f / (float)bpm);
    }
    bool  IsTriggered() { return is_triggered_; }
    float Process()
    {
        counter++;
        if(counter > interval_)
        {
            counter       = 0;
            is_triggered_ = true;
            return 1.0f;
        }
        else
        {
            is_triggered_ = false;
            return 0.0f;
        }
    }
};

Metronome_WithTrigger myMetronome;
Oscillator            osc;
Adsr                  adrs;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls();

    for(size_t i = 0; i < size; i++)
    {
        /*myMetronome.Process();
        if(myMetronome.IsTriggered())
        {
            adrs.Retrigger(true);
        }

        float env = adrs.Process(false);*/

        float trigger = myMetronome.Process();
        osc.SetAmp(trigger);

        OUT_L[i] = osc.Process();
        OUT_R[i] = trigger;
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
   

    osc.Init(hw.AudioSampleRate());
    adrs.Init(hw.AudioSampleRate());
    myMetronome.Init(hw.AudioSampleRate());

//myMetronome.SetTempo((uint16_t)60);
    myMetronome.SetTriggerDuration(1.0f);
    myMetronome.SetTempo(2.0f);

    osc.SetFreq(880.0f);
    osc.SetAmp(1.0f);

    adrs.SetAttackTime(0.01f);
    adrs.SetDecayTime(0.001f);
    adrs.SetReleaseTime(0.001f);
    adrs.SetSustainLevel(0.001f);

     hw.StartAudio(AudioCallback);

    while(1) {}
}
