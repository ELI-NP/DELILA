#include "PSD2.hpp"

#include <CAEN_FELib.h>

#include <fstream>
#include <iostream>

PSD2::PSD2() {}
PSD2::~PSD2()
{
  SendCommand("/cmd/Reset");
  Close();
}

void PSD2::LoadConfig(std::string path)
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
    } else if (key == "Mod") {
      fMod = std::stoi(value);
    } else {
      fConfig.push_back({key, value});
    }
  }
}

bool PSD2::Initialize()
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

bool PSD2::Open(std::string URL)
{
  std::cout << "Open URL: " << URL << std::endl;
  auto err = CAEN_FELib_Open(URL.c_str(), &fHandle);
  CheckError(err);

  return err == CAEN_FELib_Success;
}

bool PSD2::Close()
{
  std::cout << "Close digitizer" << std::endl;
  auto err = CAEN_FELib_Close(fHandle);
  CheckError(err);

  return err == CAEN_FELib_Success;
}

bool PSD2::Configure()
{
  SendCommand("/cmd/Reset");

  auto status = true;
  for (auto &config : fConfig) {
    status &= SetParameter(config[0], config[1]);
  }

  char buf[256];
  auto err = CAEN_FELib_GetValue(fHandle, "/ch/0/par/ChRecordLengthS", buf);
  CheckError(err);
  auto rl = std::stoi(buf);
  if (rl < 0) {
    std::cerr << "Record length is not set" << std::endl;
    return false;
  }
  fRecordLength = rl;

  status &= EndpointConfigure();

  status &= SendCommand("/cmd/ArmAcquisition");

  return status;
}

bool PSD2::StartAcquisition()
{
  std::cout << "Start acquisition" << std::endl;
  fDataTakingFlag = true;
  fReadDataThread = std::thread(&PSD2::ReadDataThread, this);

  auto status = true;
  status &= SendCommand("/cmd/SwStartAcquisition");
  return status;
}

bool PSD2::StopAcquisition()
{
  std::cout << "Stop acquisition" << std::endl;
  auto status = SendCommand("/cmd/SwStopAcquisition");
  status &= SendCommand("/cmd/DisarmAcquisition");

  fDataTakingFlag = false;
  fReadDataThread.join();

  fDataBuffer.clear();

  return status;
}

bool PSD2::EndpointConfigure()
{
  // Configure endpoint
  uint64_t epHandle;
  bool status = true;
  auto err = CAEN_FELib_GetHandle(fHandle, "/endpoint/dpppsd", &epHandle);
  status &= CheckError(err);
  uint64_t epFolderHandle;
  err = CAEN_FELib_GetParentHandle(epHandle, nullptr, &epFolderHandle);
  status &= CheckError(err);
  err = CAEN_FELib_SetValue(epFolderHandle, "/par/activeendpoint", "DPPPSD");
  status &= CheckError(err);

  // Set data format
  nlohmann::json readDataJSON = GetReadDataFormatPSD2();
  std::string readData = readDataJSON.dump();
  err = CAEN_FELib_GetHandle(fHandle, "/endpoint/DPPPSD", &fReadDataHandle);
  status &= CheckError(err);
  err = CAEN_FELib_SetReadDataFormat(fReadDataHandle, readData.c_str());
  status &= CheckError(err);

  return status;
}

