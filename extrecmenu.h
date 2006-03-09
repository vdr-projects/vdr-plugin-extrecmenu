#include <vdr/plugin.h>
#include <vdr/menu.h>
#include <vdr/interface.h>
#include <vdr/status.h>
#include <vdr/skins.h>
#include <vdr/dvbplayer.h>
#include <vdr/cutter.h>
#include <vdr/videodir.h>
#include "i18n.h"

#define MODETIMEOUT 3 // seconds

static const char *VERSION        = "0.1";
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
  char *title;
  char *name;
  const char *filename;
 public:
  myMenuRecordingsItem(cRecording *Recording,int Level);
  ~myMenuRecordingsItem();
  const char *FileName(){return filename;}
  const char *Name(){return name;}
  bool IsDirectory(){return name!=NULL;}
};

// --- myMenuRecordings -------------------------------------------------------
class myMenuRecordings:public cOsdMenu
{
 private:
  bool edit;
  int level,helpkeys;
  char *base;
  bool Open();
  void SetHelpKeys();
  cRecording *GetRecording(myMenuRecordingsItem *Item);
  eOSState Play();
  eOSState Rewind();
  eOSState Delete();
  eOSState Rename();
  eOSState MoveRec();
 public:
  myMenuRecordings(const char *Base=NULL,int Level=0);
  ~myMenuRecordings();
  void Set();
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

// --- myReplayControls -------------------------------------------------------
class myReplayControl : public cDvbPlayerControl {
private:
  cSkinDisplayReplay *displayReplay;
  cMarks marks;
  bool visible, modeOnly, shown, displayFrames;
  int lastCurrent, lastTotal;
  bool lastPlay, lastForward;
  int lastSpeed;
  time_t timeoutShow;
  bool timeSearchActive, timeSearchHide;
  int timeSearchTime, timeSearchPos;
  void TimeSearchDisplay(void);
  void TimeSearchProcess(eKeys Key);
  void TimeSearch(void);
  void ShowTimed(int Seconds = 0);
  static char *fileName;
  static char *title;
  void ShowMode(void);
  bool ShowProgress(bool Initial);
  void MarkToggle(void);
  void MarkJump(bool Forward);
  void MarkMove(bool Forward);
  void EditCut(void);
  void EditTest(void);
public:
  myReplayControl(void);
  virtual ~myReplayControl();
  virtual cOsdObject *GetInfo(void);
  virtual eOSState ProcessKey(eKeys Key);
  virtual void Show(void);
  virtual void Hide(void);
  bool Visible(void) { return visible; }
  static void SetRecording(const char *FileName, const char *Title);
  static const char *LastReplayed(void);
  static void ClearLastReplayed(const char *FileName);
  };
