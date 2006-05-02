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
  int JumpRec;
  bool wasdvd; // needed for dvdarchive-patch functionality
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
  int jumprec;
 protected:
  virtual void Store();
 public:
  myMenuSetup();
};
