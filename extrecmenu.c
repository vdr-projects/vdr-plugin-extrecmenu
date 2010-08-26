/*
 * See the README file for copyright information and how to reach the author.
 */

#include <string>
#include <vdr/plugin.h>
#include "mymenusetup.h"
#include "mymenurecordings.h"
#include "tools.h"

#if defined(APIVERSNUM)
#  if APIVERSNUM < 10600
#    error "VDR-1.6.0 API version or greater is required!"
#  else
#    if APIVERSNUM >= 10700 && APIVERSNUM < 10714
#      error "VDR-1.7.14 API version or greater is required!"
#    endif
#  endif
#endif

using namespace std;

static const char *VERSION        = "1.2.1pre";
static const char *DESCRIPTION    = tr("Extended recordings menu");
static const char *MAINMENUENTRY  = "ExtRecMenu";

// --- cPluginExtrecmenu ------------------------------------------------------
class cPluginExtrecmenu:public cPlugin
{
  private:
  public:
    cPluginExtrecmenu(void);
    virtual ~cPluginExtrecmenu();
    virtual const char *Version(void){return VERSION;}
    virtual const char *Description(void){return tr(DESCRIPTION);}
    virtual const char *CommandLineHelp(void);
    virtual bool ProcessArgs(int argc,char *argv[]);
    virtual bool Initialize(void);
    virtual bool Start(void);
    virtual void Stop(void);
    virtual void Housekeeping(void);
    virtual cString Active(void);
    virtual const char *MainMenuEntry(void){return mysetup.HideMainMenuEntry?NULL:MAINMENUENTRY;}
    virtual cOsdObject *MainMenuAction(void);
    virtual cMenuSetupPage *SetupMenu(void);
    virtual bool SetupParse(const char *_Name,const char *Value);
    virtual bool Service(const char *Id,void *Data = NULL);
    virtual const char **SVDRPHelpPages(void);
    virtual cString SVDRPCommand(const char *Command,const char *Option,int &ReplyCode);
};

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

bool cPluginExtrecmenu::ProcessArgs(int /* argc */,char ** /* argv */)
{
  return true;
}

bool cPluginExtrecmenu::Initialize(void)
{
  return true;
}

bool cPluginExtrecmenu::Start(void)
{
  mySortList=new SortList;
  mySortList->ReadConfigFile();

  Icons::InitCharSet();

  MoveCutterThread=new WorkerThread();

  RecordingDirCommands.Load(AddDirectory(cPlugin::ConfigDirectory(PLUGIN_NAME_I18N), "dircmds.conf"));

  return true;
}

void cPluginExtrecmenu::Stop(void)
{
  delete mySortList;
  delete MoveCutterThread;
}

void cPluginExtrecmenu::Housekeeping(void)
{
}

cString cPluginExtrecmenu::Active(void)
{
  return MoveCutterThread->Working();
}

cOsdObject *cPluginExtrecmenu::MainMenuAction(void)
{
  return new myMenuRecordings();
}

cMenuSetupPage *cPluginExtrecmenu::SetupMenu(void)
{
  return new myMenuSetup();
}

bool cPluginExtrecmenu::SetupParse(const char *_Name,const char *Value)
{
  if(!strcasecmp(_Name,"IsOrgRecMenu"))
    return (mysetup.ReplaceOrgRecMenu==false); // vdr-replace patch

  if(!strcasecmp(_Name,"ShowRecDate"))
    mysetup.ShowRecDate=atoi(Value);
  else if(!strcasecmp(_Name,"ShowRecTime"))
    mysetup.ShowRecTime=atoi(Value);
  else if(!strcasecmp(_Name,"ShowRecLength"))
    mysetup.ShowRecLength=atoi(Value);
  else if(!strcasecmp(_Name,"ShowRecRating"))
    mysetup.ShowRecRating=atoi(Value);
  else if(!strcasecmp(_Name,"HideMainMenuEntry"))
    mysetup.HideMainMenuEntry=atoi(Value);
  else if(!strcasecmp(_Name,"ReplaceOrgRecMenu"))
    mysetup.ReplaceOrgRecMenu=atoi(Value);
  else if(!strcasecmp(_Name,"PatchNew"))
    mysetup.PatchNew=atoi(Value);
  else if(!strcasecmp(_Name,"ShowNewRecs"))
    mysetup.ShowNewRecs=atoi(Value);
  else if(!strcasecmp(_Name,"DescendSorting"))
    mysetup.DescendSorting=atoi(Value);
  else if(!strcasecmp(_Name,"GoLastReplayed"))
    mysetup.GoLastReplayed=atoi(Value);
  else if(!strcasecmp(_Name,"ReturnToPlugin"))
    mysetup.ReturnToPlugin=atoi(Value);
  else if(!strcasecmp(_Name,"LimitBandwidth"))
    mysetup.LimitBandwidth=atoi(Value);
  else if(!strcasecmp(_Name,"UseVDRsRecInfoMenu"))
    mysetup.UseVDRsRecInfoMenu=atoi(Value);
  else if(!strcasecmp(_Name,"PatchFont"))
    mysetup.PatchFont=atoi(Value);
  else if(!strcasecmp(_Name,"FileSystemFreeMB"))
    mysetup.FileSystemFreeMB=atoi(Value);
  else if(!strcasecmp(_Name,"UseCutterQueue"))
    mysetup.UseCutterQueue=atoi(Value);
  else
    return false;
  return true;
}

bool cPluginExtrecmenu::Service(const char *Id,void *Data)
{
  if(!Data)
    return true;

  cOsdMenu **menu=(cOsdMenu**)Data;
  if(mysetup.ReplaceOrgRecMenu && strcmp(Id,"MainMenuHooksPatch-v1.0::osRecordings")==0)
  {
    if(menu)
      *menu=(cOsdMenu*)MainMenuAction();

    return true;
  }
  return false;
}

const char **cPluginExtrecmenu::SVDRPHelpPages(void)
{
 return NULL;
}

cString cPluginExtrecmenu::SVDRPCommand(const char * /* Command */,const char * /* Option */,int & /* ReplyCode */)
{
 return NULL;
}

VDRPLUGINCREATOR(cPluginExtrecmenu); // Don't touch this!

