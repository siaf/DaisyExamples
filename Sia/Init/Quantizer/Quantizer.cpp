#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM hw;
Switch       push_button;
Switch       switch_button;
Oscillator   vibrato;

//Transpose Gate 1 variables
int keyPtr = 0; //when zero the first note of range is used. Gate_1 adds to it.

float QuantizeChromatic(float in)
{
    float note = (roundf(
        in * 60)); // there are 60 (midi) notes in 5 oct (5 vol limit of CV_In)
    return note / 60.0f;
};

float QuantizeEqualDistMajorScale(float in)
{
    // Major Scale equal distribution algorythm
    // here we map the range of 1 vol equally to the 7 notes in the scale
    // so each note get an equal range associated to it.

    float vin      = in * 5;         //convert to 0-5 volt.
    float fraction = vin - (int)vin; //take the fraction.
    float qf;                        //quantized fraction


    /*
		.000 - .071 = C (.0)
		.071 - .214 = D (.167)
		.214 - .357 = E (.333)
		.375 - .500 = F (.417)
		.500 - .642 = G (.583)
		.642 - .785 = A (.750)
		.785 - .928 = B (.917)
		.928 - 1.00 = C (1.00) next oct
		*/

    if(fraction < 0.071f) //C
        qf = 0.0f;
    else if(fraction < 0.214f) //D
        qf = 0.167;
    else if(fraction < 0.357f) //E
        qf = 0.333;
    else if(fraction < 0.500f) //F
        qf = 0.417f;
    else if(fraction < 0.642f) //G
        qf = 0.583f;
    else if(fraction < 0.785f) //A
        qf = 0.750f;
    else if(fraction < 0.928f) //B
        qf = 0.917f;
    else //C next oct
        qf = 1.0f;

    float vout = (int)vin + qf; //apply the quantized fraction to the volage
    return vout / 5.0f;
}

float QuantizeNormalDistMajorScale(float in)
{
    //Major Scale normal distribution algorythm

    // here we map the range of 1 vol equally to 12 notes,
    // and for notes that aren't in the scale associate lower half of their range to lower note in scale
    // and vice versa for upper half of their range
    // so notes like E & B get a smaller portion of the range
    // however the upside is that this distribution is exact meaning that the same
    // CV input produces the same note in chromatic scale and major scale if it's available
    // probably a btter algorithm if multiple quantizers are used.

    float vin      = in * 5;         //convert to 0-5 volt.
    float fraction = vin - (int)vin; //take the fraction.
    float qf;                        //quantized fraction

    /*
		.000 - .083 = C (.0)
		.083 - .250 = D (.167)
		.250 - .375 = E (.333)
		.375 - .500 = F (.417)
		.500 - .667 = G (.583)
		.667 - .833 = A (.750)
		.833 - .958 = B (.917)
		.958 - 1.00 = C (1.00) next oct
		*/
    if(fraction < 0.083f) //C
        qf = 0.0;
    else if(fraction < 0.250f) //D
        qf = 0.167;
    else if(fraction < 0.375f) //E
        qf = 0.333;
    else if(fraction < 0.500f) //F
        qf = 0.417f;
    else if(fraction < 0.667f) //G
        qf = 0.583f;
    else if(fraction < 0.833f) //A
        qf = 0.750f;
    else if(fraction < 0.958f) //B
        qf = 0.917f;
    else //C next oct
        qf = 1.0f;

    float vout = (int)vin + qf; //apply the quantized fraction to the volage
    return vout / 5.0f;
}

