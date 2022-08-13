#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM hw;


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

#define PI_F 3.1415927410125732421875f
#define TWOPI_F (2.0f * PI_F)
constexpr float TWO_PI_RECIP = 1.0f / TWOPI_F;

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

Oscillator osc;

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

    float sat = (hw.GetAdcValue(CV_4) * 10.0f) + 1.0f;
    sosc.SetSat(sat);

    for(size_t i = 0; i < size; i++)
    {
        float sig  = osc.Process();
        float sig2 = sosc.Process();
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

    osc.SetAmp(0.5f);
    osc.SetFreq(440.f);
    osc.SetWaveform(osc.WAVE_TRI);

    sosc.SetFreq(440.f);
    sosc.SetAmp(0.5f);

    hw.StartAudio(AudioCallback);
    while(1) {}
}
