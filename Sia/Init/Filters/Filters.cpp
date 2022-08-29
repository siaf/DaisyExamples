#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "sia_lib.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM hw;

BiQuadFilter filter;
void         AudioCallback(AudioHandle::InputBuffer  in,
                           AudioHandle::OutputBuffer out,
                           size_t                    size)
{
    
    hw.ProcessAllControls();

	float freq = (hw.GetAdcValue(CV_1) * 5000.0f) + 100.0f;
    float q    = (hw.GetAdcValue(CV_2) * 10.0f) + 0.5f;
    filter.calculate_coefficients(hw.AudioSampleRate(), freq, q);

    for(size_t i = 0; i < size; i++)
    {
        OUT_L[i] = filter.process(IN_L[i]);
        OUT_R[i] = IN_R[i];
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(128); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.StartAudio(AudioCallback);
    while(1) {}
}
