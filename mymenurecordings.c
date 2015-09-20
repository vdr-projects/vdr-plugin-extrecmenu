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

#define DISKSPACECHEK     5 // seconds between disk space checks

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
    virtual  ~myMenuRecordingInfo(void);
#ifdef USE_GRAPHTFT
    virtual const char* MenuKind(){return "MenuExtRecording";}
#endif
};

myMenuRecordingInfo::myMenuRecordingInfo(const cRecording *Recording, bool WithButtons):cOsdMenu(trVDR("Recording info"))
{
#if VDRVERSNUM >= 10734
    if (mysetup.SetRecordingCat){
      SetMenuCategory(mcRecordingInfo);
    }
#endif
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
#if VDRVERSNUM > 10720
    time_t start = recording->Start();
#else
    time_t start = recording->start;
#endif
    text << *DateString(start) << ", " << *TimeString(start) << "\n\n";

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

#if VDRVERSNUM >= 20301
		LOCK_CHANNELS_READ
    const cChannel *channel=Channels->GetByChannelID(((cRecordingInfo*)recording->Info())->ChannelID());
#else
    cChannel *channel=Channels.GetByChannelID(((cRecordingInfo*)recording->Info())->ChannelID());
#endif
    if(channel)
      text << tr("Channel") << ": " << *ChannelString(channel,0) << "\n";

    int recmb=DirSizeMB(recording->FileName());
    if(recmb<0)
      recmb=0;
    if(recmb > 1023)
      text << tr("Size") << ": " << setprecision(3) << recmb/1024.0 << " GB\n";
    else
      text << tr("Size") << ": " << recmb << " MB\n";

#if VDRVERSNUM > 10720
    int prio = recording->Priority();
    int lft = recording->Lifetime();
#else
    int prio = recording->priority;
    int lft = recording->lifetime;
#endif
    text << trVDR("Priority") << ": " << prio << "\n";
    text << trVDR("Lifetime") << ": " << lft << "\n";

    DisplayMenu()->SetText(text.str().c_str(),false);
    cStatus::MsgOsdTextItem(text.str().c_str());
  }
}

