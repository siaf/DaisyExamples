#pragma once
#include "math.h"

#define PI_F 3.1415927410125732421875f
#define TWOPI_F (2.0f * PI_F)
constexpr float TWO_PI_RECIP = 1.0f / TWOPI_F;

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