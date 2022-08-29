#include <vector>
#include <complex.h>
#include "../resources.h"

class OscilatorAdditiveVectorRotation
{
    float samplerate_;
    float amp_       = 0.5f;
    float freq_      = 440.0;
    int   harmonics_ = 64;

    std::vector<std::complex<float>> multiplier;
    std::vector<std::complex<float>> value;
    std::vector<float>               amplitudes;

  public:
    void Init(float sample_rate)
    {
        samplerate_ = sample_rate;
        SetFreq(440.0f);

        for(int i = 0; i < harmonics_; i++)
        {
            multiplier.push_back(1.0f);
            value.push_back(1.0f);
            amplitudes.push_back(0.0f);
        }
    };

    void SetAmp(float amp) { amp_ = amp; }

    void SetFreq(float freq)
    {
        freq_ = freq;
        for(int i = 0; i < harmonics_; i++)
        {
            float harmonicFrequency = freq_ * float(i + 1);
            float phaseIncrement = (harmonicFrequency * TWOPI_F / samplerate_);
            multiplier[i].real(cos(phaseIncrement));
            multiplier[i].imag(sin(phaseIncrement));

            if(harmonicFrequency < (samplerate_ / 2))
                amplitudes[i] = 1.0f / float(i + 1);
        }
    }

    float Process()
    {
        // multiplication of complex numbers causes rotation around the circle and thus calcs the sin.
        // see: https://www.youtube.com/watch?v=TBCf1p7BSek
        // src: https://github.com/cesaref/ADC2021/blob/main/demos/Demo1/CordicSine/CordicSine.soul
        float out = 0.0f;

        for(int i = 0; i < harmonics_; i++)
        {
            value[i] = value[i] * multiplier[i];
            out += value[i].imag() * amplitudes[i];
        }

        return amp_ * out;
    }
};