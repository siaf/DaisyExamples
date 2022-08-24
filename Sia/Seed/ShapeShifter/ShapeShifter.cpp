#include "daisy_seed.h"
#include "daisysp.h"
#include "ShapeShifterHW.h"
#include "UI/MainMenuItem.cpp"

using namespace daisy;
using namespace daisysp;
using namespace fluid;

class CVGateGenerator
{
  public:
    uint32_t trigger_update_last_;
    uint32_t trigger_update_period_;
    int      outputChannel_;
    bool     triggerOn = false;

    void Setup(int outputChannel, uint32_t now, uint32_t trigger_update_period)
    {
        trigger_update_last_   = now;
        trigger_update_period_ = trigger_update_period;
        outputChannel_         = outputChannel;
    }

    //Sends trigger based on time
    //can update to return the value of the gate only, and leave hardware handles to another class.
    bool Process(ShapeShifterHW& hw)
    {
        if(triggerOn)
        {
            hw.WriteCvOut(outputChannel_, 5.0);
        }
        else
        {
            hw.WriteCvOut(outputChannel_, 0.0);
        }

        if(hw.GetNow() - trigger_update_last_ > trigger_update_period_)
        {
            trigger_update_last_ = hw.GetNow();
            triggerOn            = !triggerOn;
        }
        return triggerOn;
    }
};

class TestKnobs : public Applet
{
  private:
    char strbuff[128];

  public:
    TestKnobs(const char* title)
    {
        sprintf(name, title);
        AudioProcessing     = false;
        TimerbasedProcssing = false;
    };
    ~TestKnobs() override{};

    void ProcessTimeBased(ShapeShifterHW& hw) override{};
    void ProcessAudio(const float inL,
                      const float inR,
                      float&      outL,
                      float&      outR) override{};
    void RenderDisplay(ShapeShifterHW& hw) override
    {
        sprintf(strbuff, "CTRL1: %d", hw.CtrlToPercentage(CTRL1));
        hw.display.SetCursor(0, 20);
        hw.display.WriteString(strbuff, Font_7x10, false);

        sprintf(strbuff, "CTRL2: %d", hw.CtrlToPercentage(CTRL2, true));
        hw.display.SetCursor(0, 28);
        hw.display.WriteString(strbuff, Font_7x10, false);

        sprintf(strbuff, "CTRL3: %d", hw.CtrlToPercentage(CTRL3, true));
        hw.display.SetCursor(0, 36);
        hw.display.WriteString(strbuff, Font_7x10, false);

        sprintf(strbuff, "CTRL4: %d", hw.CtrlToPercentage(CTRL4, true));
        hw.display.SetCursor(0, 44);
        hw.display.WriteString(strbuff, Font_7x10, false);

        sprintf(strbuff, "CTRL5: %d", hw.CtrlToPercentage(CTRL5, true));
        hw.display.SetCursor(0, 52);
        hw.display.WriteString(strbuff, Font_7x10, false);
    };
};

class TestCvsIns : public Applet
{
  private:
    char strbuff[128];

  public:
    TestCvsIns(const char* title)
    {
        sprintf(name, title);
        AudioProcessing     = false;
        TimerbasedProcssing = false;
    };
    ~TestCvsIns() override{};

    void ProcessTimeBased(ShapeShifterHW& hw) override{};
    void ProcessAudio(const float inL,
                      const float inR,
                      float&      outL,
                      float&      outR) override{};

    void RenderDisplay(ShapeShifterHW& hw) override
    {
        sprintf(strbuff, "CTRL6: %d", hw.CtrlToPercentage(CTRL6, true));
        hw.display.SetCursor(0, 20);
        hw.display.WriteString(strbuff, Font_7x10, false);

        sprintf(strbuff, "CTRL7: %d", hw.CtrlToPercentage(CTRL7, true));
        hw.display.SetCursor(0, 28);
        hw.display.WriteString(strbuff, Font_7x10, false);

        sprintf(strbuff, "CTRL8: %d", hw.CtrlToPercentage(CTRL8, true));
        hw.display.SetCursor(0, 36);
        hw.display.WriteString(strbuff, Font_7x10, false);

        sprintf(strbuff, "CTRL9: %d", hw.CtrlToPercentage(CTRL9, true));
        hw.display.SetCursor(0, 44);
        hw.display.WriteString(strbuff, Font_7x10, false);
    };
};

// Sends Gate Signals on CV Outs
class CVOutGate : public Applet
{
  private:
    char            strbuff[128];
    CVGateGenerator cvClock1;
    CVGateGenerator cvClock2;

