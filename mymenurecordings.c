/*
 * See the README file for copyright information and how to reach the author.
 */

#include <vdr/interface.h>
#include <vdr/videodir.h>
#include <vdr/status.h>
#include <vdr/plugin.h>
#include <vdr/cutter.h>
#include "myreplaycontrol.h"
#include "mymenurecordings.h"
#include "mymenusetup.h"
#include "mymenucommands.h"
#include "patchfont.h"
#include "tools.h"

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

myMenuRecordingInfo::myMenuRecordingInfo(const cRecording *Recording, bool WithButtons):cOsdMenu(tr("Recording info"))
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

eOSState myMenuRecordingInfo::ProcessKey(eKeys Key)
{
 switch(Key)
 {
  case kUp|k_Repeat:
  case kUp:
  case kDown|k_Repeat:
  case kDown:
  case kLeft|k_Repeat:
  case kLeft:
  case kRight|k_Repeat:
  case kRight: DisplayMenu()->Scroll(NORMALKEY(Key)==kUp||NORMALKEY(Key)==kLeft,NORMALKEY(Key)==kLeft||NORMALKEY(Key)==kRight);
               cStatus::MsgOsdTextItem(NULL, NORMALKEY(Key) == kUp);
               return osContinue;
  default: break;
 }

 eOSState state=cOsdMenu::ProcessKey(Key);
 if(state==osUnknown)
 {
  switch(Key)
  {
   case kRed: if(withButtons)
              Key=kOk; // will play the recording, even if recording commands are defined
   case kGreen: if(!withButtons)
                 break;
                cRemote::Put(Key,true);
   case kOk: return osBack;
   default: break;
  }
 }
 return state;
}

// --- myMenuRecordingsItem ---------------------------------------------------
myMenuRecordingsItem::myMenuRecordingsItem(cRecording *Recording,int Level)
{
 totalentries=newentries=0;
 isdvd=false;
 isvideodvd=false;
 name=NULL;
 id=NULL;

 strn0cpy(dvdnr,"",sizeof(dvdnr));
 bool isnew=Recording->IsNew();
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
  const char *p=s;
  while(*++s)
  {
   if(*s == '~')
   {
    if(Level--)
     p=s+1;
    else
     break;
   }
  }
  title=MALLOC(char,s-p+3);
  *title='\t';
  *(title+1)='\t';
  strn0cpy(title+2,p,s-p+1);
  name=strdup(title+2);
 }
 else
 {
  if(Level==level) // recording entries
  {
   s=strrchr(Recording->Name(),'~');

   // date and time of recording
   struct tm tm_r;
   struct tm *t=localtime_r(&Recording->start,&tm_r);

   char reclength[21];
   char *indexfilename;

   // recording length
   asprintf(&indexfilename,"%s/index.vdr",filename);
   int haslength=!access(indexfilename,R_OK);
   if(haslength) // calculate recording length from the size of index.vdr
   {
    struct stat buf;
    if(!stat(indexfilename,&buf))
     snprintf(reclength,sizeof(reclength),"%d'",(int)(buf.st_size/12000));
   }
   else // no index -> is there a length.vdr, containing recording length as a string?
   {
    free(indexfilename);
    asprintf(&indexfilename,"%s/length.vdr",filename);
    haslength=!access(indexfilename,R_OK);
    if(haslength)
    {
     FILE *f;
     if((f=fopen(indexfilename,"r"))!=NULL)
     {
      char buffer[8];
      if(fgets(buffer,sizeof(buffer),f))
      {
       char *p=strchr(buffer,'\n');
       if(p)
        *p=0;
      }
      fclose(f);
      snprintf(reclength,sizeof(reclength),"%s'\n",buffer);
     }
    }
   }
   free(indexfilename);

   // dvdarchive-patch functionality
   asprintf(&indexfilename,"%s/dvd.vdr",filename);
   isdvd=!access(indexfilename,R_OK);
   if(isdvd)
   {
    FILE *f;
    if((f=fopen(indexfilename,"r"))!=NULL)
    {
     // get the dvd id
     if(fgets(dvdnr,sizeof(dvdnr),f))
     {
      char *p=strchr(dvdnr,'\n');
      if(p)
       *p=0;
     }
     // determine if the dvd is a video dvd
     char tmp[BUFSIZ];
     if(fgets(tmp,sizeof(tmp),f))
      isvideodvd=true;
     
     fclose(f);
    }
   }
   free(indexfilename);

   char recdate[9],rectime[6],recdelimiter[2]={'\t',0};
   char dvd[2]={0,0};
   if(isdvd)
    dvd[0]=char(129);
   else
    if(isnew&&!mysetup.PatchNew)
     dvd[0]='*';
    else
     if(!isnew&&mysetup.PatchNew)
      dvd[0]=char(128);

   snprintf(recdate,sizeof(recdate),"%02d.%02d.%02d",t->tm_mday,t->tm_mon+1,t->tm_year%100);
   snprintf(rectime,sizeof(rectime),"%02d:%02d",t->tm_hour,t->tm_min);
   asprintf(&title,"%s%s%s%s%s%s%s%s%s%s",
                   (mysetup.ShowRecDate?recdate:""), // show recording date?
                   (mysetup.ShowRecDate?recdelimiter:""), // tab
                   (mysetup.ShowRecTime?rectime:""), // show recording time?
                   (mysetup.ShowRecTime?recdelimiter:""), // tab
                   ((haslength&&mysetup.ShowRecLength)?reclength:""), // show recording length?
                   (mysetup.ShowRecLength?recdelimiter:""), // tab
                   dvd, // dvd/new marker
                   (mysetup.ShowDvdNr?dvdnr:""), // show dvd nummber
                   ((isdvd&&mysetup.ShowDvdNr)?" ":""), // space for fancy looking
                   s?s+1:Recording->Name()); // recording name

   asprintf(&id,"%s %s %s",recdate,rectime,Recording->Name());
  }
  else
  {
   if(Level>level) // any other
    title="";
  }
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
 if(mysetup.ShowNewRecs)
  asprintf(&buffer,"%d\t%d\t%s",totalentries,newentries,name);
 else
  asprintf(&buffer,"%d\t%s",totalentries,name);

 // don't show '-', '.', '$', 'ª' or '·' if the directory name ends with one of it
 if(buffer[strlen(buffer)-1]=='.'||buffer[strlen(buffer)-1]=='-'||buffer[strlen(buffer)-1]=='$'||buffer[strlen(buffer)-1]==char(170)||buffer[strlen(buffer)-1]==char(183))
  buffer[strlen(buffer)-1]=0;

 SetText(buffer,false);
}

