#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "quantizers.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM hw;
Switch       push_button;
Switch       switch_button;
Oscillator   vibrato;

//Transpose Gate 1 variables
int keyPtr = 0; //when zero the first note of range is used. Gate_1 adds to it.


/** Similar to the audio callback, you can generate audio rate CV signals out of the CV outputs. 
 *  These signals are 12-bit DC signals that range from 0-5V out of the Patch SM
*/
void DacCallback(uint16_t **output, size_t size)
{
    /** Process the controls */
    hw.ProcessAnalogControls();
    push_button.Debounce();
    switch_button.Debounce();


    if(hw.gate_in_1.Trig())
    {
        //increment the keyPtr for tranpose effect.
        keyPtr++;
    }

    float cv1In = hw.GetAdcValue(CV_5); // val is 0 to 1;

    float quantized;
    if(push_button.Pressed())
    {
        //Chromatic
        quantized = QuantizeChromatic(cv1In);
    }
    else
    {
        quantized = QuantizeFlexibleImpl(cv1In,
                                         major_scale_eq_dist_qranges,
                                         major_scale_qranges_len,
                                         keyPtr);

        /*
		//Major Scale
        if(switch_button.Pressed())
        {
            quantized = QuantizeNormalDistMajorScale(cv1In);
        }
        else
        {
            quantized = QuantizeEqualDistMajorScale(cv1In);
        }*/
    }

    //Add vibrato to the quantized value
    float vibrato_range_knob = hw.GetAdcValue(CV_1);
    vibrato.SetAmp(vibrato_range_knob);

    float vibrato_speed_knob = hw.GetAdcValue(CV_2);
    float vibrato_speed      = fmap(vibrato_speed_knob, 0.1f, 10.f);
    vibrato.SetFreq(vibrato_speed);

    for(size_t i = 0; i < size; i++)
    {
        float vibration_amt
            = vibrato.Process()
              / 60.f; // limiting it 1 semitone (60 semitones per 5v)

        uint16_t out = ((quantized + vibration_amt)
                        * 4095.0); //// convert to 12-bit integer (0-4095)
        output[0][i] = out;        /**< To CV OUT 1 - Jack */
        output[1][i] = out;        /**< To CV OUT 2 - LED */
    }
}

int main(void)
{
    hw.Init();

    push_button.Init(hw.B7, 1000);
    switch_button.Init(hw.B8, 1000);

    vibrato.Init(hw.AudioSampleRate());
    vibrato.SetWaveform(vibrato.WAVE_SIN);

    hw.StartDac(DacCallback);
    while(1) {}
}
