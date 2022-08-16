#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "wavetables.h"
#include <complex.h>

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM hw;

//converts decibel amplitude to linear
inline float dbtolin(float db)
{
    return powf(10, db / 20.0f);
}

//converts midi value to freq in hz, 69 is middle A = 440Hz. (A above middle C (60))
//note 1.0f value is 1 semitone, 0.5f is a quarter tone, and you can still use fractions
inline float midi_to_hz(float pitch)
{
    return 440.0f * powf(2.0, (pitch - 69.0f) / 12.0f);
}

//converts midi value to freq in hz, 69 is middle A = 440Hz. (A above middle C (60))
//note 1.0f value is 1 semitone, 0.5f is a quarter tone, and you can still use fractions
inline float hz_to_midi(float freq)
{
    return 69.0f + 12.0f * log2f(freq / 440.0f);
}

#define PI_F 3.1415927410125732421875f
#define TWOPI_F (2.0f * PI_F)
constexpr float TWO_PI_RECIP = 1.0f / TWOPI_F;

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

class OscilatorWaveTable
{
    float modulo_counter_
        = 0; //here it's equal to readPointer of the wave table
    float phase_inc_;

    float  samplerate_;
    float  amp_            = 0.5f;
    int    waveTableLength = 512;
    float* waveTable;

  public:
    void Init(float sample_rate)
    {
        samplerate_ = sample_rate;
        //Generate_AdditiveSaw_WaveTable();
        //Populate_Wv(fmish_wt);
        Generate_Sin_WaveTable();
        //Generate_Triangle_WaveTable();
    };

    void Init(float sample_rate, float* wave_table)
    {
        samplerate_ = sample_rate;
        waveTable   = wave_table;
    }

    void SetAmp(float amp) { amp_ = amp; }

    void SetFreq(float freq)
    {
        phase_inc_ = waveTableLength * (freq / samplerate_);
    }

    // Processes the waveform to be generated, returning one sample. This should be called once per sample period.
    float Process()
    {
        //Wraps Modulo Counter between the length of wavetable, to act as a circular buffer
        if(modulo_counter_ >= waveTableLength)
        {
            modulo_counter_ -= waveTableLength;
        };

        float out;
        //Method 1: no interpolation
        //float out = waveTable[(int)modulo_counter_];

        //Method 2: linear interpolatation.
        int currentSample = (int)modulo_counter_;
        int nextSample    = currentSample + 1;
        if(nextSample >= waveTableLength)
        {
            nextSample = 0;
        }

        out = Linear_Interpolate(waveTable[currentSample],
                                 waveTable[nextSample],
                                 modulo_counter_ - currentSample);

        //Method 3: Cubic interpolation
        out = Hermite_Interpolate(
            modulo_counter_ - currentSample,
            waveTable[(currentSample - 1 + waveTableLength) % waveTableLength],
            waveTable[(currentSample + waveTableLength) % waveTableLength],
            waveTable[(currentSample + 1 + waveTableLength) % waveTableLength],
            waveTable[(currentSample + 2 + waveTableLength) % waveTableLength]);

        // add phase inc, which also defines the frequency.
        modulo_counter_ += phase_inc_;

        return amp_ * out;
    }

  private:
    void Populate_Wv(const float (wt)[512])
    {
        waveTable = new float[waveTableLength];
        for(int n = 0; n < waveTableLength; n++)
        {
            waveTable[n] = wt[n];
        }
    }

    void Generate_Triangle_WaveTable()
    {
        waveTable = new float[waveTableLength];

        //Genrate a triangle wave form ramping from -1 to 1 for half of the table
        //then from 1 to -1 for the other half
        for(int n = 0; n < waveTableLength / 2; n++)
        {
            waveTable[n] = -1.0f + (4.0f * (float)n / (float)waveTableLength);
        }
        for(int n = waveTableLength / 2; n < waveTableLength; n++)
        {
            waveTable[n] = +1.0f
                           - (4.0f * (float)(n - (waveTableLength / 2))
                              / (float)waveTableLength);
        }
    };