  public:
    CVOutGate(const char* title,
              float       samplerate,
              uint32_t    now,
              uint32_t    trigger_update_period)
    {
        sprintf(name, title);
        AudioProcessing     = false;
        TimerbasedProcssing = true;
        cvClock1.Setup(CV_OUT_1, now, trigger_update_period);
        cvClock2.Setup(CV_OUT_2, now, trigger_update_period);
    };
    ~CVOutGate() override{};

    void ProcessTimeBased(ShapeShifterHW& hw) override
    {
        //Update periods
        uint32_t period1 = 1000 * hw.CtrlVal(CTRL9, true);
        uint32_t period2 = 1000 * hw.CtrlVal(CTRL8, true);
        uint32_t fine1   = 100 * hw.CtrlVal(CTRL6, true);
        uint32_t fine2   = 100 * hw.CtrlVal(CTRL7, true);

        cvClock1.trigger_update_period_ = period1 + fine1;
        cvClock2.trigger_update_period_ = period2 + fine2;

        bool gt1On = cvClock1.Process(hw);
        bool gt2On = cvClock2.Process(hw);

        //update Leds based on gates
        if(gt1On)
        {
            hw.led1.Set(1.0f);
            dsy_gpio_write(&hw.gate_out_1, true);
        }
        else
        {
            dsy_gpio_write(&hw.gate_out_1, false);
            hw.led1.Set(0.0f);
        }
        hw.led1.Update();

        //update Leds based on gates
        if(gt2On)
        {
            hw.led2.Set(1.0f);
            dsy_gpio_write(&hw.gate_out_1, true);
        }
        else
        {
            hw.led2.Set(0.0f);
            dsy_gpio_write(&hw.gate_out_2, false);
        }
        hw.led2.Update();
    };

    void ProcessAudio(const float inL,
                      const float inR,
                      float&      outL,
                      float&      outR) override{};

    void RenderDisplay(ShapeShifterHW& hw) override
    {
        sprintf(strbuff,
                "Gate 1: ms=%d",
                static_cast<int>(cvClock1.trigger_update_period_));
        hw.display.SetCursor(0, 20);
        hw.display.WriteString(strbuff, Font_7x10, false);

        sprintf(strbuff,
                "Gate 2: ms=%d",
                static_cast<int>(cvClock2.trigger_update_period_));
        hw.display.SetCursor(0, 28);
        hw.display.WriteString(strbuff, Font_7x10, false);
    };
};

class SimpleOscMenu : public Applet
{
  private:
    char       strbuff[128];
    float      FM;
    float      Freq;
    float      fine;
    float      FMwidth;
    float      spread;
    Oscillator osc1;
    Oscillator osc2;
    Oscillator osc3;
    Oscillator osc4;
    Oscillator osc5;

  public:
    SimpleOscMenu(const char* title, float samplerate)
    {
        sprintf(name, title);
        AudioProcessing     = true;
        TimerbasedProcssing = true;

        //Set up oscillators
        osc1.Init(samplerate);
        osc1.SetWaveform(osc1.WAVE_POLYBLEP_SAW);
        osc1.SetAmp(0.f);
        osc1.SetFreq(220);

        //Set up oscillators
        osc2.Init(samplerate);
        osc2.SetWaveform(osc2.WAVE_POLYBLEP_SAW);
        osc2.SetAmp(0.f);
        osc2.SetFreq(220);

        //Set up oscillators
        osc3.Init(samplerate);
        osc3.SetWaveform(osc3.WAVE_POLYBLEP_SAW);
        osc3.SetAmp(0.f);
        osc3.SetFreq(220);

        //Set up oscillators
        osc4.Init(samplerate);
        osc4.SetWaveform(osc1.WAVE_POLYBLEP_SAW);
        osc4.SetAmp(0.f);
        osc4.SetFreq(220);
    };
    ~SimpleOscMenu() override{};

    void ProcessTimeBased(ShapeShifterHW& hw) override
    {
        Freq    = hw.CtrlVal(CTRL1) * 2000;
        fine    = (hw.CtrlVal(CTRL8, true) * 100) - 50;
        FMwidth = hw.CtrlVal(CTRL9, true) * 100;
        spread  = hw.CtrlVal(CTRL6, true) * 100;
        FM      = hw.CtrlVal(CTRL2, true);

        osc1.SetAmp(0.5f);
        osc1.SetFreq(Freq + fine + (FM * FMwidth) + spread);

        osc2.SetAmp(1.0f);
        osc2.SetFreq(Freq + fine + (FM * FMwidth) + spread);

        osc3.SetAmp(1.0f);
        osc3.SetFreq(Freq + fine + (FM * FMwidth));

        osc4.SetAmp(1.0f);
        osc4.SetFreq(Freq + fine + (FM * FMwidth) - spread);
    };

