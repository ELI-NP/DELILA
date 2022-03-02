#include <fstream>
#include <iostream>
#include <string>
#include <array>
#include <memory>

#include <TF1.h>

auto fParFile = "./dummy.dat";

static constexpr int kgBrds = 8;
static constexpr int kgChs = 16;
std::array<std::array<std::array<double, 2> , kgChs>, kgBrds> fCalPar;
std::array<std::array<std::unique_ptr<TF1>, kgChs>, kgBrds> fCalFnc;

void reader()
{
  std::ifstream fin(fParFile);

  int mod, ch;
  double p0, p1;

  if(fin.is_open()){
    while(true) {
      fin >> mod >> ch >> p0 >> p1;
      if(fin.eof()) break;
      
      std::cout << mod <<" "<< ch <<" "<< p0 <<" "<< p1 << std::endl;
      if(mod >= 0 && mod < kgBrds &&
	 ch >= 0 && ch < kgChs) {
	TString fncName = Form("fnc%02d_%02d", mod, ch);
	fCalFnc[mod][ch].reset(new TF1(fncName, "pol1"));
	fCalFnc[mod][ch]->SetParameters(p0, p1);
      }
    }
  }
  
  fin.close();
}
