#include "extrecmenu.h"

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
}

eOSState myMenuNewName::ProcessKey(eKeys Key)
{
 eOSState state=cOsdMenu::ProcessKey(Key);
 
 if(state==osUnknown)
 {
  if(Key==kOk)
  {
   strn0cpy(newname,name,sizeof(newname));
   state=osBack;
  }
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
  myMenuMoveRecordingItem(cRecording *Recording,int Level);
};

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
  while(Level)
  {
   s=strchr(Recording->Name(),'~')+1;
   Level--;
  }
  title=strdup(s);
  char *p=strchr(title,'~');
  if(p)
   *p=0;
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
 SetHelp(tr("Button$Cancel"),NULL,tr("Button$Create"),tr("Button$Select"));
}

myMenuMoveRecording::~myMenuMoveRecording()
{
 free(base);
}

void myMenuMoveRecording::Set()
{
 char *lastitemtext=NULL;
 for(cRecording *recording=Recordings.First();recording;recording=Recordings.Next(recording))
 {
  if(!base||(strstr(recording->Name(),base)==recording->Name()&&recording->Name()[strlen(base)]=='~'))
  {
   myMenuMoveRecordingItem *item=new myMenuMoveRecordingItem(recording,level);
   if(*item->Text()&&(!lastitemtext||strcmp(lastitemtext,item->Text())))
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
}

eOSState myMenuMoveRecording::Open()
{
 if(newname[0]!=0)
 {
  const char *t;
  char buffer[MaxFileName];
  if(base)
   snprintf(buffer,sizeof(buffer),"%s~%s",base,newname);
  else
   snprintf(buffer,sizeof(buffer),"%s",newname);
  t=buffer;
  return AddSubMenu(new myMenuMoveRecording(recording,menurecordings,t,level+1));
 }
 else
 {
  myMenuMoveRecordingItem *item=(myMenuMoveRecordingItem*)Get(Current());
  if(item)
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
 char *name;
 
 char *p=strrchr(recording->Name(),'~');
 if(p)
 {
  if(base)
   asprintf(&name,"%s~%s",base,++p);
  else
   asprintf(&name,"%s",++p);
 }
 else
 {
  if(base)
   asprintf(&name,"%s~%s",base,recording->Name());
  else
   asprintf(&name,"%s",recording->Name());
 }

 char *newfilename;
 asprintf(&newfilename,"%s/%s/%s",VideoDirectory,ExchangeChars(name,true),strrchr(recording->FileName(),'/')+1);

 int result=MakeDirs(newfilename,true);
 if(result)
 {
  result=RenameVideoFile(recording->FileName(),newfilename);
  if(result)
  {
   // update recordings list
   Recordings.Update(true);
   // update menu
   menurecordings->Set();
   // close move-recordings-menu
   clearall=true;
  }
  else
   Skins.Message(mtError,tr("Error while accessing recording!"));
 }
 else
  Skins.Message(mtError,tr("Error while accessing recording!"));

 return osContinue;
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
  return Open();

 if(clearall)
  return osBack;

 return state;
}
