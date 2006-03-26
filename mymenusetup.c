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
 
 Add(new cMenuEditBoolItem(tr("Show recording date"),&showrecdate));
 Add(new cMenuEditBoolItem(tr("Show recording time"),&showrectime));
 Add(new cMenuEditBoolItem(tr("Show recording length"),&showreclength));
 Add(new cMenuEditBoolItem(tr("Hide main menu entry"),&hidemainmenuentry));
 Add(new cMenuEditBoolItem(tr("Replace original recordings menu"),&replaceorgrecmenu));
 Add(new cMenuEditBoolItem(tr("Show alternative new marker"),&patchnew));
 Add(new cMenuEditBoolItem(tr("Show alternative dvd marker"),&patchdvd));
 Add(new cMenuEditBoolItem(tr("Show dvd number"),&showdvdnr));
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
}
