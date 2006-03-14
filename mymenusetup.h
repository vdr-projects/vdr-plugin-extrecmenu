#include <vdr/menu.h>

class mySetup
{
 public:
  int ShowRecDate;
  int ShowRecTime;
  int ShowRecLength;
  int HideMainMenuEntry;
  int ReplaceOrgRecMenu;
};

extern mySetup mysetup;

class myMenuSetup:public cMenuSetupPage
{
 private:
  int showrecdate;
  int showrectime;
  int showreclength;
  int hidemainmenuentry;
  int replaceorgrecmenu;
 protected:
  virtual void Store();
 public:
  myMenuSetup();
};
