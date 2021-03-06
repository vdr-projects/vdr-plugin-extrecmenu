/*
 * See the README file for copyright information and how to reach the author.
 */

#include <langinfo.h>
#include <string>
#include <fstream>
#include <iostream>
#include <vdr/plugin.h>
#include <vdr/videodir.h>
#include <vdr/recording.h>
#include <vdr/cutter.h>
#include "tools.h"
#include "mymenusetup.h"

using namespace std;
extern bool VfatFileSytem;

#define CONFIGFILE "/extrecmenu.sort.conf"
#define BUFFERSIZE 20972 // (2*1024*1024)/100

SortList *mySortList;
WorkerThread *MoveCutterThread;

string myStrEscape(string S,const char *Chars)
{
  int i=0;
  while(Chars[i]!=0)
  {
    string::size_type j=0;
    while((j=S.find(Chars[i],j))!=string::npos)
    {
      S.insert(j,1,'\\');
      j+=2;
     }
     i++;
  }
  return S;
}

string myStrReplace(string S,char C1,const char* C2)
{
  string::size_type i=0;
  while((i=S.find(C1,i))!=string::npos)
  {
    S.replace(i,1,C2);
    i++;
  }
  return S;
}

// --- SortList ---------------------------------------------------------------
void SortList::ReadConfigFile()
{
  string configfile(cPlugin::ConfigDirectory());
  configfile+=CONFIGFILE;

  ifstream in(configfile.c_str());
  if(in)
  {
    string buf;
    while(!in.eof())
    {
      getline(in,buf);
      if(buf.length()>0)
        Add(new SortListItem(buf));
    }
  }
}

void SortList::WriteConfigFile()
{
  string configfile(cPlugin::ConfigDirectory());
  configfile+=CONFIGFILE;

  ofstream outfile(configfile.c_str());

  for(SortListItem *item=First();item;item=Next(item))
    outfile << item->Path() << endl;
}

bool SortList::Find(string Path)
{
  for(SortListItem *item=First();item;item=Next(item))
  {
   if(item->Path()==Path)
     return true;
  }
  return false;
}

// --- MoveRename -------------------------------------------------------------
// creates the necassery directories and renames the given old name to the new name
bool MoveRename(const char *OldName,const char *NewName,cRecording *Recording,bool Move)
{
  char *buf=NULL;

  if(!strcmp(OldName,NewName))
    return true;

  if(Recording)
  {
    isyslog("[extrecmenu] moving %s to %s",OldName,NewName);

    if(!MakeDirs(NewName,true))
    {
      Skins.Message(mtError,tr("Creating directories failed!"));
      return false;
    }

    if(rename(OldName,NewName)==-1)
    {
      remove(NewName); // remove created directory
      Skins.Message(mtError,tr("Rename/Move failed!"));
      esyslog("[extrecmenu] MoveRename() - rename() - %s",strerror(errno));
      return false;
    }

#if VDRVERSNUM >= 20301
    LOCK_RECORDINGS_WRITE
    Recordings->DelByName(OldName);
    Recordings->AddByName(NewName);
#else
    cThreadLock RecordingsLock(&Recordings);
    Recordings.DelByName(OldName);
    Recordings.AddByName(NewName);
#endif

    // set user command for '-r'-option of VDR
    if(-1!=asprintf(&buf,"%s \"%s\"",Move?"move":"rename",*strescape(OldName,"'\\\"$")))
    {
      cRecordingUserCommand::InvokeCommand(buf,NewName);
      free(buf);
    }
    buf=NULL;
  }
  else
  {
    // is the new path within the old?
    if(-1!=asprintf(&buf,"%s/",OldName)) // we have to append a / to make sure that we search for a directory
    {
      if(!strncmp(buf,NewName,strlen(buf)))
      {
        Skins.Message(mtError,tr("Moving into own sub-directory not allowed!"));
        free(buf);
        return false;
      }
      free(buf);
    }
    buf=NULL;

    myRecList *list=new myRecList();
#if VDRVERSNUM >= 20301
    LOCK_RECORDINGS_READ
    for(const cRecording *recording=Recordings->First();recording;recording=Recordings->Next(recording))
#else
    for(cRecording *recording=Recordings.First();recording;recording=Recordings.Next(recording))
#endif
      list->Add(new myRecListItem(recording));

    myRecListItem *item=list->First();
    while(item)
    {
      if(!strncmp(OldName,item->recording->FileName(),strlen(OldName)))
      {
#if APIVERSNUM > 20101
        buf=strdup(OldName+strlen(cVideoDirectory::Name())+1);
#else
        buf=strdup(OldName+strlen(VideoDirectory)+1);
#endif
        if(buf)
        {
          buf=ExchangeChars(buf,false);

          if(strcmp(item->recording->Name(),buf))
          {
            free(buf);
            if(-1!=asprintf(&buf,"%s%s",NewName,item->recording->FileName()+strlen(OldName)))
            {
              if(!MakeDirs(buf,true))
              {
                Skins.Message(mtError,tr("Creating directories failed!"));
                free(buf);
                delete list;
                return false;
              }
              if(MoveRename(item->recording->FileName(),buf,item->recording,Move)==false)
              {
                free(buf);
                delete list;
                return false;
              }
            }
            buf=NULL;
          }
          free(buf);
        }
      }
      item=list->Next(item);
    }
    delete list;
  }
  return true;
}

