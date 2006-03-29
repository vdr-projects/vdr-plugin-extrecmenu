#include "mymenusetup.h"

mySetup mysetup;

myMenuSetup::myMenuSetup()
{
 hidemainmenuentry=mysetup.HideMainMenuEntry;
 patchnew=mysetup.PatchNew;
 patchdvd=mysetup.PatchDvd;
 replaceorgrecmenu=mysetup.ReplaceOrgRecMenu;
 showrecdate=mysetup.ShowRecDate;
 showrectime=mysetup.ShowRecTime;
 showreclength=mysetup.ShowRecLength;
 showdvdnr=mysetup.ShowDvdNr;
 shownewrecs=mysetup.ShowNewRecs;
 
 Add(new cMenuEditBoolItem(tr("Show recording date"),&showrecdate));
 Add(new cMenuEditBoolItem(tr("Show recording time"),&showrectime));
 Add(new cMenuEditBoolItem(tr("Show recording length"),&showreclength));
 Add(new cMenuEditBoolItem(tr("Show \"new recordings column\""),&shownewrecs));
 Add(new cMenuEditBoolItem(tr("Show alternative to new marker"),&patchnew));
 Add(new cMenuEditBoolItem(tr("Show alternative dvd marker"),&patchdvd));
 Add(new cMenuEditBoolItem(tr("Show dvd number"),&showdvdnr));
 Add(new cMenuEditBoolItem(tr("Hide main menu entry"),&hidemainmenuentry));
 Add(new cMenuEditBoolItem(tr("Replace original recordings menu"),&replaceorgrecmenu));
}

void myMenuSetup::Store()
{
 SetupStore("HideMainMenuEntry",mysetup.HideMainMenuEntry=hidemainmenuentry);
 SetupStore("PatchNew",mysetup.PatchNew=patchnew);
 SetupStore("PatchDvd",mysetup.PatchDvd=patchdvd);
 SetupStore("ShowDvdNr",mysetup.ShowDvdNr=showdvdnr);
 SetupStore("ReplaceOrgRecMenu",mysetup.ReplaceOrgRecMenu=replaceorgrecmenu);
 SetupStore("ShowRecDate",mysetup.ShowRecDate=showrecdate);
 SetupStore("ShowRecTime",mysetup.ShowRecTime=showrectime);
 SetupStore("ShowRecLength",mysetup.ShowRecLength=showreclength);
 SetupStore("ShowNewRecs",mysetup.ShowNewRecs=shownewrecs);
}
