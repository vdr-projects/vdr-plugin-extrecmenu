/*
 * See the README file for copyright information and how to reach the author.
 *
 * The code for sort recordings is adopted from the SortRecordings-patch
 * copyright by FrankJepsen and FRank99 from vdr-portal.de
 */

#include <vdr/videodir.h>
#include <vdr/recording.h>
#include "tools.h"
#include "mymenusetup.h"

bool MoveVideoFile(cRecording *Recording,char *NewName)
{
 if(!strcmp(Recording->FileName(),NewName))
  return true;

 isyslog("[extrecmenu] moving file %s to %s",Recording->FileName(),NewName);
 int result=MakeDirs(NewName);
 if(result)
 {
  result=RenameVideoFile(Recording->FileName(),NewName);
  if(result)
  {
   // update recordings list
   Recordings.AddByName(NewName);
   Recordings.Del(Recording,false);
   return true;
  }
 }
 isyslog("[extrecmenu] moving failed");
 return false;
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
