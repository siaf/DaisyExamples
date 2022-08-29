
#include <complex.h>
#include "../resources.h"

class OscilatorSinVectorRotation
{
    float samplerate_;
    float amp_ = 0.5f;
    float freq_;

    std::complex<float> multiplier = 1;
    std::complex<float> value      = 1;

  public:
    void Init(float sample_rate)
    {
        samplerate_ = sample_rate;
        SetFreq(440.0f);
    };

    void SetAmp(float amp) { amp_ = amp; }

    void SetFreq(float freq)
    {
        float phaseIncrement = (freq * TWOPI_F / samplerate_);
        multiplier.real(cos(phaseIncrement));
        multiplier.imag(sin(phaseIncrement));
    }

    float Process()
    {
        // multiplication of complex numbers causes rotation around the circle and thus calcs the sin.
        // see: https://www.youtube.com/watch?v=TBCf1p7BSek
        // src: https://github.com/cesaref/ADC2021/blob/main/demos/Demo1/CordicSine/CordicSine.soul
        value = value * multiplier;
        return amp_ * value.imag();
    }
};