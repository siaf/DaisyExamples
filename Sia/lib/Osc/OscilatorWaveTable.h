#include "../resources.h"

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
        //Generate_Sin_WaveTable();
        Generate_Triangle_WaveTable();

        SetFreq(440.f);
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

        int currentSample = (int)modulo_counter_;
        //Method 2: linear interpolatation.
        /* 
        
        int nextSample    = currentSample + 1;
        if(nextSample >= waveTableLength)
        {
            nextSample = 0;
        }

       out = Linear_Interpolate(waveTable[currentSample],
                                 waveTable[nextSample],
                                 modulo_counter_ - currentSample);*/

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
    void Populate_Wv(const float(wt)[512])
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
};
