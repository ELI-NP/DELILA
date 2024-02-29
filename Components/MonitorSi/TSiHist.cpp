#include "TSiHist.hpp"

#include <TMath.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace SiDetector
{
bool operator<(const Position &lhs, const Position &rhs) noexcept
{
  return std::tie(lhs.ring, lhs.sector, lhs.plane) <
         std::tie(rhs.ring, rhs.sector, rhs.plane);
};

bool operator<(const Digitizer &lhs, const Digitizer &rhs) noexcept
{
  return std::tie(lhs.brd, lhs.ch) < std::tie(rhs.brd, rhs.ch);
};

TSiHist::TSiHist(std::string name, double innerDiameter, double outerDiameter,
                 int nRings, int nSectorsRear, int nSectorsFront)
{
  fName = name;
  fInnerDiameter = innerDiameter;
  fOuterDiameter = outerDiameter;
  fNRings = nRings;
  fNSectorsRear = nSectorsRear;
  fNSectorsFront = nSectorsFront;
  InitHists();
}

TSiHist::TSiHist(std::string name, std::string configFileName)
{
  fName = name;

  std::ifstream fin(configFileName);
  if (fin.is_open()) {
    while (true) {
      std::string key, val;
      fin >> key >> val;
      if (fin.eof()) break;

      // Inner 25.92
      // Outer 70.09
      // Rings 45
      // SectorRear 16
      // SectorFront 1
      if (key == "Inner") fInnerDiameter = std::stod(val);
      if (key == "Outer") fOuterDiameter = std::stod(val);
      if (key == "Rings") fNRings = std::stoi(val);
      if (key == "SectorRear") fNSectorsRear = std::stoi(val);
      if (key == "SectorFront") fNSectorsFront = std::stoi(val);
    }

    InitHists();
  } else {
    std::cerr << "Error: cannot open file " << configFileName << std::endl;
  }
}
  
void TSiHist::InitHists()
{
  InitFront();
  InitRear();
  InitMatrix();
}

void TSiHist::InitFront()
{
  fHistFront = new TH2Poly();
  fHistFront->SetName(Form("%s_front", fName.c_str()));
  fHistFront->SetTitle(Form("%s_front", fName.c_str()));

  int nPoints = fSmoothFactor / fNSectorsFront;
  if (nPoints < 2) nPoints = 2;
  double deltaPhi = (2 * TMath::Pi() / fNSectorsFront) / (nPoints - 1);
  double deltaR = (fOuterDiameter - fInnerDiameter) / 2 / fNRings;

  for (auto iSector = 0; iSector < fNSectorsFront; iSector++) {
    double phiMin = 2 * TMath::Pi() / fNSectorsFront * iSector;
    for (auto iRing = 0; iRing < fNRings; iRing++) {
      double innerRadius = fInnerDiameter / 2 + iRing * deltaR + fRadiusOffset;
      double outerRadius = fInnerDiameter / 2 + (iRing + 1) * deltaR;

      std::vector<double> x;
      std::vector<double> y;

      for (auto iPoint = 0; iPoint < nPoints; iPoint++) {
        auto phi = phiMin + deltaPhi * iPoint;
        if (iPoint == 0) {
          phi += fPhiOffset;
        }
        x.push_back(innerRadius * TMath::Cos(phi));
        y.push_back(innerRadius * TMath::Sin(phi));
      }
      for (auto iPoint = 0; iPoint < nPoints; iPoint++) {
        auto phi = phiMin + deltaPhi * (nPoints - iPoint - 1);
        if (iPoint == nPoints - 1) {
          phi += fPhiOffset;
        }
        x.push_back(outerRadius * TMath::Cos(phi));
        y.push_back(outerRadius * TMath::Sin(phi));
      }
      fHistFront->AddBin(x.size(), x.data(), y.data());
    }
  }
}

void TSiHist::InitRear()
{
  fHistRear = new TH2Poly();
  fHistRear->SetName(Form("%s_rear", fName.c_str()));
  fHistRear->SetTitle(Form("%s_rear", fName.c_str()));

  int nPoints = fSmoothFactor / fNSectorsRear;
  if (nPoints < 2) nPoints = 2;
  double deltaPhi = (2 * TMath::Pi() / fNSectorsRear) / (nPoints - 1);

  for (auto iSector = 0; iSector < fNSectorsRear; iSector++) {
    double phiMin = 2 * TMath::Pi() / fNSectorsRear * iSector;
    double innerRadius = fInnerDiameter / 2 + fRadiusOffset;
    double outerRadius = fOuterDiameter / 2;

    std::vector<double> x;
    std::vector<double> y;

    for (auto iPoint = 0; iPoint < nPoints; iPoint++) {
      auto phi = phiMin + deltaPhi * iPoint;
      if (iPoint == 0) {
        phi += fPhiOffset;
      }
      x.push_back(innerRadius * TMath::Cos(phi));
      y.push_back(innerRadius * TMath::Sin(phi));
    }
    for (auto iPoint = 0; iPoint < nPoints; iPoint++) {
      auto phi = phiMin + deltaPhi * (nPoints - iPoint - 1);
      if (iPoint == nPoints - 1) {
        phi += fPhiOffset;
      }
      x.push_back(outerRadius * TMath::Cos(phi));
      y.push_back(outerRadius * TMath::Sin(phi));
    }
    fHistRear->AddBin(x.size(), x.data(), y.data());
  }
}

void TSiHist::InitMatrix()
{
  fHistMatrix = new TH2Poly();
  fHistMatrix->SetName(Form("%s_matrix", fName.c_str()));
  fHistMatrix->SetTitle(Form("%s_matrix", fName.c_str()));

  int nPoints = fSmoothFactor / fNSectorsRear;
  if (nPoints < 2) nPoints = 2;
  double deltaPhi = (2 * TMath::Pi() / fNSectorsRear) / (nPoints - 1);
  double deltaR = (fOuterDiameter - fInnerDiameter) / 2 / fNRings;

  for (auto iSector = 0; iSector < fNSectorsRear; iSector++) {
    double phiMin = 2 * TMath::Pi() / fNSectorsRear * iSector;
    for (auto iRing = 0; iRing < fNRings; iRing++) {
      double innerRadius = fInnerDiameter / 2 + iRing * deltaR + fRadiusOffset;
      double outerRadius = fInnerDiameter / 2 + (iRing + 1) * deltaR;

      std::vector<double> x;
      std::vector<double> y;

      for (auto iPoint = 0; iPoint < nPoints; iPoint++) {
        auto phi = phiMin + deltaPhi * iPoint;
        if (iPoint == 0) {
          phi += fPhiOffset;
        }
        x.push_back(innerRadius * TMath::Cos(phi));
        y.push_back(innerRadius * TMath::Sin(phi));
      }
      for (auto iPoint = 0; iPoint < nPoints; iPoint++) {
        auto phi = phiMin + deltaPhi * (nPoints - iPoint - 1);
        if (iPoint == nPoints - 1) {
          phi += fPhiOffset;
        }
        x.push_back(outerRadius * TMath::Cos(phi));
        y.push_back(outerRadius * TMath::Sin(phi));
      }
      fHistMatrix->AddBin(x.size(), x.data(), y.data());
    }
  }
}

Coordinates TSiHist::GetFrontCoordinates(int ringNumber, int sectorNumber)
{
  double deltaPhi = 2 * TMath::Pi() / fNSectorsFront;
  double startPhi = deltaPhi / 2;
  double phi = startPhi + sectorNumber * deltaPhi;
  double deltaR = (fOuterDiameter - fInnerDiameter) / 2 / fNRings;
  double startR = fInnerDiameter / 2 + deltaR / 2;
  double r = startR + ringNumber * deltaR;
  double x = r * TMath::Cos(phi);
  double y = r * TMath::Sin(phi);

  return Coordinates(x, y);
}

Coordinates TSiHist::GetRearCoordinates(int sectorNumber)
{
  double deltaPhi = 2 * TMath::Pi() / fNSectorsRear;
  double phi = deltaPhi / 2 + sectorNumber * deltaPhi;
  double r = (fInnerDiameter + fOuterDiameter) / 4;
  double x = r * TMath::Cos(phi);
  double y = r * TMath::Sin(phi);

  return Coordinates(x, y);
}

Coordinates TSiHist::GetMatrixCoordinates(int ringNumber, int sectorNumber)
{
  // Assuming (fNSectorRear > fNSectorFront) && (fNSectorRear % fNSectorFront == 0)
  double deltaPhi = 2 * TMath::Pi() / fNSectorsRear;
  double startPhi = deltaPhi / 2;
  double phi = startPhi + sectorNumber * deltaPhi;
  double deltaR = (fOuterDiameter - fInnerDiameter) / 2 / fNRings;
  double startR = fInnerDiameter / 2 + deltaR / 2;
  double r = startR + ringNumber * deltaR;
  double x = r * TMath::Cos(phi);
  double y = r * TMath::Sin(phi);

  return Coordinates(x, y);
}

std::vector<std::string> TSiHist::SplitString(const std::string &str,
                                              char delim)
{
  std::vector<std::string> internal;
  std::stringstream ss(str);
  std::string tok;

  while (std::getline(ss, tok, delim)) {
    internal.push_back(tok);
  }

  return internal;
}

void TSiHist::LoadConfig(std::string configFileName)
{
  fDigitizerMap.clear();

  std::ifstream file(configFileName);
  if (!file.is_open()) {
    std::cerr << "Error: cannot open file " << configFileName << std::endl;
    return;
  }

  std::string line;
  while (std::getline(file, line)) {
    if (line[0] == '#' || line.size() == 0) continue;
    auto tokens = SplitString(line);
    if (tokens.size() != 5) {
      std::cerr << "Error: wrong number of tokens in line " << line
                << std::endl;
      continue;
    }

    // std::cout << "Si plane: " << tokens[0] << ", sector: " << tokens[1]
    //           << ", ring: " << tokens[2] << ", brd: " << tokens[3]
    //           << ", ch: " << tokens[4] << std::endl;

    auto plane = Plane::Rear;
    if (tokens[0] == "front") {
      plane = Plane::Front;
    } else if (tokens[0] == "rear") {
      plane = Plane::Rear;
    } else {
      std::cerr << "Error: wrong plane " << tokens[0] << std::endl;
      continue;
    }
    auto sector = std::stoi(tokens[1]);
    auto ring = std::stoi(tokens[2]);
    auto brd = std::stoi(tokens[3]);
    auto ch = std::stoi(tokens[4]);

    auto digitizer = Digitizer(brd, ch);
    auto position = Position(ring, sector, plane);

    fDigitizerMap[digitizer] = position;
  }

  file.close();

  std::cout << "Loaded " << fDigitizerMap.size() << " digitizer channels"
            << std::endl;
  for (auto &it : fDigitizerMap) {
    std::cout << "Digitizer: " << it.first.brd << ", " << it.first.ch
              << " -> ring: " << it.second.ring
              << ", sector: " << it.second.sector << ", plane: "
              << (it.second.plane == Plane::Front ? "front" : "rear")
              << std::endl;
  }
}

void TSiHist::FillByPositionRear(Position position, double weight)
{
  if (position.plane == Plane::Rear) {
    auto coordinates = GetRearCoordinates(position.sector);
    fHistRear->Fill(coordinates.x, coordinates.y, weight);
  } else {
    std::cerr << "Error: wrong plane for rear position" << std::endl;
  }
}

void TSiHist::FillByPositionFront(Position position, double weight)
{
  if (position.plane == Plane::Front) {
    auto coordinates = GetFrontCoordinates(position.ring, position.sector);
    fHistFront->Fill(coordinates.x, coordinates.y, weight);
  } else {
    std::cerr << "Error: wrong plane for front position" << std::endl;
  }
}

void TSiHist::FillByPositionMatrix(Position front, Position rear, double weight)
{
  if (front.plane == Plane::Front && rear.plane == Plane::Rear) {
    auto coordinates = GetMatrixCoordinates(front.ring, rear.sector);
    fHistMatrix->Fill(coordinates.x, coordinates.y, weight);
  } else {
    std::cerr << "Error: wrong plane for front or rear position" << std::endl;
  }
}

void TSiHist::FillByDigitizer(Digitizer digitizer, double weight)
{
  auto it = fDigitizerMap.find(digitizer);
  if (it != fDigitizerMap.end()) {
    auto position = it->second;
    if (position.plane == Plane::Front) {
      FillByPositionFront(position, weight);
    } else if (position.plane == Plane::Rear) {
      FillByPositionRear(position, weight);
    }
  } else {
    std::cerr << "Error: digitizer " << digitizer.brd << " " << digitizer.ch
              << " not found" << std::endl;
  }
}

void TSiHist::FillByDigitizerMatrix(Digitizer digitizer1, Digitizer digitizer2,
                                    double weight)
{
  auto it1 = fDigitizerMap.find(digitizer1);
  auto it2 = fDigitizerMap.find(digitizer2);
  if (it1 != fDigitizerMap.end() && it2 != fDigitizerMap.end()) {
    auto position1 = it1->second;
    auto position2 = it2->second;
    if (position1.plane == Plane::Front && position2.plane == Plane::Rear) {
      FillByPositionMatrix(position1, position2, weight);
    } else {
      std::cerr << "Error: wrong plane for front or rear position "
                << digitizer1.brd << " " << digitizer1.ch << " and "
                << digitizer2.brd << " " << digitizer2.ch << std::endl;
    }
  } else {
    std::cerr << "Error: digitizer " << digitizer1.brd << " " << digitizer1.ch
              << " or " << digitizer2.brd << " " << digitizer2.ch
              << " not found" << std::endl;
  }
}

}  // namespace SiDetector
