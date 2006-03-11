#include "mydvbplayer.h"
#include <stdlib.h>
#include <vdr/recording.h>
#include <vdr/remux.h>
#include <vdr/ringbuffer.h>
#include <vdr/thread.h>
#include <vdr/tools.h>

// --- myBackTrace ----------------------------------------------------------

#define AVG_FRAME_SIZE 15000         // an assumption about the average frame size
#define DVB_BUF_SIZE   (256 * 1024)  // an assumption about the dvb firmware buffer size
#define BACKTRACE_ENTRIES (DVB_BUF_SIZE / AVG_FRAME_SIZE + 20) // how many entries are needed to backtrace buffer contents

class myBackTrace {
private:
  int index[BACKTRACE_ENTRIES];
  int length[BACKTRACE_ENTRIES];
  int pos, num;
public:
  myBackTrace(void);
  void Clear(void);
  void Add(int Index, int Length);
  int Get(bool Forward);
  };

myBackTrace::myBackTrace(void)
{
  Clear();
}

void myBackTrace::Clear(void)
{
  pos = num = 0;
}

void myBackTrace::Add(int Index, int Length)
{
  index[pos] = Index;
  length[pos] = Length;
  if (++pos >= BACKTRACE_ENTRIES)
     pos = 0;
  if (num < BACKTRACE_ENTRIES)
     num++;
}

int myBackTrace::Get(bool Forward)
{
  int p = pos;
  int n = num;
  int l = DVB_BUF_SIZE + (Forward ? 0 : 256 * 1024); //XXX (256 * 1024) == DVB_BUF_SIZE ???
  int i = -1;

  while (n && l > 0) {
        if (--p < 0)
           p = BACKTRACE_ENTRIES - 1;
        i = index[p] - 1;
        l -= length[p];
        n--;
        }
  return i;
}

// --- myNonBlockingFileReader ------------------------------------------------

class myNonBlockingFileReader : public cThread {
private:
  cUnbufferedFile *f;
  uchar *buffer;
  int wanted;
  int length;
  bool hasData;
  cCondWait newSet;
  cCondVar newDataCond;
  cMutex newDataMutex;
protected:
  void Action(void);
public:
  myNonBlockingFileReader(void);
  ~myNonBlockingFileReader();
  void Clear(void);
  int Read(cUnbufferedFile *File, uchar *Buffer, int Length);
  bool Reading(void) { return buffer; }
  bool WaitForDataMs(int msToWait);
  };

myNonBlockingFileReader::myNonBlockingFileReader(void)
:cThread("non blocking file reader")
{
  f = NULL;
  buffer = NULL;
  wanted = length = 0;
  hasData = false;
  Start();
}

myNonBlockingFileReader::~myNonBlockingFileReader()
{
  newSet.Signal();
  Cancel(3);
  free(buffer);
}

void myNonBlockingFileReader::Clear(void)
{
  Lock();
  f = NULL;
  free(buffer);
  buffer = NULL;
  wanted = length = 0;
  hasData = false;
  Unlock();
  newSet.Signal();
}

int myNonBlockingFileReader::Read(cUnbufferedFile *File, uchar *Buffer, int Length)
{
  if (hasData && buffer) {
     if (buffer != Buffer) {
        esyslog("ERROR: myNonBlockingFileReader::Read() called with different buffer!");
        errno = EINVAL;
        return -1;
        }
     buffer = NULL;
     return length;
     }
  if (!buffer) {
     f = File;
     buffer = Buffer;
     wanted = Length;
     length = 0;
     hasData = false;
     newSet.Signal();
     }
  errno = EAGAIN;
  return -1;
}

