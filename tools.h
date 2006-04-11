bool MoveVideoFile(cRecording *Recording,char *NewName);

class myRecListItem:public cListObject
{
 private:
  const char *filename;
  mutable char *sortbuffer;
  static char *StripEpisodeName(char *s);
  char *SortName()const;
 public:
  myRecListItem(cRecording *Recording);
  ~myRecListItem();
  virtual int Compare(const cListObject &ListObject)const;
  cRecording *recording;
};

class myRecList:public cList<myRecListItem>
{
};
