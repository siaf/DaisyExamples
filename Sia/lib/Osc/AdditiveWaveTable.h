#include "vector"
#include "stdint.h"
#include "OscilatorWaveTable.h"

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

