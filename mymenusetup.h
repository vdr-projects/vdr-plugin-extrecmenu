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
  int SortRecords;
};

extern mySetup mysetup;

class myMenuSetup:public cMenuSetupPage
{
 private:
  const char *sortrecordstext[4];
  int showrecdate;
  int showrectime;
  int showreclength;
  int hidemainmenuentry;
  int replaceorgrecmenu;
  int patchnew;
  int showdvdnr;
  int shownewrecs;
  int sortrecords;
 protected:
  virtual void Store();
 public:
  myMenuSetup();
};
