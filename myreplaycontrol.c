/*
 * See the README file for copyright information and how to reach the author.
 */

#include <vdr/interface.h>
#include <vdr/status.h>
#include "myreplaycontrol.h"

bool myReplayControl::jumprec=true;

eOSState myReplayControl::ProcessKey(eKeys Key)
{
 if(Key==kBack)
 {
  cRemote::CallPlugin("extrecmenu");
  return osEnd;
 }

 return cReplayControl::ProcessKey(Key);
}
