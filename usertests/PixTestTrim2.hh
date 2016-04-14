#ifndef PixTestTrim22_H
#define PixTestTrim22_H

#include "PixTest.hh"

using std::vector;

class DLLEXPORT PixTestTrim2: public PixTest {
public:
  PixTestTrim2(PixSetup *, std::string);
  PixTestTrim2();
  virtual ~PixTestTrim2();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 

  void runCommand(std::string); 
  void trimTest();

  void setTrimBits(int itrim = -1); 
  void getCaldelVthrcomp();
  void doTest();
  int getNumberOfDeadPixels();
  void setVthrcomp(int vthrcomp = 50);
  void getVtrimVthrcomp();
  double getEfficiency();
  void retrimStep();
  void findVthrcomp();
  void final();
  
private:

  int     fParVcal, fParNtrig; 
  std::vector<std::pair<int, int> > fPIX; 
  int fTrimBits[16][52][80]; 
  vector< int > fCaldelVthrcompLUT;
  int fParNSteps;

  ClassDef(PixTestTrim2, 1)

};
#endif
