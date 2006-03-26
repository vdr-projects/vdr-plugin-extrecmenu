/*
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <vdr/videodir.h>
#include <vdr/menu.h>
#include <vdr/remote.h>
#include "mymenurecordings.h"
#include "tools.h"

bool clearall;
char newname[128];

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
myMenuMoveRecording::myMenuMoveRecording(cRecording *Recording,myMenuRecordings *MenuRecordings,const char *Base,int Level):cOsdMenu(Base?Base:"")
{
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
   return AddSubMenu(new myMenuMoveRecording(recording,menurecordings,t,level+1));
  }
 }
 return osContinue;
}

eOSState myMenuMoveRecording::MoveRec()
{
 char *dir=NULL,*p=NULL,*name,*newfilename;
 eOSState state=osContinue;
 
 myMenuMoveRecordingItem *item=(myMenuMoveRecordingItem*)Get(Current());
 if(item)
 {
  
  if(!strcmp(tr("[base dir]"),item->Text()))
   dir="";
  else
   asprintf(&dir,"%s~",item->Text());
  
  p=strrchr(recording->Name(),'~');
  if(p)
  {
   if(base)
    asprintf(&name,"%s~%s%s",base,dir,++p);
   else
    asprintf(&name,"%s%s",dir,++p);
  }
  else
  {
   if(base)
    asprintf(&name,"%s~%s%s",base,dir,recording->Name());
   else
    asprintf(&name,"%s%s",dir,recording->Name());
  }
 
  asprintf(&newfilename,"%s/%s/%s",VideoDirectory,ExchangeChars(name,true),strrchr(recording->FileName(),'/')+1);
 }
 else
 {
  p=strrchr(recording->Name(),'~');
  if(p)
   asprintf(&name,"%s~%s",base,++p);
  else
   asprintf(&name,"%s~%s",base,recording->Name());
  
  asprintf(&newfilename,"%s/%s/%s",VideoDirectory,ExchangeChars(name,true),strrchr(recording->FileName(),'/')+1);
 }

 if(MoveVideoFile(recording,newfilename))
 {
  menurecordings->Set(true);
  clearall=true; // close move recording menu
  state=osBack;
 }
 else
  Skins.Message(mtError,tr("Error while accessing recording!"));
 
 if(dir!="")
  free(dir);
 free(name);
 free(newfilename);
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
