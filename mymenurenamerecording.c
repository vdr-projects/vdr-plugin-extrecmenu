#include <vdr/videodir.h>
#include <vdr/menu.h>
#include "mymenurecordings.h"

myMenuRenameRecording::myMenuRenameRecording(cRecording *Recording,myMenuRecordings *MenuRecordings):cOsdMenu(tr("Rename recording"),12)
{
 recording=Recording;
 menurecordings=MenuRecordings;
 
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
 Add(new cMenuEditStrItem(tr("Name"),name,sizeof(name),tr(FileNameChars)));
}

eOSState myMenuRenameRecording::ProcessKey(eKeys Key)
{
 eOSState state=cOsdMenu::ProcessKey(Key);
 if(state==osUnknown)
 {
  if(Key==kOk)
  {
   int result;
   char *buffer;
   char *newFileName;
   
   if(strlen(path))
    asprintf(&buffer,"%s~%s",path,name);
   else
    asprintf(&buffer,"%s",name);
   
   asprintf(&newFileName,"%s/%s/%s",VideoDirectory,ExchangeChars(buffer,true),strrchr(recording->FileName(),'/')+1);
   
   result=MakeDirs(newFileName,true);
   if(result)
   {
    result=RenameVideoFile(recording->FileName(),newFileName);
    if(result)
    {
     // update recordings list
     Recordings.AddByName(newFileName);
     Recordings.Del(recording);
     // update menu
     menurecordings->Set(true);
     return osBack;
    }
    else
    {
     Skins.Message(mtError,tr("Error while accessing recording!"));
     state=osContinue;
    }
   }
   else
   {
    Skins.Message(mtError,tr("Error while accessing recording!"));
    state=osContinue;
   }
   free(buffer);
   free(newFileName);
  }
 }
 return state;
}
