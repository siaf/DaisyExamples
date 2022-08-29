#include "../resources.h"

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