eOSState myMenuRecordingInfo::ProcessKey(eKeys Key)
{
  switch((int)Key)
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

myMenuRecordingInfo::~myMenuRecordingInfo(void) {
}

// --- myMenuRecordingsItem ---------------------------------------------------
myMenuRecordingsItem::myMenuRecordingsItem(cRecording *Recording,int Level)
{
#if VDRVERSNUM >= 10733
  recording=Recording;
  recording_copy=NULL;
#endif
  totalentries=newentries=0;
  isdvd=false;
  ishdd=false;
  dirismoving=true;
  name=NULL;
  filename=Recording->FileName();
#if VDRVERSNUM > 10713
  isPesRecording=Recording->IsPesRecording();
#else
  isPesRecording=true;
#endif
	int savedlevel=Level;
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
    title=MALLOC(char,s-p+1);
    strn0cpy(title,p,s-p+1);
    name=strdup(title);
  }
  else
  {
    if(Level==level) // recording entry
    {
      string buffer;
      stringstream titlebuffer;
      stringstream idbuffer;

      // date and time of recording
      struct tm tm_r;
#if VDRVERSNUM > 10720
      time_t start = Recording->Start();
#else
      time_t start = Recording->start;
#endif
      struct tm *t=localtime_r(&start,&tm_r);

      // display symbol
      buffer=filename;
      if (isPesRecording)
        buffer+="/001.vdr";
      else
        buffer+="/00001.ts";

      if(access(buffer.c_str(),R_OK))
      {
        buffer=filename;
        buffer+="/dvd.vdr";
        isdvd=!access(buffer.c_str(),R_OK);
        buffer=filename;
        buffer+="/hdd.vdr";
        ishdd=!access(buffer.c_str(),R_OK);
      }

      if(MoveCutterThread->IsMoving(filename))
        titlebuffer << Icons::MovingRecording(); // moving recording
      else if(ishdd)
        titlebuffer << Icons::HDD(); // archive hdd
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


      // loop all columns and write each output to ostringstream
      for (int i=0; i<MAX_RECLIST_COLUMNS; i++) {
        ostringstream sbuffer;

        if(mysetup.RecListColumn[i].Type == COLTYPE_DATE) {
          sbuffer << setw(2) << setfill('0') << t->tm_mday << '.'
                  << setw(2) << setfill('0') << t->tm_mon+1 << '.'
                  << setw(2) << setfill('0') << t->tm_year%100;
        }

        if(mysetup.RecListColumn[i].Type == COLTYPE_TIME) {
          sbuffer << setw(2) << setfill('0') << t->tm_hour << '.'
                  << setw(2) << setfill('0') << t->tm_min;
        }

        if(mysetup.RecListColumn[i].Type == COLTYPE_DATETIME) {
          sbuffer << setw(2) << setfill('0') << t->tm_mday << '.'
                  << setw(2) << setfill('0') << t->tm_mon+1 << '.'
                  << setw(2) << setfill('0') << t->tm_year%100;
          sbuffer << Icons::FixedBlank();
          sbuffer << setw(2) << setfill('0') << t->tm_hour << '.'
                  << setw(2) << setfill('0') << t->tm_min;
        }

        if(mysetup.RecListColumn[i].Type == COLTYPE_LENGTH) {
          buffer=filename;
          if (isPesRecording)
            buffer+="/index.vdr";
          else
            buffer+="/index";

          struct stat statbuf;
          if(!stat(buffer.c_str(),&statbuf))
          {
#if APIVERSNUM >= 10714
            sbuffer << (int)(statbuf.st_size/480/Recording->FramesPerSecond()) << "'";
#else
            sbuffer << (int)(statbuf.st_size/12000) << "'";
#endif
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
              sbuffer << buffer << "'";
              in.close();
            }
          }
        }

        if(mysetup.RecListColumn[i].Type == COLTYPE_RATING) {
          // get recording rating from file 'rated.vdr'
          buffer=filename;
          buffer+="/rated.vdr";

          ifstream in(buffer.c_str());
          if(in)
          {
            if(!in.eof())
              getline(in,buffer);
            int rated=atoi(buffer.c_str());
            if (rated>10)
              rated=10;

            if (rated>0) {
              while (rated>1) {
                sbuffer << Icons::StarFull();
                rated = rated-2;
              }
              if (rated>0) {
                sbuffer << Icons::StarHalf();
                rated--;
              }
            }
            in.close();
          }
        }

        if(mysetup.RecListColumn[i].Type == COLTYPE_FILE ||
           mysetup.RecListColumn[i].Type == COLTYPE_FILETHENCOMMAND) {
          // get content from file
          buffer=filename;
          buffer+="/";
          buffer+=mysetup.RecListColumn[i].Op1;

          ifstream in(buffer.c_str());
          if(in)
          {
            if(!in.eof())
              getline(in,buffer);

            // cut to maximum width
            buffer = buffer.substr(0, mysetup.RecListColumn[i].Width);
            sbuffer << buffer;
            in.close();
          }
          else
          {
            if(mysetup.RecListColumn[i].Type == COLTYPE_FILETHENCOMMAND) {
              // execute the command given by Op2
              char result [1024];
              strcpy(result, mysetup.RecListColumn[i].Op2);
              strcat(result, " \"");
              strcat(result, filename);
              strcat(result, "\"");
              FILE *fp = popen(result, "r");
              int read = fread(result, 1, sizeof(result), fp);
              pclose (fp);

              if(read>0) {
                // use output of command
                // strip trailing whitespaces
                result[read]=0;
                while (strlen(result)>0 && 
                       (result[strlen(result)-1]==0x0a || result[strlen(result)-1]==0x0d || result[strlen(result)-1]==' ')) {
                  result[strlen(result)-1]=0;
                }
                result[mysetup.RecListColumn[i].Width]=0;
                sbuffer << result;
              } else {
                // retry reading the file (useful when the execution of the command created the file)
                buffer=filename;
                buffer+="/";
                buffer+=mysetup.RecListColumn[i].Op1;

                ifstream in(buffer.c_str());
                if(in)
                {
                  if(!in.eof())
                    getline(in,buffer);

                  // cut to maximum width
                  buffer = buffer.substr(0, mysetup.RecListColumn[i].Width);
                  sbuffer << buffer;
                  in.close();
                }
              }
            }
          }
        }

        // adjust alignment
        int iLeftBlanks=0;
        switch (mysetup.RecListColumn[i].Align) {
          case 1:
            // center alignment
            iLeftBlanks = (mysetup.RecListColumn[i].Width - strlen(sbuffer.str().c_str())) / 2; // sbuffer.width()) / 2;
            break;
          case 2:
            // right alignment
            iLeftBlanks = (mysetup.RecListColumn[i].Width - strlen(sbuffer.str().c_str())); // sbuffer.width());
            break;
          default:
            // left alignment
            break;
        }

        if(iLeftBlanks>0) {
          for (int j=0; j<iLeftBlanks; j++) {
            titlebuffer << Icons::FixedBlank();
          }
        }

        titlebuffer << sbuffer.str() << '\t';
      } // loop all columns

      // recording title
      string _s=Recording->Name();
      string::size_type i=_s.rfind('~');
      if(i!=string::npos)
      {
        titlebuffer << _s.substr(i+1,_s.length()-i);
      }
      else
      {
        titlebuffer << _s;
      }

      title=strdup(titlebuffer.str().c_str());
    }
    else
    {
      if(Level>level) // any other
        title=strdup("");
    }
  }
	level=savedlevel;
  SetText(title);
}

