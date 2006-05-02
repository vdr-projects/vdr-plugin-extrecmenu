/*
 * See the README file for copyright information and how to reach the author.
 */

#include <vdr/videodir.h>
#include <vdr/remote.h>
#include <vdr/menu.h>
#include "mymenurecordings.h"
#include "tools.h"

bool clearall;
char newname[128];

// --- myMenuRenameRecording --------------------------------------------------
myMenuRenameRecording::myMenuRenameRecording(myMenuRecordings *MenuRecordings,cRecording *Recording,const char *DirBase,const char *DirName):cOsdMenu(tr("Rename"),12)
{
 isdir=false;
 menurecordings=MenuRecordings;
 recording=Recording;
 dirbase=DirBase?strdup(DirBase):NULL;
 dirname=DirName?strdup(DirName):NULL;

 if(recording)
 {
  char *p=strrchr(recording->Name(),'~');
  if(p)
  {
   strn0cpy(name,++p,sizeof(name));
   strn0cpy(path,recording->Name(),sizeof(path));
   
   p=strrchr(path,'~');
   if(p)
    *p=0;
  }
  else
  {
   strn0cpy(name,recording->Name(),sizeof(name));
   strn0cpy(path,"",sizeof(path));
  }
 }
 else
 {
  isdir=true;
  strn0cpy(name,DirName,sizeof(name));
  strn0cpy(path,DirBase?DirBase:"",sizeof(path));
 }
 Add(new cMenuEditStrItem(tr("Name"),name,sizeof(name),tr(FileNameChars)));
 cRemote::Put(kRight);
}

myMenuRenameRecording::~myMenuRenameRecording()
{
 free(dirbase);
 free(dirname);
}

eOSState myMenuRenameRecording::ProcessKey(eKeys Key)
{
 eOSState state=cOsdMenu::ProcessKey(Key);
 if(state==osContinue)
 {
  if(Key==kOk)
  {
   char *oldname=NULL;
   char *newname=NULL;

   if(isdir)
    asprintf(&oldname,"%s%s%s/%s",VideoDirectory,path[0]?"/":"",dirbase?ExchangeChars(dirbase,true):"",ExchangeChars(dirname,true));
   else
    oldname=strdup(recording->FileName());

   asprintf(&newname,"%s%s%s/%s%s",VideoDirectory,path[0]?"/":"",ExchangeChars(path,true),ExchangeChars(name,true),isdir?"":strrchr(recording->FileName(),'/'));
   
   if(strcmp(oldname,newname))
   {
    if(MakeDirs(newname,true)==false)
    {
     Skins.Message(mtError,"Error while accessing recording!");
     state=osContinue;
    }
    else
    {
     isyslog("[extrecmenu] moving %s to %s",oldname,newname);
    
     if(rename(oldname,newname)==-1)
     {
      esyslog("[extrecmenu] error while moving: %s",strerror(errno));
      Skins.Message(mtError,tr("Error while accessing recording!"));
      state=osContinue;
     }
     else
     {
      cRecordingUserCommand::InvokeCommand("rename",newname);
      if(isdir)
       Recordings.Update(true);
      else
      {
       free(newname);
       asprintf(&newname,"%s%s%s/%s%s",VideoDirectory,path[0]?"/":"",path,name,strrchr(recording->FileName(),'/'));
       Recordings.AddByName(newname);
       Recordings.Del(recording,false);
      }
      menurecordings->Set(true);
      state=osBack;
     }
    }
   }
   else
    state=osBack;

   free(oldname);
   free(newname);
  }
  if(Key==kBack)
   return osBack;
 }
 return state;
}

// --- myMenuNewName ----------------------------------------------------------
class myMenuNewName:public cOsdMenu
{
 private:
  char name[128];
 public:
  myMenuNewName();
  virtual eOSState ProcessKey(eKeys Key);
};

myMenuNewName::myMenuNewName():cOsdMenu(tr("New folder"),12)
{
 strn0cpy(name,tr("New folder"),sizeof(name));
 Add(new cMenuEditStrItem(tr("Name"),name,sizeof(name),tr(FileNameChars)));
 cRemote::Put(kRight);
}