    void ProcessAudio(const float inL,
                      const float inR,
                      float&      outL,
                      float&      outR) override
    {
        float osc_out, osc1_out, osc2_out, osc3_out, osc4_out;

        osc1_out = osc1.Process();
        osc2_out = osc2.Process();
        osc3_out = osc3.Process();
        osc4_out = osc4.Process();
        osc_out  = osc1_out + osc2_out + osc3_out + osc4_out;

        outL = osc_out;
        outR = osc_out;
    };

    void RenderDisplay(ShapeShifterHW& hw) override
    {
        Freq    = hw.CtrlVal(CTRL1) * 2000;
        fine    = (hw.CtrlVal(CTRL8, true) * 100) - 50;
        FMwidth = hw.CtrlVal(CTRL9, true) * 100;
        spread  = hw.CtrlVal(CTRL6, true) * 100;
        FM      = hw.CtrlVal(CTRL2, true);

        sprintf(strbuff, "Freq: %d", static_cast<int>(Freq + fine));
        hw.display.SetCursor(0, 20);
        hw.display.WriteString(strbuff, Font_7x10, false);

        sprintf(strbuff, "Spread: %d", static_cast<int>(spread));
        hw.display.SetCursor(0, 28);
        hw.display.WriteString(strbuff, Font_7x10, false);

        sprintf(strbuff, "FM Width: %d", static_cast<int>(FMwidth));
        hw.display.SetCursor(0, 36);
        hw.display.WriteString(strbuff, Font_7x10, false);

        sprintf(strbuff, "FM CV 2: %d", static_cast<int>(FM));
        hw.display.SetCursor(0, 44);
        hw.display.WriteString(strbuff, Font_7x10, false);
    };
};

class Osciloscope : public Applet
{
  private:
    char strbuff[128];

    float    buf1[280];
    uint16_t buf1Size  = 280;
    uint16_t readBuf1  = 0;
    uint16_t writeBuf1 = 0;

    float    buf2[280] = {0};
    uint16_t buf2Size  = 280;
    uint16_t readBuf2  = 0;
    uint16_t writeBuf2 = 0;


    void DrawScopeBox(ShapeShifterHW& hw)
    {
        hw.display.DrawRect(8, 32, 120, 62, false, true);
        hw.display.DrawLine(8, 47, 13, 47, true);
        hw.display.DrawLine(117, 47, 120, 47, true);

        //hw.display.DrawPixel(20, 43, true);
    }

  public:
    Osciloscope(const char* title, float samplerate)
    {
        sprintf(name, title);
        AudioProcessing     = true;
        TimerbasedProcssing = false;
    };
    ~Osciloscope() override{};

    void ProcessTimeBased(ShapeShifterHW& hw) override{};

    void ProcessAudio(const float inL,
                      const float inR,
                      float&      outL,
                      float&      outR) override
    {
        //pupulate buffers
        buf1[(readBuf1++ + buf1Size) % buf1Size] = inL;
        buf2[(readBuf2++ + buf2Size) % buf2Size] = inR;
    };

    void RenderDisplay(ShapeShifterHW& hw) override
    {
        //look for the trigger in the first half of the buffer
        uint8_t  samples_per_point1 = 4;
        uint16_t readPtr            = readBuf1;
        float    trigger            = 0.f;
        for(int i = 0; i < buf1Size / samples_per_point1; i++)
        {
            int   ptr = (((i * samples_per_point1) + readBuf1 + buf1Size));
            float p1  = buf1[ptr % buf1Size];
            float p2  = buf1[(ptr + samples_per_point1) % buf1Size];

            if(p1 < trigger && p2 > trigger)
            {
                readPtr = ptr % buf1Size;
            }
        }

        //draw buffer 1
        uint_fast8_t y_offset1 = 38;
        uint_fast8_t y_scale1  = 40;
        for(int i = 0; i < buf1Size / samples_per_point1; i++)
        {
            int8_t p1 = (buf1[(((i * samples_per_point1) + readPtr + buf1Size)
                               % buf1Size)]
                         * y_scale1);


            int8_t p2 = (buf1[(((i * samples_per_point1) + samples_per_point1
                                + readPtr + buf1Size)
                               % buf1Size)]
                         * y_scale1);


            if(abs(p1 - p2) < 2)
            {
                hw.display.DrawPixel(i, y_offset1 + p1, false);
            }
            else
            {
                hw.display.DrawLine(
                    i, y_offset1 + p1, i + 1, y_offset1 + p2, false);
            }
        }


        uint_fast8_t y_offset2          = 38;
        uint_fast8_t y_scale2           = 30;
        uint8_t      samples_per_point2 = 4;
        for(int i = 0; i < buf2Size / samples_per_point2; i++)
        {
            int8_t p1 = (buf2[(((i * samples_per_point2) + readBuf2 + buf2Size)
                               % buf2Size)]
                         * y_scale2);


            int8_t p2 = (buf2[(((i * samples_per_point2) + samples_per_point2
                                + readBuf2 + buf2Size)
                               % buf2Size)]
                         * y_scale2);

            if(abs(p1 - p2) < 2)
            {
                hw.display.DrawPixel(i, y_offset2 + p1, false);
            }
            else
            {
                hw.display.DrawLine(
                    i, y_offset2 + p1, i + 1, y_offset2 + p2, false);
            }
        }
    };
};