void PSD2::ReadDataThread()
{
  std::cout << "Read data thread started" << std::endl;

  std::vector<std::unique_ptr<PSD2Data>> localData;
  constexpr auto sizeTh = 1024;
  localData.reserve(sizeTh * 2);
  auto counter = 0;

  PSD2Data data;
  data.Resize(fRecordLength);
  data.module = fMod;
  constexpr auto timeOut = 100;

  while (fDataTakingFlag) {
    // auto err = CAEN_FELib_ReadData(
    //     fReadDataHandle, timeOut, &data.channel, &data.timeStamp,
    //     &data.timeStampNs, &data.fineTimeStamp, &data.energy, &data.energyShort,
    //     &data.flagsLowPriority, &data.flagsHighPriority, &data.triggerThr,
    //     &data.timeResolution, data.analogProbe1.data(), &data.analogProbe1Type,
    //     data.analogProbe2.data(), &data.analogProbe2Type,
    //     data.digitalProbe1.data(), &data.digitalProbe1Type,
    //     data.digitalProbe2.data(), &data.digitalProbe2Type,
    //     data.digitalProbe3.data(), &data.digitalProbe3Type,
    //     data.digitalProbe4.data(), &data.digitalProbe4Type, &data.waveformSize,
    //     &data.eventSize, &data.boardFail, &data.flush, &data.aggregateCounter);
    auto err = CAEN_FELib_ReadData(
        fReadDataHandle, timeOut, &data.channel, &data.timeStamp,
        &data.timeStampNs, &data.fineTimeStamp, &data.energy, &data.energyShort,
        &data.flagsLowPriority, &data.flagsHighPriority, &data.triggerThr,
        &data.timeResolution, data.analogProbe1.data(), &data.analogProbe1Type,
        &data.waveformSize);
    if (err == CAEN_FELib_Success) {
      localData.emplace_back(std::make_unique<PSD2Data>(data));
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

std::vector<std::unique_ptr<PSD2Data>> PSD2::GetData()
{
  std::vector<std::unique_ptr<PSD2Data>> retBuffer;
  {
    std::lock_guard<std::mutex> lock(fDataMutex);
    retBuffer = std::move(fDataBuffer);
  }
  return retBuffer;
}

nlohmann::json PSD2::GetReadDataFormatPSD2()
{
  nlohmann::json readDataJSON;
  nlohmann::json channelJSON;
  channelJSON["name"] = "CHANNEL";
  channelJSON["type"] = "U8";
  channelJSON["dim"] = 0;
  readDataJSON.push_back(channelJSON);
  nlohmann::json timeStampJSON;
  timeStampJSON["name"] = "TIMESTAMP";
  timeStampJSON["type"] = "U64";
  timeStampJSON["dim"] = 0;
  readDataJSON.push_back(timeStampJSON);
  nlohmann::json timeStampNsJSON;
  timeStampNsJSON["name"] = "TIMESTAMP_NS";
  timeStampNsJSON["type"] = "DOUBLE";
  timeStampNsJSON["dim"] = 0;
  readDataJSON.push_back(timeStampNsJSON);
  nlohmann::json fineTimeStampJSON;
  fineTimeStampJSON["name"] = "FINE_TIMESTAMP";
  fineTimeStampJSON["type"] = "U16";
  fineTimeStampJSON["dim"] = 0;
  readDataJSON.push_back(fineTimeStampJSON);
  nlohmann::json energyJSON;
  energyJSON["name"] = "ENERGY";
  energyJSON["type"] = "U16";
  energyJSON["dim"] = 0;
  readDataJSON.push_back(energyJSON);
  nlohmann::json energyShortJSON;
  energyShortJSON["name"] = "ENERGY_SHORT";
  energyShortJSON["type"] = "I16";
  energyShortJSON["dim"] = 0;
  readDataJSON.push_back(energyShortJSON);
  nlohmann::json flagsLowPriorityJSON;
  flagsLowPriorityJSON["name"] = "FLAGS_LOW_PRIORITY";
  flagsLowPriorityJSON["type"] = "U16";
  flagsLowPriorityJSON["dim"] = 0;
  readDataJSON.push_back(flagsLowPriorityJSON);
  nlohmann::json flagsHighPriorityJSON;
  flagsHighPriorityJSON["name"] = "FLAGS_HIGH_PRIORITY";
  flagsHighPriorityJSON["type"] = "U16";
  flagsHighPriorityJSON["dim"] = 0;
  readDataJSON.push_back(flagsHighPriorityJSON);
  nlohmann::json triggerThrJSON;
  triggerThrJSON["name"] = "TRIGGER_THR";
  triggerThrJSON["type"] = "U16";
  triggerThrJSON["dim"] = 0;
  readDataJSON.push_back(triggerThrJSON);
  nlohmann::json timeResolutionJSON;
  timeResolutionJSON["name"] = "TIME_RESOLUTION";
  timeResolutionJSON["type"] = "U8";
  timeResolutionJSON["dim"] = 0;
  readDataJSON.push_back(timeResolutionJSON);
  nlohmann::json analogProbe1JSON;
  analogProbe1JSON["name"] = "ANALOG_PROBE_1";
  analogProbe1JSON["type"] = "I32";
  analogProbe1JSON["dim"] = 1;
  readDataJSON.push_back(analogProbe1JSON);
  nlohmann::json analogProbe1TypeJSON;
  analogProbe1TypeJSON["name"] = "ANALOG_PROBE_1_TYPE";
  analogProbe1TypeJSON["type"] = "U8";
  analogProbe1TypeJSON["dim"] = 0;
  readDataJSON.push_back(analogProbe1TypeJSON);
  // nlohmann::json analogProbe2JSON;
  // analogProbe2JSON["name"] = "ANALOG_PROBE_2";
  // analogProbe2JSON["type"] = "I32";
  // analogProbe2JSON["dim"] = 1;
  // readDataJSON.push_back(analogProbe2JSON);
  // nlohmann::json analogProbe2TypeJSON;
  // analogProbe2TypeJSON["name"] = "ANALOG_PROBE_2_TYPE";
  // analogProbe2TypeJSON["type"] = "U8";
  // analogProbe2TypeJSON["dim"] = 0;
  // readDataJSON.push_back(analogProbe2TypeJSON);
  // nlohmann::json digitalProbe1JSON;
  // digitalProbe1JSON["name"] = "DIGITAL_PROBE_1";
  // digitalProbe1JSON["type"] = "U8";
  // digitalProbe1JSON["dim"] = 1;
  // readDataJSON.push_back(digitalProbe1JSON);
  // nlohmann::json digitalProbe1TypeJSON;
  // digitalProbe1TypeJSON["name"] = "DIGITAL_PROBE_1_TYPE";
  // digitalProbe1TypeJSON["type"] = "U8";
  // digitalProbe1TypeJSON["dim"] = 0;
  // readDataJSON.push_back(digitalProbe1TypeJSON);
  // nlohmann::json digitalProbe2JSON;
  // digitalProbe2JSON["name"] = "DIGITAL_PROBE_2";
  // digitalProbe2JSON["type"] = "U8";
  // digitalProbe2JSON["dim"] = 1;
  // readDataJSON.push_back(digitalProbe2JSON);
  // nlohmann::json digitalProbe2TypeJSON;
  // digitalProbe2TypeJSON["name"] = "DIGITAL_PROBE_2_TYPE";
  // digitalProbe2TypeJSON["type"] = "U8";
  // digitalProbe2TypeJSON["dim"] = 0;
  // readDataJSON.push_back(digitalProbe2TypeJSON);
  // nlohmann::json digitalProbe3JSON;
  // digitalProbe3JSON["name"] = "DIGITAL_PROBE_3";
  // digitalProbe3JSON["type"] = "U8";
  // digitalProbe3JSON["dim"] = 1;
  // readDataJSON.push_back(digitalProbe3JSON);
  // nlohmann::json digitalProbe3TypeJSON;
  // digitalProbe3TypeJSON["name"] = "DIGITAL_PROBE_3_TYPE";
  // digitalProbe3TypeJSON["type"] = "U8";
  // digitalProbe3TypeJSON["dim"] = 0;
  // readDataJSON.push_back(digitalProbe3TypeJSON);
  // nlohmann::json digitalProbe4JSON;
  // digitalProbe4JSON["name"] = "DIGITAL_PROBE_4";
  // digitalProbe4JSON["type"] = "U8";
  // digitalProbe4JSON["dim"] = 1;
  // readDataJSON.push_back(digitalProbe4JSON);
  // nlohmann::json digitalProbe4TypeJSON;
  // digitalProbe4TypeJSON["name"] = "DIGITAL_PROBE_4_TYPE";
  // digitalProbe4TypeJSON["type"] = "U8";
  // digitalProbe4TypeJSON["dim"] = 0;
  // readDataJSON.push_back(digitalProbe4TypeJSON);
  nlohmann::json waveformSizeJSON;
  waveformSizeJSON["name"] = "WAVEFORM_SIZE";
  waveformSizeJSON["type"] = "SIZE_T";
  waveformSizeJSON["dim"] = 0;
  readDataJSON.push_back(waveformSizeJSON);
  // nlohmann::json eventSizeJSON;
  // eventSizeJSON["name"] = "EVENT_SIZE";
  // eventSizeJSON["type"] = "SIZE_T";
  // eventSizeJSON["dim"] = 0;
  // readDataJSON.push_back(eventSizeJSON);
  // nlohmann::json boardFailJSON;
  // boardFailJSON["name"] = "BOARD_FAIL";
  // boardFailJSON["type"] = "BOOL";
  // boardFailJSON["dim"] = 0;
  // readDataJSON.push_back(boardFailJSON);
  // nlohmann::json flushJSON;
  // flushJSON["name"] = "FLUSH";
  // flushJSON["type"] = "BOOL";
  // flushJSON["dim"] = 0;
  // readDataJSON.push_back(flushJSON);
  // nlohmann::json aggregateCounterJSON;
  // aggregateCounterJSON["name"] = "AGGREGATE_COUNTER";
  // aggregateCounterJSON["type"] = "U32";
  // aggregateCounterJSON["dim"] = 0;
  // readDataJSON.push_back(aggregateCounterJSON);

  return readDataJSON;
}

bool PSD2::CheckError(int err)
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

bool PSD2::SendCommand(std::string path)
{
  auto err = CAEN_FELib_SendCommand(fHandle, path.c_str());
  CheckError(err);

  return err == CAEN_FELib_Success;
}

bool PSD2::GetParameter(std::string path, std::string &value)
{
  char buf[256];
  auto err = CAEN_FELib_GetValue(fHandle, path.c_str(), buf);
  CheckError(err);
  value = std::string(buf);

  return err == CAEN_FELib_Success;
}

bool PSD2::SetParameter(std::string path, std::string value)
{
  auto err = CAEN_FELib_SetValue(fHandle, path.c_str(), value.c_str());
  CheckError(err);

  return err == CAEN_FELib_Success;
}