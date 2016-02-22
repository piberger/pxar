// -- author: Pirmin Berger
#ifndef PIXTESTUT61EDAC_H
#define PIXTESTUT61EDAC_H

#include "PixTest.hh"

class DLLEXPORT PixTestUT61EVsDac: public PixTest {
public:
  PixTestUT61EVsDac(PixSetup *, std::string);
  PixTestUT61EVsDac();
  virtual ~PixTestUT61EVsDac();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void doTest(); 

private:

  std::string fParDAC; 
  int fParDelay;
  double fParFactor;
  int fParDACStep;

  ClassDef(PixTestUT61EVsDac, 1)

};
#endif
