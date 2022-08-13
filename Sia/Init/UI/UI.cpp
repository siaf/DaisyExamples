#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "dev/oled_ssd130x.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;


/** Typedef the OledDisplay to make syntax cleaner below 
 *  This is a 4Wire SPI Transport controlling an 128x64 sized SSDD1306
 * 
 *  There are several other premade test 
*/
using FluidOledDisplay = OledDisplay<SSD130xI2c128x64Driver>;

DaisyPatchSM     hw;
FluidOledDisplay display;
Encoder          encoder;
int              inc             = 0;
bool             encoder_pressed = false;
void             AudioCallback(AudioHandle::InputBuffer  in,
                               AudioHandle::OutputBuffer out,
                               size_t                    size)
{
    hw.ProcessAllControls();

    encoder.Debounce();
    inc += encoder.Increment();
	
    encoder_pressed = encoder.Pressed();

    for(size_t i = 0; i < size; i++)
    {
        OUT_L[i] = IN_L[i];
        OUT_R[i] = IN_R[i];
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.StartAudio(AudioCallback);

    //Encoder init
    encoder.Init(hw.D2, hw.D3, hw.D1);

    /** Configure the Display */
    FluidOledDisplay::Config disp_cfg;
    disp_cfg.driver_config.transport_config.i2c_config.pin_config.scl = hw.B7;
    disp_cfg.driver_config.transport_config.i2c_config.pin_config.sda = hw.B8;
    disp_cfg.driver_config.transport_config.i2c_address               = 0x3C;

    /** And Initialize */
    display.Init(disp_cfg);

    uint32_t screen_update_last_   = hw.system.GetNow();
    uint32_t screen_update_period_ = 17; //58.8Hz (targeting 60Hz)
    char     strbuff[128];

    while(1)
    {
        if(hw.system.GetNow() - screen_update_last_ > screen_update_period_)
        {
            //sprintf(strbuff, "Hi");

            if(encoder_pressed)
            {
                sprintf(strbuff, "P %d", inc);
            }
            else
            {
                sprintf(strbuff, "NP %d", inc);
            }

            screen_update_last_ = hw.system.GetNow();
            display.Fill(true);

            display.SetCursor(0, 0);
            display.WriteString(strbuff, Font_11x18, false);

            display.Update();
        }
    }
}
