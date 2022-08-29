#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "wavetables.h"
#include "../../lib/Osc/oscs.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM hw;


OscillatorUsing0to1ModuloCounter sosc;
OscilatorWaveTable               wtOsc;
Oscillator                       osc;
AdditiveWaveTable                additive;
OscilatorAdditiveVectorRotation  sinAdditiveOsc;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls();

    float pw = hw.GetAdcValue(CV_1);
    sosc.SetPulseWidth(pw);
    //osc.SetFreq(440.0f);

    float coarse = hw.GetAdcValue(CV_2) * 1000;
    float fine   = hw.GetAdcValue(CV_3) * 50;

    sosc.SetFreq(coarse + fine);
    wtOsc.SetFreq(coarse + fine);
    additive.SetFreq(coarse + fine);
    sinAdditiveOsc.SetFreq(coarse + fine);

    float sat = (hw.GetAdcValue(CV_4) * 10.0f) + 1.0f;
    sosc.SetSat(sat);

    for(size_t i = 0; i < size; i++)
    {
        float sig  = wtOsc.Process();
        float sig2 = additive.Process();
        OUT_L[i]   = sig;
        OUT_R[i]   = sig2;
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

    osc.Init(hw.AudioSampleRate());
    sosc.Init(hw.AudioSampleRate());
    wtOsc.Init(hw.AudioSampleRate());
    additive.Init(hw.AudioSampleRate());
    sinAdditiveOsc.Init(hw.AudioSampleRate());

    osc.SetAmp(0.5f);
    osc.SetFreq(440.f);
    osc.SetWaveform(osc.WAVE_TRI);

    sosc.SetFreq(440.f);
    sosc.SetAmp(0.5f);

    wtOsc.SetFreq(440.f);
    wtOsc.SetAmp(0.5f);

    additive.SetFreq(440.0f);
    additive.SetAmp(0.5f);
    //additive.SetHarmonic(2, 0.5f);

    for(int i = 0; i < 48; i++)
    {
        additive.SetHarmonic(i, 1.0f / (i + 1));
    }

    hw.StartAudio(AudioCallback);
    while(1) {}
}
