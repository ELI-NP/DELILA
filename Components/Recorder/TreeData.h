#ifndef TreeData_hpp
#define TreeData_hpp 1

#include <CAENDigitizerType.h>

class TreeData
{  // no getter setter.  using public member variables.
 public:
  TreeData()
      : Trace1(nullptr), Trace2(nullptr), DTrace1(nullptr), DTrace2(nullptr){};

  TreeData(uint32_t nSamples)
  {
    RecordLength = nSamples;
    Trace1 = new uint16_t[nSamples];
    Trace2 = new uint16_t[nSamples];
    DTrace1 = new uint8_t[nSamples];
    DTrace2 = new uint8_t[nSamples];
  };

  ~TreeData()
  {
    delete[] Trace1;
    Trace1 = nullptr;
    delete[] Trace2;
    Trace2 = nullptr;
    delete[] DTrace1;
    DTrace1 = nullptr;
    delete[] DTrace2;
    DTrace2 = nullptr;
  };

  unsigned char Mod;
  unsigned char Ch;
  uint64_t TimeStamp;
  uint16_t FineTS;
  uint16_t Energy;
  uint32_t Extras;
  uint32_t RecordLength;
  uint16_t *Trace1;
  uint16_t *Trace2;
  uint8_t *DTrace1;
  uint8_t *DTrace2;
};
typedef TreeData TreeData_t;

#endif

