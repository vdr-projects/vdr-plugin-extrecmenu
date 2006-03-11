#include "myreplaycontrol.h"

myMenuRecordingInfo::myMenuRecordingInfo(const cRecording *Recording,bool WithButtons):cOsdMenu(tr("Recording info"))
{
 recording=Recording;
 withButtons=WithButtons;
 if(withButtons)
  SetHelp(tr("Button$Play"),tr("Button$Rewind"));
}

void myMenuRecordingInfo::Display(void)
{
 cOsdMenu::Display();
 DisplayMenu()->SetRecording(recording);
 cStatus::MsgOsdTextItem(recording->Info()->Description());
}

eOSState myMenuRecordingInfo::Play()
{
 if(recording)
 {
  myReplayControl::SetRecording(recording->FileName(),recording->Title());
  cControl::Shutdown(); // stop running playbacks
  cControl::Launch(new myReplayControl); // start playback
  return osEnd; // close plugin
 }
 return osContinue;
}

eOSState myMenuRecordingInfo::Rewind()
{
 if(recording)
 {
  cDevice::PrimaryDevice()->StopReplay();
  cResumeFile ResumeFile(recording->FileName());
  ResumeFile.Delete();
  return Play();
 }
 return osContinue;
}

eOSState myMenuRecordingInfo::ProcessKey(eKeys Key)
{
 switch (Key)
 {
  case kUp|k_Repeat:
  case kUp:
  case kDown|k_Repeat:
  case kDown:
  case kLeft|k_Repeat:
  case kLeft:
  case kRight|k_Repeat:
  case kRight: DisplayMenu()->Scroll(NORMALKEY(Key)==kUp||NORMALKEY(Key)==kLeft,NORMALKEY(Key)==kLeft||NORMALKEY(Key)==kRight);
               cStatus::MsgOsdTextItem(NULL,NORMALKEY(Key)==kUp);
               return osContinue;
  default: break;
 }

 eOSState state=cOsdMenu::ProcessKey(Key);

 if(state==osUnknown)
 {
  switch (Key)
  {
   case kRed: return Play();
   case kGreen: return Rewind();
   case kOk: return osBack;
   default: break;
  }
 }
 return state;
}