void myNonBlockingFileReader::Action(void)
{
  while (Running()) {
        Lock();
        if (!hasData && f && buffer) {
           int r = f->Read(buffer + length, wanted - length);
           if (r >= 0) {
              length += r;
              if (!r || length == wanted) { // r == 0 means EOF
                 cMutexLock NewDataLock(&newDataMutex);
                 hasData = true;
                 newDataCond.Broadcast();
                 }
              }
           else if (r < 0 && FATALERRNO) {
              LOG_ERROR;
              length = r; // this will forward the error status to the caller
              hasData = true;
              }
           }
        Unlock();
        newSet.Wait(1000);
        }
}

bool myNonBlockingFileReader::WaitForDataMs(int msToWait)
{
  cMutexLock NewDataLock(&newDataMutex);
  if (hasData)
     return true;
  return newDataCond.TimedWait(newDataMutex, msToWait);
}

// --- myDvbPlayer ------------------------------------------------------------

#define PLAYERBUFSIZE  MEGABYTE(1)

// The number of frames to back up when resuming an interrupted replay session:
#define RESUMEBACKUP (10 * FRAMESPERSEC)

class myDvbPlayer : public cPlayer, cThread {
private:
  enum ePlayModes { pmPlay, pmPause, pmSlow, pmFast, pmStill };
  enum ePlayDirs { pdForward, pdBackward };
  static int Speeds[];
  myNonBlockingFileReader *nonBlockingFileReader;
  cRingBufferFrame *ringBuffer;
  myBackTrace *backTrace;
  cFileName *fileName;
  cIndexFile *index;
  cUnbufferedFile *replayFile;
  bool eof;
  bool firstPacket;
  ePlayModes playMode;
  ePlayDirs playDir;
  int trickSpeed;
  int readIndex, writeIndex;
  cFrame *readFrame;
  cFrame *playFrame;
  void TrickSpeed(int Increment);
  void Empty(void);
  bool NextFile(uchar FileNumber = 0, int FileOffset = -1);
  int Resume(void);
  bool Save(void);
protected:
  virtual void Activate(bool On);
  virtual void Action(void);
public:
  myDvbPlayer(const char *FileName);
  virtual ~myDvbPlayer();
  bool Active(void) { return cThread::Running(); }
  void Pause(void);
  void Play(void);
  void Forward(void);
  void Backward(void);
  int SkipFrames(int Frames);
  void SkipSeconds(int Seconds);
  void Goto(int Position, bool Still = false);
  virtual bool GetIndex(int &Current, int &Total, bool SnapToIFrame = false);
  virtual bool GetReplayMode(bool &Play, bool &Forward, int &Speed);
  };

#define MAX_VIDEO_SLOWMOTION 63 // max. arg to pass to VIDEO_SLOWMOTION // TODO is this value correct?
#define NORMAL_SPEED  4 // the index of the '1' entry in the following array
#define MAX_SPEEDS    3 // the offset of the maximum speed from normal speed in either direction
#define SPEED_MULT   12 // the speed multiplier
int myDvbPlayer::Speeds[] = { 0, -2, -4, -8, 1, 2, 4, 12, 0 };

myDvbPlayer::myDvbPlayer(const char *FileName)
:cThread("dvbplayer")
{
  nonBlockingFileReader = NULL;
  ringBuffer = NULL;
  backTrace = NULL;
  index = NULL;
  eof = false;
  firstPacket = true;
  playMode = pmPlay;
  playDir = pdForward;
  trickSpeed = NORMAL_SPEED;
  readIndex = writeIndex = -1;
  readFrame = NULL;
  playFrame = NULL;
  isyslog("replay %s", FileName);
  fileName = new cFileName(FileName, false);
  replayFile = fileName->Open();
  if (!replayFile)
     return;
  ringBuffer = new cRingBufferFrame(PLAYERBUFSIZE);
  // Create the index file:
  index = new cIndexFile(FileName, false);
  if (!index)
     esyslog("ERROR: can't allocate index");
  else if (!index->Ok()) {
     delete index;
     index = NULL;
     }
  backTrace = new myBackTrace;
}

myDvbPlayer::~myDvbPlayer()
{
  Detach();
  Save();
  delete readFrame; // might not have been stored in the buffer in Action()
  delete index;
  delete fileName;
  delete backTrace;
  delete ringBuffer;
}

