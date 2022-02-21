#ifndef TreeData_hpp
#define TreeData_hpp 1

#include <vector>

#include <CAENDigitizerType.h>

class TreeData
{  // no getter setter.  using public member variables.
 public:
  TreeData(){};

  TreeData(uint32_t nSamples)
  {
    RecordLength = nSamples;
    Trace1.resize(nSamples);
    Trace2.resize(nSamples);
    DTrace1.resize(nSamples);
    DTrace2.resize(nSamples);
  };

  ~TreeData(){};

  unsigned char Mod;
  unsigned char Ch;
  uint64_t TimeStamp;
  double FineTS;
  uint16_t ChargeLong;
  uint16_t ChargeShort;
  uint32_t Extras;
  uint32_t RecordLength;
  std::vector<uint16_t> Trace1;
  std::vector<uint16_t> Trace2;
  std::vector<uint8_t> DTrace1;
  std::vector<uint8_t> DTrace2;
};
typedef TreeData TreeData_t;

#endif

