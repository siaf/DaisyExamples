//y[n] = x[n] - x[n-1]
// it's an FIR filter, because there's no dependency on previous output
// it's a Highpass filter
//
class FirstOrderDifferenceFilter
{
    float previousSample_ = 0.0f;
    float process(float in)
    {
        float out       = in - previousSample_;
        previousSample_ = in;
        return out;
    }
};