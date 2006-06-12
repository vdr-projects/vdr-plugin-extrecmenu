#include <string>
#include <fstream>
#include <iostream>

using namespace std;

class SortListItem:public cListObject
{
 private:
  string path;
 public:
  SortListItem(const char *Path){path=Path;};
  const char *Path(){return path.c_str();}
};

class SortList:public cList<SortListItem>
{
 public:
  void ReadConfigFile();
  void WriteConfigFile();
  bool Find(char *Path);
};

bool MoveRename(const char *OldName,const char *NewName,cRecording *Recording,bool Move);

class myRecListItem:public cListObject
{
 friend class myRecList;
 private:
  static bool SortByName;
  char *filename;
  static char *StripEpisodeName(char *s);
 public:
  myRecListItem(cRecording *Recording);
  ~myRecListItem();
  virtual int Compare(const cListObject &ListObject)const;
  cRecording *recording;
};

class myRecList:public cList<myRecListItem>
{
 public:
  void Sort(bool SortByName);
};