// --- myRecListItem ----------------------------------------------------------
bool myRecListItem::SortByName=false;

#if VDRVERSNUM >= 20301
myRecListItem::myRecListItem(const cRecording *Recording)
#else
myRecListItem::myRecListItem(cRecording *Recording)
#endif
{
  recording=(cRecording *)Recording;
  filename=strdup(recording->FileName());
  sortBufferName = sortBufferTime = NULL;
}

myRecListItem::~myRecListItem()
{
  free(sortBufferName);
  free(sortBufferTime);
  free(filename);
}

char *myRecListItem::StripEpisodeName(char *s)
{
  char *t=s,*s1=NULL,*s2=NULL;
  while(*t)
  {
    if(*t=='/')
    {
      if(s1)
      {
        if(s2)
          s1=s2;
        s2=t;
      }
      else
        s1=t;
    }
    t++;
  }
  if(s1&&s2)
  {
    // To have folders sorted before plain recordings, the '/' s1 points to
    // is replaced by the character '1'. All other slashes will be replaced
    // by '0' in SortName() (see below), which will result in the desired
    // sequence:
    *s1=mysetup.DescendSorting ? '0' : '1';
    if(!SortByName)
    {
      s1++;
      memmove(s1,s2,t-s2+1);
    }
  }
  return s;
}

char *myRecListItem::SortName(void) const
{
  char **sb=SortByName?&sortBufferName:&sortBufferTime;
  if(!*sb)
  {
#if APIVERSNUM > 20101
    char *s=StripEpisodeName(strdup(recording->FileName()+strlen(cVideoDirectory::Name())));
#else
    char *s=StripEpisodeName(strdup(recording->FileName()+strlen(VideoDirectory)));
#endif
    strreplace(s,'/',mysetup.DescendSorting ? '1' : '0'); // some locales ignore '/' when sorting
    int l=strxfrm(NULL,s,0)+1;
    *sb=MALLOC(char,l);
    strxfrm(*sb,s,l);
    free(s);
  }
  return *sb;
}

int myRecListItem::Compare(const cListObject &ListObject)const
{
  myRecListItem *r=(myRecListItem*)&ListObject;

  if(mysetup.DescendSorting)
    return strcasecmp(r->SortName(),SortName());
  else
    return strcasecmp(SortName(),r->SortName());
}

// --- myRecList --------------------------------------------------------------
void myRecList::Sort(bool SortByName)
{
 myRecListItem::SortByName=SortByName;
 cListBase::Sort();
}