void myDvbPlayer::TrickSpeed(int Increment)
{
  int nts = trickSpeed + Increment;
  if (Speeds[nts] == 1) {
     trickSpeed = nts;
     if (playMode == pmFast)
        Play();
     else
        Pause();
     }
  else if (Speeds[nts]) {
     trickSpeed = nts;
     int Mult = (playMode == pmSlow && playDir == pdForward) ? 1 : SPEED_MULT;
     int sp = (Speeds[nts] > 0) ? Mult / Speeds[nts] : -Speeds[nts] * Mult;
     if (sp > MAX_VIDEO_SLOWMOTION)
        sp = MAX_VIDEO_SLOWMOTION;
     DeviceTrickSpeed(sp);
     }
}

void myDvbPlayer::Empty(void)
{
  LOCK_THREAD;
  if (nonBlockingFileReader)
     nonBlockingFileReader->Clear();
  if ((readIndex = backTrace->Get(playDir == pdForward)) < 0)
     readIndex = writeIndex;
  delete readFrame; // might not have been stored in the buffer in Action()
  readFrame = NULL;
  playFrame = NULL;
  ringBuffer->Clear();
  backTrace->Clear();
  DeviceClear();
  firstPacket = true;
}

bool myDvbPlayer::NextFile(uchar FileNumber, int FileOffset)
{
  if (FileNumber > 0)
     replayFile = fileName->SetOffset(FileNumber, FileOffset);
  else if (replayFile && eof)
     replayFile = fileName->NextFile();
  eof = false;
  return replayFile != NULL;
}

int myDvbPlayer::Resume(void)
{
  if (index) {
     int Index = index->GetResume();
     if (Index >= 0) {
        uchar FileNumber;
        int FileOffset;
        if (index->Get(Index, &FileNumber, &FileOffset) && NextFile(FileNumber, FileOffset))
           return Index;
        }
     }
  return -1;
}

bool myDvbPlayer::Save(void)
{
  if (index) {
     int Index = writeIndex;
     if (Index >= 0) {
        Index -= RESUMEBACKUP;
        if (Index > 0)
           Index = index->GetNextIFrame(Index, false);
        else
           Index = 0;
        if (Index >= 0)
           return index->StoreResume(Index);
        }
     }
  return false;
}

void myDvbPlayer::Activate(bool On)
{
  if (On) {
     if (replayFile)
        Start();
     }
  else
     Cancel(9);
}

