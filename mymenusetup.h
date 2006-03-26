#include <vdr/menu.h>

class mySetup
{
 public:
  int ShowRecDate;
  int ShowRecTime;
  int ShowRecLength;
  int HideMainMenuEntry;
  int ReplaceOrgRecMenu;
  int PatchNew;
  int PatchDvd;
  int ShowDvdNr;
  bool wasdvd; // needed for dvdarchive-patch functionality
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
  int patchnew;
  int patchdvd;
  int showdvdnr;
 protected:
  virtual void Store();
 public:
  myMenuSetup();
};
