/*
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <vdr/interface.h>
#include <vdr/videodir.h>
#include "myreplaycontrol.h"
#include "mymenurecordings.h"
#include "mymenusetup.h"
#include "mymenucommands.h"
#include "patchfont.h"

// --- myMenuRecordingsItem ---------------------------------------------------
myMenuRecordingsItem::myMenuRecordingsItem(cRecording *Recording,int Level)
{
 totalentries=newentries=0;
 isdvd=false;
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
  if(Level==level) // recording entries
  {
   s=strrchr(Recording->Name(),'~');
   
   // date and time of recording
   struct tm tm_r;
   struct tm *t=localtime_r(&Recording->start,&tm_r);

   char RecLength[21];
   char *indexfilename;
   
   // recording length
   asprintf(&indexfilename,"%s/index.vdr",filename);
   int haslength=!access(indexfilename,R_OK);
   if(haslength) // calculate recording length from the size of index.vdr
   {
    struct stat buf;
    if(!stat(indexfilename,&buf))
     snprintf(RecLength,sizeof(RecLength),"%d'",(int)(buf.st_size/12000));
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
      snprintf(RecLength,sizeof(RecLength),"%s'\n",buffer);
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
     if(fgets(dvdnr,sizeof(dvdnr),f))
     {
      char *p=strchr(dvdnr,'\n');
      if(p)
       *p=0;
     }
     fclose(f);
    }
   }
   free(indexfilename);

   char RecDate[9],RecTime[6],RecDelimiter[2]={'\t',0};
   char New[2]={0,0};
   if(isdvd)
    New[0]=mysetup.PatchDvd?char(129):'~';
   else
    if(isnew&&!mysetup.PatchNew)
     New[0]='*';
    else
     if(!isnew&&mysetup.PatchNew)
      New[0]=char(128);

   snprintf(RecDate,sizeof(RecDate),"%02d.%02d.%02d",t->tm_mday,t->tm_mon+1,t->tm_year%100);
   snprintf(RecTime,sizeof(RecTime),"%02d:%02d",t->tm_hour,t->tm_min);
   asprintf(&title,"%s%s%s%s%s%s%s%s%s%s",
                   (mysetup.ShowRecDate?RecDate:""), // show recording date?
                   (mysetup.ShowRecDate?RecDelimiter:""), // tab
                   (mysetup.ShowRecTime?RecTime:""), // show recording time?
                   (mysetup.ShowRecTime?RecDelimiter:""), // tab
                   ((haslength&&mysetup.ShowRecLength)?RecLength:""), // show recording length?
                   (mysetup.ShowRecLength?RecDelimiter:""), // tab
                   New, // dvd/new marker
                   (mysetup.ShowDvdNr?dvdnr:""), // show dvd nummber
                   ((isdvd&&mysetup.ShowDvdNr)?" ":""), // space for fancy looking
                   s?s+1:Recording->Name()); // recording name
   
   asprintf(&id,"%s %s %s",RecDate,RecTime,Recording->Name());
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
 if(mysetup.ShowNewRecs)
  asprintf(&buffer,"%d\t%d\t%s",totalentries,newentries,name);
 else
  asprintf(&buffer,"%d\t%s",totalentries,name);
 
 SetText(buffer,false);
}

// --- myMenuRecordings -------------------------------------------------------
myMenuRecordings::myMenuRecordings(const char *Base,int Level):cOsdMenu(Base?Base:tr("Extended recordings menu"))
{
 // patch font
 if(Level==0&&(mysetup.PatchNew||mysetup.PatchDvd))
 {
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
 Set();
 if(Current()<0)
  SetCurrent(First());
 else
  if(myReplayControl::LastReplayed())
  {
   if(mysetup.wasdvd&&!cControl::Control())
   {
    char *cmd;
    asprintf(&cmd,"dvdarchive.sh umount \"%s\"",myReplayControl::LastReplayed());
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
    
    mysetup.wasdvd=false;
   }
   Open();
  }
 
 Display();
 SetHelpKeys();
}

myMenuRecordings::~myMenuRecordings()
{
 free(base);

 if(level==0&&(mysetup.PatchNew||mysetup.PatchDvd))
 {
  if(Setup.UseSmallFont==2)
   cFont::SetFont(fontSml);
  else
   cFont::SetFont(fontOsd);
 }
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
     case 1: SetHelp(tr("Button$Open"));break;
     case 2: SetHelp(RecordingCommands.Count()?tr("Button$Commands"):tr("Button$Play"),tr("Button$Rewind"),tr("Button$Edit"),NULL);break;
     case 3: SetHelp(RecordingCommands.Count()?tr("Button$Commands"):tr("Button$Play"),tr("Button$Rewind"),tr("Button$Edit"),tr("Button$Info"));break;
    }
   }
   helpkeys=newhelpkeys;
  }
 }
}

// create the menu list
void myMenuRecordings::Set(bool Refresh)
{
 const char *lastreplayed=myReplayControl::LastReplayed();
 
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
    {
     lastitem->IncrementCounter(recording->IsNew()); // counts the number of entries in a directory
     if(lastreplayed&&!strcmp(lastreplayed,recording->FileName()))
      SetCurrent(lastitem);
    }
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
   if(item->ID()&&(!lastitem||strcmp(lastitemtext,item->ID())))
   {
    if(!item->IsDirectory())
    {
     Add(item);
     inlist=true;
    }
    lastitem=item;
    free(lastitemtext);
    lastitemtext=strdup(lastitem->ID());
   }
   else
    delete item;
   
   if(lastitem)
   {
    if(!item->IsDirectory()&&lastreplayed&&!strcmp(lastreplayed,recording->FileName()))
    {
     SetCurrent(lastitem);
     if(!cControl::Control())
      myReplayControl::ClearLastReplayed(recording->FileName());
    }
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
      strcpy(path,recording->FileName());
      name=strrchr(path,'/')+1;
      asprintf(&msg,"dvdarchive.sh mount \"%s\" \"%s\"",path,name); // call the dvdarchive.sh script

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
       if(result==127)
        Skins.Message(mtError,tr("Script 'dvdarchive.sh' not found!"));
       return osContinue;
      }
      mysetup.wasdvd=true;
     }
     else
     {
      free(msg);
      return osContinue;
     }
    }
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
    case kOk: return Play();
    case kRed: return (helpkeys>1&&RecordingCommands.Count())?Commands():Play();
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
    case k1...k9: return Commands(Key);
    default: break;
   }
  }
  // refresh list after submenu has closed
  if(hadsubmenu&&!HasSubMenu()&&Recordings.StateChanged(recordingsstate))
   Set(true);
  
  // go back if list is empty
  if(!Count())
   state=osBack;

  if(!HasSubMenu()&&Key!=kNone);
   SetHelpKeys();
 }
 return state;
}
