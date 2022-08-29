
#include "../resources.h"

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