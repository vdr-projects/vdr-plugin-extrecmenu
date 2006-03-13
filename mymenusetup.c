#include "mymenusetup.h"

mySetup mysetup;

myMenuSetup::myMenuSetup()
{
 hidemainmenuentry=mysetup.HideMainMenuEntry;
 showrecdate=mysetup.ShowRecDate;
 showrectime=mysetup.ShowRecTime;
 showreclength=mysetup.ShowRecLength;
 
 Add(new cMenuEditBoolItem(tr("Hide main menu entry"),&hidemainmenuentry));
 Add(new cMenuEditBoolItem(tr("Show recording date"),&showrecdate));
 Add(new cMenuEditBoolItem(tr("Show recording time"),&showrectime));
 Add(new cMenuEditBoolItem(tr("Show recording length"),&showreclength));
}

void myMenuSetup::Store()
{
 SetupStore("HideMainMenuEntry",mysetup.HideMainMenuEntry=hidemainmenuentry);
 SetupStore("ShowRecDate",mysetup.ShowRecDate=showrecdate);
 SetupStore("ShowRecTime",mysetup.ShowRecTime=showrectime);
 SetupStore("ShowRecLength",mysetup.ShowRecLength=showreclength);
}
