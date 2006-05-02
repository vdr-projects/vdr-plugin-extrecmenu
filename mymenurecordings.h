extern bool clearall; // needed for myMenuMoveRecording

// --- myMenuRecordingsItem ---------------------------------------------------
class myMenuRecordingsItem:public cOsdItem
{
 private:
  bool isdvd;
  bool isvideodvd;
  char dvdnr[BUFSIZ];
  int level,isdirectory;
  int totalentries,newentries;
  char *title;
  char *name;
  const char *filename;
  char *id; // this is the unique name that identifies a recording
 public:
  myMenuRecordingsItem(cRecording *Recording,int Level);
  ~myMenuRecordingsItem();
  const char *FileName(){return filename;}
  const char *Name(){return name;}
  bool IsDirectory(){return name!=NULL;}
  void IncrementCounter(bool IsNew);
  bool IsDVD(){return isdvd;}
  bool IsVideoDVD(){return isvideodvd;}
  char *DvdNr(){return dvdnr;}
  const char *ID(){return id;}
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
  eOSState Details();
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
  bool isdir;
  char *dirbase,*dirname;
  char name[MaxFileName];
  char path[MaxFileName];
  cRecording *recording;
  myMenuRecordings *menurecordings;
 public:
  myMenuRenameRecording(myMenuRecordings *MenuRecordings,cRecording *Recording,const char *DirBase,const char *DirName);
  ~myMenuRenameRecording();
  virtual eOSState ProcessKey(eKeys Key);
};

// --- myMenuMoveRecording ----------------------------------------------------
class myMenuMoveRecording:public cOsdMenu
{
 private:
  int level;
  char *base;
  char *dirbase,*dirname;
  cRecording *recording;
  myMenuRecordings *menurecordings;
  void Set();
  eOSState Open();
  eOSState MoveRec();
  eOSState Create();
 public:
  myMenuMoveRecording(myMenuRecordings *MenuRecordings,cRecording *Recording,const char *DirBase,const char *DirName,const char *Base=NULL,int Level=0);
  myMenuMoveRecording::~myMenuMoveRecording();
  virtual eOSState ProcessKey(eKeys Key);
};

// --- myMenuRecordingDetails -------------------------------------------------
class myMenuRecordingDetails:public cOsdMenu
{
 private:
  int priority,lifetime;
  cRecording *recording;
  myMenuRecordings *menurecordings;
 public:
  myMenuRecordingDetails(cRecording *Recording,myMenuRecordings *MenuRecordings);
  virtual eOSState ProcessKey(eKeys Key);
};