eOSState myMenuNewName::ProcessKey(eKeys Key)
{
 eOSState state=cOsdMenu::ProcessKey(Key);
 
 if(state==osContinue)
 {
  if(Key==kOk)
  {
   strn0cpy(newname,name,sizeof(newname));
   state=osBack;
  }
  if(Key==kBack)
   state=osBack;
 }
 
 return state;
}

// --- myMenuMoveRecordingItem ------------------------------------------------
class myMenuMoveRecordingItem:public cOsdItem
{
 private:
  int level;
  char *title;
 public:
  myMenuMoveRecordingItem(const char *Title,int Level);
  myMenuMoveRecordingItem(cRecording *Recording,int Level);
  int Level(){return level;}
  void SetLevel(int Level){level=Level;}
};

myMenuMoveRecordingItem::myMenuMoveRecordingItem(const char *Title,int Level)
{
 level=Level;
 title=strdup(Title);
 SetText(title);
}

myMenuMoveRecordingItem::myMenuMoveRecordingItem(cRecording *Recording,int Level)
{
 level=0;

 const char *s=Recording->Name();
 while(*++s)
 {
  if(*s=='~')
   level++;
 }
 if(Level<level)
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
  title=MALLOC(char,s-p+1);
  strn0cpy(title,p,s-p+1);
  SetText(title);
 }
 else
  SetText("");
}

// --- myMenuMoveRecording ----------------------------------------------------
myMenuMoveRecording::myMenuMoveRecording(myMenuRecordings *MenuRecordings,cRecording *Recording,const char *DirBase,const char *DirName,const char *Base,int Level):cOsdMenu(Base?Base:"")
{
 dirbase=DirBase?strdup(DirBase):NULL;
 dirname=DirName?strdup(DirName):NULL;
 strn0cpy(newname,"",sizeof(newname));
 recording=Recording;
 menurecordings=MenuRecordings;
 base=Base?strdup(Base):NULL;
 level=Level;
 Set();
 SetHelp(tr("Button$Cancel"),NULL,tr("Button$Create"),tr("Button$Move"));
}

myMenuMoveRecording::~myMenuMoveRecording()
{
 free(base);
 free(dirbase);
 free(dirname);
}

void myMenuMoveRecording::Set()
{
 if(level==0)
  Add(new myMenuMoveRecordingItem(tr("[base dir]"),0));

 char *lastitemtext=NULL;
 myMenuMoveRecordingItem *lastitem=NULL;
 for(cRecording *recording=Recordings.First();recording;recording=Recordings.Next(recording))
 {
  if(!base||(strstr(recording->Name(),base)==recording->Name()&&recording->Name()[strlen(base)]=='~'))
  {
   myMenuMoveRecordingItem *item=new myMenuMoveRecordingItem(recording,level);
   if(*item->Text())
   {
    if(lastitemtext&&!strcmp(lastitemtext,item->Text())) // same text
    {
     if(lastitem&&lastitem->Level()<item->Level()) // if level of the previous item is lower, set it to the new value
     {
      lastitem->SetLevel(item->Level());
     }
     delete item;
    }
    else
    {
     Add(item); // different text means a new item to add
     lastitem=item;
     free(lastitemtext);
     lastitemtext=strdup(lastitem->Text());
    }
   }
   else
    delete item;
  }
 }
 free(lastitemtext);
}

eOSState myMenuMoveRecording::Open()
{
 myMenuMoveRecordingItem *item=(myMenuMoveRecordingItem*)Get(Current());
 if(item)
 {
  if(item->Level()>level)
  {
   const char *t=item->Text();
   char buffer[MaxFileName];
   if(base)
   {
    snprintf(buffer,sizeof(buffer),"%s~%s",base,t);
    t=buffer;
   }
   return AddSubMenu(new myMenuMoveRecording(menurecordings,recording,dirbase,dirname,t,level+1));
  }
 }
 return osContinue;
}

