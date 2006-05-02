/*
 * See the README file for copyright information and how to reach the author.
 */

#include "mymenusetup.h"
#include "mymenurecordings.h"
#include "extrecmenu.h"
#include "i18n.h"

cPluginExtrecmenu::cPluginExtrecmenu(void)
{
}

cPluginExtrecmenu::~cPluginExtrecmenu()
{
}

const char *cPluginExtrecmenu::CommandLineHelp(void)
{
 return NULL;
}

bool cPluginExtrecmenu::ProcessArgs(int argc, char *argv[])
{
 return true;
}

bool cPluginExtrecmenu::Initialize(void)
{
 RegisterI18n(Phrases);
 
 return true;
}

bool cPluginExtrecmenu::Start(void)
{
 return true;
}

void cPluginExtrecmenu::Stop(void)
{
}

void cPluginExtrecmenu::Housekeeping(void)
{
}

cOsdObject *cPluginExtrecmenu::MainMenuAction(void)
{
 return new myMenuRecordings();
}

cMenuSetupPage *cPluginExtrecmenu::SetupMenu(void)
{
 return new myMenuSetup();
}

bool cPluginExtrecmenu::SetupParse(const char *Name, const char *Value)
{
 if(!strcasecmp(Name,"IsOrgRecMenu"))
  return (mysetup.ReplaceOrgRecMenu==false); // vdr-replace patch

 if(!strcasecmp(Name,"ShowRecDate"))
  mysetup.ShowRecDate=atoi(Value);
 else
  if(!strcasecmp(Name,"ShowRecTime"))
   mysetup.ShowRecTime=atoi(Value);
  else
   if(!strcasecmp(Name,"ShowRecLength"))
    mysetup.ShowRecLength=atoi(Value);
   else
    if(!strcasecmp(Name,"HideMainMenuEntry"))
     mysetup.HideMainMenuEntry=atoi(Value);
    else
     if(!strcasecmp(Name,"ReplaceOrgRecMenu"))
      mysetup.ReplaceOrgRecMenu=atoi(Value);
     else
      if(!strcasecmp(Name,"PatchNew"))
       mysetup.PatchNew=atoi(Value);
      else
       if(!strcasecmp(Name,"ShowDvdNr"))
        mysetup.ShowDvdNr=atoi(Value);
       else
        if(!strcasecmp(Name,"ShowNewRecs"))
         mysetup.ShowNewRecs=atoi(Value);
        else
         if(!strcasecmp(Name,"SortRecords"))
          mysetup.SortRecords=atoi(Value);
         else
          if(!strcasecmp(Name,"JumpRec"))
           mysetup.JumpRec=atoi(Value);
          else
           return false;
 return true;
}

bool cPluginExtrecmenu::Service(const char *Id, void *Data)
{
 return false;
}

const char **cPluginExtrecmenu::SVDRPHelpPages(void)
{
 return NULL;
}

cString cPluginExtrecmenu::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
 return NULL;
}

VDRPLUGINCREATOR(cPluginExtrecmenu); // Don't touch this!
