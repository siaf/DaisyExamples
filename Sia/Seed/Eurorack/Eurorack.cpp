#include <stdio.h>
#include <string.h>

#include "FluidModule.h"
#include "daisysp.h"

// Use the daisy namespace to prevent having to type
// daisy:: before all libdaisy functions
using namespace daisy;
using namespace fluid;

using namespace daisysp;


class SubMenuItem
{
  public:
    char name[8];
    bool AudioProcessing     = false;
    bool TimerbasedProcssing = false;

    void virtual RenderDisplay(FluidModule& hw)    = 0;
    void virtual ProcessTimeBased(FluidModule& hw) = 0;
    void virtual ProcessAudio(const float inL,
                              const float inR,
                              float&      outL,
                              float&      outR)
        = 0;
    virtual ~SubMenuItem(){};
};

class MenuItem
{
  protected:
    int currentSubMenu = 0;
    int MaxSubMenus    = 0;

  public:
    SubMenuItem** subMenus; //should remove in constructor & initialize
    char          name[8];

    virtual ~MenuItem()
    {
        for(int i = 0; i < MaxSubMenus; i++)
        {
            delete subMenus[i];
        }
        delete subMenus;
    };

    virtual int GetLen() = 0;

    SubMenuItem* GetCurrent() { return subMenus[currentSubMenu]; };

    void Next()
    {
        currentSubMenu++;
        if(currentSubMenu == MaxSubMenus)
        {
            currentSubMenu = 0;
        }
    };
};

class TestKnobs : public SubMenuItem
{
  private:
    char strbuff[128];

  public:
    TestKnobs()
    {
        sprintf(name, "Knobs");
        AudioProcessing     = false;
        TimerbasedProcssing = false;
    };
    ~TestKnobs() override{};

    void ProcessTimeBased(FluidModule& hw) override{};
    void ProcessAudio(const float inL,
                      const float inR,
                      float&      outL,
                      float&      outR) override{};
    void RenderDisplay(FluidModule& hw) override
    {
        sprintf(strbuff, "Knob1: %d", hw.GetAdc(knob1));
        hw.display.SetCursor(0, 20);
        hw.display.WriteString(strbuff, Font_7x10, false);

        sprintf(strbuff, "Knob2: %d", hw.GetAdc(knob2));
        hw.display.SetCursor(0, 28);
        hw.display.WriteString(strbuff, Font_7x10, false);

        sprintf(strbuff, "Knob3: %d", hw.GetAdc(knob3));
        hw.display.SetCursor(0, 36);
        hw.display.WriteString(strbuff, Font_7x10, false);

        sprintf(strbuff, "Knob4: %d", hw.GetAdc(knob4));
        hw.display.SetCursor(0, 44);
        hw.display.WriteString(strbuff, Font_7x10, false);

        sprintf(strbuff, "Knob5: %d", hw.GetAdc(knob5));
        hw.display.SetCursor(0, 52);
        hw.display.WriteString(strbuff, Font_7x10, false);
    };
};

class TestCvsIns : public SubMenuItem
{
  private:
    char strbuff[128];

  public:
    TestCvsIns()
    {
        sprintf(name, "CV Ins");
        AudioProcessing     = false;
        TimerbasedProcssing = false;
    };
    ~TestCvsIns() override{};

    void ProcessTimeBased(FluidModule& hw) override{};
    void ProcessAudio(const float inL,
                      const float inR,
                      float&      outL,
                      float&      outR) override{};

