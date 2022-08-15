#include "daisy_patch_sm.h"
#include "daisysp.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM hw;

// *** Constants: change these to alter the sound of the Shepard-Risset effect
// How many simultaneous oscillators?
const unsigned int kNumOscillators = 8;

// Ratio between oscillators. 2.0 = octave. Try also powf(2.0, 1.0/3.0)
const float kFrequencyRatio = 2.0;

// Starting frequency of the lowest oscillator
// The highest frequency will be L*2^N where L is the lowest frequency and N is the number of oscillators
// Make sure the highest frequency is less than the Nyquist rate (22050Hz in this case)
const float kLowestBaseFrequency = 30.0;

// Size of the window that provides spectral rolloff
const unsigned int kSpectralWindowSize = 1024;

// Amplitude normalisation to avoid clipping. Adjust depending on kNumOscillators.
const float kAmplitude = 0.1;

// This value is pre-calculated for efficiency in render(). Don't change it,
// change the values above.
const float kMaxFrequencyRatio
    = log(powf(kFrequencyRatio, kNumOscillators)) / log(2.0);

// How often to update the oscillator frequencies
// Don't do this every sample as it's inefficient and will run into
// numerical precision issues
const unsigned int kUpdateInterval = 64;
unsigned int gUpdateCount = 0; // counting samples to update the oscillators

// *** Global variables: these keep track of the current state of the
std::vector<Oscillator> gOscillators; // Oscillator bank
std::vector<float> gLogFrequencies; // Log-scale frequencies for each oscillator
std::vector<float> gAmplitudes;     // Amplitudes of each oscillator
std::vector<float> gSpectralWindow; // Window defining spectral rolloff

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls();

    // How long (in seconds) to complete one cycle, i.e. an increase by kFrequencyRatio
    // The effect is better when this is longer
    // Logarithmic mapping between 0.1 and 20.0
    float cycleTime=20.0f;

    // Amount to update the frequency by on a normalised 0-1 scale
    // Controls how fast the glissando moves
    // In the time span of cycleTime, the frequency should go up to the spacing between oscillators
    // (i.e. complete one cycle) -- so it traverses a span of (1.0 / kNumOscillators) over cycleTime seconds
    float logFrequencyIncrement
        = (float)kUpdateInterval
          / (kNumOscillators * cycleTime * hw.AudioSampleRate());

    for(size_t i = 0; i < size; i++)
    {
        float out_val = 0;

        if(gUpdateCount >= kUpdateInterval)
        {
            gUpdateCount = 0;

            // Update the oscillator frequencies and amplitudes
            for(unsigned int i = 0; i < kNumOscillators; i++)
            {
                // Calculate the actual frequency from the normalised log-frequency
                float frequency
                    = kLowestBaseFrequency
                      * powf(2.0, gLogFrequencies[i] * kMaxFrequencyRatio);
                gOscillators[i].SetFreq(frequency);

                // Calculate the amplitude of this oscillator by finding its position in the
                // window on a normalised logarithmic frequency scale
                gAmplitudes[i] = gSpectralWindow[(int)(gLogFrequencies[i]
                                                       * kSpectralWindowSize)];

                // Update the frequency of this oscillator and wrap around if it falls
                // off the end of the range
                gLogFrequencies[i] += logFrequencyIncrement;
                if(gLogFrequencies[i] >= 1.0)
                {
                    // Recalculate all the other oscillator frequencies as a function of this one
                    // to prevent numerical precision errors from accumulating
                    unsigned int osc = i;
                    for(unsigned int k = 0; k < kNumOscillators; k++)
                    {
                        gLogFrequencies[osc]
                            = (float)k / (float)kNumOscillators;
                        osc++;
                        if(osc >= kNumOscillators)
                            osc = 0;
                    }
                }
                if(gLogFrequencies[i] < 0.0)
                    gLogFrequencies[i] += 1.0;
            }
        }

        ++gUpdateCount;

        // Compute the oscillator outputs every sample
        for(unsigned int i = 0; i < kNumOscillators; i++)
        {
            // Mix this oscillator into the audio output
            out_val += gOscillators[i].Process() * gAmplitudes[i] * kAmplitude;
        }

        // Write the output to all the audio channels
        OUT_L[i] = out_val;
        OUT_R[i] = out_val;
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(128); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.StartAudio(AudioCallback);

    // Initialise the oscillator bank and set its sample rate
    gOscillators.resize(kNumOscillators);
    for(unsigned int i = 0; i < kNumOscillators; i++)
    {
        gOscillators[i].SetWaveform(Oscillator::WAVE_SIN);
        gOscillators[i].Init(hw.AudioSampleRate());
    }

    // Initialise arrays of log-frequencies and set them to
    // span a range from 0 to 1. This will be used to look up
    // the amplitude from the spectral window and also to calculate
    // the actual frequency of that oscillator
    gLogFrequencies.resize(kNumOscillators);
    for(unsigned int i = 0; i < kNumOscillators; i++)
    {
        gLogFrequencies[i] = (float)i / (float)kNumOscillators;
    }

    // Initialise array of amplitudes for each oscillator. These will be updated
    // when the frequencies change.
    gAmplitudes.resize(kNumOscillators);
    for(unsigned int i = 0; i < kNumOscillators; i++)
    {
        gAmplitudes[i] = 0;
    }

    // Initialise a Hann window for spectral rolloff. This makes the lowest and highest
    // frequencies fade out smoothly, improving the realism of the effect.
    gSpectralWindow.resize(kSpectralWindowSize);
    for(unsigned int n = 0; n < kSpectralWindowSize; n++)
    {
        gSpectralWindow[n]
            = 0.5f
              * (1.0f
                 - cosf(2.0 * M_PI * n / (float)(kSpectralWindowSize - 1)));
    }

    while(1) {}
}
