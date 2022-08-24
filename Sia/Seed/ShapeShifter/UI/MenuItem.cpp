
#include <stdio.h>
#include <vector>
#include "../applet.cpp"

using namespace std;


class MenuItem
{
  protected:
    unsigned int currentSubMenu = 0;

  public:
    vector<Applet*> subMenus = {};
    char            name[8];

    MenuItem(const char* menu_name) { sprintf(name, menu_name); };

    ~MenuItem()
    {
        for(unsigned int i = 0; i < subMenus.size(); i++)
        {
            delete subMenus[i];
        }
    };

    int GetLen() { return subMenus.size(); };

    Applet* GetCurrent() { return subMenus[currentSubMenu]; };

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
