/*
 * See the README file for copyright information and how to reach the author.
 */

#include <vdr/videodir.h>
#include <vdr/recording.h>
#include "tools.h"
#include "mymenusetup.h"

// creates the necassery directories and renames the given old name to the new name
bool MoveRename(const char *OldName,const char *NewName,cRecording *Recording,bool Move)
{
 char *buf=NULL;

 // is OldName different to NewName
 if(!strcmp(OldName,NewName))
  return true;

 // move/rename a recording
 if(Recording)
 {
  if(!MakeDirs(NewName,true))
  {
   Skins.Message(mtError,tr("Creating directories failed!"));
   return false;
  }
  isyslog("[extrecmenu] moving %s to %s",OldName,NewName);
  
  if(rename(OldName,NewName)==-1)
  {
   Skins.Message(mtError,tr("Rename/Move failed!"));
   return false;
  }
  
  // set user command for '-r'-option of VDR
  asprintf(&buf,"%s \"%s\"",Move?"move":"rename",*strescape(OldName,"'\\\"$"));
  cRecordingUserCommand::InvokeCommand(buf,NewName);
  free(buf);

  // update recordings list
  Recordings.AddByName(NewName);
  Recordings.Del(Recording);
 }
 // move/rename a directory
 else
 {
  // is the new path within the old?
  asprintf(&buf,"%s/",OldName); // we have to append a / to make sure that we search for a directory
  printf("%s\n%s\n",buf,NewName);
  if(!strncmp(buf,NewName,strlen(buf)))
  {
   Skins.Message(mtError,tr("Moving into own sub-directory not allowed!"));
   free(buf);
   return false;
  }
  free(buf);
 
  // build my own recordings list
  myRecList *list=new myRecList();
  for(cRecording *recording=Recordings.First();recording;recording=Recordings.Next(recording))
   list->Add(new myRecListItem(recording));

  myRecListItem *item=list->First();
  while(item)
  {
   // find recordings within the path of OldName
   if(!strncmp(OldName,item->recording->FileName(),strlen(OldName)))
   {
    buf=strdup(OldName+strlen(VideoDirectory)+1);
    ExchangeChars(buf,false);
    
    // exclude recordings with the same name as OldName
    if(strcmp(item->recording->Name(),buf))
    {
     free(buf);
     asprintf(&buf,"%s%s",NewName,item->recording->FileName()+strlen(OldName));
      // move/rename the recording
     MoveRename(item->recording->FileName(),buf,item->recording,Move);
    }
    free(buf);
   }
   item=list->Next(item);
  }
  delete list;
 }
 return true;
}

// --- myRecListItem ----------------------------------------------------------
myRecListItem::myRecListItem(cRecording *Recording)
{
 recording=Recording;
 filename=recording->FileName();
 sortbuffer=NULL;
}

myRecListItem::~myRecListItem()
{
 free(sortbuffer);
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
/*
 * The code for sort recordings is adopted from the SortRecordings-patch
 * copyright by FrankJepsen and Frank99 from vdr-portal.de
*/
 *s1=255;
 if(s1&&s2&&(s1==s&&(mysetup.SortRecords&1)||s1!=s&&(mysetup.SortRecords==3||mysetup.SortRecords!=2&&!strchr(".-$ª·",*(s1-1)))))
  memmove(s1+1,s2,t-s2+1);
 return s;
}

char *myRecListItem::SortName()const
{
 if(!sortbuffer)
 {
  char *s=StripEpisodeName(strdup(filename+strlen(VideoDirectory)));
  strreplace(s,'/','a');
  int l=strxfrm(NULL,s,0)+1;
  sortbuffer=MALLOC(char,l);
  strxfrm(sortbuffer,s,l);
  free(s);
 }
 return sortbuffer;
}

int myRecListItem::Compare(const cListObject &ListObject)const
{
 myRecListItem *item=(myRecListItem*)&ListObject;
 return strcasecmp(SortName(),item->SortName());
}
