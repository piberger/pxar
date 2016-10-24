#ifndef PixTestXPixelAlive2_H
#define PixTestXPixelAlive2_H

#include "PixTest.hh"
#include "PHCalibration.hh"
#include "shist256.hh"

using std::vector;

class DLLEXPORT PixTestXPixelAlive2: public PixTest {
public:
  PixTestXPixelAlive2(PixSetup *, std::string);
  PixTestXPixelAlive2();
  virtual ~PixTestXPixelAlive2();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 

  void xpixelalive();
  void doCalDelScan();
  void doXNoiseMaps();
  std::pair<vector <TH2D*>,vector <TH2D*> > getdata();
  void runCommand(std::string command);
  void scurveAna(std::vector<shist256*> maps, std::vector<TH1*> &resultMaps, int result);
  bool threshold(TH1 *h);


  void doTest();
  
private:

  int fParVcal, fParNtrig, fParReset, fParMask;
  int fParNSteps;
  bool fParDelayTBM;

  bool          fPhCalOK;
  PHCalibration fPhCal;
  double               fThreshold, fThresholdE, fSigma, fSigmaE;  ///< variables for passing back s-curve results
  double               fThresholdN; ///< variable for passing back the threshold where noise leads to loss of efficiency

  ClassDef(PixTestXPixelAlive2, 1)

};
#endif
