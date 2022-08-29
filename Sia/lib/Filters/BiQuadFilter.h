
#include "../resources.h"

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