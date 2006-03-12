class mySetup
{
 public:
  int ShowRecDate;
  int ShowRecTime;
  int ShowRecLength;
};

extern mySetup mysetup;

class myMenuSetup:public cMenuSetupPage
{
 private:
  int showrecdate;
  int showrectime;
  int showreclength;
 protected:
  virtual void Store();
 public:
  myMenuSetup();
};
