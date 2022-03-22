#include <fstream>
#include <iostream>

static constexpr int kgBrds = 8;
static constexpr int kgChs = 16;

void dummy()
{
  std::ofstream fout("dummy.dat");

  auto p0 = 0.0;
  auto p1 = 1.5;
  
  for(auto iBrd = 0; iBrd < kgBrds; iBrd++) {
    for(auto iCh = 0; iCh < kgChs; iCh++) {
      fout << iBrd <<" "
	   << iCh <<" "
	   << p0 <<" "
	   << p1 << std::endl;
      
      std::cout << iBrd <<" "
		<< iCh <<" "
		<< p0 <<" "
		<< p1 << std::endl;
    }
  }

  fout.close();
}
