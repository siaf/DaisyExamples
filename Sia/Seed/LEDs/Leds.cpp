#include "daisy_seed.h"

// Use the daisy namespace to prevent having to type
// daisy:: before all libdaisy functions
using namespace daisy;

// Declare a DaisySeed object called hardware
DaisySeed hw;

int main(void)
{
    // Configure and Initialize the Daisy Seed
    // These are separate to allow reconfiguration of any of the internal
    // components before initialization.
    hw.Configure();
    hw.Init();

    //Configure and initialize button
    Switch button1;

    //Set button to pin 0, to be updated at a 1kHz  samplerate
    button1.Init(hw.GetPin(0), 1000);

    Switch button2;
    //Set button to pin 0, to be updated at a 1kHz  samplerate
    button2.Init(hw.GetPin(1), 1000);

    Led led1;
    led1.Init(hw.GetPin(2), false, 1000);
    led1.Set(1.0f);
    led1.Update();

    Led led2;
    led2.Init(hw.GetPin(3), false, 1000);
    led2.Set(1.0f);
    led2.Update();

    Led led3;
    led3.Init(hw.GetPin(4), false, 1000);
    led3.Set(1.0f);
    led3.Update();

    Led led4;
    led4.Init(hw.GetPin(5), false, 1000);
    led4.Set(1.0f);
    led4.Update();

    bool status = false;
    int  i      = 0; //active led;
    // Loop forever
    for(;;)
    {
        //Debounce the button
        button1.Debounce();
        button2.Debounce();

        if(button1.RisingEdge())
        {
            status = !status;
            i++;
            if(i > 3)
            {
                i = 0;
            }
        }

        if(button2.RisingEdge())
        {
            status = !status;
            i--;
            if(i < 0)
            {
                i = 3;
            }
        }

        hw.SetLed(status);

        led1.Set(0.0f);
        led2.Set(0.0f);
        led3.Set(0.0f);
        led4.Set(0.0f);

        switch(i)
        {
            case 0: led1.Set(1.0f); break;
            case 1: led2.Set(1.0f); break;
            case 2: led3.Set(1.0f); break;
            case 3: led4.Set(1.0f); break;
        }
        
        led1.Update();
        led2.Update();
        led3.Update();
        led4.Update();

        //wait 1 ms
        System::Delay(1);
    }
}