ShapeShifterHW hw;

int       MainMenuCurrent = 0;
const int MaxMenuCount    = 3;
MenuItem* menu[MaxMenuCount];

void ConfigureMenu()
{
    //How many samples we'll output per second
    float samplerate = hw.AudioSampleRate();

    //Setup Menus
    menu[0] = new MenuItem("Test");
    menu[0]->subMenus.push_back(new TestKnobs("1-5"));
    menu[0]->subMenus.push_back(new TestCvsIns("6-9"));
    menu[0]->subMenus.push_back(new Osciloscope("scope", samplerate));

    menu[1] = new MenuItem("Osc"); // Multiple SAWs, Physical Osc, FM Osc, etc.
    menu[1]->subMenus.push_back(new SimpleOscMenu("Simp", samplerate)); // SAW
    menu[1]->subMenus.push_back(new TestKnobs("1-5"));
    menu[1]->subMenus.push_back(new SimpleOscMenu("Simp2", samplerate)); // SAW
    menu[1]->subMenus.push_back(new TestCvsIns("6-9"));

    menu[2] = new MenuItem("CVs");
    // 2Hz by default
    menu[2]->subMenus.push_back(
        new CVOutGate("Cv_Gt", samplerate, hw.GetNow(), 500));
    menu[2]->subMenus.push_back(new TestKnobs("1-5"));
    menu[2]->subMenus.push_back(new TestCvsIns("6-9"));
}


class DisplayManager
{
  private:
    char     strbuff[128];
    char     strbuff2[128];
    uint32_t screen_update_last_;
    uint32_t screen_update_period_;


  public:
    void Setup()
    {
        //Construct of display
        screen_update_last_   = hw.GetNow();
        screen_update_period_ = 17; //58.8Hz (targeting 60Hz)
    }
    void UpdateDisplay()
    {
        if(hw.GetNow() - screen_update_last_ > screen_update_period_)
        {
            screen_update_last_ = hw.GetNow();
            hw.display.Fill(true);


            // Draw stuff on display based on current Menu:

            //Print Title
            sprintf(strbuff2,
                    "%s>%s",
                    menu[MainMenuCurrent]->name,
                    menu[MainMenuCurrent]->GetCurrent()->name);

            hw.display.SetCursor(0, 0);
            hw.display.WriteString(strbuff2, Font_11x18, false);

            //Print menu specif
            menu[MainMenuCurrent]->GetCurrent()->RenderDisplay(hw);


            hw.display.Update();
        }
    }


    inline void DrawKnobValRound(float val, uint_fast8_t x, uint_fast8_t y)
    {
        int_fast16_t valRad = -12 + (-290 * val);
        hw.display.DrawCircle(x, y, 3, true);
        hw.display.DrawArc(x, y, 6, -120, valRad, true);
        hw.display.DrawArc(x, y, 7, -120, valRad, true);
    }


    void UpdateDisplayx()
    {
        if(hw.GetNow() - screen_update_last_ > screen_update_period_)
        {
            screen_update_last_ = hw.GetNow();
            hw.display.Fill(false);


            // Draw stuff on display based on current Menu:
            // hw.display.DrawRect(0, 0, hw.display.Width(), 18, true, true);

            //Print Title
            sprintf(strbuff2, "%s>%s", "Menu", "SUB");

            hw.display.SetCursor(0, 0);
            hw.display.WriteString(strbuff2, Font_11x18, true);


            uint_fast8_t w = hw.display.Width();

            DrawKnobValRound(hw.CtrlVal(CTRL9, true), w - 10, 24);
            DrawKnobValRound(hw.CtrlVal(CTRL4, true), w - 10, 39);
            DrawKnobValRound(hw.CtrlVal(CTRL7, true), w - 10, 54);

            DrawKnobValRound(hw.CtrlVal(CTRL1), w - 26, 24);
            DrawKnobValRound(hw.CtrlVal(CTRL2, true), w - 26, 39);
            DrawKnobValRound(hw.CtrlVal(CTRL3, true), w - 26, 54);

            DrawKnobValRound(hw.CtrlVal(CTRL8, true), w - 42, 24);
            DrawKnobValRound(hw.CtrlVal(CTRL5, true), w - 42, 39);
            DrawKnobValRound(hw.CtrlVal(CTRL6, true), w - 42, 54);

            hw.display.Update();
        }
    }
};


