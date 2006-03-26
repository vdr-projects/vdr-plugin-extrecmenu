#include <vdr/videodir.h>
#include <vdr/recording.h>
#include "tools.h"

bool MoveVideoFile(cRecording *Recording,char *NewName)
{
 if(!strcmp(Recording->FileName(),NewName))
  return true;
 
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
 return false;
}
