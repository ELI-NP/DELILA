#ifndef TSiHist_hpp
#define TSiHist_hpp 1

// Define Si hit map and fill it

#include <TH2Poly.h>

#include <map>
#include <string>
#include <vector>

namespace SiDetector
{
struct Coordinates {
  Coordinates(double x, double y) : x(x), y(y){};
  double x;
  double y;
};

enum class Plane { Rear, Front };
struct Position {
  Position(){};  // Needed for map
  Position(int ring, int sector, Plane plane)
      : ring(ring), sector(sector), plane(plane){};
  int ring;
  int sector;
  Plane plane;
};

struct Digitizer {
  Digitizer(int brd, int ch) : brd(brd), ch(ch){};
  int brd;
  int ch;
};

class TSiHist
{
 public:
  TSiHist(){};
  TSiHist(std::string name, double innerDiameter, double outerDiameter,
          int nRings, int nSectorsRear, int nSectorsFront = 1);
  TSiHist(std::string name, std::string configFileName = "SiConf.conf");

  ~TSiHist(){};

  void InitHists();

  void LoadConfig(std::string configFileName = "SiMap.conf");

  void FillByPositionFront(Position position, double weight = 1);
  void FillByPositionRear(Position position, double weight = 1);
  void FillByPositionMatrix(Position front, Position rear, double weight = 1);

  void FillByDigitizer(Digitizer digitizer, double weight = 1);
  void FillByDigitizerMatrix(Digitizer digitizer1, Digitizer digitizer2,
                             double weight = 1);

  TH2Poly *GetHistFront() { return fHistFront; };
  TH2Poly *GetHistRear() { return fHistRear; };
  TH2Poly *GetHistMatrix() { return fHistMatrix; };

  Coordinates GetFrontCoordinates(int ringNumber, int sectorNumber = 0);
  Coordinates GetRearCoordinates(int sectorNumber);
  // Matrix uses front ring and rear sector
  Coordinates GetMatrixCoordinates(int ringNumber, int sectorNumber);

 private:
  std::string fName;
  double fInnerDiameter = 25.92;
  double fOuterDiameter = 70.09;
  int fNRings = 45;
  int fNSectorsRear = 16;
  int fNSectorsFront = 1;

  void InitFront();
  void InitRear();
  void InitMatrix();
  TH2Poly *fHistFront;
  TH2Poly *fHistRear;
  TH2Poly *fHistMatrix;

  // Map is only for rear and front
  std::map<Digitizer, Position> fDigitizerMap;
  std::vector<std::string> SplitString(const std::string &str,
                                       char delim = ' ');

  // How many points to calculate the position in 2Pi
  // Same points but not in the same bin are calculated differently
  const int fSmoothFactor = 128;

  // gap area between bins
  const double fPhiOffset = 0.01;     // radians
  const double fRadiusOffset = 0.00;  // mm
};
}  // namespace SiDetector
#endif
