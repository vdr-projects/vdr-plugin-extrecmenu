/*
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <vdr/videodir.h>
#include <vdr/menu.h>
#include <vdr/remote.h>
#include "mymenurecordings.h"
#include "tools.h"

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
 cRemote::Put(kRight);
}

eOSState myMenuRenameRecording::ProcessKey(eKeys Key)
{
 eOSState state=cOsdMenu::ProcessKey(Key);
 if(state==osContinue)
 {
  if(Key==kOk)
  {
   char *buffer;
   char *newfilename;
   
   if(strlen(path))
    asprintf(&buffer,"%s~%s",path,name);
   else
    asprintf(&buffer,"%s",name);
   
   asprintf(&newfilename,"%s/%s/%s",VideoDirectory,ExchangeChars(buffer,true),strrchr(recording->FileName(),'/')+1);

   if(MoveVideoFile(recording,newfilename))
   {
    menurecordings->Set(true);
    state=osBack;
   }
   else
   {
    Skins.Message(mtError,tr("Error while accessing recording!"));
    state=osContinue;
   }
   free(buffer);
   free(newfilename);
  }
  if(Key==kBack)
   return osBack;
 }
 return state;
}
