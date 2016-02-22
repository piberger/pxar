// -- author: Pirmin Berger
#ifndef PIXTESTUT61EDACDAC_H
#define PIXTESTUT61EDACDAC_H

#include "PixTest.hh"

class DLLEXPORT PixTestUT61EVsDacDac: public PixTest {
public:
  PixTestUT61EVsDacDac(PixSetup *, std::string);
  PixTestUT61EVsDacDac();
  virtual ~PixTestUT61EVsDacDac();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void doTest(); 

private:

  std::string fParDAC1; 
  std::string fParDAC2; 
  int fParDelay;
  double fParFactor;
  int fParDACStep1;
  int fParDACStep2;

  ClassDef(PixTestUT61EVsDacDac, 1)

};
#endif