    void Generate_Sin_WaveTable()
    {
        waveTable = new float[waveTableLength];

        for(int n = 0; n < waveTableLength; n++)
        {
            //waveTable[n] =0;
            waveTable[n] = sin(TWOPI_F * (float)n / (float)waveTableLength);
        }
    };

    void Generate_AdditiveSaw_WaveTable()
    {
        waveTable = new float[waveTableLength];

        for(int n = 0; n < waveTableLength; n++)
        {
            //waveTable[n] =0;
            waveTable[n]
                = 0.5 * sin(TWOPI_F * (float)n / (float)waveTableLength)
                  + 0.25f
                        * sin(2.0 * TWOPI_F * (float)n / (float)waveTableLength)
                  + 0.125f
                        * sin(3.0 * TWOPI_F * (float)n / (float)waveTableLength)
                  + 0.0625f
                        * sin(4.0 * TWOPI_F * (float)n / (float)waveTableLength)
                  + 0.03125f
                        * sin(5.0 * TWOPI_F * (float)n
                              / (float)waveTableLength);
        }
    };

    float inline Linear_Interpolate(float s1, float s2, float f)
    {
        //return a weighted average of 2 samples based on fraction (f). (f is between 0 to 1)
        return (s1 * (1.0f - f)) + (s2 * f);
    }

    // a form of cubic interpolate, other common one is called newton.
    // from:https://www.musicdsp.org/en/latest/Other/93-hermite-interpollation.html
    float inline Hermite_Interpolate(float sample_offset,
                                     float value0,
                                     float value1,
                                     float value2,
                                     float value3)
    {
        const float slope0 = (value2 - value0) * 0.5f;
        const float slope1 = (value3 - value1) * 0.5f;
        const float v      = value1 - value2;
        const float w      = slope0 + v;
        const float a      = w + v + slope1;
        const float b_neg  = w + a;

        return ((((a * sample_offset) - b_neg) * sample_offset + slope0)
                    * sample_offset
                + value1);
    }
};

//contains multiple wave table oscs
class AdditiveWaveTable
{
  private:
    uint16_t                        numOfOscs       = 48;
    uint16_t                        numOfActiveOscs = 1;
    std::vector<OscilatorWaveTable> wts;
    float                           samplerate_;
    float                           nyquest_;
    float                           amp_            = 0.5f;
    float                           freq_           = 440.0;
    int                             waveTableLength = 512;
    std::vector<float>              waveTable;

    void Generate_Sin_WaveTable()
    {
        for(int n = 0; n < waveTableLength; n++)
        {
            waveTable.push_back(
                sin(TWOPI_F * (float)n / (float)waveTableLength));
        }
    };

  public:
    void Init(float sample_rate)
    {
        samplerate_ = sample_rate;
        nyquest_    = sample_rate / 2.0f;
        Generate_Sin_WaveTable();
        for(uint16_t i = 0; i < numOfOscs; i++)
        {
            OscilatorWaveTable wt;
            wt.Init(sample_rate, waveTable.data());
            wt.SetAmp(0.0f);
            wts.push_back(wt);
        }
        wts[0].SetAmp(1.0f);
    };

    void SetAmp(float amp) { amp_ = amp; }

    void SetFreq(float freq)
    {
        freq_ = freq;
        //for each osc set the harmonic
        for(uint16_t i = 0; i < numOfOscs; i++)
        {
            float harmonic = freq * (i + 1);
            wts[i].SetFreq(freq * (i + 1));

            if(harmonic < nyquest_)
            {
                numOfActiveOscs
                    = i; //used to avoid aliasing by keeping only harmonics below nyquist active.
            }
        }
    }

    void SetHarmonic(int harmonic, float amp) { wts[harmonic].SetAmp(amp); }

    float Process()
    {
        float out = 0.0f;
        for(uint16_t i = 0; i < numOfActiveOscs; i++)
        {
            out += wts[i].Process();
        }
        return amp_ * out;
    }
};

