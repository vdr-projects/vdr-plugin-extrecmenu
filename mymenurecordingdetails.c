/*
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <vdr/videodir.h>
#include <vdr/menu.h>
#include "mymenurecordings.h"
#include "tools.h"

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
   char *newfilename=strdup(recording->FileName());

   sprintf(newfilename+strlen(newfilename)-9,"%02d.%02d.rec",priority,lifetime);

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

   free(newfilename);
  }
 }
 return state;
}