void myDvbPlayer::Action(void)
{
  uchar *b = NULL;
  uchar *p = NULL;
  int pc = 0;

  readIndex = Resume();
  if (readIndex >= 0)
     isyslog("resuming replay at index %d (%s)", readIndex, *IndexToHMSF(readIndex, true));

  nonBlockingFileReader = new myNonBlockingFileReader;
  int Length = 0;
  bool Sleep = false;
  bool WaitingForData = false;

  while (Running() && (NextFile() || readIndex >= 0 || ringBuffer->Available() || !DeviceFlush(100))) {
        if (Sleep) {
           if (WaitingForData)
              nonBlockingFileReader->WaitForDataMs(3); // this keeps the CPU load low, but reacts immediately on new data
           else
              cCondWait::SleepMs(3); // this keeps the CPU load low
           Sleep = false;
           }
        cPoller Poller;
        if (DevicePoll(Poller, 100)) {

           LOCK_THREAD;

           // Read the next frame from the file:

           if (playMode != pmStill && playMode != pmPause) {
              if (!readFrame && (replayFile || readIndex >= 0)) {
                 if (!nonBlockingFileReader->Reading()) {
                    if (playMode == pmFast || (playMode == pmSlow && playDir == pdBackward)) {
                       uchar FileNumber;
                       int FileOffset;
                       int Index = index->GetNextIFrame(readIndex, playDir == pdForward, &FileNumber, &FileOffset, &Length, true);
                       if (Index >= 0) {
                          if (!NextFile(FileNumber, FileOffset))
                             continue;
                          }
                       else {
                          // hit begin of recording: wait for device buffers to drain
                          // before changing play mode:
                          if (!DeviceFlush(100))
                             continue;
                          // can't call Play() here, because those functions may only be
                          // called from the foreground thread - and we also don't need
                          // to empty the buffer here
                          DevicePlay();
                          playMode = pmPlay;
                          playDir = pdForward;
                          continue;
                          }
                       readIndex = Index;
                       }
                    else if (index) {
                       uchar FileNumber;
                       int FileOffset;
                       readIndex++;
                       if (!(index->Get(readIndex, &FileNumber, &FileOffset, NULL, &Length) && NextFile(FileNumber, FileOffset))) {
                          readIndex = -1;
                          eof = true;
                          continue;
                          }
                       }
                    else // allows replay even if the index file is missing
                       Length = MAXFRAMESIZE;
                    if (Length == -1)
                       Length = MAXFRAMESIZE; // this means we read up to EOF (see cIndex)
                    else if (Length > MAXFRAMESIZE) {
                       esyslog("ERROR: frame larger than buffer (%d > %d)", Length, MAXFRAMESIZE);
                       Length = MAXFRAMESIZE;
                       }
                    b = MALLOC(uchar, Length);
                    }
                 int r = nonBlockingFileReader->Read(replayFile, b, Length);
                 if (r > 0) {
                    WaitingForData = false;
                    readFrame = new cFrame(b, -r, ftUnknown, readIndex); // hands over b to the ringBuffer
                    b = NULL;
                    }
                 else if (r == 0)
                    eof = true;
                 else if (r < 0 && errno == EAGAIN)
                    WaitingForData = true;
                 else if (r < 0 && FATALERRNO) {
                    LOG_ERROR;
                    break;
                    }
                 }

              // Store the frame in the buffer:

              if (readFrame) {
                 if (ringBuffer->Put(readFrame))
                    readFrame = NULL;
                 }
              }
           else
              Sleep = true;

           // Get the next frame from the buffer:

           if (!playFrame) {
              playFrame = ringBuffer->Get();
              p = NULL;
              pc = 0;
              }

           // Play the frame:

           if (playFrame) {
              if (!p) {
                 p = playFrame->Data();
                 pc = playFrame->Count();
                 if (p) {
                    if (firstPacket) {
                       PlayPes(NULL, 0);
                       cRemux::SetBrokenLink(p, pc);
                       firstPacket = false;
                       }
                    }
                 }
              if (p) {
                 int w = PlayPes(p, pc, playMode != pmPlay);
                 if (w > 0) {
                    p += w;
                    pc -= w;
                    }
                 else if (w < 0 && FATALERRNO) {
                    LOG_ERROR;
                    break;
                    }
                 }
              if (pc == 0) {
                 writeIndex = playFrame->Index();
                 backTrace->Add(playFrame->Index(), playFrame->Count());
                 ringBuffer->Drop(playFrame);
                 playFrame = NULL;
                 p = NULL;
                 }
              }
           else
              Sleep = true;
           }
        }

  myNonBlockingFileReader *nbfr = nonBlockingFileReader;
  nonBlockingFileReader = NULL;
  delete nbfr;
}

void myDvbPlayer::Pause(void)
{
  if (playMode == pmPause || playMode == pmStill)
     Play();
  else {
     LOCK_THREAD;
     if (playMode == pmFast || (playMode == pmSlow && playDir == pdBackward))
        Empty();
     DeviceFreeze();
     playMode = pmPause;
     }
}

void myDvbPlayer::Play(void)
{
  if (playMode != pmPlay) {
     LOCK_THREAD;
     if (playMode == pmStill || playMode == pmFast || (playMode == pmSlow && playDir == pdBackward))
        Empty();
     DevicePlay();
     playMode = pmPlay;
     playDir = pdForward;
    }
}