float QuantizeEqualDistMajorScaleFlexibleImpl(float in)
{
    //an implementation of the same algorythm done so that
    // it uses array of ranges instead of hard coding them
    // benefit is code flexibility, and parameterization of various quantization algorythms
    // down side is that it's going to have 2*number of notes in scale times of memory access
    // so less efficient but probably okay
    // other down side is that this is easy to make mistakes, so best
    // to include tones of unit test for this model.

    float qranges[8][2] = {
        {0.071f, 0.000f}, //.000 - .071 = C (.0)
        {0.214f, 0.167f}, //.071 - .214 = D (.167)
        {0.357f, 0.333f}, //.214 - .357 = E (.333)
        {0.500f, 0.417f}, //.375 - .500 = F (.417)
        {0.642f, 0.583f}, //.500 - .642 = G (.583)
        {0.785f, 0.750f}, //.642 - .785 = A (.750)
        {0.928f, 0.917f}, //.785 - .928 = B (.917)
        {1.000f, 1.000f}, //.928 - 1.00 = C (1.00) next oct
    };

    int len = sizeof(qranges) / (sizeof(qranges[0]) * 2);

    // Major Scale equal distribution algorythm
    // here we map the range of 1 vol equally to the 7 notes in the scale
    // so each note get an equal range associated to it.


    float vin      = in * 5;         //convert to 0-5 volt.
    float fraction = vin - (int)vin; //take the fraction.
    float qf;                        //quantized fraction

    for(int i = 0; i < len; i++)
    {
        if(fraction < qranges[i][0])
        {
            qf = qranges[i][1];
            break;
        }
        else
        {
            continue; //the else is not needed, added as a good practice.
        }
    }

    float vout = (int)vin + qf; //apply the quantized fraction to the volage
    return vout / 5.0f;
}

// The generic implementation allowing for inclusion of many scales in shorter code.

// Major Scale equal distribution algorythm
// here we map the range of 1 vol equally to the 7 notes in the scale
// so each note get an equal range associated to it.
static const float major_scale_eq_dist_qranges[8][2] = {
    {0.071f, 0.000f}, //.000 - .071 = C (.0)
    {0.214f, 0.167f}, //.071 - .214 = D (.167)
    {0.357f, 0.333f}, //.214 - .357 = E (.333)
    {0.500f, 0.417f}, //.375 - .500 = F (.417)
    {0.642f, 0.583f}, //.500 - .642 = G (.583)
    {0.785f, 0.750f}, //.642 - .785 = A (.750)
    {0.928f, 0.917f}, //.785 - .928 = B (.917)
    {1.000f, 1.000f}, //.928 - 1.00 = C (1.00) next oct
};

static int major_scale_qranges_len = sizeof(major_scale_eq_dist_qranges)
                                     / (sizeof(major_scale_eq_dist_qranges[0]));

float QuantizeFlexibleImpl(float in, const float qranges[][2], int rlen)
{
    //See QuantizeEqualDistMajorScaleFlexibleImpl(float in) for documentation.

    float vin      = in * 5;         //convert to 0-5 volt.
    float fraction = vin - (int)vin; //take the fraction.
    float qf;                        //quantized fraction

    for(int i = 0; i < rlen; i++)
    {
        if(fraction < qranges[i][0])
        {
            //choose the key based on the tranposable keyptr.
            //if key ptr is more than zero it means we need to tranpose
            //and take X number of notes above.
            int tranpose_index
                = keyPtr
                  % (rlen - 1); //ensure tranpose index is within the scale.
            int tranposed_note_index
                = (i + (tranpose_index))
                  % rlen; //ensure tranpose is applied and within array range.
            //transpose has a bug (the case that both low C and high C are in the array causes problems, revisit)
            qf = qranges[tranposed_note_index][1];

            if(i < tranpose_index)
            { //this is avoid the 1 octave drop when tranposing for notes that come before the key of scale
                qf += 1.0f;
            }

            //todo: add a bias mechanism to prefer the previously quantized note
            //otherwise minor changes when happening on the exact transition points can cause
            // rapid switches between the notes.

            //root note detected Set Gate 1 on
            if(i == 0 || i == rlen - 1)
            {
                /** Set the gate high */
                dsy_gpio_write(&hw.gate_out_1, true);
            }
            else
            {
                dsy_gpio_write(&hw.gate_out_1, false);
            }

            //root note generated after transpose Set Gate 2 on
            if(tranposed_note_index == 0 || tranposed_note_index == rlen - 1)
            {
                /** Set the gate high */
                dsy_gpio_write(&hw.gate_out_2, true);
            }
            else
            {
                dsy_gpio_write(&hw.gate_out_2, false);
            }
            break;
        }
        else
        {
            continue; //the else is not needed, added as a good practice.
        }
    }

    float vout = (int)vin + qf; //apply the quantized fraction to the volage
    return vout / 5.0f;
}

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
        quantized = QuantizeFlexibleImpl(
            cv1In, major_scale_eq_dist_qranges, major_scale_qranges_len);

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
