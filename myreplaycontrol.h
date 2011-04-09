class myReplayControl:public cReplayControl
{
 private:
   bool timesearchactive;
   eOSState lastState;
 public:
   myReplayControl();
   ~myReplayControl();
   eOSState ProcessKey(eKeys Key);
};
