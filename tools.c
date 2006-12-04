/*
 * See the README file for copyright information and how to reach the author.
 */

#include <vdr/plugin.h>
#include <vdr/videodir.h>
#include <vdr/recording.h>
#include "tools.h"
#include "mymenusetup.h"

#define CONFIGFILE "/extrecmenu.sort.conf"

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
    Add(new SortListItem(buf.c_str()));
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

bool SortList::Find(char *Path)
{
 for(SortListItem *item=First();item;item=Next(item))
 {
  if(!strcmp(item->Path(),Path))
   return true;
 }
 return false;
}

// --- MoveRename -------------------------------------------------------------
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
   esyslog("[extrecmenu] %s",strerror(errno));
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
    buf=ExchangeChars(buf,false);
    
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
bool myRecListItem::SortByName=false;

myRecListItem::myRecListItem(cRecording *Recording)
{
 recording=Recording;
 filename=strdup(recording->FileName());
}

myRecListItem::~myRecListItem()
{
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
 if(mysetup.DescendSorting)
 {
   if(SortByName)
     *s1=1;
   else
     *(s2+1)=255;
 }
 else
   *s1=255;
 
 if(s1&&s2&&!SortByName)
   memmove(s1+1,s2,t-s2+1);

 return s;
}

int myRecListItem::Compare(const cListObject &ListObject)const
{
 myRecListItem *item=(myRecListItem*)&ListObject;
 
 char *s1=StripEpisodeName(strdup(filename+strlen(VideoDirectory)));
 char *s2=StripEpisodeName(strdup(item->filename+strlen(VideoDirectory)));

 int compare;
 if(mysetup.DescendSorting)
   compare=strcasecmp(s2,s1);
 else
   compare=strcasecmp(s1,s2);
 
 free(s1);
 free(s2);
 
 return compare;
}

// --- myRecList --------------------------------------------------------------
void myRecList::Sort(bool SortByName)
{
 myRecListItem::SortByName=SortByName;
 cListBase::Sort();
}