// --- WorkerThread -----------------------------------------------------------
WorkerThread::WorkerThread():cThread("extrecmenu worker thread")
{
  cancelmove=cancelcut=false;
  CutterQueue=new CutterList();
  MoveBetweenFileSystemsList=new MoveList();

  Start();
}

WorkerThread::~WorkerThread()
{
  Cancel(3);

  delete CutterQueue;
  delete MoveBetweenFileSystemsList;
}

const char *WorkerThread::Working()
{
  if(CutterQueue->First()!=NULL)
    return tr("Cutter queue not empty");

  if(MoveBetweenFileSystemsList->First()!=NULL)
    return tr("Move recordings in progress");

  return NULL;
}

void WorkerThread::Action()
{
  CutterListItem *cutteritem=NULL;
  MoveListItem *moveitem=NULL;

  SetPriority(19);
#if VDRVERSNUM >= 10706
	SetIOPriority(7);
#endif

  while(Running())
  {
    if((cutteritem=CutterQueue->First())!=NULL)
    {
      cutteritem->SetCutInProgress();

#ifdef USE_VDR_CUTTER
#if VDRVERSNUM > 10713
      if(!CutRecording(cutteritem->FileName().c_str()))
#else
      if (!cCutter::Start(cutteritem->FileName().c_str()))
#endif
        Skins.QueueMessage(mtError,tr("Can't start editing process!"));
#else
      // create filename for edited recording, check for recordings with this name, if exists -> delete recording
      // (based upon VDR's code (cutter.c))
      cRecording rec(cutteritem->FileName().c_str());
      const char *editedfilename=rec.PrefixFileName('%');
      if(editedfilename && RemoveVideoFile(editedfilename) && MakeDirs(editedfilename,true))
      {
        char *s=strdup(editedfilename);
        char *e=(char*)strrchr(s,'.'); //TODO
        if(e)
        {
          if(!strcmp(e,".rec"))
          {
            strcpy(e,".del");
            RemoveVideoFile(s);
          }
        }
        free(s);
        rec.WriteInfo(); // don't know why, but VDR also does it
        Recordings.AddByName(editedfilename);
        cutteritem->SetNewFileName(editedfilename);
        Cut(cutteritem->FileName(),editedfilename);
      }
      else
        Skins.QueueMessage(mtError,tr("Can't start editing process!"));
#endif
      CutterQueue->Del(cutteritem);

#if VDRVERSNUM >= 20301
	//TODO???
#else
      Recordings.ChangeState();
#endif
    }

    if((moveitem=MoveBetweenFileSystemsList->First())!=NULL)
    {
      moveitem->SetMoveInProgress();
      if(Move(moveitem->From(),moveitem->To()))
        MoveBetweenFileSystemsList->Del(moveitem);
      else
        // error occured -> empty move queue
        MoveBetweenFileSystemsList->Clear();

#if VDRVERSNUM >= 20301
	//TODO???
#else
      Recordings.ChangeState();
#endif
    }

    sleep(1);
  }
}

void WorkerThread::AddToCutterQueue(std::string Path)
{
  CutterQueue->Add(new CutterListItem(Path));
}

bool WorkerThread::IsCutting(string Path)
{
  for(CutterListItem *item=CutterQueue->First();item;item=CutterQueue->Next(item))
  {
    if(Path==item->FileName() || Path==item->NewFileName())
      return true;
  }
  return false;
}

void WorkerThread::CancelCut(string Path)
{
  for(CutterListItem *item=CutterQueue->First();item;item=CutterQueue->Next(item))
  {
    if(item->FileName()==Path || item->NewFileName()==Path)
    {
      if(item->GetCutInProgress())
        cancelcut=true;
      else
        CutterQueue->Del(item);

      return;
    }
  }
}

