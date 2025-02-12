#ifndef Scope2_HPP
#define Scope2_HPP 1

#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <vector>

// data class of PSD event
class Scope2Data
{
 public:
  Scope2Data(size_t size = 0)
      : timeStamp(0),
        timeStampNs(0),
        triggerID(0),
        eventSize(0),
        flags(0),
        boardFail(false),
        samplesOverlapped(0)
  {
    if (size > 0) Resize(size);
  };
  ~Scope2Data() {
    // if (waveformSize != nullptr) delete[] waveformSize;
    // if (waveform != nullptr) {
    //   constexpr uint32_t nCh = 32;
    //   for (size_t i = 0; i < nCh; i++) {
    //     delete[] waveform[i];
    //   }
    //   delete[] waveform;
    // }
  };

  // Copy constructor
  Scope2Data(const Scope2Data &data)
  {
    timeStamp = data.timeStamp;
    timeStampNs = data.timeStampNs;
    triggerID = data.triggerID;
    waveform = data.waveform;
    waveformSize = data.waveformSize;
    eventSize = data.eventSize;
    flags = data.flags;
    boardFail = data.boardFail;
    samplesOverlapped = data.samplesOverlapped;
  };

  void Resize(size_t size)
  {
    constexpr uint32_t nCh = 32;
    waveformSize = new size_t[nCh];
    waveform = new uint16_t *[nCh];
    for (size_t i = 0; i < nCh; i++) {
      waveform[i] = new uint16_t[size];
    }
  };

  uint64_t timeStamp;
  uint64_t timeStampNs;
  uint32_t triggerID;
  uint16_t **waveform = nullptr;
  size_t *waveformSize = nullptr;
  size_t eventSize;
  uint16_t flags;
  bool boardFail;
  uint8_t samplesOverlapped;
};

class Scope2
{
 public:
  Scope2();
  ~Scope2();

  bool Initialize();
  bool Configure();
  bool StartAcquisition();
  bool StopAcquisition();

  bool CheckStatus();

  void LoadConfig(std::string path);

  std::vector<std::unique_ptr<Scope2Data>> GetData();

 private:
  uint64_t fHandle;

  std::string fURL = "";
  std::vector<std::array<std::string, 2>> fConfig;

  bool CheckError(int err);
  bool SendCommand(std::string path);
  bool GetParameter(std::string path, std::string &value);
  bool SetParameter(std::string path, std::string value);
  nlohmann::json GetReadDataFormatScope2();

  bool Open(std::string URL);
  bool Close();

  std::mutex fDataMutex;
  std::thread fReadDataThread;
  std::vector<std::unique_ptr<Scope2Data>> fDataBuffer;
  bool fDataTakingFlag;
  void ReadDataThread();

  // Configure and read data structure
  uint64_t fReadDataHandle;
  uint64_t fRecordLength;
  bool EndpointConfigure();
};

#endif  // Scope2_HPP