// --- myMenuRecordings -------------------------------------------------------
bool myMenuRecordings::golastreplayed=false;
bool myMenuRecordings::wasdvd;

myMenuRecordings::myMenuRecordings(const char *Base,int Level):cOsdMenu(Base?Base:"")
{
 // only called if plugin menu is opened
 if(Level==0)
 {
  // patch font
  if(Setup.UseSmallFont==2)
   PatchFont(fontSml);
  else
   PatchFont(fontOsd);
 }
 // set tabs
 if(mysetup.ShowRecDate&&mysetup.ShowRecTime&&!mysetup.ShowRecLength) // recording date and time are shown, recording length not
  SetCols(8,6);
 else
  if(mysetup.ShowRecDate&&mysetup.ShowRecTime) // all details are shown
   SetCols(8,6,4);
  else
   if(mysetup.ShowRecDate&&!mysetup.ShowRecTime) // recording time is not shown
    SetCols(8,4);
   else
    if(!mysetup.ShowRecDate&&mysetup.ShowRecTime&&mysetup.ShowRecLength) // recording date is not shown
     SetCols(6,4);
    else // recording date and time are not shown; even if recording length should be not shown we must set two tabs because the details of the directories
     SetCols(4,3);

 edit=false;
 level=Level;
 helpkeys=-1;
 base=Base?strdup(Base):NULL;

 Recordings.StateChanged(recordingsstate);

 Display();

 if(wasdvd&&!cControl::Control())
 {
  char *cmd=NULL;
  asprintf(&cmd,"dvdarchive.sh umount \"%s\"",*strescape(myReplayControl::LastReplayed(),"'\\\"$"));
  isyslog("[extrecmenu] calling %s to unmount dvd",cmd);
  int result=SystemExec(cmd);
  if(result)
  {
   result=result/256;
   if(result==1)
    Skins.Message(mtError,tr("Error while mounting DVD!"));
  }
  isyslog("[extrecmenu] dvdarchive.sh returns %d",result);
  free(cmd);

  wasdvd=false;
 }

 Set();
 if(myReplayControl::LastReplayed())
  Open();

 Display();
 SetHelpKeys();
}

myMenuRecordings::~myMenuRecordings()
{
 free(base);

 if(level==0)
 {
  if(Setup.UseSmallFont==2)
   cFont::SetFont(fontSml);
  else
   cFont::SetFont(fontOsd);
 }
}

