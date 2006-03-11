#include "mymenurecordings.h"

// --- myMenuRecordingsItem ---------------------------------------------------
myMenuRecordingsItem::myMenuRecordingsItem(cRecording *Recording,int Level)
{
 char isnew=Recording->IsNew()?'*':' ';
 name=NULL;
 filename=Recording->FileName();

 // get the level of this recording
 level=0;
 const char *s=Recording->Name();
 while(*++s)
 {
  if(*s=='~')
   level++;
 }

 // create the title of this item
 if(Level<level)
 {
  s=Recording->Name();
  while(Level)
  {
   s=strchr(Recording->Name(),'~')+1;
   Level--;
  }
  title=strdup(s);
  char *p=strchr(title,'~');
  if(p)
   *p=0;
  name=strdup(title);
 }
 else
  if(Level==level)
  {
   s=strrchr(Recording->Name(),'~');
   
   struct tm tm_tmp;
   localtime_r(&Recording->start,&tm_tmp);
   asprintf(&title,"%02d.%02d.%02d\t%02d:%02d%c\t%s",
            tm_tmp.tm_mday,
            tm_tmp.tm_mon+1,
            tm_tmp.tm_year%100,
            tm_tmp.tm_hour,
            tm_tmp.tm_min,
            isnew,
            s?s+1:Recording->Name());
  }
  else 
   if(Level>level)
   {
    title="";
   }

 SetText(title);
}

myMenuRecordingsItem::~myMenuRecordingsItem()
{
 free(title);
 free(name);
}

// --- myMenuRecordings -------------------------------------------------------
myMenuRecordings::myMenuRecordings(const char *Base,int Level):cOsdMenu(Base?Base:tr(DESCRIPTION),8,6)
{
 edit=false;
 level=Level;
 helpkeys=-1;
 base=Base?strdup(Base):NULL;
 
 Display();
 Set();
 SetHelpKeys();
}

myMenuRecordings::~myMenuRecordings()
{
 free(base);
}

void myMenuRecordings::Set()
{
 char *lastitemtext=NULL;

 cThreadLock RecordingsLock(&Recordings);
 Clear();
 Recordings.Sort();
 
 // add first the directories
 for(cRecording *recording=Recordings.First();recording;recording=Recordings.Next(recording))
 {
  if(!base||(strstr(recording->Name(),base)==recording->Name()&&recording->Name()[strlen(base)]=='~'))
  {
   myMenuRecordingsItem *item=new myMenuRecordingsItem(recording,level);
   if(item->IsDirectory()&&*item->Text()&&(!lastitemtext||strcmp(lastitemtext,item->Text())))
   {
    Add(item);
    free(lastitemtext);
    lastitemtext=strdup(item->Text());
   }
   else
    delete item;
  }
 }
 // and now the recordings
 for(cRecording *recording=Recordings.First();recording;recording=Recordings.Next(recording))
 {
  if(!base||(strstr(recording->Name(),base)==recording->Name()&&recording->Name()[strlen(base)]=='~'))
  {
   myMenuRecordingsItem *item=new myMenuRecordingsItem(recording,level);
   if(!item->IsDirectory()&&*item->Text()&&(!lastitemtext||strcmp(lastitemtext,item->Text())))
   {
    Add(item);
    free(lastitemtext);
    lastitemtext=strdup(item->Text());
   }
   else
    delete item;
  }
 }
 free(lastitemtext);
 Display();
}

void myMenuRecordings::SetHelpKeys()
{
 if(!HasSubMenu())
 {
  myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
  int newhelpkeys=0;
  if(item)
  {
   if(item->IsDirectory())
    newhelpkeys=1;
   else
   {
    newhelpkeys=2;
   }
   if(newhelpkeys!=helpkeys)
   {
    switch(newhelpkeys)
    {
     case 0: SetHelp(NULL);break;
     case 1: SetHelp(tr("Button$Open"));break;
     case 2: 
     case 3: SetHelp(tr("Button$Play"),tr("Button$Rewind"),tr("Button$Edit"),tr("Button$Info"));break;
    }
   }
   helpkeys=newhelpkeys;
  }
 }
}

cRecording *myMenuRecordings::GetRecording(myMenuRecordingsItem *Item)
{
 cRecording *recording=Recordings.GetByName(Item->FileName());
 if(!recording)
  Skins.Message(mtError,tr("Error while accessing recording!"));
 return recording;
}

bool myMenuRecordings::Open()
{
 myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
 if(item&&item->IsDirectory())
 {
  const char *t=item->Name();
  char *buffer=NULL;
  if(base)
  {
   asprintf(&buffer,"%s~%s",base,t);
   t=buffer;
  }
  AddSubMenu(new myMenuRecordings(t,level+1));
  free(buffer);
  return true;
 }
 return false;
}

