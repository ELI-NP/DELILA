#include "TDataContainer.hpp"

#include <iostream>
#include <stdexcept>

TDataContainer::TDataContainer() { fMaxSize = 200000000; }
TDataContainer::TDataContainer(unsigned int maxSize) { fMaxSize = maxSize; }

TDataContainer::~TDataContainer() {}

void TDataContainer::AddData(std::vector<char> data)
{
  fDataQueue.push_back(data);
}

std::vector<char> TDataContainer::GetPacket()
{
  std::vector<char> retData;
  auto packetSize = 0;

  while (true) {
    if (fDataQueue.size() > 0) {
      packetSize += fDataQueue.front().size();
      if (packetSize < fMaxSize) {
        const auto nBytes = fDataQueue.front().size();
        for (auto i = 0; i < nBytes; i++) {
          retData.push_back(fDataQueue.front().at(i));
        }
        fDataQueue.pop_front();
      } else {
        break;
      }
    } else {
      // std::cerr << "No more data in the queue!" << std::endl;
      break;  // No throwing!!
    }
  }

  return retData;
}

std::vector<char> TDataContainer::GetFront()
{
  std::vector<char> retData;
  if (fDataQueue.size() > 0) {
    retData = fDataQueue.front();
    fDataQueue.pop_front();
  } else {
    // std::cerr << "No more data in the queue!" << std::endl;
    // throw std::runtime_error("No more data in the queue!");
  }

  return retData;
}
