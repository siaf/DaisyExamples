#include <stdint.h>

class Metronome
{
  private:
    float    sample_rate_;
    float    interval_; //how many samples for each cycle of delay;
    uint32_t counter       = 0;
    bool     is_triggered_ = false;

  public:
    void Init(float sample_rate) { sample_rate_ = sample_rate; }

    void SetTempo(float sec) { interval_ = sample_rate_ * sec; }
    void SetTempo(uint16_t bpm)
    {
        interval_ = sample_rate_ * (60.0f / (float)bpm);
    }
    bool  IsTriggered() { return is_triggered_; }
    float Process()
    {
        counter++;
        if(counter > interval_)
        {
            counter       = 0;
            is_triggered_ = true;
            return 1.0f;
        }
        else
        {
            is_triggered_ = false;
            return 0.0f;
        }
    }
};