#ifndef USE_VDR_CUTTER
// this based mainly upon VDR's code (cutter.c)
void WorkerThread::Cut(string From,string To)
{
  cUnbufferedFile *fromfile=NULL,*tofile=NULL;
  cFileName *fromfilename=NULL,*tofilename=NULL;
  cIndexFile *fromindex=NULL,*toindex=NULL;
  cMarks frommarks,tomarks;
  cMark *mark;
  const char *error=NULL;
  uchar buffer[MAXFRAMESIZE];
  int index,currentfilenumber=0,lastiframe=0;
  bool lastmark=false,cutin=true;
  off_t maxVideoFileSize=MEGABYTE(Setup.MaxVideoFileSize);

#if VDRVERSNUM > 10713
  bool isPesRecording;
  uint16_t filenumber;
  off_t fileoffset,filesize=0;
  int length;
  bool picturetype;

  cRecording Recording(From.c_str());
  isPesRecording=Recording.IsPesRecording();
  if(frommarks.Load(From.c_str(),Recording.FramesPerSecond(),isPesRecording) && frommarks.Count())
  {
    fromfilename=new cFileName(From.c_str(),false,true,isPesRecording);
    tofilename=new cFileName(To.c_str(),true,false,isPesRecording);
    fromindex=new cIndexFile(From.c_str(),false,isPesRecording);
    toindex=new cIndexFile(To.c_str(),true,isPesRecording);
    tomarks.Load(To.c_str(),Recording.FramesPerSecond(),isPesRecording);
    if(isPesRecording && maxVideoFileSize > MEGABYTE(MAXVIDEOFILESIZEPES))
      maxVideoFileSize=MEGABYTE(MAXVIDEOFILESIZEPES);
  }
  else
  {
    esyslog("[extrecmenu] no editing marks found for %s",From.c_str());
    return;
  }
#else
  uchar filenumber;
  int fileoffset,length,filesize=0;
  uchar picturetype;

  if(frommarks.Load(From.c_str()) && frommarks.Count())
  {
    fromfilename=new cFileName(From.c_str(),false,true);
    tofilename=new cFileName(To.c_str(),true,false);
    fromindex=new cIndexFile(From.c_str(),false);
    toindex=new cIndexFile(To.c_str(),true);
    tomarks.Load(To.c_str());
  }
  else
  {
    esyslog("[extrecmenu] no editing marks found for %s",From.c_str());
    return;
  }
#endif

  if((mark=frommarks.First())!=NULL)
  {
    if(!(fromfile=fromfilename->Open()) || !(tofile=tofilename->Open()))
      return;
    fromfile->SetReadAhead(MEGABYTE(20));
#if VDRVERSNUM > 10720
    index=mark->Position();
#else
    index=mark->position;
#endif
    mark=frommarks.Next(mark);
    tomarks.Add(0);
    tomarks.Save();
  }
  else
  {
    esyslog("[extrecmenu] no editing marks found for %s",From.c_str());
    return;
  }

  isyslog("[extecmenu] editing %s",From.c_str());
  while(fromindex->Get(index++,&filenumber,&fileoffset,&picturetype,&length) && Running() && !cancelcut)
  {
    AssertFreeDiskSpace(-1);
    
    if(filenumber!=currentfilenumber)
    {
      fromfile=fromfilename->SetOffset(filenumber,fileoffset);
      fromfile->SetReadAhead(MEGABYTE(20));
      currentfilenumber=filenumber;
    }
    if(fromfile)
    {
      int len=ReadFrame(fromfile,buffer,length,sizeof(buffer));
      if(len<0)
      {
        error="ReadFrame";
        break;
      }
      if(len!=length)
      {
        currentfilenumber=0;
        length=len;
      }
    }
    else
    {
      error="fromfile";
      break;
    }
#if VDRVERSNUM > 10713
    if(picturetype)
#else
    if(picturetype==I_FRAME)
#endif
    {
      if(lastmark)
        break;
      if(filesize > maxVideoFileSize)
      {
        tofile=tofilename->NextFile();
        if(!tofile)
        {
          error="tofile 1";
          break;
        }
        filesize=0;
      }
      lastiframe=0;
      if(cutin)
      {
#if VDRVERSNUM > 10713
        if(isPesRecording)
          cRemux::SetBrokenLink(buffer,length);
        else
          TsSetTeiOnBrokenPackets(buffer,length);
#else
        cRemux::SetBrokenLink(buffer,length);
#endif
        cutin=false;
      }
    }
    if(tofile->Write(buffer,length)<0)
    {
      error="safe_write";
      break;
    }
    if(!toindex->Write(picturetype,(uint16_t)(tofilename->Number()),filesize))
    {
      error="toindex";
      break;
    }
    filesize+=length;
    if(!lastiframe)
      lastiframe=toindex->Last();
      
#if VDRVERSNUM > 10720
    if(mark && index >= mark->Position())
#else
    if(mark && index >= mark->position)
#endif
    {
      mark=frommarks.Next(mark);
      tomarks.Add(lastiframe);
      if(mark)
        tomarks.Add(toindex->Last()+1);
      tomarks.Save();
      if(mark)
      {
#if VDRVERSNUM > 10720
        index=mark->Position();
#else
        index=mark->position;
#endif
        mark=frommarks.Next(mark);
        currentfilenumber=0;
        cutin=true;
        if(Setup.SplitEditedFiles)
        {
          tofile=tofilename->NextFile();
          if(!tofile)
          {
            error="tofile 2";
            break;
          }
          filesize=0;
        }
      }
      else
        lastmark=true;
    }

    if(mysetup.LimitBandwidth)
      usleep(10);
  }
  if(!Running() || cancelcut || error)
  {
    if(error)
      esyslog("[extrecmenu] ERROR: '%s' during editing process",error);
    else
      isyslog("[extrecmenu] editing process canceled, deleting edited recording");

    cancelcut=false;
    RemoveVideoFile(To.c_str());
    Recordings.DelByName(To.c_str());
  }
  else
  {
    isyslog("[extrecmenu] editing process ended");
    cRecordingUserCommand::InvokeCommand(RUC_EDITEDRECORDING,To.c_str());
  }
  Recordings.TouchUpdate();
  delete fromfilename;
  delete tofilename;
  delete fromindex;
  delete toindex;
}
#endif

