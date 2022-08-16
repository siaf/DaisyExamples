#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM hw;

#define PI_F 3.1415927410125732421875f

/*
Types of filters:
1. Low Pass
2. High Pass
3. Band Pass
4. Notch/Peak
5. Shelf
6. All Pass
*/

//y[n] = x[n] - x[n-1]
// it's an FIR filter, because there's no dependency on previous output
// it's a Highpass filter
//
class FirstOrderDifferenceFilter
{
    float previousSample_ = 0.0f;
    float process(float in)
    {
        float out       = in - previousSample_;
        previousSample_ = in;
        return out;
    }
};
// y[n] = a.y[n-1] + (1-a).x[n]
class FirstOrderIIRLowPassFilter
{
    float previousOutput_ = 0.0f;
    float alpha           = 0.99f;
    float process(float in)
    {
        float out       = (alpha * previousOutput_) + ((1 - alpha) * in);
        previousOutput_ = out;
        return out;
    }
};

//second order, IIR, can be used to implement many types of filters.
// y[n] = b0.x[n] + b1.x[n-1] + b2.x[n-2] - a1.y[n-1] - a2.y[n-2]
class BiQuadFilter
{
    float previousOutput1_ = 0.0f; // y[n-1]
    float previousOutput2_ = 0.0f; // y[n-2]
    float previousInput1_  = 0.0f; // x[n-1]
    float previousInput2_  = 0.0f; // x[n-2]

    float a1, a2, b0, b1, b2;

  public:
    float process(float in)
    {
        // y[n] = b0.x[n] + b1.x[n-1] + b2.x[n-2] - a1.y[n-1] - a2.y[n-2]
        float out = (b0 * in) + (b1 * previousInput1_) + (b2 * previousInput2_)
                    - (a1 * previousOutput1_) - (a2 * previousOutput2_);

        previousInput2_ = previousInput1_;
        previousInput1_ = in;

        previousOutput2_ = previousOutput1_;
        previousOutput1_ = out;

        return out;
    }

    // Calculate the filter coefficients based on the given parameters
    // Borrows code from the Bela Biquad library, itself based on code by
    // Nigel Redmon
    // This gives a low pass resosant filter.
	// more examples here: https://www.earlevel.com/main/2012/11/26/biquad-c-source-code/
	// also in bela library and daisy dsp library.
    void calculate_coefficients(float sampleRate, float frequency, float q)
    {
        float k    = tanf(PI_F * frequency / sampleRate);
        float norm = 1.0 / (1 + k / q + k * k);

        b0 = k * k * norm;
        b1 = 2.0 * b0;
        b2 = b0;
        a1 = 2 * (k * k - 1) * norm;
        a2 = (1 - k / q + k * k) * norm;
    }
};

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