myMenuRecordingsItem::~myMenuRecordingsItem()
{
#if VDRVERSNUM >= 10733
  delete recording_copy;
#endif
  free(title);
  free(name);
}

#if VDRVERSNUM >= 10733
void myMenuRecordingsItem::rec_copy(void)
{
  if(mysetup.SetRecordingCat)
    recording_copy=new cRecording(recording->FileName());
  else
    recording_copy=NULL;
}
#endif

void myMenuRecordingsItem::IncrementCounter(bool IsNew)
{
  totalentries++;
  if(IsNew)
    newentries++;

  char *buffer=NULL;

  ostringstream entries;
  entries << setw(mysetup.RecsPerDir+1) << totalentries;

  if(mysetup.ShowNewRecs)
  {
    if(-1==asprintf(&buffer,"%s\t%s\t(%d)\t\t\t%s",
                            GetDirIsMoving()?Icons::MovingDirectory():Icons::Directory(),
                            // replace leading spaces with fixed blank (right align)
                            myStrReplace(entries.str(),' ',Icons::FixedBlank()).c_str(),
                            newentries,
                            name))
      buffer=NULL;
  }
  else
  {
    if(-1==asprintf(&buffer,"%s\t%s\t\t\t\t%s",
                            GetDirIsMoving()?Icons::MovingDirectory():Icons::Directory(),
                            // replace leading spaces with fixed blank (right align)
                            myStrReplace(entries.str(),' ',Icons::FixedBlank()).c_str(),
                            name))
      buffer=NULL;
  }
  if(buffer)
    SetText(buffer,false);
}

#if VDRVERSNUM >= 10733
void myMenuRecordingsItem::SetMenuItem(cSkinDisplayMenu *displaymenu,int index,bool current,bool selectable)
{
#if VDRVERSNUM >= 10733
  if (!(recording_copy && displaymenu->SetItemRecording(recording_copy,index,current,selectable,level,totalentries,newentries)))
#endif
    displaymenu->SetItem(Text(),index,current,selectable);
}
#endif

// --- myMenuRecordings -------------------------------------------------------
#define MB_PER_MINUTE 25.75 // this is just an estimate! (taken over from VDR)

bool myMenuRecordings::wasdvd;
bool myMenuRecordings::washdd;
dev_t myMenuRecordings::fsid=0;
time_t myMenuRecordings::lastDiskSpaceCheck=0;
int myMenuRecordings::lastFreeMB=-1;

