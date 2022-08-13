#include "daisy_patch_sm.h"
//#include "daisysp.h"
#include "bass_drum.h"
#include "snare_drum.h"
#include "high_hat.h"
#include "fm_drum.h"

using namespace daisy;
using namespace patch_sm;
//using namespace daisysp;
using namespace peaks;

DaisyPatchSM hw;
BassDrum     bdrum;
SnareDrum    snrdrum;
HighHat      hh;
FmDrum       fmDrum;

void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
    hw.ProcessAllControls();
    uint16_t knb1  = hw.GetAdcValue(CV_1) * 65535;
    uint16_t knb2  = hw.GetAdcValue(CV_2) * 65535;
    uint16_t knb3  = hw.GetAdcValue(CV_3) * 65535;
    uint16_t knb4  = hw.GetAdcValue(CV_4) * 65535;
    bool     gate1 = hw.gate_in_1.Trig();
    bool     gate2 = hw.gate_in_2.Trig();

    bdrum.Configure(knb1, knb2, knb3, knb4);
    snrdrum.Configure(knb1, knb2, knb3, knb4);
    hh.Configure(); //not needed
    fmDrum.Configure(knb1,knb2,knb3,knb4);
    for(size_t i = 0; i < size; i += 2)
    {
        float drmout = bdrum.Process(gate1);
        float snrout = snrdrum.Process(gate2, hw.GetRandomValue());
        float hhout  = hh.Process(gate1);
        float fmdrmout = fmDrum.Process(gate2, hw.GetRandomValue());
        //first just passthrough
        out[i]     = drmout;
        out[i + 1] = snrout;
    }
}

int main(void)
{
    hw.Init();
    bdrum.Init();
    snrdrum.Init();
    hh.Init();
    fmDrum.Init();

    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.StartAudio(AudioCallback);
    while(1) {}
}