void myMenuRecordings::SetFreeSpaceTitle()
{
 int freemb;
 VideoDiskSpace(&freemb);
 int minutes=int(double(freemb)/MB_PER_MINUTE);
 char buffer[40];
 snprintf(buffer,sizeof(buffer),"%s - %2d:%02d %s",tr("Recordings"),minutes/60,minutes%60,tr("free"));
 SetTitle(buffer);
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
    cRecording *recording=GetRecording(item);
    if(recording&&recording->Info()->Title())
     newhelpkeys=3;
   }
   if(newhelpkeys!=helpkeys)
   {
    switch(newhelpkeys)
    {
     case 0: SetHelp(NULL);break;
     case 1: SetHelp(tr("Button$Open"),NULL,tr("Button$Edit"));break;
     case 2: SetHelp(RecordingCommands.Count()?tr("Button$Commands"):tr("Button$Play"),tr("Button$Rewind"),tr("Button$Edit"),NULL);break;
     case 3: SetHelp(RecordingCommands.Count()?tr("Button$Commands"):tr("Button$Play"),tr("Button$Rewind"),tr("Button$Edit"),tr("Button$Info"));break;
    }
   }
   helpkeys=newhelpkeys;
  }
 }
}

// create the menu list
void myMenuRecordings::Set(bool Refresh,char *current)
{
 const char *lastreplayed=current?current:myReplayControl::LastReplayed();
  
 if(level==0)
  SetFreeSpaceTitle();

 cThreadLock RecordingsLock(&Recordings);

 if(Refresh&&!current)
 {
  myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
  if(item)
  {
   cRecording *recording=Recordings.GetByName(item->FileName());
   if(recording)
    lastreplayed=recording->FileName();
  }
 }
 
 Clear();
 
 // create my own recordings list from VDR's
 myRecList *list=new myRecList();
 for(cRecording *recording=Recordings.First();recording;recording=Recordings.Next(recording))
  list->Add(new myRecListItem(recording));
 // sort my recordings list
 list->Sort();

 // needed for move recording menu
 Recordings.Sort();

 char *lastitemtext=NULL;
 myMenuRecordingsItem *lastitem=NULL;
 for(myRecListItem *listitem=list->First();listitem;listitem=list->Next(listitem))
 {
  cRecording *recording=listitem->recording;
  if(!base||(strstr(listitem->recording->Name(),base)==listitem->recording->Name()&&listitem->recording->Name()[strlen(base)]=='~'))
  {
   myMenuRecordingsItem *recitem=new myMenuRecordingsItem(listitem->recording,level);
   if(*recitem->Text()&&(!lastitem||strcmp(recitem->Text(),lastitemtext)))
   {
    Add(recitem);
    lastitem=recitem;
    free(lastitemtext);
    lastitemtext=strdup(lastitem->Text());
   }
   else
    delete recitem;

   if(lastitem)
   {
    if(lastitem->IsDirectory())
     lastitem->IncrementCounter(recording->IsNew());
    if(lastreplayed&&!strcmp(lastreplayed,recording->FileName()))
    {
     if(golastreplayed||Refresh)
     {
      SetCurrent(lastitem);
      if(recitem&&!recitem->IsDirectory())
      golastreplayed=false;
     }
     if(recitem&&!recitem->IsDirectory()&&recitem->IsDVD()&&!cControl::Control())
      cReplayControl::ClearLastReplayed(cReplayControl::LastReplayed());
    }
   }
  }
 }
 free(lastitemtext);
 delete list;
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
 char *msg=NULL;
 char *name=NULL;

 char path[MaxFileName];

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
    if(item->IsDVD())
    {
     asprintf(&msg,tr("Please insert DVD %s"),item->DvdNr());
     if(Interface->Confirm(msg))
     {
      free(msg);
      // recording is a video dvd
      if(item->IsVideoDVD())
      {
       cPlugin *plugin=cPluginManager::GetPlugin("dvd");
       if(plugin)
       {
        cOsdObject *osd=plugin->MainMenuAction();
        delete osd;
        osd=NULL;
        return osEnd;            
       }
       else
       {
        Skins.Message(mtError,tr("DVD plugin is not installed!"));
        return osContinue;
       }
      }
      // recording is a archive dvd
      else
      {
       strcpy(path,recording->FileName());
       name=strrchr(path,'/')+1;
       asprintf(&msg,"dvdarchive.sh mount \"%s\" '%s'",*strescape(path,"'"),*strescape(name,"'\\\"$"));
 
       isyslog("[extrecmenu] calling %s to mount dvd",msg);
       int result=SystemExec(msg);
       isyslog("[extrecmenu] dvdarchive.sh returns %d",result);
       free(msg);
       if(result)
       {
        result=result/256;
        if(result==1)
         Skins.Message(mtError,tr("Error while mounting DVD!"));
        if(result==2)
         Skins.Message(mtError,tr("No DVD in drive!"));
        if(result==3)
         Skins.Message(mtError,tr("Recording not found on DVD!"));
        if(result==4)
         Skins.Message(mtError,tr("Error while linking [0-9]*.vdr!"));
        if(result==5)
         Skins.Message(mtError,tr("sudo or mount --bind / umount error (vfat system)"));
        if(result==127)
         Skins.Message(mtError,tr("Script 'dvdarchive.sh' not found!"));
        return osContinue;
       }
       wasdvd=true;
      }
     }
     else
     {
      free(msg);
      return osContinue;
     }
    }
    golastreplayed=true;
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
     cRecordingUserCommand::InvokeCommand("delete",item->FileName());
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
 if(item)
 {
  if(item->IsDirectory())
   return AddSubMenu(new myMenuRenameRecording(this,NULL,base,item->Name()));
  else
  {
   cRecording *recording=GetRecording(item);
   if(recording)
    return AddSubMenu(new myMenuRenameRecording(this,recording,NULL,NULL));
  }
 }
 return osContinue;
}