bool WorkerThread::IsMoving(string Path)
{
  for(MoveListItem *item=MoveBetweenFileSystemsList->First();item;item=MoveBetweenFileSystemsList->Next(item))
  {
    if(Path==item->From() && !item->GetMoveCanceled())
      return true;
  }
  return false;
}

void WorkerThread::CancelMove(string Path)
{
  for(MoveListItem *item=MoveBetweenFileSystemsList->First();item;item=MoveBetweenFileSystemsList->Next(item))
  {
    if(Path==item->From())
    {
      if(item->GetMoveInProgress())
      {
        cancelmove=true;
        item->SetMoveCanceled();
      }
      else
        MoveBetweenFileSystemsList->Del(item);

      return;
    }
  }
}

void WorkerThread::AddToMoveList(string From,string To)
{
  MoveBetweenFileSystemsList->Add(new MoveListItem(From,To));
#if VDRVERSNUM >= 20301
	//TODO???
#else
  Recordings.ChangeState();
#endif
}

bool WorkerThread::Move(string From,string To)
{
  if(!MakeDirs(To.c_str(),true))
  {
    Skins.QueueMessage(mtError,tr("Creating directories failed!"));
    return false;
  }

  isyslog("[extrecmenu] moving '%s' to '%s'",From.c_str(),To.c_str());

  DIR *dir=NULL;
  struct dirent *entry;
  int infile=-1,outfile=-1;

  if((dir=opendir(From.c_str()))!=NULL)
  {
    bool ok=true;
    // copy each file in this dir, except sub-dirs
    while((entry=readdir(dir))!=NULL)
    {
      string from,to;
      from=From+"/"+entry->d_name;
      to=To+"/"+entry->d_name;

      AssertFreeDiskSpace(-1);

      struct stat st;
      if(stat(from.c_str(),&st)==0)
      {
        if(S_ISREG(st.st_mode))
        {
          isyslog("[extrecmenu] moving '%s'",entry->d_name);

          ssize_t sz,sz_read=1,sz_write;
          if(stat(from.c_str(),&st)==0 && (infile=open(from.c_str(),O_RDONLY))!=-1 && (outfile=open(to.c_str(),O_WRONLY|O_CREAT|O_EXCL,st.st_mode))!=-1)
          {
            char buf[BUFFERSIZE];
            while(sz_read>0 && (sz_read=read(infile,buf,BUFFERSIZE))>0)
            {
              AssertFreeDiskSpace(-1);

              sz_write=0;
              do
              {
                if(cancelmove || !Running())
                {
                  cancelmove=false;

                  close(infile);
                  close(outfile);
                  closedir(dir);

                  isyslog("[extrecmenu] moving canceled");

#if APIVERSNUM > 20101
                  cVideoDirectory::RemoveVideoFile(To.c_str());
#else
                  RemoveVideoFile(To.c_str());
#endif

                  return true;
                }

                if((sz=write(outfile,buf+sz_write,sz_read-sz_write))<0)
                {
                  close(infile);
                  close(outfile);
                  closedir(dir);

                  Skins.Message(mtError,tr("Rename/Move failed!"));
                  esyslog("[extrecmenu] WorkerThread::Move() - write() - %s",strerror(errno));
                  return false;
                }
                sz_write+=sz;
              }
              while(sz_write<sz_read);

              if(mysetup.LimitBandwidth)
                usleep(10);
            }
            close(infile);
            close(outfile);
          }
          else
          {
            ok=false;
            break;
          }
        }
      }
      else
      {
        ok=false;
        break;
      }
    }
    if(ok)
    {
      closedir(dir);

#if VDRVERSNUM >= 20301
			LOCK_RECORDINGS_WRITE
      cRecording rec(From.c_str());
      rec.Delete();
      Recordings->DelByName(From.c_str());
      Recordings->AddByName(To.c_str());
#else
      cThreadLock RecordingsLock(&Recordings);
      cRecording rec(From.c_str());
      rec.Delete();
      Recordings.DelByName(From.c_str());
      Recordings.AddByName(To.c_str());
#endif

      string cmdstring="move \"";
      cmdstring+=myStrEscape(From,"'\\\"$");
      cmdstring+="\"";
      cRecordingUserCommand::InvokeCommand(cmdstring.c_str(),To.c_str());

#if VDRVERSNUM >= 20301
      Recordings->TouchUpdate();
#else
      Recordings.TouchUpdate();
#endif

      return true;
    }
  }
  if(dir)
    closedir(dir);
  if(infile!=-1)
    close(infile);
  if(outfile!=-1)
    close(outfile);

  Skins.QueueMessage(mtError,tr("Rename/Move failed!"));
  esyslog("[extrecmenu] WorkerThread::Move() - %s",strerror(errno));
  return false;
}

// --- Icons ------------------------------------------------------------------
bool Icons::IsUTF8=false;

void Icons::InitCharSet()
{
  // Taken from VDR's vdr.c
  char *CodeSet=NULL;
  if(setlocale(LC_CTYPE, ""))
    CodeSet=nl_langinfo(CODESET);
  else
  {
    char *LangEnv=getenv("LANG"); // last resort in case locale stuff isn't installed
    if(LangEnv)
    {
      CodeSet=strchr(LangEnv,'.');
      if(CodeSet)
        CodeSet++; // skip the dot
    }
  }

  if(CodeSet && strcasestr(CodeSet,"UTF-8")!=0)
    IsUTF8=true;
}
