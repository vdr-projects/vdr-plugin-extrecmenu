#include <vdr/interface.h>
#include "mymenurecordings.h"
#include "mymenusetup.h"

// --- myMenuRecordingsItem ---------------------------------------------------
myMenuRecordingsItem::myMenuRecordingsItem(cRecording *Recording,int Level)
{
 totalentries=newentries=0;

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
 if(Level<level) // directory entries
 {
  s=Recording->Name();
  while(Level)
  {
   s=strchr(Recording->Name(),'~')+1;
   Level--;
  }
  asprintf(&title,"\t\t%s",s);
  char *p=strchr(title,'~');
  if(p)
   *p=0;
  name=strdup(title+2);
 }
 else
  if(Level==level) // recording entries
  {
   s=strrchr(Recording->Name(),'~');
   
   // date and time of recording
   struct tm t;
   localtime_r(&Recording->start,&t);

   // recording length
   struct tIndex{int offset;uchar type;uchar number;short reserved;};
   char RecLength[21],RecDate[9],RecTime[6],RecDelimiter[2]={'\t',0};
   int last=-1;
   char *indexfilename;
   
   asprintf(&indexfilename,"%s/index.vdr",filename);
   int delta=0;
   if(!access(indexfilename,R_OK))
   {
    struct stat buf;
    if(!stat(indexfilename,&buf))
    {
     delta=buf.st_size%sizeof(tIndex);
     if(delta)
      delta=sizeof(tIndex)-delta;
     last=(buf.st_size+delta)/sizeof(tIndex)+1;
     char hour[2],min[3];
     snprintf(RecLength,sizeof(RecLength),"%s",*IndexToHMSF(last,true));
     snprintf(hour,sizeof(hour),"%c",RecLength[0]);
     snprintf(min,sizeof(min),"%c%c",RecLength[2],RecLength[3]);
     snprintf(RecLength,sizeof(RecLength),"%d'",(atoi(hour)*60)+atoi(min));
    }
   }   
   free(indexfilename);

   snprintf(RecDate,sizeof(RecDate),"%02d.%02d.%02d",t.tm_mday,t.tm_mon,t.tm_year%100);
   snprintf(RecTime,sizeof(RecTime),"%02d:%02d",t.tm_hour,t.tm_min);
   asprintf(&title,"%s%s%s%s%s%s%c%s",
                   (mysetup.ShowRecDate?RecDate:""),
                   (mysetup.ShowRecDate?RecDelimiter:""),
                   (mysetup.ShowRecTime?RecTime:""),
                   (mysetup.ShowRecTime?RecDelimiter:""),
                   (mysetup.ShowRecLength?RecLength:""),
                   (mysetup.ShowRecLength?RecDelimiter:""),
                   isnew,
                   s?s+1:Recording->Name());
  }
  else 
   if(Level>level) // any other
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

void myMenuRecordingsItem::IncrementCounter(bool IsNew)
{
 totalentries++;
 if(IsNew)
  newentries++;
 
 char *buffer=NULL;
 asprintf(&buffer,"%d\t%d\t%s",totalentries,newentries,name);
 SetText(buffer,false);
}

// --- myMenuRecordings -------------------------------------------------------
myMenuRecordings::myMenuRecordings(const char *Base,int Level):cOsdMenu(Base?Base:tr("Extended recordings menu"))
{
 // set tabs
 if(mysetup.ShowRecDate&&mysetup.ShowRecTime&&mysetup.ShowRecLength) // all details are shown
  SetCols(8,6,4);
 else
  if(mysetup.ShowRecDate&&!mysetup.ShowRecTime&&mysetup.ShowRecLength) // recording time is not shown
   SetCols(8,4);
  else
   if(!mysetup.ShowRecDate&&mysetup.ShowRecTime&&mysetup.ShowRecLength) // recording date is not shown
    SetCols(6,4);
   else // recording date and time are not shown; even if recording length should be not shown we must set two tabs because the details of the directories
    SetCols(4,4);

 edit=false;
 level=Level;
 helpkeys=-1;
 base=Base?strdup(Base):NULL;
 
 Recordings.StateChanged(recordingsstate);
 
 Display();
 Set();
 /*
 if(Current()<0)
  SetCurrent(First());
 else
  if(myReplayControl::LastReplayed&&Open())
   return;
 */
 Display();
 SetHelpKeys();
}

myMenuRecordings::~myMenuRecordings()
{
 free(base);
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

void myMenuRecordings::Set(bool Refresh)
{
// const char *currentrecording=myReplayControl::LastReplayed();

 cThreadLock RecordingsLock(&Recordings);
 Clear();
 Recordings.Sort();
 
 char *lastitemtext=NULL;
 myMenuRecordingsItem *lastitem=NULL;
 bool  inlist=false;
 // add first the directories
 for(cRecording *recording=Recordings.First();recording;recording=Recordings.Next(recording))
 {
  if(!base||(strstr(recording->Name(),base)==recording->Name()&&recording->Name()[strlen(base)]=='~'))
  {
   myMenuRecordingsItem *item=new myMenuRecordingsItem(recording,level);
   if(*item->Text()&&(!lastitem||strcmp(item->Text(),lastitemtext)))
   {
    if(item->IsDirectory())
    {
     Add(item);
     inlist=true;
    }

    lastitem=item;
    free(lastitemtext);
    lastitemtext=strdup(lastitem->Text());
   }
   else
    delete item;
   
   if(lastitem)
   {
    if(lastitem->IsDirectory())
     lastitem->IncrementCounter(recording->IsNew()); // counts the number of entries in a directory
    // delete items that are not in the list
    if(!inlist)
    {
     delete lastitem;
     lastitem=NULL;
     inlist=false;
    }
   }
  }
 }
 inlist=false;
 lastitem=NULL;
 // and now the recordings
 for(cRecording *recording=Recordings.First();recording;recording=Recordings.Next(recording))
 {
  if(!base||(strstr(recording->Name(),base)==recording->Name()&&recording->Name()[strlen(base)]=='~'))
  {
   myMenuRecordingsItem *item=new myMenuRecordingsItem(recording,level);
   if(*item->Text()&&(!lastitem||strcmp(lastitemtext,item->Text())))
   {
    if(!item->IsDirectory())
    {
     Add(item);
     inlist=true;
    }     
    lastitem=item;
    free(lastitemtext);
    lastitemtext=strdup(lastitem->Text());
   }
   else
    delete item;
   
   if(lastitem)
   {
    // delete items that are not in the list
    if(!inlist)
    {
     delete lastitem;
     lastitem=NULL;
     inlist=false;
    }
   }
  }
 }
 free(lastitemtext);
 if(Refresh)
  Display();
}

// returns the corresponding recording to an item
cRecording *myMenuRecordings::GetRecording(myMenuRecordingsItem *Item)
{
 cRecording *recording=Recordings.GetByName(Item->FileName());
 if(!recording)
  Skins.Message(mtError,tr("Error while accessing recording!"));
 return recording;
}

// opens a subdirectory
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

// plays a recording
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

// plays a recording from the beginning
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

// delete a recording
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

// renames a recording
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

// moves a recording
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

// opens an info screen to a recording
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
    case kNone: if(Recordings.StateChanged(recordingsstate))
                 Set(true);
    default: break;
   }
  }
  // go back if list is empty
  if(!Count())
   state=osBack;

  if(!HasSubMenu()&&Key!=kNone);
   SetHelpKeys();
 }
 return state;
}
