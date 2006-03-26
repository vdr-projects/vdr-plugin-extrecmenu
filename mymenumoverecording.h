extern bool clearall; // needed for myMenuMoveRecording

// --- myMenuRecordingsItem ---------------------------------------------------
class myMenuRecordingsItem:public cOsdItem
{
 private:
  int level,isdirectory;
  int totalentries,newentries;
  char *title;
  char *name;
  const char *filename;
 public:
  myMenuRecordingsItem(cRecording *Recording,int Level);
  ~myMenuRecordingsItem();
  const char *FileName(){return filename;}
  const char *Name(){return name;}
  bool IsDirectory(){return name!=NULL;}
  void IncrementCounter(bool IsNew);
};

// --- myMenuRecordings -------------------------------------------------------
class myMenuRecordings:public cOsdMenu
{
 private:
  bool edit;
  int level,helpkeys;
  int recordingsstate;
  char *base;
  bool Open();
  void SetHelpKeys();
  cRecording *GetRecording(myMenuRecordingsItem *Item);
  eOSState Play();
  eOSState Rewind();
  eOSState Delete();
  eOSState Rename();
  eOSState MoveRec();
  eOSState Info();
  eOSState Commands(eKeys Key=kNone);
 public:
  myMenuRecordings(const char *Base=NULL,int Level=0);
  ~myMenuRecordings();
  void Set(bool Refresh=false);
  virtual eOSState ProcessKey(eKeys Key);
};

// --- myMenuRenameRecording --------------------------------------------------
class myMenuRenameRecording:public cOsdMenu
{
 private:
  char name[MaxFileName];
  char path[MaxFileName];
  cRecording *recording;
  myMenuRecordings *menurecordings;
 public:
  myMenuRenameRecording(cRecording *Recording,myMenuRecordings *MenuRecordings);
  virtual eOSState ProcessKey(eKeys Key);
};

// --- myMenuMoveRecording ----------------------------------------------------
class myMenuMoveRecording:public cOsdMenu
{
 private:
  int level;
  char *base;
  cRecording *recording;
  myMenuRecordings *menurecordings;
  void Set();
  eOSState Open();
  eOSState MoveRec();
  eOSState Create();
 public:
  myMenuMoveRecording(cRecording *Recording,myMenuRecordings *MenuRecordings,const char *Base=NULL,int Level=0);
  myMenuMoveRecording::~myMenuMoveRecording();
  virtual eOSState ProcessKey(eKeys Key);
};
