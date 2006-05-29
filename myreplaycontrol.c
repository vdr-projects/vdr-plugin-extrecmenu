/*
 * See the README file for copyright information and how to reach the author.
 */

#include <vdr/interface.h>
#include <vdr/status.h>
#include "myreplaycontrol.h"
#include "mymenusetup.h"

myReplayControl::~myReplayControl()
{
 cRemote::CallPlugin("extrecmenu");
}

eOSState myReplayControl::ProcessKey(eKeys Key)
{
 if(Key==kBack)
  return osEnd;

 return cReplayControl::ProcessKey(Key);
}
