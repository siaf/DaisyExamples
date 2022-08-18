#include <stdint.h>

class Metronome_WithTrigger
{
  private:
    float sample_rate_;
    float interval_;         //how many samples for each cycle of delay;
    float trigger_duration_; // how many samples will the trigger last.
    float trigger_inc;

    uint32_t counter       = 0;
    bool     is_triggered_ = false;
    float    trigger_value = 0.f;

  public:
    void Init(float sample_rate)
    {
        sample_rate_ = sample_rate;
        SetTempo(10.0f);
        SetTriggerDuration(1.0f);
    }

    void SetTempo(float sec) { interval_ = sample_rate_ * sec; }

    void SetTempo(uint16_t bpm)
    {
        interval_ = sample_rate_ * (60.0f / (float)bpm);
    }

    void SetTriggerDuration(float sec)
    {
        trigger_duration_ = sample_rate_ * sec;

        //calculate how much the trigger should be incremented to reach 1.0f in half trigger duration
        trigger_inc = 1.0f / trigger_duration_ / 2.0f;
    }

    bool IsTriggered() { return is_triggered_; }

    float Process()
    {
        counter++;
        if(is_triggered_)
        {
            //count until trigger duration is passed.
            if(counter > trigger_duration_)
            {
                is_triggered_ = false;
            }

            if(trigger_value < 1.0f)
            {
                //trigger to be incremented until it reaches 1.0f
                trigger_value += trigger_inc;
            }
        }
        else
        {
            //trigger to be decremented until it reaches 0.0f
            if(trigger_value > 0.0f)
            {
                trigger_value -= trigger_inc;
            }
        }

        if(counter > interval_)
        {
            counter       = 0;
            is_triggered_ = true;
        }

        return trigger_value;
    }
};