void myDvbPlayer::Forward(void)
{
  if (index) {
     switch (playMode) {
       case pmFast:
            if (Setup.MultiSpeedMode) {
               TrickSpeed(playDir == pdForward ? 1 : -1);
               break;
               }
            else if (playDir == pdForward) {
               Play();
               break;
               }
            // run into pmPlay
       case pmPlay: {
            LOCK_THREAD;
            Empty();
            DeviceMute();
            playMode = pmFast;
            playDir = pdForward;
            trickSpeed = NORMAL_SPEED;
            TrickSpeed(Setup.MultiSpeedMode ? 1 : MAX_SPEEDS);
            }
            break;
       case pmSlow:
            if (Setup.MultiSpeedMode) {
               TrickSpeed(playDir == pdForward ? -1 : 1);
               break;
               }
            else if (playDir == pdForward) {
               Pause();
               break;
               }
            // run into pmPause
       case pmStill:
       case pmPause:
            DeviceMute();
            playMode = pmSlow;
            playDir = pdForward;
            trickSpeed = NORMAL_SPEED;
            TrickSpeed(Setup.MultiSpeedMode ? -1 : -MAX_SPEEDS);
            break;
       }
     }
}

void myDvbPlayer::Backward(void)
{
  if (index) {
     switch (playMode) {
       case pmFast:
            if (Setup.MultiSpeedMode) {
               TrickSpeed(playDir == pdBackward ? 1 : -1);
               break;
               }
            else if (playDir == pdBackward) {
               Play();
               break;
               }
            // run into pmPlay
       case pmPlay: {
            LOCK_THREAD;
            Empty();
            DeviceMute();
            playMode = pmFast;
            playDir = pdBackward;
            trickSpeed = NORMAL_SPEED;
            TrickSpeed(Setup.MultiSpeedMode ? 1 : MAX_SPEEDS);
            }
            break;
       case pmSlow:
            if (Setup.MultiSpeedMode) {
               TrickSpeed(playDir == pdBackward ? -1 : 1);
               break;
               }
            else if (playDir == pdBackward) {
               Pause();
               break;
               }
            // run into pmPause
       case pmStill:
       case pmPause: {
            LOCK_THREAD;
            Empty();
            DeviceMute();
            playMode = pmSlow;
            playDir = pdBackward;
            trickSpeed = NORMAL_SPEED;
            TrickSpeed(Setup.MultiSpeedMode ? -1 : -MAX_SPEEDS);
            }
            break;
       }
     }
}

int myDvbPlayer::SkipFrames(int Frames)
{
  if (index && Frames) {
     int Current, Total;
     GetIndex(Current, Total, true);
     int OldCurrent = Current;
     // As GetNextIFrame() increments/decrements at least once, the
     // destination frame (= Current + Frames) must be adjusted by
     // -1/+1 respectively.
     Current = index->GetNextIFrame(Current + Frames + (Frames > 0 ? -1 : 1), Frames > 0);
     return Current >= 0 ? Current : OldCurrent;
     }
  return -1;
}

void myDvbPlayer::SkipSeconds(int Seconds)
{
  if (index && Seconds) {
     LOCK_THREAD;
     Empty();
     int Index = writeIndex;
     if (Index >= 0) {
        Index = max(Index + Seconds * FRAMESPERSEC, 0);
        if (Index > 0)
           Index = index->GetNextIFrame(Index, false, NULL, NULL, NULL, true);
        if (Index >= 0)
           readIndex = writeIndex = Index - 1; // Action() will first increment it!
        }
     Play();
     }
}