myMenuRecordings::myMenuRecordings(const char *Base,int Level):cOsdMenu("")
{
#if VDRVERSNUM >= 10728
  if(mysetup.SetRecordingCat){
    SetMenuCategory(mcRecording);
  }
#endif
  int c[MAX_RECLIST_COLUMNS],i=0;

  for (i=0; i<MAX_RECLIST_COLUMNS; i++) {
    c[i] = 1;
    if ((mysetup.RecListColumn[i].Type != COLTYPE_NONE) &&
        (mysetup.RecListColumn[i].Width > 0)) {
      c[i] = mysetup.RecListColumn[i].Width+1;
    }
  }

  // widen the first column if there isn't enough space for the number of recordings in a direcory
  if (c[0] < mysetup.RecsPerDir+1) {
    c[0] = mysetup.RecsPerDir+1;
  }
  // widen the second column if the number of new recordings should be displayed and
  // there isn't enough space for the number of new recordings in a direcory
  if (mysetup.ShowNewRecs && c[1] < mysetup.RecsPerDir+3) {
    c[1] = mysetup.RecsPerDir+3;
  }

  SetCols(2, c[0], c[1], c[2], c[3]);

  edit=false;
  level=Level;
  helpkeys=-1;
  base=Base?strdup(Base):NULL;

#if VDRVERSNUM >= 20301
	//TODO???
#else
  Recordings.StateChanged(recordingsstate);
#endif

  //Display();

  if(wasdvd&&!cControl::Control())
  {
    char *cmd=NULL;
    if(-1!=asprintf(&cmd,"dvdarchive.sh umount \"%s\"",*strescape(myReplayControl::LastReplayed(),"'\\\"$")))
    {
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
    }

    wasdvd=false;
  }
  if(washdd&&!cControl::Control())
  {
    char *cmd=NULL;
    if(-1!=asprintf(&cmd,"hddarchive.sh umount \"%s\"",*strescape(myReplayControl::LastReplayed(),"'\\\"$")))
    {
      isyslog("[extrecmenu] calling %s to unmount Archive-HDD",cmd);
      int result=SystemExec(cmd);
      if(result)
      {
        result=result/256;
        if(result==1)
          Skins.Message(mtError,tr("Error while mounting Archive-HDD!"));
      }
      isyslog("[extrecmenu] hddarchive.sh returns %d",result);
      free(cmd);
    }

    washdd=false;
  }

  Set();

  if((mysetup.GoLastReplayed || mysetup.ReturnToRec) && myReplayControl::LastReplayed())
    Open();
  mysetup.ReturnToRec = false;

  Display();
  SetHelpKeys();
}

myMenuRecordings::~myMenuRecordings()
{
  free(base);
}

int myMenuRecordings::FreeMB()
{
  if (lastFreeMB<=0||time(NULL)-lastDiskSpaceCheck>DISKSPACECHEK)
  {
    int freediskspace=0;
    if(mysetup.FileSystemFreeMB)
    {
#if APIVERSNUM > 20101
      string path=cVideoDirectory::Name();
#else
      string path=VideoDirectory;
#endif
      path+="/";
      char *tmpbase=base?ExchangeChars(strdup(base),true):NULL;
      if(tmpbase)
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
            freediskspace=int((double)fsstat.f_bavail/(double)(1024.0*1024.0/fsstat.f_bsize));

#if VDRVERSNUM >= 20301
						LOCK_DELETEDRECORDINGS_READ
            for(const cRecording *rec=DeletedRecordings->First();rec;rec=DeletedRecordings->Next(rec))
#else
            for(cRecording *rec=DeletedRecordings.First();rec;rec=DeletedRecordings.Next(rec))
#endif
            {
              if(!stat(rec->FileName(),&statdir))
              {
                if(statdir.st_dev==fsid)
                {
                  int ds=DirSizeMB(rec->FileName());
                  if(ds>0)
                    freediskspace+=ds;
                  else
                    esyslog("[extrecmenu] DirSizeMB(%s) failed!", rec->FileName());
                }
              }
            }
          }
          else
          {
            esyslog("[extrecmenu] error while getting filesystem size - statvfs (%s): %s",path.c_str(),strerror(errno));
            freediskspace=0;
          }
        }
        else
        {
          freediskspace=lastFreeMB;
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
#if APIVERSNUM > 20101
      cVideoDirectory::VideoDiskSpace(&freediskspace);
#else
      VideoDiskSpace(&freediskspace);
#endif
    }
    lastFreeMB=freediskspace;
    lastDiskSpaceCheck=time(NULL);
  }

  return lastFreeMB;
}