    void RenderDisplay(FluidModule& hw) override
    {
        sprintf(strbuff, "CV In1: %d", hw.GetAdc(cvIn1));
        hw.display.SetCursor(0, 20);
        hw.display.WriteString(strbuff, Font_7x10, false);

        sprintf(strbuff, "CV In2: %d", hw.GetAdc(cvIn2));
        hw.display.SetCursor(0, 28);
        hw.display.WriteString(strbuff, Font_7x10, false);

        sprintf(strbuff, "CV In3: %d", hw.GetAdc(cvIn3));
        hw.display.SetCursor(0, 36);
        hw.display.WriteString(strbuff, Font_7x10, false);

        sprintf(strbuff, "CV In4: %d", hw.GetAdc(cvIn4));
        hw.display.SetCursor(0, 44);
        hw.display.WriteString(strbuff, Font_7x10, false);

        sprintf(strbuff, "CV In5: %d", hw.GetAdc(cvIn5));
        hw.display.SetCursor(0, 52);
        hw.display.WriteString(strbuff, Font_7x10, false);
    };
};

class TestMenu : public MenuItem
{
  public:
    TestMenu()
    {
        sprintf(name, "Test");
        currentSubMenu = 0;
        MaxSubMenus    = 2;
        subMenus       = new SubMenuItem*[2];
        subMenus[0]    = new TestKnobs;
        subMenus[1]    = new TestCvsIns;
    };

    int GetLen() override { return MaxSubMenus; };
};

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
    bool Process(FluidModule& hw)
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

// Sends Gate Signals on CV Outs
class CVOutGate : public SubMenuItem
{
  private:
    char            strbuff[128];
    CVGateGenerator cvClock1;
    CVGateGenerator cvClock2;

  public:
    CVOutGate(float samplerate, uint32_t now, uint32_t trigger_update_period)
    {
        sprintf(name, "Gate");
        AudioProcessing     = false;
        TimerbasedProcssing = true;
        cvClock1.Setup(CV_OUT_1, now, trigger_update_period);
        cvClock2.Setup(CV_OUT_2, now, trigger_update_period);
    };
    ~CVOutGate() override{};

    void ProcessTimeBased(FluidModule& hw) override
    {
        //Update periods
        uint32_t period1 = 1000 * hw.GetAdcFloat(knob1);
        uint32_t period2 = 1000 * hw.GetAdcFloat(knob2);
        uint32_t fine1   = 100 * hw.GetAdcFloat(knob3);
        uint32_t fine2   = 100 * hw.GetAdcFloat(knob4);

        cvClock1.trigger_update_period_ = period1 + fine1;
        cvClock2.trigger_update_period_ = period2 + fine2;

        bool gt1On = cvClock1.Process(hw);
        bool gt2On = cvClock2.Process(hw);

        //update Leds based on gates
        if(gt1On)
        {
            hw.led1.Set(1.0f);
        }
        else
        {
            hw.led1.Set(0.0f);
        }
        hw.led1.Update();

        //update Leds based on gates
        if(gt2On)
        {
            hw.led2.Set(1.0f);
        }
        else
        {
            hw.led2.Set(0.0f);
        }
        hw.led2.Update();
    };

    void ProcessAudio(const float inL,
                      const float inR,
                      float&      outL,
                      float&      outR) override{};

    void RenderDisplay(FluidModule& hw) override
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

class CvOutMenu : public MenuItem
{
  public:
    CvOutMenu(float samplerate, uint32_t now, uint32_t trigger_update_period)
    {
        sprintf(name, "CV O");
        currentSubMenu = 0;
        MaxSubMenus    = 2;
        subMenus       = new SubMenuItem*[2];
        subMenus[0]
            = new CVOutGate(samplerate, now, trigger_update_period); // SAW
        subMenus[1]
            = new TestCvsIns; // Multiple SAWs, Physical Osc, FM Osc, etc.
    };

