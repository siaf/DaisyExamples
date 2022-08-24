
#include <stdio.h>
#include <vector>
#include "MenuItem.cpp"
using namespace std;

class MainMenuItem
{
  protected:
    unsigned int currentSubMenu = 0;

  public:
    vector<MenuItem*> subMenus = {};
    char              name[8];

    MainMenuItem(const char* menu_name) { sprintf(name, menu_name); };

    ~MainMenuItem()
    {
        for(unsigned int i = 0; i < subMenus.size(); i++)
        {
            delete subMenus[i];
        }
    };

    int GetLen() { return subMenus.size(); };

    MenuItem* GetCurrent() { return subMenus[currentSubMenu]; };

    void Next()
    {
        if(currentSubMenu == subMenus.size() - 1)
        {
            currentSubMenu = 0;
        }
        else
        {
            currentSubMenu++;
        }
    };

    void Previous()
    {
        if(currentSubMenu == 0)
        {
            currentSubMenu = subMenus.size() - 1;
        }
        else
        {
            currentSubMenu--;
        }
    };
};
