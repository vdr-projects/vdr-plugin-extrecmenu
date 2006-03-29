/*
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "mymenusetup.h"
#include "mymenurecordings.h"
#include "extrecmenu.h"
#include "i18n.h"

cPluginExtrecmenu::cPluginExtrecmenu(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginExtrecmenu::~cPluginExtrecmenu()
{
  // Clean up after yourself!
}

const char *cPluginExtrecmenu::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginExtrecmenu::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginExtrecmenu::Initialize(void)
{
 RegisterI18n(Phrases);
 
 mysetup.wasdvd=false;

 return true;
}

bool cPluginExtrecmenu::Start(void)
{
  // Start any background activities the plugin shall perform.
  return true;
}

void cPluginExtrecmenu::Stop(void)
{
  // Stop any background activities the plugin shall perform.
}

void cPluginExtrecmenu::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

cOsdObject *cPluginExtrecmenu::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  return new myMenuRecordings();
}

cMenuSetupPage *cPluginExtrecmenu::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
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
       if(!strcasecmp(Name,"PatchDvd"))
        mysetup.PatchDvd=atoi(Value);
       else
        if(!strcasecmp(Name,"ShowDvdNr"))
         mysetup.ShowDvdNr=atoi(Value);
        else
         if(!strcasecmp(Name,"ShowNewRecs"))
          mysetup.ShowNewRecs=atoi(Value);
         else
          return false;
 return true;
}

bool cPluginExtrecmenu::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  return false;
}

const char **cPluginExtrecmenu::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  return NULL;
}

cString cPluginExtrecmenu::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  return NULL;
}

VDRPLUGINCREATOR(cPluginExtrecmenu); // Don't touch this!