void myMenuRecordings::Title()
{
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

#if VDRVERSNUM >= 10728
  if(mysetup.SetRecordingCat == 0){
#endif
    int freemb=FreeMB();
#if VDRVERSNUM >= 10727
# if VDRVERSNUM >= 20301
		LOCK_RECORDINGS_READ
    double MBperMinute = Recordings->MBperMinute();
# else
    double MBperMinute = Recordings.MBperMinute();
# endif
    int minutes=int(double(freemb)/(MBperMinute>0?MBperMinute:MB_PER_MINUTE));
#else
    int minutes=int(double(freemb)/MB_PER_MINUTE);
#endif

    buffer << " ("
           << minutes/60 << ":"
           << setw(2) << setfill('0') << minutes%60 << " "
           << trVDR("free")
           << ")";
#if VDRVERSNUM >= 10728
  }
#endif

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
void myMenuRecordings::Set(bool Refresh)
{
  const char *lastreplayed=myReplayControl::LastReplayed();

#if VDRVERSNUM >= 20301
	LOCK_RECORDINGS_READ
#else
  cThreadLock RecordingsLock(&Recordings);
#endif
  if(Refresh)
  {
    fsid=0;
    myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
    if(item)
    {
#if VDRVERSNUM >= 20301
      const cRecording *recording=Recordings->GetByName(item->FileName());
#else
      cRecording *recording=Recordings.GetByName(item->FileName());
#endif
      if(recording)
        lastreplayed=recording->FileName();
    }
  }

  Clear();

  // create my own recordings list from VDR's
  myRecList *list=new myRecList();
#if VDRVERSNUM >= 20301
  for(const cRecording *recording=Recordings->First();recording;recording=Recordings->Next(recording))
#else
  for(cRecording *recording=Recordings.First();recording;recording=Recordings.Next(recording))
#endif
    list->Add(new myRecListItem(recording));
  // sort my recordings list
#if APIVERSNUM > 20101
  string path=cVideoDirectory::Name();
#else
  string path=VideoDirectory;
#endif
  path+="/";
  if(base)
    path+=base;
  list->Sort(mySortList->Find(path));

#ifdef USE_PINPLUGIN
   bool hidepinprotectedrecs=false;
   cPlugin *pinplugin=cPluginManager::GetPlugin("pin");
   if(pinplugin)
     hidepinprotectedrecs=pinplugin->SetupParse("hideProtectedRecordings","1");
#endif

  myMenuRecordingsItem *lastitem=NULL;
  for(myRecListItem *listitem=list->First();listitem;listitem=list->Next(listitem))
  {
    cRecording *recording=listitem->recording;
    if(!base||(strstr(recording->Name(),base)==recording->Name()&&recording->Name()[strlen(base)]=='~'))
    {
      myMenuRecordingsItem *recitem=new myMenuRecordingsItem(recording,level);
      myMenuRecordingsItem *lastdir=NULL;
      if(recitem->IsDirectory())
      {
        // Sorting may ignore non-alphanumeric characters, so we need to explicitly handle directories in case they only differ in such characters:
        for(myMenuRecordingsItem *p=lastitem;p;p=dynamic_cast<myMenuRecordingsItem *>(p->Prev()))
        {
          if(p->Name()&&strcmp(p->Name(),recitem->Name())==0)
          {
            lastdir=p;
            break;
          }
        }
      }
      if(*recitem->Text()&&!lastdir)
      {
#ifdef USE_PINPLUGIN
        if(!(hidepinprotectedrecs && cStatus::MsgReplayProtected(recording,recitem->Name(),base,recitem->IsDirectory(),true)))
        {
#endif
#if VDRVERSNUM >= 10733
          recitem->rec_copy();
#endif
          Add(recitem);
          lastitem=recitem;
          if(recitem->IsDirectory())
            lastdir=recitem;
#ifdef USE_PINPLUGIN
        }
        else
          lastitem=NULL;
#endif
      }
      else
      {
        delete recitem;
        recitem=NULL;
      }

      if(lastitem||lastdir)
      {
        if(lastitem)
        {
          if(!MoveCutterThread->IsMoving(recording->FileName()))
            lastitem->SetDirIsMoving(false);
        }

        if(lastreplayed&&strcmp(lastreplayed,recording->FileName())==0)
        {
          SetCurrent(lastdir?lastdir:lastitem);
          if(recitem&&!recitem->IsDirectory()&&(recitem->IsDVD()||recitem->IsHDD())&&!cControl::Control())
            cReplayControl::ClearLastReplayed(cReplayControl::LastReplayed());
        }

        if(lastdir){
          lastdir->IncrementCounter(recording->IsNew());
        }
      }
    }
  }
  delete list;

  ForceFreeMbUpdate();
  Title();
  if(Refresh)
    Display();
}

// returns the corresponding recording to an item
cRecording *myMenuRecordings::GetRecording(myMenuRecordingsItem *Item)
{
#if VDRVERSNUM >= 20301
  LOCK_RECORDINGS_READ
  cRecording *recording=(cRecording *)Recordings->GetByName(Item->FileName());
#else
  cRecording *recording=Recordings.GetByName(Item->FileName());
#endif
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
      if(-1==asprintf(&buffer,"%s~%s",base,t))
        buffer=NULL;
      t=buffer;
    }
    if(t)
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
#ifdef USE_PINPLUGIN
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
        if(item->IsHDD())
        {
          char hddnr[BUFSIZ];
          char *buffer=NULL;
          FILE *f;

          if(-1!=asprintf(&buffer,"%s/hdd.vdr",recording->FileName()))
          {
            if((f=fopen(buffer,"r"))!=NULL)
            {
              // get the hdd id
              if(fgets(hddnr,sizeof(hddnr),f))
              {
                char *p=strchr(hddnr,'\n');
                if(p)
                  *p=0;
              }
              fclose(f);
            }
            free(buffer);
            buffer=NULL;

            if(-1!=asprintf(&msg,tr("Please attach Archive-HDD %s"),hddnr))
            {
              if(Interface->Confirm(msg))
              {
                free(msg);
                // recording is an archive hdd
                strcpy(path,recording->FileName());
                name=strrchr(path,'/')+1;
                if(-1!=asprintf(&msg,"hddarchive.sh mount \"%s\" '%s'",*strescape(path,"'"),*strescape(name,"'\\\"$")))
                {
                  isyslog("[extrecmenu] calling %s to mount Archive-HDD",msg);
                  int result=SystemExec(msg);
                  isyslog("[extrecmenu] hddarchive.sh returns %d",result);
                  free(msg);
                  msg=NULL;
                  if(result)
                  {
                    result=result/256;
                    if(result==1)
                      Skins.Message(mtError,tr("Error while mounting Archive-HDD!"));
                    if(result==3)
                      Skins.Message(mtError,tr("Recording not found on Archive-HDD!"));
                    if(result==4)
                      Skins.Message(mtError,tr("Error while linking [0-9]*.vdr!"));
                    if(result==5)
                      Skins.Message(mtError,tr("sudo or mount --bind / umount error (vfat system)"));
                    if(result==127)
                      Skins.Message(mtError,tr("Script 'hddarchive.sh' not found!"));
                    return osContinue;
                  }
                  washdd=true;
                }
                msg=NULL;
              }
              else
              {
                free(msg);
                return osContinue;
              }
            }
            msg=NULL;
          }
          buffer=NULL;
        } else if(item->IsDVD())
        {
          bool isvideodvd=false;
          char dvdnr[BUFSIZ];
          char *buffer=NULL;
          FILE *f;

          if(-1!=asprintf(&buffer,"%s/dvd.vdr",recording->FileName()))
          {
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
            buffer=NULL;

            if(-1!=asprintf(&msg,tr("Please insert DVD %s"),dvdnr))
            {
              if(Interface->Confirm(msg))
              {
                free(msg);
                msg=NULL;
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
                  if(-1!=asprintf(&msg,"dvdarchive.sh mount \"%s\" '%s'",*strescape(path,"'"),*strescape(name,"'\\\"$")))
                  {
                    isyslog("[extrecmenu] calling %s to mount dvd",msg);
                    int result=SystemExec(msg);
                    isyslog("[extrecmenu] dvdarchive.sh returns %d",result);
                    free(msg);
                    msg=NULL;
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
                  msg=NULL;
                }
              }
              else
              {
                free(msg);
                return osContinue;
              }
            }
            msg=NULL;
          }
          buffer=NULL;
        }
#if VDRVERSNUM >= 10728
        myReplayControl::SetRecording(recording->FileName());
#else
        myReplayControl::SetRecording(recording->FileName(),recording->Title());
#endif
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
#if VDRVERSNUM > 10713
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
#if VDRVERSNUM >= 20301
						LOCK_TIMERS_WRITE
            cRecordControls::Process(Timers,time(NULL));
#else
            cRecordControls::Process(time(NULL));
#endif
            if(timer->IsSingleEvent())
            {
              isyslog("deleting timer %s",*timer->ToDescr());
#if VDRVERSNUM >= 20301
              Timers->Del(timer);
#else
              Timers.Del(timer);
#endif
            }
#if VDRVERSNUM >= 20301
            Timers->SetModified();
#else
            Timers.SetModified();
#endif
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
#if VDRVERSNUM >= 20301
          LOCK_RECORDINGS_WRITE
          Recordings->DelByName(item->FileName());
#else
          Recordings.DelByName(item->FileName());
#endif
          cOsdMenu::Del(Current());
          SetHelpKeys();
          ForceFreeMbUpdate();
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
    if(recording)
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
#if APIVERSNUM > 20101
        if(-1==asprintf(&parameter,"\"%s/%s/%s\"",cVideoDirectory::Name(),strBase?strBase:"", strName))
#else
        if(-1==asprintf(&parameter,"\"%s/%s/%s\"",VideoDirectory,strBase?strBase:"", strName))
#endif
          parameter=NULL;
        free(strBase);
        free(strName);
      }
      else
      {
        if(-1==asprintf(&parameter,"\"%s\"",recording->FileName()))
          parameter=NULL;
      }
      myMenuCommands *menu;
      eOSState state=AddSubMenu(menu=new myMenuCommands(trVDR("Recording commands"),item->IsDirectory() ? &RecordingDirCommands : &RecordingCommands,parameter?parameter:""));
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
#if APIVERSNUM > 20101
  string path=cVideoDirectory::Name();
