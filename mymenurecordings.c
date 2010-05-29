/*
 * See the README file for copyright information and how to reach the author.
 */

#include <sys/statvfs.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <vdr/interface.h>
#include <vdr/videodir.h>
#include <vdr/status.h>
#include <vdr/plugin.h>
#include <vdr/cutter.h>
#include <vdr/menu.h>
#include "myreplaycontrol.h"
#include "mymenurecordings.h"
#include "mymenusetup.h"
#include "mymenucommands.h"
#include "tools.h"

using namespace std;

// --- myMenuRecordingInfo ----------------------------------------------------
// this class is mostly a copy of VDR's cMenuRecording class, I only adjusted
// the Display()-method
// ----------------------------------------------------------------------------
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
#ifdef USE_GRAPHTFT
    virtual const char* MenuKind(){return "MenuExtRecording";}
#endif
};

myMenuRecordingInfo::myMenuRecordingInfo(const cRecording *Recording, bool WithButtons):cOsdMenu(trVDR("Recording info"))
{
  recording=Recording;
  withButtons=WithButtons;
  if(withButtons)
    SetHelp(tr("Button$Play"),tr("Button$Rewind"),NULL,tr("Button$Back"));
}

void myMenuRecordingInfo::Display(void)
{
  cOsdMenu::Display();

#ifdef USE_GRAPHTFT
  cStatus::MsgOsdSetRecording(recording);
#endif

  if(mysetup.UseVDRsRecInfoMenu)
  {
    DisplayMenu()->SetRecording(recording);
    if(recording->Info()->Description())
      cStatus::MsgOsdTextItem(recording->Info()->Description());
  }
  else
  {
    stringstream text;
    text << *DateString(recording->start) << ", " << *TimeString(recording->start) << "\n\n";

    if(recording->Info()->Title())
    {
      text << recording->Info()->Title() << "\n\n";
      if(recording->Info()->Description())
        text << recording->Info()->Description() << "\n\n";
    }
 
    string recname=recording->Name();
    string::size_type i=recname.rfind('~');
    if(i!=string::npos)
      text << tr("Name") << ": " << recname.substr(i+1,recname.length()) << "\n"
           << tr("Path") << ": " << recname.substr(0,i) << "\n";
    else
      text << tr("Name") << ": " << recname << "\n";
 
    cChannel *chan=Channels.GetByChannelID(((cRecordingInfo*)recording->Info())->ChannelID());
    if(chan)
      text << tr("Channel") << ": " << *ChannelString(chan,0) << "\n";

    int recmb=DirSizeMB(recording->FileName());
    if(recmb<0)
      recmb=0;
    if(recmb > 1023)
      text << tr("Size") << ": " << setprecision(3) << recmb/1024.0 << " GB\n";
    else
      text << tr("Size") << ": " << recmb << " MB\n";

    text << trVDR("Priority") << ": " << recording->priority << "\n";
    text << trVDR("Lifetime") << ": " << recording->lifetime << "\n";

    DisplayMenu()->SetText(text.str().c_str(),false);
    cStatus::MsgOsdTextItem(text.str().c_str());
  }
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
                   Key=kOk;
      case kGreen: if(!withButtons)
                     break;
                   cRemote::Put(Key,true);
      case kBlue:
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
  dirismoving=true;
  name=NULL;
  filename=Recording->FileName();
#if VDRVERSNUM >= 10703
  isPesRecording=Recording->IsPesRecording();
#endif

  // get the level of this recording
  level=0;
  const char *s=Recording->Name();
  while(*++s)
  {
    if(*s=='~')
      level++;
  }

  // create the title of this item
  if(Level<level) // directory entry
  {
    s=Recording->Name();
    const char *p=s;
    while(*++s)
    {
      if(*s=='~')
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
    uniqid=name;
  }
  else
  {
    if(Level==level) // recording entry
    {
      string buffer;
      stringstream titlebuffer;
      stringstream idbuffer;
    
      buffer=filename;
#if VDRVERSNUM > 10703
      if (isPesRecording)
        buffer+="/001.vdr";
      else
        buffer+="/00001.ts";
#else
      buffer+="/001.vdr";
#endif
      if(access(buffer.c_str(),R_OK))
      {
        buffer=filename;
        buffer+="/dvd.vdr";
        isdvd=!access(buffer.c_str(),R_OK);
      }
    
      if(MoveCutterThread->IsMoving(filename))
        titlebuffer << Icons::MovingRecording(); // moving recording
      else if(isdvd)
        titlebuffer << Icons::DVD(); // archive dvd
      else if(MoveCutterThread->IsCutting(filename))
        titlebuffer << Icons::Scissor(); // cutting recording
      else if(Recording->IsNew() && !mysetup.PatchNew)
        titlebuffer << '*';
      else if(!Recording->IsNew() && mysetup.PatchNew)
        titlebuffer << Icons::Continue(); // alternative to new marker / rewind / continue
      else titlebuffer << ' '; // no icon
     
      titlebuffer << '\t';
    
      // date and time of recording
      struct tm tm_r;
      struct tm *t=localtime_r(&Recording->start,&tm_r);
     
      if(mysetup.ShowRecDate)
        titlebuffer << setw(2) << setfill('0') << t->tm_mday << '.'
                    << setw(2) << setfill('0') << t->tm_mon+1 << '.'
                    << setw(2) << setfill('0') << t->tm_year%100 << '\t';
     
      if(mysetup.ShowRecTime)
        titlebuffer << setw(2) << setfill('0') << t->tm_hour << '.'
                    << setw(2) << setfill('0') << t->tm_min << '\t';
      
      
      idbuffer << t->tm_mday << t->tm_mon << t->tm_year
               << t->tm_hour << t->tm_min;

      // recording length
      if(mysetup.ShowRecLength)
      {
        buffer=filename;
#if VDRVERSNUM > 10703
        if (isPesRecording)
          buffer+="/index.vdr";
        else
          buffer+="/index";
#else
        buffer+="/index.vdr";
#endif
        struct stat statbuf;
        if(!stat(buffer.c_str(),&statbuf))
        {
          ostringstream strbuf;
          strbuf << setw(3) << (int)(statbuf.st_size/12000) << "'"; //TODO: 12000 still OK for TS recordings?
          // replace leading spaces with fixed blank (right align)
          titlebuffer << myStrReplace(strbuf.str(),' ',Icons::FixedBlank()) << '\t';
        }
        else
        {
          // get recording length from file 'length.vdr'
          buffer=filename;
          buffer+="/length.vdr";
 
          ifstream in(buffer.c_str());
          if(in)
          {
            if(!in.eof())
              getline(in,buffer);
          
            buffer+="'";
            // replace leading spaces with fixed blank (right align)
            while(buffer.length()<=3)
              buffer.insert(0,Icons::FixedBlank());

            titlebuffer << buffer << '\t';
        
            in.close();
          }
          else
            titlebuffer << '\t';
        }
      }
      if(!mysetup.ShowRecDate && !mysetup.ShowRecTime && !mysetup.ShowRecLength)
        titlebuffer << '\t';
    
      // recording title
      string _s=Recording->Name();
      string::size_type i=_s.rfind('~');
      if(i!=string::npos)
      {
        titlebuffer << _s.substr(i+1,_s.length()-i);
        idbuffer << _s.substr(i+1,_s.length()-i);
      }
      else
      {
        titlebuffer << _s;
        idbuffer << _s;
      }
    
      title=strdup(titlebuffer.str().c_str());
      uniqid=idbuffer.str();
    }
    else
    {
      if(Level>level) // any other
        title=strdup("");
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

  ostringstream entries;
  entries << setw(3) << totalentries;

  if(mysetup.ShowNewRecs)
  {
    asprintf(&buffer,"%s\t%s (%d)%s%s%s%s%s",
                     GetDirIsMoving()?Icons::MovingDirectory():Icons::Directory(),
                     // replace leading spaces with fixed blank (right align)
                     myStrReplace(entries.str(),' ',Icons::FixedBlank()).c_str(),
                     newentries,
                     (!mysetup.ShowRecDate&&!mysetup.ShowRecTime&&!mysetup.ShowRecLength)?"\t":"",
                     (mysetup.ShowRecDate||mysetup.ShowRecTime||mysetup.ShowRecLength)?"\t":"",
                     ((mysetup.ShowRecDate&&mysetup.ShowRecTime)||(mysetup.ShowRecTime&&mysetup.ShowRecLength)||(mysetup.ShowRecLength&&mysetup.ShowRecDate))?"\t":"",
                     (mysetup.ShowRecDate&&mysetup.ShowRecTime&&mysetup.ShowRecLength)?"\t":"",
                     name);
  }
  else
  {
    asprintf(&buffer,"%s\t%s%s%s%s%s%s",
                     GetDirIsMoving()?Icons::MovingDirectory():Icons::Directory(),
                     // replace leading spaces with fixed blank (right align)
                     myStrReplace(entries.str(),' ',Icons::FixedBlank()).c_str(),
                     (!mysetup.ShowRecDate&&!mysetup.ShowRecTime&&!mysetup.ShowRecLength)?"\t":"",
                     (mysetup.ShowRecDate||mysetup.ShowRecTime||mysetup.ShowRecLength)?"\t":"",
                     ((mysetup.ShowRecDate&&mysetup.ShowRecTime)||(mysetup.ShowRecTime&&mysetup.ShowRecLength)||(mysetup.ShowRecLength&&mysetup.ShowRecDate))?"\t":"",
                     (mysetup.ShowRecDate&&mysetup.ShowRecTime&&mysetup.ShowRecLength)?"\t":"",
                     name);
  }
  SetText(buffer,false);
}

// --- myMenuRecordings -------------------------------------------------------
#define MB_PER_MINUTE 25.75 // this is just an estimate! (taken over from VDR)

bool myMenuRecordings::golastreplayed=false;
bool myMenuRecordings::wasdvd;
dev_t myMenuRecordings::fsid=0;
int myMenuRecordings::freediskspace=0;

myMenuRecordings::myMenuRecordings(const char *Base,int Level):cOsdMenu("")
{
   // set tabs
   if(mysetup.ShowRecDate&&mysetup.ShowRecTime&&!mysetup.ShowRecLength) // recording date and time are shown, recording length not
     SetCols(2,8,6);
   else if(mysetup.ShowRecDate&&mysetup.ShowRecTime) // all details are shown
     SetCols(2,8,6,4);
   else if(mysetup.ShowRecDate&&!mysetup.ShowRecTime) // recording time is not shown
     SetCols(2,8,4);
   else if(!mysetup.ShowRecDate&&mysetup.ShowRecTime&&mysetup.ShowRecLength) // recording date is not shown
     SetCols(2,6,4);
   else // recording date and time are not shown; even if recording length should be not shown we must set two tabs because of the details of the directories
   {
     if(mysetup.ShowNewRecs)
       SetCols(2,8,3);
     else
       SetCols(2,4,3);
   }

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
}

int myMenuRecordings::FreeMB()
{
  if(mysetup.FileSystemFreeMB)
  {
    string path=VideoDirectory;
    path+="/";
    char *tmpbase=base?ExchangeChars(strdup(base),true):NULL;
    if(base)
      path+=tmpbase;

    struct stat statdir;
    if(!stat(path.c_str(),&statdir))
    {
      if(statdir.st_dev!=fsid)
      {
        fsid=statdir.st_dev;
    
        struct statvfs fsstat;
        if(!statvfs(path.c_str(),&fsstat))
        {
          freediskspace=int((double)fsstat.f_bavail/(1024.0*1024.0/fsstat.f_bsize));

          for(cRecording *rec=DeletedRecordings.First();rec;rec=DeletedRecordings.Next(rec))
          {
            if(!stat(rec->FileName(),&statdir))
            {
              if(statdir.st_dev==fsid)
                freediskspace+=DirSizeMB(rec->FileName());
            }
          } 
        }
        else
        {
          esyslog("[extrecmenu] error while getting filesystem size - statvfs (%s): %s",path.c_str(),strerror(errno));
          freediskspace=0;
        }
      }
    }
    else
    {
      esyslog("[extrecmenu] error while getting filesystem size - stat (%s): %s",path.c_str(),strerror(errno));
      freediskspace=0;
    }
    free(tmpbase);
  }
  else
  {
    int freemb;
    VideoDiskSpace(&freemb);
    return freemb;
  }

  return freediskspace;
}

void myMenuRecordings::Title()
{
  int freemb=FreeMB();
  int minutes=int(double(freemb)/MB_PER_MINUTE);

  stringstream buffer;
  if(MoveCutterThread->IsMoveListEmpty())
    buffer << Icons::MovingRecording();

  if(MoveCutterThread->IsCutterQueueEmpty())
    buffer << Icons::Scissor();

  if(MoveCutterThread->IsMoveListEmpty() || MoveCutterThread->IsCutterQueueEmpty())
    buffer << ' ';

  if(base)
    buffer << base;
  else
    buffer << trVDR("Recordings");
    
  buffer << " ("
         << minutes/60 << ":"
         << setw(2) << setfill('0') << minutes%60 << " "
         << trVDR("free")
         << ")";
  
  SetTitle(buffer.str().c_str());
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
      {
        if(item->GetDirIsMoving())
          newhelpkeys=6;
        else
          newhelpkeys=1;
      }
      else
      {
        newhelpkeys=2;
        cRecording *recording=GetRecording(item);
        if(recording)
        {
          if(mysetup.UseVDRsRecInfoMenu)
          {
            if(!recording->Info()->Title())
            {
              newhelpkeys=4;
              if(MoveCutterThread->IsMoving(recording->FileName()) || MoveCutterThread->IsCutting(recording->FileName()))
                newhelpkeys=5;
            }
          }
          if(MoveCutterThread->IsMoving(recording->FileName()) || MoveCutterThread->IsCutting(recording->FileName()))
            newhelpkeys=3;
        }
      }
      if(newhelpkeys!=helpkeys)
      {
        switch(newhelpkeys)
        {
          case 0: SetHelp(NULL);break;
          case 1: SetHelp(RecordingDirCommands.Count()?tr("Button$Commands"):tr("Button$Open"),NULL,tr("Button$Edit"));break;
          case 2: SetHelp(RecordingCommands.Count()?tr("Button$Commands"):tr("Button$Play"),tr("Button$Rewind"),tr("Button$Edit"),tr("Button$Info"));break;
          case 3: SetHelp(RecordingCommands.Count()?tr("Button$Commands"):tr("Button$Play"),tr("Button$Rewind"),tr("Button$Cancel"),tr("Button$Info"));break;
          case 4: SetHelp(RecordingCommands.Count()?tr("Button$Commands"):tr("Button$Play"),tr("Button$Rewind"),tr("Button$Edit"),NULL);break;
          case 5: SetHelp(RecordingCommands.Count()?tr("Button$Commands"):tr("Button$Play"),tr("Button$Rewind"),tr("Button$Cancel"),NULL);break;
          case 6: SetHelp(tr("Button$Open"),NULL,tr("Button$Cancel"));break;
        }
      }
      helpkeys=newhelpkeys;
    }
  }
}

// create the menu list
void myMenuRecordings::Set(bool Refresh,char *_current)
{
  const char *lastreplayed=_current?_current:myReplayControl::LastReplayed();

  cThreadLock RecordingsLock(&Recordings);

  if(Refresh && !_current)
  {
    fsid=0;
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
  string path=VideoDirectory;
  path+="/";
  if(base)
    path+=base;
  list->Sort(mySortList->Find(path));

  char *lastitemtext=NULL;
  myMenuRecordingsItem *lastitem=NULL;
  for(myRecListItem *listitem=list->First();listitem;listitem=list->Next(listitem))
  {
    cRecording *recording=listitem->recording;
    if(!base||(strstr(listitem->recording->Name(),base)==listitem->recording->Name()&&listitem->recording->Name()[strlen(base)]=='~'))
    {
      myMenuRecordingsItem *recitem=new myMenuRecordingsItem(listitem->recording,level);
#ifdef WITHPINPLUGIN
      bool hidepinprotectedrecs=false;
      cPlugin *pinplugin=cPluginManager::GetPlugin("pin");
      if(pinplugin)
        hidepinprotectedrecs=pinplugin->SetupParse("hideProtectedRecordings","1");

      if((*recitem->UniqID() && ((!lastitem || strcmp(recitem->UniqID(),lastitemtext)))) &&
         !((cStatus::MsgReplayProtected(GetRecording(recitem),recitem->Name(),base,recitem->IsDirectory(),true)) && hidepinprotectedrecs))
#else
      if(*recitem->UniqID() && ((!lastitem || strcmp(recitem->UniqID(),lastitemtext))))
#endif
      {
        Add(recitem);
        lastitem=recitem;
        free(lastitemtext);
        lastitemtext=strdup(lastitem->UniqID());
      }
      else
        delete recitem;

      if(lastitem)
      {
        if(!MoveCutterThread->IsMoving(recording->FileName()))
          lastitem->SetDirIsMoving(false);
        
        if(lastitem->IsDirectory())
          lastitem->IncrementCounter(recording->IsNew());
        if(lastreplayed && !strcmp(lastreplayed,recording->FileName()))
        {
          if(golastreplayed||Refresh)
          {
            SetCurrent(lastitem);
            if(recitem && !recitem->IsDirectory() && !cControl::Control() && !mysetup.GoLastReplayed)
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

  Title();
  if(Refresh)
    Display();
}

// returns the corresponding recording to an item
cRecording *myMenuRecordings::GetRecording(myMenuRecordingsItem *Item)
{
  cRecording *recording=Recordings.GetByName(Item->FileName());
  if(!recording)
    Skins.Message(mtError,trVDR("Error while accessing recording!"));
  return recording;
}

// opens a subdirectory
bool myMenuRecordings::Open()
{
  myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
  if(item && item->IsDirectory())
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
  const char *name=NULL;

  char path[MaxFileName];

  myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
  if(item)
  {
#ifdef WITHPINPLUGIN
    if(cStatus::MsgReplayProtected(GetRecording(item),item->Name(),base,item->IsDirectory())==true)
      return osContinue;
#endif
    if(item->IsDirectory())
      Open();
    else
    {
      cRecording *recording=GetRecording(item);
      if(recording)
      {
        if(item->IsDVD())
        {
          bool isvideodvd=false;
          char dvdnr[BUFSIZ];
          char *buffer=NULL;
          FILE *f;
      
          asprintf(&buffer,"%s/dvd.vdr",recording->FileName());
          if((f=fopen(buffer,"r"))!=NULL)
          {
            // get the dvd id
            if(fgets(dvdnr,sizeof(dvdnr),f))
            {
              char *p=strchr(dvdnr,'\n');
              if(p)
                *p=0;
            }
            // determine if dvd is a video dvd
            char tmp[BUFSIZ];
            if(fgets(tmp,sizeof(dvdnr),f))
            isvideodvd=true;
        
            fclose(f);
          }
          free(buffer);

          asprintf(&msg,tr("Please insert DVD %s"),dvdnr);
          if(Interface->Confirm(msg))
          {
            free(msg);
            // recording is a video dvd
            if(isvideodvd)
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
            // recording is an archive dvd
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
        cControl::Shutdown();
        isyslog("[extrecmenu] starting replay of recording");
        cControl::Launch(new myReplayControl());
        return osEnd;
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
#if VDRVERSNUM >= 10703
    cResumeFile ResumeFile(item->FileName(), item->IsPesRecording());
#else
    cResumeFile ResumeFile(item->FileName());
#endif
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
    if(Interface->Confirm(trVDR("Delete recording?")))
    {
      cRecordControl *rc=cRecordControls::GetRecordControl(item->FileName());
      if(rc)
      {
        if(Interface->Confirm(trVDR("Timer still recording - really delete?")))
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
          Skins.Message(mtError,trVDR("Error while deleting recording!"));
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
      return AddSubMenu(new myMenuRenameRecording(NULL,base,item->Name()));
    else
    {
      cRecording *recording=GetRecording(item);
      if(recording)
        return AddSubMenu(new myMenuRenameRecording(recording,NULL,NULL));
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
  if(item && !item->IsDirectory())
  {
    cRecording *recording=GetRecording(item);
#if VDRVERSNUM >= 10703
    if(recording && recording->IsPesRecording())
#else
    if(recording)
#endif
      return AddSubMenu(new myMenuRecordingDetails(recording));
  }
  return osContinue;
}

// moves a recording
eOSState myMenuRecordings::MoveRec()
{
  if(HasSubMenu() || Count()==0)
    return osContinue;

  myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
  if(item)
  {
    myMenuMoveRecording::clearall=false;
    if(item->IsDirectory())
      return AddSubMenu(new myMenuMoveRecording(NULL,base,item->Name()));
    else
    {
      cRecording *recording=GetRecording(item);
      if(recording)
        return AddSubMenu(new myMenuMoveRecording(recording,NULL,NULL));
    }
  }
  return osContinue;
}

// opens an info screen to a recording
eOSState myMenuRecordings::Info(void)
{
  if(HasSubMenu() || Count()==0)
    return osContinue;

  myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
  if(item && !item->IsDirectory())
  {
    cRecording *recording=GetRecording(item);
    if(mysetup.UseVDRsRecInfoMenu && (!recording || (recording && !recording->Info()->Title())))
      return osContinue;
    else
      return AddSubMenu(new myMenuRecordingInfo(recording,true));
  }
  return osContinue;
}

// execute a command for a recording
eOSState myMenuRecordings::Commands(eKeys Key)
{
  if(HasSubMenu() || Count()==0)
    return osContinue;
 
  myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
  if(item)
  {
    cRecording *recording=GetRecording(item);
    if(recording)
    {
      char *parameter=NULL;
      if (item->IsDirectory())
      {
        char *strBase=base?ExchangeChars(strdup(base), true):NULL;
        char *strName=ExchangeChars(strdup(item->Name()), true);
        asprintf(&parameter,"\"%s/%s/%s\"",VideoDirectory,strBase?strBase:"", strName);
        free(strBase);
        free(strName);
      }
      else
        asprintf(&parameter,"\"%s\"",recording->FileName());
      myMenuCommands *menu;
      eOSState state=AddSubMenu(menu=new myMenuCommands(trVDR("Recording commands"),item->IsDirectory() ? &RecordingDirCommands : &RecordingCommands,parameter));
      free(parameter);
      if(Key!=kNone)
        state=menu->ProcessKey(Key);
      return state;
    }
  }
  return osContinue;
}

// change sorting
eOSState myMenuRecordings::ChangeSorting()
{
  string path=VideoDirectory;
  path+="/";
  if(base)
    path+=base;

  for(SortListItem *item=mySortList->First();item;item=mySortList->Next(item))
  {
    if(path==item->Path())
    {
      mySortList->Del(item);
      mySortList->WriteConfigFile();
      Set(true);

      Skins.Message(mtInfo,tr("Sort by date"),1);

      return osContinue;
    }
  }
  mySortList->Add(new SortListItem(path)); 
  mySortList->WriteConfigFile();
  Set(true);

  Skins.Message(mtInfo,tr("Sort by name"),1);
 
  return osContinue;
}

eOSState myMenuRecordings::ProcessKey(eKeys Key)
{
  eOSState state;

  if(edit)
  {
    myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
    if(Key==kRed || Key==kGreen || Key==kYellow || (!item->IsDVD() && Key==kBlue) || Key==kBack)
    {
      edit=false;
      helpkeys=-1;
    }
    switch(Key)
    {
      case kRed: return Rename();
      case kGreen: return MoveRec();
      case kYellow: return Delete();
#if VDRVERSNUM >= 10703
      case kBlue: if(item&&!item->IsDVD()&&item->IsPesRecording())
#else
      case kBlue: if(item&&!item->IsDVD())
#endif
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
    state=cOsdMenu::ProcessKey(Key);
    if(state==osUnknown)
    {
      myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());

      switch(Key)
      {
        case kOk: return Play();
        case kRed: return (helpkeys>0 && item && ((item->IsDirectory() && RecordingDirCommands.Count()) || (!item->IsDirectory() && RecordingCommands.Count())))?Commands():Play();
        case kGreen: return Rewind();
        case kYellow: {
                        if(!HasSubMenu())
                        {
                          if(item)
                          {
                            if(item->IsDirectory())
                            {
                              if(item->GetDirIsMoving())
                              {
                                string path;
                                if(base)
                                {
                                  path=base;
                                  path+="~";
                                  path+=item->Name();
                                 }
                                 else
                                  path=item->Name();

                                if(Interface->Confirm(tr("Cancel moving?")))
                                {
                                  for(cRecording *rec=Recordings.First();rec;rec=Recordings.Next(rec))
                                  {
                                    if(!strncmp(path.c_str(),rec->Name(),path.length()))
                                      MoveCutterThread->CancelMove(rec->FileName());
                                  }
                                  Set(true);
                                }
                              }
                              else
                              {
                                edit=true;
                                SetHelp(tr("Button$Rename"),tr("Button$Move"));
                              }
                            }
                            else
                            {
                              cRecording *rec=GetRecording(item);
                              if(rec)
                              {
#ifdef WITHPINPLUGIN
                                if(cStatus::MsgReplayProtected(rec,item->Name(),base,item->IsDirectory())==true)
                                  break;
#endif
                                if(MoveCutterThread->IsCutting(rec->FileName()))
                                {
                                  if(Interface->Confirm(trVDR("Cancel editing?")))
                                  {
                                    MoveCutterThread->CancelCut(rec->FileName());
                                    Set(true);
                                  }
                                }
                                else if(MoveCutterThread->IsMoving(rec->FileName()))
                                {
                                  if(Interface->Confirm(tr("Cancel moving?")))
                                  {
                                    MoveCutterThread->CancelMove(rec->FileName());
                                    Set(true);
                                  }
                                }
                                else
                                {
                                  edit=true;
#if VDRVERSNUM >= 10703
                                  SetHelp(tr("Button$Rename"),tr("Button$Move"),tr("Button$Delete"),(item->IsPesRecording()&&!item->IsDVD())?tr("Details"):NULL);
#else
                                  SetHelp(tr("Button$Rename"),tr("Button$Move"),tr("Button$Delete"),(!item->IsDVD())?tr("Details"):NULL);
#endif
                                }
                              }
                            }
                          }
                        }
                      }
                      break;
        case kBlue: return Info();
        case k1...k9: return Commands(Key);
        case k0: return ChangeSorting();
        default: break;
      }
    }
    bool stateChanged = Recordings.StateChanged(recordingsstate);
    if(stateChanged || MoveCutterThread->IsCutterQueueEmpty())
      Set(true);    
    
    if(!Count() && level>0)
      state=osBack;
 
    if((!HasSubMenu() && Key!=kNone) || stateChanged)
      SetHelpKeys();
  }
  return state;
}
