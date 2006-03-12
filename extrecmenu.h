#include <vdr/plugin.h>
#include <vdr/menu.h>
#include <vdr/skins.h>
#include <vdr/videodir.h>
#include "i18n.h"

static const char *VERSION        = "0.3";
static const char *DESCRIPTION    = "Extended recordings menu";
static const char *MAINMENUENTRY  = "ExtRecMenu";

extern bool clearall; // needed for myMenuMoveRecording

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
  virtual const char *MainMenuEntry(void){return MAINMENUENTRY;}
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name,const char *Value);
  virtual bool Service(const char *Id,void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command,const char *Option,int &ReplyCode);
};

// --- myMenuRecordingsItem ---------------------------------------------------
class myMenuRecordingsItem:public cOsdItem
{
 private:
  int level,isdirectory;
  int totalentries,newentries;
  char *title;
  char *name;
  const char *filename;
 public:
  myMenuRecordingsItem(cRecording *Recording,int Level);
  ~myMenuRecordingsItem();
  const char *FileName(){return filename;}
  const char *Name(){return name;}
  bool IsDirectory(){return name!=NULL;}
  void IncrementCounter(bool IsNew);
};

// --- myMenuRecordings -------------------------------------------------------
class myMenuRecordings:public cOsdMenu
{
 private:
  bool edit;
  int level,helpkeys;
  int recordingsstate;
  char *base;
  bool Open();
  void SetHelpKeys();
  cRecording *GetRecording(myMenuRecordingsItem *Item);
  eOSState Play();
  eOSState Rewind();
  eOSState Delete();
  eOSState Rename();
  eOSState MoveRec();
  eOSState Info();
 public:
  myMenuRecordings(const char *Base=NULL,int Level=0);
  ~myMenuRecordings();
  void Set(bool Refresh=false);
  virtual eOSState ProcessKey(eKeys Key);
};

// --- myMenuRenameRecording --------------------------------------------------
class myMenuRenameRecording:public cOsdMenu
{
 private:
  char name[MaxFileName];
  char path[MaxFileName];
  cRecording *recording;
  myMenuRecordings *menurecordings;
 public:
  myMenuRenameRecording(cRecording *Recording,myMenuRecordings *MenuRecordings);
  virtual eOSState ProcessKey(eKeys Key);
};

// --- myMenuMoveRecording ----------------------------------------------------
class myMenuMoveRecording:public cOsdMenu
{
 private:
  int level;
  char *base;
  cRecording *recording;
  myMenuRecordings *menurecordings;
  void Set();
  eOSState Open();
  eOSState MoveRec();
  eOSState Create();
 public:
  myMenuMoveRecording(cRecording *Recording,myMenuRecordings *MenuRecordings,const char *Base=NULL,int Level=0);
  myMenuMoveRecording::~myMenuMoveRecording();
  virtual eOSState ProcessKey(eKeys Key);
};
