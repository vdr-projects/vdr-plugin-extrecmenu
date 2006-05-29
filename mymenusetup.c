/*
 * See the README file for copyright information and how to reach the author.
 */

#include <vdr/menu.h>
#include "mymenusetup.h"

mySetup::mySetup()
{
 mysetup.HideMainMenuEntry=0;
 mysetup.PatchNew=0;
 mysetup.ShowDvdNr=0;
 mysetup.ReplaceOrgRecMenu=0;
 mysetup.ShowRecDate=1;
 mysetup.ShowRecTime=1;
 mysetup.ShowRecLength=0;
 mysetup.ShowNewRecs=1;
 mysetup.SortRecords=0;
}

mySetup mysetup;

myMenuSetup::myMenuSetup()
{
 sortrecordstext[0]=tr("alphabet for main-, flexible for subdirectories");
 sortrecordstext[1]=tr("date for main-, flexible for subdirectories");
 sortrecordstext[2]=tr("alphabet for all directories");
 sortrecordstext[3]=tr("date for all directories");

 hidemainmenuentry=mysetup.HideMainMenuEntry;
 patchnew=mysetup.PatchNew;
 replaceorgrecmenu=mysetup.ReplaceOrgRecMenu;
 showrecdate=mysetup.ShowRecDate;
 showrectime=mysetup.ShowRecTime;
 showreclength=mysetup.ShowRecLength;
 showdvdnr=mysetup.ShowDvdNr;
 shownewrecs=mysetup.ShowNewRecs;
 sortrecords=mysetup.SortRecords;
 
 Add(new cMenuEditBoolItem(tr("Hide main menu entry"),&hidemainmenuentry));
 Add(new cMenuEditBoolItem(tr("Replace original recordings menu"),&replaceorgrecmenu));
 Add(new cMenuEditBoolItem(tr("Show recording date"),&showrecdate));
 Add(new cMenuEditBoolItem(tr("Show recording time"),&showrectime));
 Add(new cMenuEditBoolItem(tr("Show recording length"),&showreclength));
 Add(new cMenuEditBoolItem(tr("Show \"new recordings column\""),&shownewrecs));
 Add(new cMenuEditBoolItem(tr("Show alternative to new marker"),&patchnew));
 Add(new cMenuEditBoolItem(tr("Show dvd id"),&showdvdnr));
 Add(new cMenuEditStraItem(tr("Sort recordings by"),&sortrecords,4,sortrecordstext));
}

void myMenuSetup::Store()
{
 SetupStore("HideMainMenuEntry",mysetup.HideMainMenuEntry=hidemainmenuentry);
 SetupStore("PatchNew",mysetup.PatchNew=patchnew);
 SetupStore("ShowDvdNr",mysetup.ShowDvdNr=showdvdnr);
 SetupStore("ReplaceOrgRecMenu",mysetup.ReplaceOrgRecMenu=replaceorgrecmenu);
 SetupStore("ShowRecDate",mysetup.ShowRecDate=showrecdate);
 SetupStore("ShowRecTime",mysetup.ShowRecTime=showrectime);
 SetupStore("ShowRecLength",mysetup.ShowRecLength=showreclength);
 SetupStore("ShowNewRecs",mysetup.ShowNewRecs=shownewrecs);
 SetupStore("SortRecords",mysetup.SortRecords=sortrecords);
}