class OscillatorUsing0to1ModuloCounter
{
    float modulo_counter_ = 0;
    float phase_inc_;
    float pulse_width_ = 0.5f;

    float samplerate_;
    float amp_ = 0.5f;
    float sat_ = 1.0f; //can be any value but 0, usually use a min of 1.

  public:
    void Init(float sample_rate) { samplerate_ = sample_rate; };
    void SetAmp(float amp) { amp_ = amp; }
    void SetSat(float sat) { sat_ = sat; }

    void SetFreq(float freq) { phase_inc_ = (freq / samplerate_); }

    void SetPulseWidth(float pw) { pulse_width_ = pw; }

    // Processes the waveform to be generated, returning one sample. This should be called once per sample period.
    float Process()
    {
        //Wraps Modulo Counter betweek 0-1 (a unipolar counter going though a duty cycle)
        if(modulo_counter_ >= 1.0f)
        {
            modulo_counter_ -= 1.0f;
        };


        float out = Trivial_Triangle(tanh(sat_ * modulo_counter_) / tan(sat_));

        //add Blep (either PolyBelp, or Belp using Wavetable or BLIT, etc) to remove harmonics caused by aliiasing.

        //add saturation using Wave Shapping.
        out = tanh(pulse_width_ * out) / tan(pulse_width_);

        // add phase inc, which also defines the frequency.
        modulo_counter_ += phase_inc_;

        return amp_ * out;
    }

  private:
    //to make a bi polar sawtooth or ramp (change range to -1 to +1)
    float inline UniToBiPolar(float t) { return (2.0f * t - 1.0f); }

    float inline Trivial_Ramp(float t) { return UniToBiPolar(t); }

    float inline Trivial_SAW(float t) { return -1.0f * Trivial_Ramp(t); }
    float inline Trivial_Triangle(float t)
    {
        return UniToBiPolar(fabs(Trivial_Ramp(t)));
    }
    //PW a value between 0 to 1.0, default is assumed 0.5
    float inline Trivial_Square(float t, float pw)
    {
        return t > pw ? -1.0f : 1.0f;
    }
};


class OscillatorUsingTwoPieModuloCounter
{
    float modulo_counter_ = 0.0f;
    float phase_inc_;
    float samplerate_;
    float amp_         = 0.5f;
    float pulse_width_ = PI_F;

  public:
    void Init(float sample_rate) { samplerate_ = sample_rate; };
    void SetAmp(float amp) { amp_ = amp; }
    void SetFreq(float freq) { phase_inc_ = (TWOPI_F * freq / samplerate_); }
    void SetPulseWidth(float pw) { pulse_width_ = pw * TWOPI_F; }

    // Processes the waveform to be generated, returning one sample. This should be called once per sample period.
    float Process()
    {
        //Wraps Modulo Counter betweek 0-Two Pies (a unipolar counter going though a duty cycle)
        if(modulo_counter_ >= TWOPI_F)
        {
            modulo_counter_ -= TWOPI_F;
        };

        float out = Trivial_Triangle(modulo_counter_);

        // add phase inc, which also defines the frequency.
        modulo_counter_ += phase_inc_;

        return amp_ * out;
    }

  private:
    //to make a bi polar sawtooth (change range from -Pi to +Pi & then turn to -1 to +1)
    float inline UniToBiPolar(float t)
    {
        return (t - PI_F) * 2.0f * TWO_PI_RECIP;
    }

    float inline Trivial_Ramp(float t) { return UniToBiPolar(t); }

    float inline Trivial_SAW(float t) { return -1.0f * Trivial_Ramp(t); }

    float inline Trivial_Triangle(float t)
    {
        return (2.0f * (fabs(Trivial_Ramp(t)))) - 1.0f;
    }

    //PW a value between 0 to 2 Pi, default is assumed Pi or half of the duty cycle
    float inline Trivial_Square(float t, float pw)
    {
        return t > pw ? -1.0f : 1.0f;
    }
};

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
