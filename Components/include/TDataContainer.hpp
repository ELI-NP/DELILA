#ifndef TDataContainer_hpp
#define TDataContainer_hpp 1

#include <deque>
#include <vector>

class TDataContainer
{
 public:
  TDataContainer();
  TDataContainer(unsigned int maxSize);
  ~TDataContainer();

  std::vector<char> GetPacket();
  std::vector<char> GetFront();

  void AddData(std::vector<char> data);

  unsigned int GetSize() { return fDataQueue.size(); };

 private:
  std::deque<std::vector<char>> fDataQueue;
  unsigned int fMaxSize;
};

#endif