// edit details of a recording
eOSState myMenuRecordings::Details()
{
 if(HasSubMenu()||Count()==0)
  return osContinue;

 myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
 if(item&&!item->IsDirectory())
 {
  cRecording *recording=GetRecording(item);
  if(recording)
   return AddSubMenu(new myMenuRecordingDetails(recording,this));
 }
 return osContinue;
}

// moves a recording
eOSState myMenuRecordings::MoveRec()
{
 if(HasSubMenu()||Count()==0)
  return osContinue;

 myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
 if(item)
 {
  myMenuMoveRecording::clearall=false;
  if(item->IsDirectory())
   return AddSubMenu(new myMenuMoveRecording(this,NULL,base,item->Name()));
  else
  {
   cRecording *recording=GetRecording(item);
   if(recording)
    return AddSubMenu(new myMenuMoveRecording(this,recording,NULL,NULL));
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

// execute a command for a recording
eOSState myMenuRecordings::Commands(eKeys Key)
{
 if(HasSubMenu()||Count()==0)
  return osContinue;
 myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
 if(item&&!item->IsDirectory())
 {
  cRecording *recording=GetRecording(item);
  if(recording)
  {
   char *parameter=NULL;
   asprintf(&parameter,"'%s'",recording->FileName());
   myMenuCommands *menu;
   eOSState state=AddSubMenu(menu=new myMenuCommands(tr("Recording commands"),&RecordingCommands,parameter));
   free(parameter);
   if(Key!=kNone)
    state=menu->ProcessKey(Key);
   return state;
  }
 }
 return osContinue;
}

eOSState myMenuRecordings::ProcessKey(eKeys Key)
{
 eOSState state;

 if(edit)
 {
  myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
  if(Key==kRed||Key==kGreen||Key==kYellow||(!item->IsDVD()&&Key==kBlue)||Key==kBack)
  {
   edit=false;
   helpkeys=-1;
  }
  switch(Key)
  {
   case kRed: return Rename();
   case kGreen: return MoveRec();
   case kYellow: return Delete();
   case kBlue: if(item&&!item->IsDVD())
                return Details();
               else
                break;
   case kBack: SetHelpKeys();
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
    case kOk: return Play();
    case kRed: return (helpkeys>1&&RecordingCommands.Count())?Commands():Play();
    case kGreen: return Rewind();
    case kYellow: if(cCutter::Active())
                   Skins.Message(mtError,tr("Editing not allowed while cutting!"));
                  else
                  {
                   myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
                   if(!HasSubMenu()&&item)
                   {
                    edit=true;
                    if(item->IsDirectory())
                     SetHelp(tr("Button$Rename"),tr("Button$Move"));
                    else
                     SetHelp(tr("Button$Rename"),tr("Button$Move"),tr("Button$Delete"),(!item->IsDVD())?tr("Details"):NULL);
                   }
                  }
                  break;
    case kBlue: return Info();
    case k1...k9: return Commands(Key);
    default: break;
   }
  }
  // go back if list is empty
  if(!Count()&&level>0)
   state=osBack;

  // update menu list after sub menu has closed
  if(hadsubmenu&&!HasSubMenu()||Recordings.StateChanged(recordingsstate)||cCutter::Active())
   Set(true);

  if(!HasSubMenu()&&Key!=kNone);
   SetHelpKeys();
 }
 return state;
}