void myDvbPlayer::Goto(int Index, bool Still)
{
  if (index) {
     LOCK_THREAD;
     Empty();
     if (++Index <= 0)
        Index = 1; // not '0', to allow GetNextIFrame() below to work!
     uchar FileNumber;
     int FileOffset, Length;
     Index = index->GetNextIFrame(Index, false, &FileNumber, &FileOffset, &Length);
     if (Index >= 0 && NextFile(FileNumber, FileOffset) && Still) {
        uchar b[MAXFRAMESIZE + 4 + 5 + 4];
        int r = ReadFrame(replayFile, b, Length, sizeof(b));
        if (r > 0) {
           if (playMode == pmPause)
              DevicePlay();
           // append sequence end code to get the image shown immediately with softdevices
           if (r > 6 && (b[3] & 0xF0) == 0xE0) { // make sure to append it only to a video packet
              b[r++] = 0x00;
              b[r++] = 0x00;
              b[r++] = 0x01;
              b[r++] = b[3];
              if (b[6] & 0x80) { // MPEG 2
                 b[r++] = 0x00;
                 b[r++] = 0x07;
                 b[r++] = 0x80;
                 b[r++] = 0x00;
                 b[r++] = 0x00;
                 }
              else { // MPEG 1
                 b[r++] = 0x00;
                 b[r++] = 0x05;
                 b[r++] = 0x0F;
                 }
              b[r++] = 0x00;
              b[r++] = 0x00;
              b[r++] = 0x01;
              b[r++] = 0xB7;
              }
           DeviceStillPicture(b, r);
           }
        playMode = pmStill;
        }
     readIndex = writeIndex = Index;
     }
}

bool myDvbPlayer::GetIndex(int &Current, int &Total, bool SnapToIFrame)
{
  if (index) {
     if (playMode == pmStill)
        Current = max(readIndex, 0);
     else {
        Current = max(writeIndex, 0);
        if (SnapToIFrame) {
           int i1 = index->GetNextIFrame(Current + 1, false);
           int i2 = index->GetNextIFrame(Current, true);
           Current = (abs(Current - i1) <= abs(Current - i2)) ? i1 : i2;
           }
        }
     Total = index->Last();
     return true;
     }
  Current = Total = -1;
  return false;
}

bool myDvbPlayer::GetReplayMode(bool &Play, bool &Forward, int &Speed)
{
  Play = (playMode == pmPlay || playMode == pmFast);
  Forward = (playDir == pdForward);
  if (playMode == pmFast || playMode == pmSlow)
     Speed = Setup.MultiSpeedMode ? abs(trickSpeed - NORMAL_SPEED) : 0;
  else
     Speed = -1;
  return true;
}

// --- myDvbPlayerControl -----------------------------------------------------

myDvbPlayerControl::myDvbPlayerControl(const char *FileName)
:cControl(player = new myDvbPlayer(FileName))
{
}

myDvbPlayerControl::~myDvbPlayerControl()
{
  Stop();
}

bool myDvbPlayerControl::Active(void)
{
  return player && player->Active();
}

void myDvbPlayerControl::Stop(void)
{
  delete player;
  player = NULL;
}

void myDvbPlayerControl::Pause(void)
{
  if (player)
     player->Pause();
}

void myDvbPlayerControl::Play(void)
{
  if (player)
     player->Play();
}

void myDvbPlayerControl::Forward(void)
{
  if (player)
     player->Forward();
}

void myDvbPlayerControl::Backward(void)
{
  if (player)
     player->Backward();
}

void myDvbPlayerControl::SkipSeconds(int Seconds)
{
  if (player)
     player->SkipSeconds(Seconds);
}

int myDvbPlayerControl::SkipFrames(int Frames)
{
  if (player)
     return player->SkipFrames(Frames);
  return -1;
}

bool myDvbPlayerControl::GetIndex(int &Current, int &Total, bool SnapToIFrame)
{
  if (player) {
     player->GetIndex(Current, Total, SnapToIFrame);
     return true;
     }
  return false;
}

bool myDvbPlayerControl::GetReplayMode(bool &Play, bool &Forward, int &Speed)
{
  return player && player->GetReplayMode(Play, Forward, Speed);
}

void myDvbPlayerControl::Goto(int Position, bool Still)
{
  if (player)
     player->Goto(Position, Still);
}
