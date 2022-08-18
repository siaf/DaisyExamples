#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "sia_lib.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM hw;

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