eOSState myMenuRecordings::Play()
{
 myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
 if(item)
 {
  if(item->IsDirectory())
   Open();
  else
  {
   cRecording *recording=GetRecording(item);
   if(recording)
   {
    myReplayControl::SetRecording(recording->FileName(),recording->Title());
    cControl::Shutdown(); // stop running playbacks
    cControl::Launch(new myReplayControl); // start playback
    return osEnd; // close plugin
   }
  }
 }
 return osContinue;
}

eOSState myMenuRecordings::Rewind()
{
 if(HasSubMenu()||Count()==0)
  return osContinue;

 myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
 if(item&&!item->IsDirectory())
 {
  cDevice::PrimaryDevice()->StopReplay();
  cResumeFile ResumeFile(item->FileName());
  ResumeFile.Delete();
  return Play();
 }
 return osContinue;
}

eOSState myMenuRecordings::Delete()
{
 if(HasSubMenu()||Count()==0)
  return osContinue;
 
 myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
 if(item&&!item->IsDirectory())
 {
  if(Interface->Confirm(tr("Delete recording?")))
  {
   cRecordControl *rc=cRecordControls::GetRecordControl(item->FileName());
   if(rc)
   {
    if(Interface->Confirm(tr("Timer still recording - really delete?")))
    {
     cTimer *timer=rc->Timer();
     if(timer)
     {
      timer->Skip();
      cRecordControls::Process(time(NULL));
      if(timer->IsSingleEvent())
      {
       isyslog("deleting timer %s",*timer->ToDescr());
       Timers.Del(timer);
      }
      Timers.SetModified();
     }
    }
    else
     return osContinue;
   }
   cRecording *recording=GetRecording(item);
   if(recording)
   {
    if(recording->Delete())
    {
     myReplayControl::ClearLastReplayed(item->FileName());
     Recordings.DelByName(item->FileName());
     cOsdMenu::Del(Current());
     SetHelpKeys();
     Display();
     if(!Count())
      return osBack;
    }
    else
     Skins.Message(mtError,tr("Error while deleting recording!"));
   }
  }
 } 
 return osContinue;
}

eOSState myMenuRecordings::Rename()
{
 if(HasSubMenu()||Count()==0)
  return osContinue;

 myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
 if(item&&!item->IsDirectory())
 {
  cRecording *recording=GetRecording(item);
  if(recording)
   return AddSubMenu(new myMenuRenameRecording(recording,this));
 }
 return osContinue;
}

eOSState myMenuRecordings::MoveRec()
{
 if(HasSubMenu()||Count()==0)
  return osContinue;
 
 myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
 if(item&&!item->IsDirectory())
 {
  cRecording *recording=GetRecording(item);
  if(recording)
  {
   clearall=false;
   return AddSubMenu(new myMenuMoveRecording(recording,this));
  }
 }
 return osContinue;
}

eOSState myMenuRecordings::Info(void)
{
 if(HasSubMenu()||Count()==0)
  return osContinue;

 myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
 if(item&&!item->IsDirectory())
 {
  cRecording *recording=GetRecording(item);
  if(recording&&recording->Info()->Title())
   return AddSubMenu(new myMenuRecordingInfo(recording,true));
 }
 return osContinue;
}

eOSState myMenuRecordings::ProcessKey(eKeys Key)
{
 eOSState state;
 
 if(edit)
 {
  switch(Key)
  {
   case kRed: edit=false;
              helpkeys=-1;
              return Rename();
   case kGreen: edit=false,
                helpkeys=-1;
                return MoveRec();
   case kYellow: edit=false;
                 helpkeys=-1;
                 return Delete();
   case kBack: edit=false;
               helpkeys=-1;
               SetHelpKeys();
   default: break;
  }
  state=osContinue;
 }
 else
 {
  bool hadsubmenu=HasSubMenu();
  state=cOsdMenu::ProcessKey(Key);
  if(state==osUnknown)
  {
   switch(Key)
   {
    case kOk:
    case kRed: return Play();
    case kGreen: return Rewind();
    case kYellow: {
                   myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
                   if(item&&!item->IsDirectory())
                   {
                    edit=true;
                    SetHelp(tr("Button$Rename"),tr("Button$Move"),tr("Button$Delete"));
                   }
                   break;
                  }
    case kBlue: return Info();
    default: break;
   }
  }
  // refresh the list after submenu has closed
  if(hadsubmenu&&!HasSubMenu())
   Set();

  // go back if list is empty
  if(!Count())
   state=osBack;

  if(!HasSubMenu()&&Key!=kNone);
   SetHelpKeys();
 }
 return state;
}