    int GetLen() override { return MaxSubMenus; };
};

class SimpleOscMenu : public SubMenuItem
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
    SimpleOscMenu(float samplerate)
    {
        sprintf(name, "Smpl");
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

    void ProcessTimeBased(FluidModule& hw) override
    {
        Freq    = hw.GetAdcFloat(knob1) * 2000;
        fine    = (hw.GetAdcFloat(knob2) * 100) - 50;
        FMwidth = hw.GetAdcFloat(knob3) * 100;
        spread  = hw.GetAdcFloat(knob4) * 100;

        //osc1.SetAmp(hardware.GetAdc(cvIn1));
        FM = hw.GetAdcFloat(cvIn2);

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

    void RenderDisplay(FluidModule& hw) override
    {
        Freq    = hw.GetAdcFloat(knob1) * 2000;
        fine    = (hw.GetAdcFloat(knob2) * 100) - 50;
        FMwidth = hw.GetAdcFloat(knob3) * 100;
        spread  = hw.GetAdcFloat(knob4) * 100;
        FM      = hw.GetAdcFloat(cvIn2);

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

class OscMenu : public MenuItem
{
  public:
    OscMenu(float samplerate)
    {
        sprintf(name, "Osc");
        currentSubMenu = 0;
        MaxSubMenus    = 2;
        subMenus       = new SubMenuItem*[2];
        subMenus[0]    = new SimpleOscMenu(samplerate); // SAW
        subMenus[1]
            = new TestCvsIns; // Multiple SAWs, Physical Osc, FM Osc, etc.
    };

    int GetLen() override { return MaxSubMenus; };
};

FluidModule hardware;

int       MainMenuCurrent = 0;
const int MaxMenuCount    = 3;
MenuItem* menu[MaxMenuCount];

class Display
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
        screen_update_last_   = hardware.GetNow();
        screen_update_period_ = 17; //58.8Hz (targeting 60Hz)
    }

    void UpdateDisplay()
    {
        if(hardware.GetNow() - screen_update_last_ > screen_update_period_)
        {
            screen_update_last_ = hardware.GetNow();
            hardware.display.Fill(true);


            // Draw stuff on display based on current Menu:

            //Print Title
            sprintf(strbuff2,
                    "%s > %s",
                    menu[MainMenuCurrent]->name,
                    menu[MainMenuCurrent]->GetCurrent()->name);

            hardware.display.SetCursor(0, 0);
            hardware.display.WriteString(strbuff2, Font_11x18, false);

            //Print menu specif
            menu[MainMenuCurrent]->GetCurrent()->RenderDisplay(hardware);


            hardware.display.Update();
        }
    }

    void DrawScopeBox()
    {
        hardware.display.DrawRect(8, 32, 120, 62, false, true);
        hardware.display.DrawLine(8, 47, 13, 47, true);
        hardware.display.DrawLine(117, 47, 120, 47, true);

        //hardware.display.DrawPixel(20, 43, true);
    }
};


Display display;

void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
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
    hardware.Init();
    hardware.SetAudioBlockSize(4);

    //How many samples we'll output per second
    float samplerate = hardware.AudioSampleRate();

    //Setup Menus
    menu[0] = new TestMenu();
    menu[1]
        = new CvOutMenu(samplerate, hardware.GetNow(), 500); //2Hz for default

    menu[2] = new OscMenu(samplerate);

    display.Setup();

    //Start the adc for CV ins & Knobs
    hardware.StartAdc();

    //Start calling the audio callback
    hardware.StartAudio(AudioCallback);

    //Start DAC for CVOuts
    hardware.StartDac(nullptr);
}

void HandleButtonsForMenu()
{
    hardware.button1.Debounce();

    if(hardware.button1.FallingEdge())
    { //iterate menu items
        MainMenuCurrent++;
        if(MainMenuCurrent == MaxMenuCount)
        {
            MainMenuCurrent = 0;
        }
    }

    hardware.button2.Debounce();

    if(hardware.button2.FallingEdge())
    { //iterate submenu items
        menu[MainMenuCurrent]->Next();
    }
}

int main(void)
{
    init();
    while(1)
    {
        HandleButtonsForMenu();

        for(int i = 0; i < MaxMenuCount; i++)
        {
            int length = menu[i]->GetLen();
            for(int j = 0; j < length; j++)
            {
                if(menu[i]->subMenus[j]->TimerbasedProcssing)
                {
                    menu[i]->subMenus[j]->ProcessTimeBased(hardware);
                }
            }
        }

        display.UpdateDisplay();
    }
}
