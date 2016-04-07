#ifndef PixTestFeedback_H
#define PixTestFeedback_H

#include "PixTest.hh"

using std::vector;

class DLLEXPORT PixTestFeedback: public PixTest {
public:
  PixTestFeedback(PixSetup *, std::string);
  PixTestFeedback();
  virtual ~PixTestFeedback();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 

  void runCommand(std::string); 
  void doTest();
  std::vector< std::pair<double, int> > getEfficiency();
  void efficiencyVsFeedback();
  
private:

  int     fParVcal, fParStep; 

  ClassDef(PixTestFeedback, 1)

};
#endif
