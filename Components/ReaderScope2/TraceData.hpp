#ifndef TraceData_hpp
#define TraceData_hpp 1

#include <cstdint>
#include <vector>

class TraceData
{  // no getter setter.  using public member variables.
 public:
  TraceData() {};

  TraceData(uint16_t nSamples)
  {
    RecordLength = nSamples;
    Trace1.resize(nSamples);
  };

  ~TraceData() {};

  uint8_t Mod;
  uint8_t Ch;
  double FineTS;
  uint16_t RecordLength;
  std::vector<int32_t> Trace1;

  static const uint16_t OneHitSize =
      sizeof(Mod) + sizeof(Ch) + sizeof(FineTS) + sizeof(RecordLength);
};
typedef TraceData TraceData_t;

#endif
