// y[n] = a.y[n-1] + (1-a).x[n]
class FirstOrderIIRLowPassFilter
{
    float previousOutput_ = 0.0f;
    float alpha           = 0.99f;
    float process(float in)
    {
        float out       = (alpha * previousOutput_) + ((1 - alpha) * in);
        previousOutput_ = out;
        return out;
    }
};