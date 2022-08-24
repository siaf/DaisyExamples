#include "ShapeShifterHW.h"
using namespace fluid;
class Applet
{
  public:
    char name[8];
    bool AudioProcessing     = false;
    bool TimerbasedProcssing = false;

    void virtual RenderDisplay(ShapeShifterHW& hw)    = 0;
    void virtual ProcessTimeBased(ShapeShifterHW& hw) = 0;
    void virtual ProcessAudio(const float inL,
                              const float inR,
                              float&      outL,
                              float&      outR)
        = 0;
    virtual ~Applet(){};
};
