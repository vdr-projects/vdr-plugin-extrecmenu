#include "mydvbplayer.h"

#define MODETIMEOUT 3 // seconds

// --- myReplayControls -------------------------------------------------------
class myReplayControl : public myDvbPlayerControl {
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

// --- myMenuRecordingInfo ----------------------------------------------------
class myMenuRecordingInfo:public cOsdMenu
{
 private:
  const cRecording *recording;
  bool withButtons;
  eOSState Play();
  eOSState Rewind();
 public:
  myMenuRecordingInfo(const cRecording *Recording,bool WithButtons = false);
  virtual void Display(void);
  virtual eOSState ProcessKey(eKeys Key);
};