DisplayManager display;


uint32_t buttons_update_last_;
uint32_t buttons_update_period_ = 300;

uint32_t enc_update_last_;
uint32_t enc_update_period_ = 300;


void HandleButtonsForMenu()
{
    hw.encoder.Debounce();

    if(hw.encoder.Increment() == -1)
    {
        //iterate submenu items
        menu[MainMenuCurrent]->Previous();
    }

    if(hw.encoder.Increment() == +1)
    {
        //iterate submenu items
        menu[MainMenuCurrent]->Next();
    }

    if(hw.encoder.TimeHeldMs() > 300)
    {
        hw.rgbLed.Set(0.f, 0.f, 1.f);
        hw.rgbLed.Update();

        //if this was triggered disable button for a bit
        enc_update_last_ = hw.GetNow();
    }

    if(hw.encoder.FallingEdge()
       && !(hw.GetNow() - enc_update_last_ < enc_update_period_))
    {
        hw.rgbLed.Set(1.f, 1.f, 1.f);
        hw.rgbLed.Update();

        //iterate menu items
        MainMenuCurrent++;
        if(MainMenuCurrent == MaxMenuCount)
        {
            MainMenuCurrent = 0;
        }
    }

    hw.button1.Debounce();
    hw.button2.Debounce();

    if(hw.button1.TimeHeldMs() >= 300 && hw.button2.TimeHeldMs() >= 300)
    {
        hw.led2.Set(1.0f);
        hw.led2.Update();
        hw.led1.Set(1.0f);
        hw.led1.Update();

        //if this was triggered disable buttons for a bit
        buttons_update_last_ = hw.GetNow();
    }

    if(hw.button1.FallingEdge()
       && !(hw.GetNow() - buttons_update_last_ < buttons_update_period_))
    {
        //iterate menu items
        MainMenuCurrent++;
        if(MainMenuCurrent == MaxMenuCount)
        {
            MainMenuCurrent = 0;
        }
    }

    if(hw.button2.FallingEdge()
       && !(hw.GetNow() - buttons_update_last_ < buttons_update_period_))
    {
        //iterate submenu items
        menu[MainMenuCurrent]->Next();
    }
}

void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
    //Handle Buttons, encoders.
    HandleButtonsForMenu();


    float ctrl1 = hw.GetAdcFloat(CTRL1);
    hw.WriteCvOut(CV_OUT_1, ctrl1 * 50.0f);

    for(size_t i = 0; i < size; i += 2)
    {
        //first just passthrough
        out[i]     = in[i];
        out[i + 1] = in[i + 1];


        //then process
        if(menu[MainMenuCurrent]->GetCurrent()->AudioProcessing)
        {
            menu[MainMenuCurrent]->GetCurrent()->ProcessAudio(
                in[i], in[i + 1], out[i], out[i + 1]);
        }

        /*    
        for(int i = 0; i < MaxMenuCount; i++)
        {
            int length = menu[i]->GetLen();
            for(int j = 0; j < length; j++)
            {
                if(menu[i]->subMenus[j]->AudioProcessing)
                {
                    menu[i]->subMenus[j]->ProcessAudio(
                        in[i], in[i + 1], out[i], out[i + 1]);
                }
            }
        }*/
    }
};


void init(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4);

    ConfigureMenu();

    display.Setup();

    //Start the adc for CV ins & Knobs
    hw.StartAdc();

    //Start calling the audio callback
    hw.StartAudio(AudioCallback);

    //Start DAC for CVOuts
    hw.StartDac(nullptr);
}


int main(void)
{
    init();

    while(1)
    {
        //Do time based processing.
        for(int i = 0; i < MaxMenuCount; i++)
        {
            int length = menu[i]->GetLen();
            for(int j = 0; j < length; j++)
            {
                if(menu[i]->subMenus[j]->TimerbasedProcssing)
                {
                    menu[i]->subMenus[j]->ProcessTimeBased(hw);
                }
            }
        }

        //Render Visuals
        display.UpdateDisplay();
    }
}