eOSState myMenuMoveRecording::MoveRec()
{
 char *oldname=NULL;
 char *newname=NULL;
 char *dir=NULL;
 char *p=NULL;
 
 eOSState state=osContinue;

 myMenuMoveRecordingItem *item=(myMenuMoveRecordingItem*)Get(Current());

 if(dirname)
  asprintf(&oldname,"%s%s%s/%s",VideoDirectory,dirbase?"/":"",dirbase?ExchangeChars(dirbase,true):"",ExchangeChars(dirname,true));
 else
  oldname=strdup(recording->FileName());
  
 p=strrchr(oldname,'/');
 if(p&&!dirname)
  *p=0;

 if(item)
 {
  if(strcmp(tr("[base dir]"),item->Text()))
  {
   asprintf(&dir,"%s%s%s",base?base:"",base?"~":"",item->Text());
   ExchangeChars(dir,true);
  }
 }
 else
 {
  asprintf(&dir,"%s",base);
  ExchangeChars(dir,true);
 }

 asprintf(&newname,"%s%s%s/%s",VideoDirectory,dir?"/":"",dir?dir:"",strrchr(oldname,'/')+1);

 if(MakeDirs(newname,true)==false)
 {
  Skins.Message(mtError,"Error while accessing recording!");
  state=osContinue;
 }
 else
 {
  isyslog("[extrecmenu] moving %s to %s",oldname,newname);
  if(rename(oldname,newname)==-1)
  {
   esyslog("[extrecmenu] error while moving: %s",strerror(errno));
   Skins.Message(mtError,"Error while accessing recording!");
   state=osContinue;
  }
  else
  {
   cRecordingUserCommand::InvokeCommand("move",newname);
   if(dirname)
    Recordings.Update(true);
   else
   {
    free(newname);
    asprintf(&newname,"%s%s%s/%s%s",VideoDirectory,dir?"/":"",dir?dir:"",strrchr(oldname,'/')+1,strrchr(recording->FileName(),'/'));
    Recordings.AddByName(newname);
    Recordings.Del(recording,false);
   }
   clearall=true;
   state=osBack;
  }
 }
 free(oldname);
 free(newname);
 free(dir);
 return state;
}

eOSState myMenuMoveRecording::Create()
{
 return AddSubMenu(new myMenuNewName);
}

eOSState myMenuMoveRecording::ProcessKey(eKeys Key)
{
 eOSState state=cOsdMenu::ProcessKey(Key);
 
 if(state==osUnknown)
 {
  switch(Key)
  {
   case kRed: clearall=true;break;
   case kYellow: return Create();
   case kBlue: return MoveRec();
   case kOk: return Open();
   default: break;
  }
 }

 if(newname[0]!=0)
 {
  Add(new myMenuMoveRecordingItem(newname,level+2));
  Display();
  strn0cpy(newname,"",sizeof(newname));
 }

 if(clearall)
  return osBack;

 return state;
}

// --- myMenuRecordingDetails -------------------------------------------------
myMenuRecordingDetails::myMenuRecordingDetails(cRecording *Recording,myMenuRecordings *MenuRecordings):cOsdMenu(tr("Details"),12)
{
 recording=Recording;
 menurecordings=MenuRecordings;
 priority=recording->priority;
 lifetime=recording->lifetime;

 Add(new cMenuEditIntItem(tr("Priority"),&priority,0,MAXPRIORITY));
 Add(new cMenuEditIntItem(tr("Lifetime"),&lifetime,0,MAXLIFETIME));
}

eOSState myMenuRecordingDetails::ProcessKey(eKeys Key)
{
 eOSState state=cOsdMenu::ProcessKey(Key);
 if(state==osUnknown)
 {
  if(Key==kOk)
  {
   char *oldname=strdup(recording->FileName());
   char *newname=strdup(recording->FileName());

   sprintf(newname+strlen(newname)-9,"%02d.%02d.rec",priority,lifetime);

   if(strcmp(oldname,newname))
   {
    isyslog("[extrecmenu] moving %s to %s",oldname,newname);
   
    if(rename(oldname,newname)==-1)
    {
     esyslog("[extrecmenu] error while moving: %s",strerror(errno));
     Skins.Message(mtError,tr("Error while accessing recording!"));
     state=osContinue;
    }
    else
    {
     Recordings.AddByName(newname);
     Recordings.Del(recording,false);
     menurecordings->Set(true);
     state=osBack;
    }
   }
   else
    state=osBack;

   free(oldname);
   free(newname);
  }
 }
 return state;
}
