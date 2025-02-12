#include "Scope2.hpp"

#include <CAEN_FELib.h>

#include <fstream>
#include <iostream>

Scope2::Scope2() {}
Scope2::~Scope2()
{
  SendCommand("/cmd/Reset");
  Close();
}

void Scope2::LoadConfig(std::string path)
{
  std::cout << "Load config: " << path << std::endl;
  std::ifstream configFile(path);
  if (!configFile.is_open()) {
    std::cerr << "Failed to open config file" << std::endl;
    exit(1);
  }

  std::string line;
  while (std::getline(configFile, line)) {
    if (line[0] == '#' || line.size() == 0) {
      continue;
    }
    // split by white space
    auto pos = line.find(" ");
    if (pos == std::string::npos) {
      std::cerr << "Invalid config file \n" << line << std::endl;
      exit(1);
    }
    auto key = line.substr(0, pos);
    auto value = line.substr(pos + 1);
    if (key == "URL") {
      fURL = value;
    } else {
      fConfig.push_back({key, value});
    }
  }
}

bool Scope2::Initialize()
{
  auto status = true;
  if (fURL != "") {
    status &= Open(fURL);
  } else {
    std::cerr << "URL is not set" << std::endl;
    status = false;
  }
  return status;
}

bool Scope2::Open(std::string URL)
{
  std::cout << "Open URL: " << URL << std::endl;
  auto err = CAEN_FELib_Open(URL.c_str(), &fHandle);
  CheckError(err);

  return err == CAEN_FELib_Success;
}

bool Scope2::Close()
{
  std::cout << "Close digitizer" << std::endl;
  auto err = CAEN_FELib_Close(fHandle);
  CheckError(err);

  return err == CAEN_FELib_Success;
}

bool Scope2::Configure()
{
  SendCommand("/cmd/Reset");

  auto status = true;
  for (auto &config : fConfig) {
    status &= SetParameter(config[0], config[1]);
  }

  char buf[256];
  auto err = CAEN_FELib_GetValue(fHandle, "/par/RecordLengthS", buf);
  CheckError(err);
  auto rl = std::stoi(buf);
  if (rl < 0) {
    std::cerr << "Record length is not set" << std::endl;
    return false;
  }
  fRecordLength = rl;

  status &= EndpointConfigure();

  return status;
}

bool Scope2::StartAcquisition()
{
  std::cout << "Start acquisition" << std::endl;
  auto status = SendCommand("/cmd/ClearData");
  status &= SendCommand("/cmd/ArmAcquisition");
  fDataTakingFlag = true;
  fReadDataThread = std::thread(&Scope2::ReadDataThread, this);

  bool swStart = false;
  std::string value;
  GetParameter("/par/StartSource", value);
  for (auto &c : value) {
    c = std::tolower(c);
  }
  if (value == "swcmd") {
    swStart = true;
  }

  if (swStart) status &= SendCommand("/cmd/SwStartAcquisition");
  return status;
}

bool Scope2::StopAcquisition()
{
  std::cout << "Stop acquisition" << std::endl;

  bool swStop = false;
  std::string value;
  GetParameter("/par/StartSource", value);
  for (auto &c : value) {
    c = std::tolower(c);
  }
  if (value == "swcmd") {
    swStop = true;
  }

  auto status = true;
  if (swStop) status &= SendCommand("/cmd/SwStopAcquisition");
  status &= SendCommand("/cmd/DisarmAcquisition");

  fDataTakingFlag = false;
  fReadDataThread.join();

  fDataBuffer.clear();

  return status;
}

bool Scope2::EndpointConfigure()
{
  // Configure endpoint
  uint64_t epHandle;
  bool status = true;
  auto err = CAEN_FELib_GetHandle(fHandle, "/endpoint/scope", &epHandle);
  status &= CheckError(err);
  uint64_t epFolderHandle;
  err = CAEN_FELib_GetParentHandle(epHandle, nullptr, &epFolderHandle);
  status &= CheckError(err);
  err = CAEN_FELib_SetValue(epFolderHandle, "/par/activeendpoint", "scope");
  status &= CheckError(err);

  // Set data format
  nlohmann::json readDataJSON = GetReadDataFormatScope2();
  std::string readData = readDataJSON.dump();
  err = CAEN_FELib_GetHandle(fHandle, "/endpoint/scope", &fReadDataHandle);
  status &= CheckError(err);
  err = CAEN_FELib_SetReadDataFormat(fReadDataHandle, readData.c_str());
  status &= CheckError(err);

  return status;
}

