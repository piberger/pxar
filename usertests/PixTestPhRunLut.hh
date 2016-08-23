#ifndef PIXTESTPHRUNLUTPH_H
#define PIXTESTPHRUNLUTPH_H

#include "PixTest.hh"

class DLLEXPORT PixTestPhRunLut: public PixTest {
public:
  PixTestPhRunLut(PixSetup *, std::string);
  PixTestPhRunLut();
  virtual ~PixTestPhRunLut();
  virtual bool setParameter(std::string parName, std::string sval);
  void init();
  void setToolTips();
  void bookHist(std::string);

  void doTest();

private:

  int     fParNtrig;
  int     fParNevents;
  int     fParNloops;
  bool    fParMPV;
  bool    fParKDE;
  bool    fParLinearFit;
  int     fParMaxHits;
  bool    fParDumpLUT;

  std::vector<std::pair<int, int> > fPIX;

  std::vector<TH2D*> fHitMap;
  std::vector<TH1D*> fCharge;
  std::vector<TH2D*> fChargeMap;

  std::vector<std::pair<std::string, uint8_t> > fPg_setup;
  ClassDef(PixTestPhRunLut, 1)

};
#endif
