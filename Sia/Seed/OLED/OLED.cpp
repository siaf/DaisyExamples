#include <stdio.h>
#include <string.h>
#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"

using namespace daisy;

/** Typedef the OledDisplay to make syntax cleaner below 
 *  This is a 4Wire SPI Transport controlling an 128x64 sized SSDD1306
 * 
 *  There are several other premade test 
*/
using MyOledDisplay = OledDisplay<SSD130xI2c128x64Driver>;

DaisySeed     hw;
MyOledDisplay display;

int main(void)
{
    uint8_t message_idx;
    hw.Configure();
    hw.Init();

    /** Configure the Display */
    MyOledDisplay::Config disp_cfg;
    disp_cfg.driver_config.transport_config.i2c_config.pin_config.scl = {DSY_GPIOB, 8};
    disp_cfg.driver_config.transport_config.i2c_config.pin_config.sda = {DSY_GPIOB, 9};
    disp_cfg.driver_config.transport_config.i2c_address =0x3C;

    /** And Initialize */
    display.Init(disp_cfg);

    message_idx = 0;
    char strbuff[128];
    char strbuff2[128];
    sprintf(strbuff2, "Title");

    while(1)
    {
        System::Delay(500);
        switch(message_idx)
        {
            case 0: sprintf(strbuff, "Testing. . ."); break;
            case 1: sprintf(strbuff, "Daisy. . ."); break;
            case 2: sprintf(strbuff, "1. . ."); break;
            case 3: sprintf(strbuff, "2. . ."); break;
            case 4: sprintf(strbuff, "3. . ."); break;
            default: break;
        }
        message_idx = (message_idx + 1) % 5;
        display.Fill(true);
        
        display.SetCursor(0, 0);
        display.WriteString(strbuff2, Font_11x18, false);
        
        display.SetCursor(0, 20);
        display.WriteString(strbuff, Font_7x10, false);
        
        display.DrawRect(8,32,120,62,false, true);
        display.DrawLine(8,47,13,47,true);
        display.DrawLine(117,47,120,47,true);

        display.DrawPixel(20,43,true);

        display.Update();
    }
}