void Scope2::ReadDataThread()
{
  std::cout << "Read data thread started" << std::endl;

  std::vector<std::unique_ptr<Scope2Data>> localData;
  constexpr auto sizeTh = 1024;
  localData.reserve(sizeTh * 2);
  auto counter = 0;

  Scope2Data data;
  data.Resize(fRecordLength);
  constexpr auto timeOut = 100;

  while (fDataTakingFlag) {
    auto err = CAEN_FELib_ReadData(
        fReadDataHandle, timeOut, &data.timeStamp, &data.timeStampNs,
        &data.triggerID, data.waveform, data.waveformSize, &data.eventSize,
        &data.flags, &data.boardFail, &data.samplesOverlapped);
    if (err == CAEN_FELib_Success) {
      localData.emplace_back(std::make_unique<Scope2Data>(data));
      std::this_thread::sleep_for(std::chrono::microseconds(1));  // Make sense?
    }

    if (err == CAEN_FELib_Timeout || counter++ > sizeTh) {
      if (localData.size() > 0) {
        std::lock_guard<std::mutex> lock(fDataMutex);
        fDataBuffer.insert(fDataBuffer.end(),
                           std::make_move_iterator(localData.begin()),
                           std::make_move_iterator(localData.end()));
      }
      localData.clear();
      counter = 0;
    }
  }

  localData.clear();

  std::cout << "Read data thread finished" << std::endl;
}

std::vector<std::unique_ptr<Scope2Data>> Scope2::GetData()
{
  std::vector<std::unique_ptr<Scope2Data>> retBuffer;
  {
    std::lock_guard<std::mutex> lock(fDataMutex);
    retBuffer = std::move(fDataBuffer);
  }
  return retBuffer;
}

nlohmann::json Scope2::GetReadDataFormatScope2()
{
  nlohmann::json readDataJSON;

  nlohmann::json timeStampJSON;
  timeStampJSON["name"] = "TIMESTAMP";
  timeStampJSON["type"] = "U64";
  timeStampJSON["dim"] = 0;
  readDataJSON.push_back(timeStampJSON);
  nlohmann::json timeStampNsJSON;
  timeStampNsJSON["name"] = "TIMESTAMP_NS";
  timeStampNsJSON["type"] = "U64";
  timeStampNsJSON["dim"] = 0;
  readDataJSON.push_back(timeStampNsJSON);
  nlohmann::json triggerID;
  triggerID["name"] = "TRIGGER_ID";
  triggerID["type"] = "U32";
  triggerID["dim"] = 0;
  readDataJSON.push_back(triggerID);
  nlohmann::json waveform;
  waveform["name"] = "WAVEFORM";
  waveform["type"] = "U16";
  waveform["dim"] = 2;
  readDataJSON.push_back(waveform);
  nlohmann::json waveformSize;
  waveformSize["name"] = "WAVEFORM_SIZE";
  waveformSize["type"] = "SIZE_T";
  waveformSize["dim"] = 1;
  readDataJSON.push_back(waveformSize);
  nlohmann::json eventSize;
  eventSize["name"] = "EVENT_SIZE";
  eventSize["type"] = "SIZE_T";
  eventSize["dim"] = 0;
  readDataJSON.push_back(eventSize);
  nlohmann::json flags;
  flags["name"] = "FLAGS";
  flags["type"] = "U16";
  flags["dim"] = 0;
  readDataJSON.push_back(flags);
  nlohmann::json boardFail;
  boardFail["name"] = "BOARD_FAIL";
  boardFail["type"] = "BOOL";
  boardFail["dim"] = 0;
  readDataJSON.push_back(boardFail);
  nlohmann::json SAMPLES_OVERLAPPED;
  SAMPLES_OVERLAPPED["name"] = "SAMPLES_OVERLAPPED";
  SAMPLES_OVERLAPPED["type"] = "U8";
  SAMPLES_OVERLAPPED["dim"] = 0;
  readDataJSON.push_back(SAMPLES_OVERLAPPED);

  return readDataJSON;
}

bool Scope2::CheckError(int err)
{
  auto errCode = static_cast<CAEN_FELib_ErrorCode>(err);
  if (errCode != CAEN_FELib_Success) {
    std::cout << "\x1b[31m";

    auto errName = std::string(32, '\0');
    CAEN_FELib_GetErrorName(errCode, errName.data());
    std::cerr << "Error code: " << errName << std::endl;

    auto errDesc = std::string(256, '\0');
    CAEN_FELib_GetErrorDescription(errCode, errDesc.data());
    std::cerr << "Error description: " << errDesc << std::endl;

    auto details = std::string(1024, '\0');
    CAEN_FELib_GetLastError(details.data());
    std::cerr << "Details: " << details << std::endl;

    std::cout << "\x1b[0m" << std::endl;
  }

  return errCode == CAEN_FELib_Success;
}

bool Scope2::SendCommand(std::string path)
{
  auto err = CAEN_FELib_SendCommand(fHandle, path.c_str());
  CheckError(err);

  return err == CAEN_FELib_Success;
}

bool Scope2::GetParameter(std::string path, std::string &value)
{
  char buf[256];
  auto err = CAEN_FELib_GetValue(fHandle, path.c_str(), buf);
  CheckError(err);
  value = std::string(buf);

  return err == CAEN_FELib_Success;
}

bool Scope2::SetParameter(std::string path, std::string value)
{
  auto err = CAEN_FELib_SetValue(fHandle, path.c_str(), value.c_str());
  CheckError(err);

  return err == CAEN_FELib_Success;
}