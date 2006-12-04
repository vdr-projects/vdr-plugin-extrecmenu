#include <vdr/menu.h>

class mySetup
{
 public:
  mySetup();
  int ShowRecDate;
  int ShowRecTime;
  int ShowRecLength;
  int HideMainMenuEntry;
  int ReplaceOrgRecMenu;
  int PatchNew;
  int ShowDvdNr;
  int ShowNewRecs;
  int DescendSorting;
};

extern mySetup mysetup;

class myMenuSetup:public cMenuSetupPage
{
 private:
  const char *sortingtypetexts[2];
  int showrecdate;
  int showrectime;
  int showreclength;
  int hidemainmenuentry;
  int replaceorgrecmenu;
  int patchnew;
  int showdvdnr;
  int shownewrecs;
  int descendsorting;
 protected:
  virtual void Store();
 public:
  myMenuSetup();
};