#else
  string path=VideoDirectory;
#endif
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
    if(Key==kRed || Key==kGreen || Key==kYellow || (!item->IsDVD() && !item->IsHDD() && Key==kBlue) || Key==kBack)
    {
      edit=false;
      helpkeys=-1;
    }
    switch(Key)
    {
      case kRed: return Rename();
      case kGreen: return MoveRec();
      case kYellow: return Delete();
      case kBlue: if(item&&!item->IsDVD()&&!item->IsHDD())
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
        case kPlay:
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
#if VDRVERSNUM >= 20301
																	LOCK_RECORDINGS_READ
                                  for(const cRecording *rec=Recordings->First();rec;rec=Recordings->Next(rec))
#else
                                  for(cRecording *rec=Recordings.First();rec;rec=Recordings.Next(rec))
#endif
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
#ifdef USE_PINPLUGIN
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
                                  SetHelp(tr("Button$Rename"),tr("Button$Move"),tr("Button$Delete"),!item->IsDVD()&&!item->IsHDD()?tr("Details"):NULL);
                                }
                              }
                            }
                          }
                        }
                      }
                      break;
        case kInfo:
        case kBlue: return Info();
        case k1...k9: return Commands(Key);
        case k0: return ChangeSorting();
        default: break;
      }
    }
#if VDRVERSNUM >= 20301
    //TODO???
    Set(true);
#else
    bool stateChanged = Recordings.StateChanged(recordingsstate);
    if(stateChanged || MoveCutterThread->IsCutterQueueEmpty())
      Set(true);
#endif

    if(!Count() && level>0)
      state=osBack;

#if VDRVERSNUM < 20301
    if((!HasSubMenu() && Key!=kNone) || stateChanged)
#endif
      SetHelpKeys();
  }
  return state;
}
