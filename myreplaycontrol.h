#include <vdr/menu.h>

class myReplayControl:public cReplayControl
{
 public:
  eOSState ProcessKey(eKeys Key);
  ~myReplayControl();
};
